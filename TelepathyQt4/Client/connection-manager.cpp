/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt4/Client/ConnectionManager>
#include "TelepathyQt4/Client/connection-manager-internal.h"

#include "TelepathyQt4/_gen/cli-connection-manager-body.hpp"
#include "TelepathyQt4/_gen/cli-connection-manager.moc.hpp"
#include "TelepathyQt4/Client/_gen/connection-manager.moc.hpp"
#include "TelepathyQt4/Client/_gen/connection-manager-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/PendingConnection>
#include <TelepathyQt4/Client/PendingReadyConnectionManager>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ManagerFile>
#include <TelepathyQt4/Types>

#include <QDBusConnectionInterface>
#include <QQueue>
#include <QStringList>
#include <QTimer>

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

/**
 * \defgroup clientcm Connection manager proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy ConnectionManager objects.
 */

namespace Telepathy
{
namespace Client
{

ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QDBusSignature &dbusSignature,
                                     QVariant defaultValue,
                                     Telepathy::ConnMgrParamFlag flags)
    : mName(name),
      mDBusSignature(dbusSignature),
      mType(ManagerFile::variantTypeFromDBusSignature(dbusSignature.signature())),
      mDefaultValue(defaultValue),
      mFlags(flags)
{
}

ProtocolParameter::~ProtocolParameter()
{
}

bool ProtocolParameter::isRequired() const
{
    return mFlags & ConnMgrParamFlagRequired;
}

bool ProtocolParameter::isSecret() const
{
    return mFlags & ConnMgrParamFlagSecret;
}

bool ProtocolParameter::requiredForRegistration() const
{
    return mFlags & ConnMgrParamFlagRegister;
}

bool ProtocolParameter::operator==(const ProtocolParameter &other) const
{
    return (mName == other.name());
}

bool ProtocolParameter::operator==(const QString &name) const
{
    return (mName == name);
}

struct ProtocolInfo::Private
{
    ProtocolParameterList params;
};

/**
 * \class ProtocolInfo
 * \ingroup clientcm
 * \headerfile TelepathyQt4/Client/connection-manager.h <TelepathyQt4/Client/ConnectionManager>
 *
 * Object representing a Telepathy protocol info.
 */

/**
 * Construct a new ProtocolInfo object.
 *
 * \param cmName Name of the connection manager.
 * \param name Name of the protocol.
 */
ProtocolInfo::ProtocolInfo(const QString &cmName, const QString &name)
    : mPriv(new Private()),
      mCmName(cmName),
      mName(name)
{
}

/**
 * Class destructor.
 */
ProtocolInfo::~ProtocolInfo()
{
    Q_FOREACH (ProtocolParameter *param, mPriv->params) {
        delete param;
    }
}

/**
 * \fn QString ProtocolInfo::cmName() const
 *
 * Get the short name of the connection manager (e.g. "gabble").
 *
 * \return The name of the connection manager.
 */

/**
 * \fn QString ProtocolInfo::name() const
 *
 * Get the string identifying the protocol as described in the Telepathy
 * D-Bus API Specification (e.g. "jabber").
 *
 * This identifier is not intended to be displayed to users directly; user
 * interfaces are responsible for mapping them to localized strings.
 *
 * \return A string identifying the protocol.
 */

/**
 * Return all supported parameters. The parameters' names
 * may either be the well-known strings specified by the Telepathy D-Bus
 * API Specification (e.g. "account" and "password"), or
 * implementation-specific strings.
 *
 * \return A list of parameters.
 */
const ProtocolParameterList &ProtocolInfo::parameters() const
{
    return mPriv->params;
}

/**
 * Return whether a given parameter can be passed to the connection
 * manager when creating a connection to this protocol.
 *
 * \param name The name of a parameter.
 * \return true if the given parameter exists.
 */
bool ProtocolInfo::hasParameter(const QString &name) const
{
    Q_FOREACH (ProtocolParameter *param, mPriv->params) {
        if (param->name() == name) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether it might be possible to register new accounts on this
 * protocol via Telepathy, by setting the special parameter named
 * <code>register</code> to <code>true</code>.
 *
 * \return The same thing as hasParameter("register").
 * \sa hasParameter()
 */
bool ProtocolInfo::canRegister() const
{
    return hasParameter(QLatin1String("register"));
}

void ProtocolInfo::addParameter(const ParamSpec &spec)
{
    QVariant defaultValue;
    if (spec.flags & ConnMgrParamFlagHasDefault) {
        defaultValue = spec.defaultValue.variant();
    }

    uint flags = spec.flags;
    if (spec.name.endsWith("password")) {
        flags |= Telepathy::ConnMgrParamFlagSecret;
    }

    ProtocolParameter *param = new ProtocolParameter(spec.name,
            QDBusSignature(spec.signature),
            defaultValue,
            (Telepathy::ConnMgrParamFlag) flags);

    mPriv->params.append(param);
}

ConnectionManager::Private::PendingNames::PendingNames(const QDBusConnection &bus)
    : PendingStringList(),
      mBus(bus)
{
    mMethodsQueue.enqueue(QLatin1String("ListNames"));
    mMethodsQueue.enqueue(QLatin1String("ListActivatableNames"));
    QTimer::singleShot(0, this, SLOT(continueProcessing()));
}

void ConnectionManager::Private::PendingNames::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        parseResult(reply.value());
        continueProcessing();
    } else {
        warning() << "Failure: error " << reply.error().name() <<
            ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void ConnectionManager::Private::PendingNames::continueProcessing()
{
    if (!mMethodsQueue.isEmpty()) {
        QLatin1String method = mMethodsQueue.dequeue();
        invokeMethod(method);
    }
    else {
        debug() << "Success: list" << mResult;
        setResult(mResult);
        setFinished();
    }
}

void ConnectionManager::Private::PendingNames::invokeMethod(const QLatin1String &method)
{
    QDBusPendingCall call = mBus.interface()->asyncCallWithArgumentList(
                method, QList<QVariant>());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onCallFinished(QDBusPendingCallWatcher *)));
}

void ConnectionManager::Private::PendingNames::parseResult(const QStringList &names)
{
    Q_FOREACH (const QString name, names) {
        if (name.startsWith("org.freedesktop.Telepathy.ConnectionManager.")) {
            mResult << name.right(name.length() - 44);
        }
    }
}

ConnectionManager::Private::Private(const QString &name, ConnectionManager *parent)
    : baseInterface(new ConnectionManagerInterface(parent->dbusConnection(),
                            parent->busName(), parent->objectPath(), parent)),
      name(name),
      ready(false),
      pendingReady(0),
      features(0)
{
    debug() << "Creating new ConnectionManager:" << parent->busName();
}

ConnectionManager::Private::~Private()
{
    Q_FOREACH (ProtocolInfo *info, protocols) {
        delete info;
    }
    delete baseInterface;
}

QString ConnectionManager::Private::makeBusName(const QString &name)
{
    return QString::fromAscii(
            TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE).append(name);
}

QString ConnectionManager::Private::makeObjectPath(const QString &name)
{
    return QString::fromAscii(
            TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE).append(name);
}

ProtocolInfo *ConnectionManager::Private::protocol(const QString &protocolName)
{
    Q_FOREACH (ProtocolInfo *info, protocols) {
        if (info->name() == protocolName) {
            return info;
        }
    }
    return NULL;
}

/**
 * \class ConnectionManager
 * \ingroup clientcm
 * \headerfile TelepathyQt4/Client/connection-manager.h <TelepathyQt4/Client/ConnectionManager>
 *
 * Object representing a Telepathy connection manager. Connection managers
 * allow connections to be made on one or more protocols.
 *
 * Most client applications should use this functionality via the
 * %AccountManager, to allow connections to be shared between client
 * applications.
 */

/**
 * Construct a new ConnectionManager object.
 *
 * \param name Name of the connection manager.
 * \param parent Object parent.
 */
ConnectionManager::ConnectionManager(const QString &name, QObject *parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            Private::makeBusName(name), Private::makeObjectPath(name),
            parent),
      OptionalInterfaceFactory<ConnectionManager>(this),
      mPriv(new Private(name, this))
{
    if (isValid()) {
        mPriv->introspectQueue.enqueue(&ConnectionManager::callReadConfig);
        QTimer::singleShot(0, this, SLOT(continueIntrospection()));
    }
}

/**
 * Construct a new ConnectionManager object.
 *
 * \param bus QDBusConnection to use.
 * \param name Name of the connection manager.
 * \param parent Object parent.
 */
ConnectionManager::ConnectionManager(const QDBusConnection &bus,
        const QString &name, QObject *parent)
    : StatelessDBusProxy(bus, Private::makeBusName(name),
            Private::makeObjectPath(name), parent),
      OptionalInterfaceFactory<ConnectionManager>(this),
      mPriv(new Private(name, this))
{
    if (isValid()) {
        mPriv->introspectQueue.enqueue(&ConnectionManager::callReadConfig);
        QTimer::singleShot(0, this, SLOT(continueIntrospection()));
    }
}

/**
 * Class destructor.
 */
ConnectionManager::~ConnectionManager()
{
    delete mPriv;
}

/**
 * Get the short name of the connection manager (e.g. "gabble").
 *
 * \return The name of the connection manager.
 */
QString ConnectionManager::name() const
{
    return mPriv->name;
}

QStringList ConnectionManager::interfaces() const
{
    return mPriv->interfaces;
}

/**
 * Get a list of strings identifying the protocols supported by this
 * connection manager, as described in the Telepathy
 * D-Bus API Specification (e.g. "jabber").
 *
 * These identifiers are not intended to be displayed to users directly; user
 * interfaces are responsible for mapping them to localized strings.
 *
 * \return A list of supported protocols.
 */
QStringList ConnectionManager::supportedProtocols() const
{
    QStringList protocols;
    Q_FOREACH (const ProtocolInfo *info, mPriv->protocols) {
        protocols.append(info->name());
    }
    return protocols;
}

/**
 * Get a list of protocols info for this connection manager.
 *
 * \return A list of ṔrotocolInfo.
 */
const ProtocolInfoList &ConnectionManager::protocols() const
{
    return mPriv->protocols;
}

/**
 * Request a Connection object representing a given account on a given protocol
 * with the given parameters.
 *
 * Return a pending operation representing the Connection object which will
 * succeed when the connection has been created or fail if an error occurred.
 *
 * \param protocol Name of the protocol to create the account for.
 * \param parameters Account parameters.
 * \return A PendingOperation which will emit PendingConnection::finished when
 *         the account has been created of failed its creation process.
 */
PendingConnection *ConnectionManager::requestConnection(const QString &protocol,
        const QVariantMap &parameters)
{
    return new PendingConnection(this, protocol, parameters);
}

/**
 * \fn DBus::propertiesInterface *ConnectionManager::propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * Properties interface is not necessarily reported by the services, so a
 * <code>check</code> parameter is not provided, and the interface is
 * always assumed to be present.
 */

/**
 * Return whether this object has finished its initial setup.
 *
 * This is mostly useful as a sanity check, in code that shouldn't be run
 * until the object is ready. To wait for the object to be ready, call
 * becomeReady() and connect to the finished signal on the result.
 *
 * \return \c true if the object has finished initial setup.
 */
bool ConnectionManager::isReady(Features features) const
{
    return mPriv->ready
        && ((mPriv->features & features) == features);
}

/**
 * Return a pending operation which will succeed when this object finishes
 * its initial setup, or will fail if a fatal error occurs during this
 * initial setup.
 *
 * \return A PendingReadyConnectionManager object which will emit finished
 *         when this object has finished or failed its initial setup.
 */
PendingReadyConnectionManager *ConnectionManager::becomeReady(Features requestedFeatures)
{
    if (!isValid()) {
        PendingReadyConnectionManager *operation =
            new PendingReadyConnectionManager(requestedFeatures, this);
        operation->setFinishedWithError(TELEPATHY_ERROR_NOT_AVAILABLE,
                "ConnectionManager is invalid");
        return operation;
    }

    if (isReady(requestedFeatures)) {
        PendingReadyConnectionManager *operation =
            new PendingReadyConnectionManager(requestedFeatures, this);
        operation->setFinished();
        return operation;
    }

    if (!mPriv->pendingReady) {
        mPriv->pendingReady =
            new PendingReadyConnectionManager(requestedFeatures, this);
    }
    return mPriv->pendingReady;
}

/**
 * Return a pending operation from which a list of all installed connection
 * manager short names (such as "gabble" or "haze") can be retrieved if it
 * succeeds.
 *
 * \return A PendingStringList which will emit PendingStringList::finished
 *         when this object has finished or failed getting the connection
 *         manager names.
 */
PendingStringList *ConnectionManager::listNames(const QDBusConnection &bus)
{
    return new ConnectionManager::Private::PendingNames(bus);
}

/**
 * Get the ConnectionManagerInterface for this ConnectionManager. This
 * method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing ConnectionManagerInterface for this
 *         ConnectionManager.
 */
ConnectionManagerInterface *ConnectionManager::baseInterface() const
{
    return mPriv->baseInterface;
}

/**** Private ****/
bool ConnectionManager::checkConfigFile()
{
    ManagerFile f(mPriv->name);
    if (!f.isValid()) {
        return false;
    }

    Q_FOREACH (QString protocol, f.protocols()) {
        ProtocolInfo *info = new ProtocolInfo(mPriv->name,
                                              protocol);
        mPriv->protocols.append(info);

        Q_FOREACH (ParamSpec spec, f.parameters(protocol)) {
            info->addParameter(spec);
        }
    }

#if 0
    Q_FOREACH (ProtocolInfo *info, mPriv->protocols) {
        qDebug() << "protocol name   :" << info->name();
        qDebug() << "protocol cn name:" << info->cmName();
        Q_FOREACH (ProtocolParameter *param, info->parameters()) {
            qDebug() << "\tparam name:       " << param->name();
            qDebug() << "\tparam is required:" << param->isRequired();
            qDebug() << "\tparam is secret:  " << param->isSecret();
            qDebug() << "\tparam value:      " << param->defaultValue();
        }
    }
#endif

    return true;
}

void ConnectionManager::callReadConfig()
{
    if (!checkConfigFile()) {
        warning() << "error parsing config file for connection manager"
                  << mPriv->name;
        mPriv->introspectQueue.enqueue(&ConnectionManager::callGetAll);
        mPriv->introspectQueue.enqueue(&ConnectionManager::callListProtocols);
    }

    continueIntrospection();
}

void ConnectionManager::callGetAll()
{
    debug() << "Calling Properties::GetAll(ConnectionManager)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            propertiesInterface()->GetAll(
                TELEPATHY_INTERFACE_CONNECTION_MANAGER), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onGetAllConnectionManagerReturn(QDBusPendingCallWatcher *)));
}

void ConnectionManager::callGetParameters()
{
    QString protocol = mPriv->getParametersQueue.dequeue();
    mPriv->protocolQueue.enqueue(protocol);
    debug() << "Calling ConnectionManager::GetParameters(" << protocol << ")";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            mPriv->baseInterface->GetParameters(protocol), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onGetParametersReturn(QDBusPendingCallWatcher *)));
}

void ConnectionManager::callListProtocols()
{
    debug() << "Calling ConnectionManager::ListProtocols";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            mPriv->baseInterface->ListProtocols(), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onListProtocolsReturn(QDBusPendingCallWatcher *)));
}

void ConnectionManager::onGetAllConnectionManagerReturn(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(ConnectionManager)";
        props = reply.value();

        // If Interfaces is not supported, the spec says to assume it's
        // empty, so keep the empty list mPriv was initialized with
        if (props.contains("Interfaces")) {
            mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
        }
    } else {
        warning().nospace() <<
            "Properties.GetAll(ConnectionManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
}

void ConnectionManager::onListProtocolsReturn(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;
    QStringList protocolsNames;

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.ListProtocols";
        protocolsNames = reply.value();
    } else {
        warning().nospace() <<
            "ConnectionManager.ListProtocols failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    Q_FOREACH (const QString &protocolName, protocolsNames) {
        mPriv->protocols.append(new ProtocolInfo(mPriv->name,
                    protocolName));

        mPriv->getParametersQueue.enqueue(protocolName);
        mPriv->introspectQueue.enqueue(&ConnectionManager::callGetParameters);
    }

    continueIntrospection();

    watcher->deleteLater();
}

void ConnectionManager::onGetParametersReturn(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ParamSpecList> reply = *watcher;
    ParamSpecList parameters;
    QString protocolName = mPriv->protocolQueue.dequeue();
    ProtocolInfo *info = mPriv->protocol(protocolName);

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.GetParameters";
        parameters = reply.value();
    } else {
        warning().nospace() <<
            "ConnectionManager.GetParameters failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    Q_FOREACH (const ParamSpec &spec, parameters) {
        debug() << "Parameter" << spec.name << "has flags" << spec.flags
            << "and signature" << spec.signature;

        info->addParameter(spec);
    }

    continueIntrospection();

    watcher->deleteLater();
}

void ConnectionManager::continueIntrospection()
{
    if (!mPriv->ready) {
        if (mPriv->introspectQueue.isEmpty()) {
            debug() << "ConnectionManager is ready";
            mPriv->ready = true;

            if (mPriv->pendingReady) {
                mPriv->pendingReady->setFinished();
                // it will delete itself later
                mPriv->pendingReady = 0;
            }
        } else {
            (this->*(mPriv->introspectQueue.dequeue()))();
        }
    }
}

} // Telepathy::Client
} // Telepathy
