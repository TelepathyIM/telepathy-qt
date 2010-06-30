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

#include <TelepathyQt4/ConnectionManager>
#include "TelepathyQt4/connection-manager-internal.h"

#include "TelepathyQt4/_gen/cli-connection-manager-body.hpp"
#include "TelepathyQt4/_gen/cli-connection-manager.moc.hpp"
#include "TelepathyQt4/_gen/connection-manager.moc.hpp"
#include "TelepathyQt4/_gen/connection-manager-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/PendingConnection>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ManagerFile>
#include <TelepathyQt4/Types>

#include <QDBusConnectionInterface>
#include <QQueue>
#include <QStringList>
#include <QTimer>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ProtocolParameter::Private
{
    Private(const QString &name, const QDBusSignature &dbusSignature, QVariant::Type type,
            const QVariant &defaultValue, ConnMgrParamFlag flags)
        : name(name), dbusSignature(dbusSignature), type(type), defaultValue(defaultValue),
          flags(flags) {}

    QString name;
    QDBusSignature dbusSignature;
    QVariant::Type type;
    QVariant defaultValue;
    ConnMgrParamFlag flags;
};

ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QDBusSignature &dbusSignature,
                                     QVariant defaultValue,
                                     ConnMgrParamFlag flags)
    : mPriv(new Private(name, dbusSignature,
                ManagerFile::variantTypeFromDBusSignature(dbusSignature.signature()), defaultValue,
                flags))
{
}

ProtocolParameter::~ProtocolParameter()
{
    delete mPriv;
}

QString ProtocolParameter::name() const
{
    return mPriv->name;
}

QDBusSignature ProtocolParameter::dbusSignature() const
{
    return mPriv->dbusSignature;
}

QVariant::Type ProtocolParameter::type() const
{
    return mPriv->type;
}

QVariant ProtocolParameter::defaultValue() const
{
    return mPriv->defaultValue;
}

bool ProtocolParameter::isRequired() const
{
    return mPriv->flags & ConnMgrParamFlagRequired;
}

bool ProtocolParameter::isSecret() const
{
    return mPriv->flags & ConnMgrParamFlagSecret;
}

bool ProtocolParameter::isRequiredForRegistration() const
{
    return mPriv->flags & ConnMgrParamFlagRegister;
}

bool ProtocolParameter::operator==(const ProtocolParameter &other) const
{
    return (mPriv->name == other.name());
}

bool ProtocolParameter::operator==(const QString &name) const
{
    return (mPriv->name == name);
}

struct TELEPATHY_QT4_NO_EXPORT ProtocolInfo::Private
{
    Private(const QString &cmName, const QString &name)
        : cmName(cmName),
          name(name),
          caps(new ConnectionCapabilities()),
          iconName(QString(QLatin1String("im-%1")).arg(name))
    {
    }

    ~Private()
    {
        delete caps;
    }

    QString cmName;
    QString name;
    ProtocolParameterList params;
    ConnectionCapabilities *caps;
    QString vcardField;
    QString englishName;
    QString iconName;
};

/**
 * \class ProtocolInfo
 * \ingroup clientcm
 * \headerfile TelepathyQt4/connection-manager.h <TelepathyQt4/ConnectionManager>
 *
 * \brief Object representing a Telepathy protocol info.
 */

/**
 * Construct a new ProtocolInfo object.
 *
 * \param cmName Name of the connection manager.
 * \param name Name of the protocol.
 */
ProtocolInfo::ProtocolInfo(const QString &cmName, const QString &name)
    : mPriv(new Private(cmName, name))
{
}

/**
 * Class destructor.
 */
ProtocolInfo::~ProtocolInfo()
{
    foreach (ProtocolParameter *param, mPriv->params) {
        delete param;
    }

    delete mPriv;
}

/**
 * Get the short name of the connection manager (e.g. "gabble").
 *
 * \return The name of the connection manager.
 */
QString ProtocolInfo::cmName() const
{
    return mPriv->cmName;
}

/**
 * Get the string identifying the protocol as described in the Telepathy
 * D-Bus API Specification (e.g. "jabber").
 *
 * This identifier is not intended to be displayed to users directly; user
 * interfaces are responsible for mapping them to localized strings.
 *
 * \return A string identifying the protocol.
 */
QString ProtocolInfo::name() const
{
    return mPriv->name;
}

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
    foreach (ProtocolParameter *param, mPriv->params) {
        if (param->name() == name) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether it might be possible to register new accounts on this
 * protocol, by setting the special parameter named
 * <code>register</code> to <code>true</code>.
 *
 * \return The same thing as hasParameter("register").
 * \sa hasParameter()
 */
bool ProtocolInfo::canRegister() const
{
    return hasParameter(QLatin1String("register"));
}

/**
 * Return the capabilities that are expected to be available from a connection
 * to this protocol, i.e. those for which Connection::createChannel() can
 * reasonably be expected to succeed.
 * User interfaces can use this information to show or hide UI components.
 *
 * @return An object representing the capabilities expected to be available from
 *         a connection to this protocol.
 */
ConnectionCapabilities *ProtocolInfo::capabilities() const
{
    return mPriv->caps;
}

/**
 * Return the name of the most common vCard field used for this protocol's
 * contact identifiers, normalized to lower case.
 *
 * \return The most common vCard field used for this protocol's contact
 *         identifiers, or an empty string if there is no such field.
 */
QString ProtocolInfo::vcardField() const
{
    return mPriv->vcardField;
}

/**
 * Return the name of this protocol in a form suitable for display to users,
 * such as "AIM" or "Yahoo!".
 *
 * \return The name of this protocol in a form suitable for display to users or
 *         an empty string if none is available.
 */
QString ProtocolInfo::englishName() const
{
    return mPriv->englishName;
}

/**
 * Return the name of an icon for this protocol in the system's icon theme,
 * such as "im-msn".
 *
 * \return The name of an icon for this protocol in the system's icon theme.
 */
QString ProtocolInfo::iconName() const
{
    return mPriv->iconName;
}

void ProtocolInfo::addParameter(const ParamSpec &spec)
{
    QVariant defaultValue;
    if (spec.flags & ConnMgrParamFlagHasDefault) {
        defaultValue = spec.defaultValue.variant();
    }

    uint flags = spec.flags;
    if (spec.name.endsWith(QLatin1String("password"))) {
        flags |= ConnMgrParamFlagSecret;
    }

    ProtocolParameter *param = new ProtocolParameter(spec.name,
            QDBusSignature(spec.signature),
            defaultValue,
            (ConnMgrParamFlag) flags);

    mPriv->params.append(param);
}

void ProtocolInfo::setVCardField(const QString &vcardField)
{
    mPriv->vcardField = vcardField;
}

void ProtocolInfo::setEnglishName(const QString &englishName)
{
    mPriv->englishName = englishName;
}

void ProtocolInfo::setIconName(const QString &iconName)
{
    mPriv->iconName = iconName;
}

void ProtocolInfo::setRequestableChannelClasses(
        const RequestableChannelClassList &caps)
{
    mPriv->caps->updateRequestableChannelClasses(caps);
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
        setResult(mResult.toList());
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
    foreach (const QString name, names) {
        if (name.startsWith(QLatin1String(TELEPATHY_INTERFACE_CONNECTION_MANAGER "."))) {
            mResult << name.right(name.length() - 44);
        }
    }
}

const Feature ConnectionManager::Private::ProtocolWrapper::FeatureCore =
    Feature(QLatin1String(ConnectionManager::Private::ProtocolWrapper::staticMetaObject.className()),
            0, true);

ConnectionManager::Private::ProtocolWrapper::ProtocolWrapper(
        const QDBusConnection &bus,
        const QString &busName, const QString &objectPath,
        const QString &cmName, const QString &name)
    : StatelessDBusProxy(bus, busName, objectPath),
      OptionalInterfaceFactory<ProtocolWrapper>(this),
      ReadyObject(this, FeatureCore),
      mReadinessHelper(readinessHelper()),
      mInfo(new ProtocolInfo(cmName, name))
{
    ReadinessHelper::Introspectables introspectables;

    // As ConnectionManager does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &ProtocolWrapper::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    mReadinessHelper->addIntrospectables(introspectables);
    mReadinessHelper->becomeReady(Features() << FeatureCore);
}

ConnectionManager::Private::ProtocolWrapper::~ProtocolWrapper()
{
}

void ConnectionManager::Private::ProtocolWrapper::introspectMain(
        ConnectionManager::Private::ProtocolWrapper *self)
{
    Client::DBus::PropertiesInterface *properties = self->propertiesInterface();
    Q_ASSERT(properties != 0);

    debug() << "Calling Properties::GetAll(Protocol)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(
                QLatin1String(TELEPATHY_INTERFACE_PROTOCOL)),
            self);
    self->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher *)));
}

void ConnectionManager::Private::ProtocolWrapper::gotMainProperties(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Protocol)";
        props = reply.value();

        setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
        mReadinessHelper->setInterfaces(interfaces());

        ParamSpecList parameters = qdbus_cast<ParamSpecList>(
                props[QLatin1String("Parameters")]);
        foreach (const ParamSpec &spec, parameters) {
            mInfo->addParameter(spec);
        }

        mInfo->setVCardField(qdbus_cast<QString>(
                    props[QLatin1String("VCardField")]));
        mInfo->setEnglishName(qdbus_cast<QString>(
                    props[QLatin1String("EnglishName")]));
        mInfo->setIconName(qdbus_cast<QString>(
                    props[QLatin1String("IconName")]));
        mInfo->setRequestableChannelClasses(qdbus_cast<RequestableChannelClassList>(
                    props[QLatin1String("RequestableChannelClasses")]));
    } else {
        warning().nospace() <<
            "Properties.GetAll(Protocol) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    mReadinessHelper->setIntrospectCompleted(FeatureCore, true);

    watcher->deleteLater();
}

ConnectionManager::Private::Private(ConnectionManager *parent, const QString &name)
    : parent(parent),
      name(name),
      baseInterface(new Client::ConnectionManagerInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      readinessHelper(parent->readinessHelper())
{
    debug() << "Creating new ConnectionManager:" << parent->busName();

    ReadinessHelper::Introspectables introspectables;

    // As ConnectionManager does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(Features() << FeatureCore);
}

ConnectionManager::Private::~Private()
{
    foreach (ProtocolWrapper *wrapper, wrappers) {
        /* wrapper->info() is not deleted by ProtocolWrapper as we borrow it in
         * case it gets ready */
        delete wrapper->info();
        delete wrapper;
    }

    foreach (ProtocolInfo *info, protocols) {
        delete info;
    }

    delete baseInterface;
}

bool ConnectionManager::Private::parseConfigFile()
{
    ManagerFile f(name);
    if (!f.isValid()) {
        return false;
    }

    foreach (QString protocol, f.protocols()) {
        ProtocolInfo *info = new ProtocolInfo(name, protocol);
        protocols.append(info);

        foreach (ParamSpec spec, f.parameters(protocol)) {
            info->addParameter(spec);
        }
        info->setRequestableChannelClasses(
                f.requestableChannelClasses(protocol));
        info->setVCardField(f.vcardField(protocol));
        info->setEnglishName(f.englishName(protocol));
        info->setIconName(f.iconName(protocol));
    }

#if 0
    foreach (ProtocolInfo *info, protocols) {
        qDebug() << "protocol name   :" << info->name();
        qDebug() << "protocol cn name:" << info->cmName();
        foreach (ProtocolParameter *param, info->parameters()) {
            qDebug() << "\tparam name:       " << param->name();
            qDebug() << "\tparam is required:" << param->isRequired();
            qDebug() << "\tparam is secret:  " << param->isSecret();
            qDebug() << "\tparam value:      " << param->defaultValue();
        }
    }
#endif

    return true;
}

void ConnectionManager::Private::introspectMain(ConnectionManager::Private *self)
{
    if (self->parseConfigFile()) {
        self->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        return;
    }

    warning() << "Error parsing config file for connection manager"
        << self->name << "- introspecting";

    Client::DBus::PropertiesInterface *properties = self->parent->propertiesInterface();
    Q_ASSERT(properties != 0);

    debug() << "Calling Properties::GetAll(ConnectionManager)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(
                QLatin1String(TELEPATHY_INTERFACE_CONNECTION_MANAGER)),
            self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher *)));
}

void ConnectionManager::Private::introspectProtocolsLegacy()
{
    debug() << "Calling ConnectionManager::ListProtocols";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            baseInterface->ListProtocols(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotProtocolsLegacy(QDBusPendingCallWatcher *)));
}

void ConnectionManager::Private::introspectParametersLegacy()
{
    foreach (const QString &protocolName, parametersQueue) {
        debug() << "Calling ConnectionManager::GetParameters(" << protocolName << ")";
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                baseInterface->GetParameters(protocolName), parent);
        parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(gotParametersLegacy(QDBusPendingCallWatcher *)));
    }
}

QString ConnectionManager::Private::makeBusName(const QString &name)
{
    return QString(QLatin1String(
                TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE)).append(name);
}

QString ConnectionManager::Private::makeObjectPath(const QString &name)
{
    return QString(QLatin1String(
                TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE)).append(name);
}

ProtocolInfo *ConnectionManager::Private::protocol(const QString &protocolName)
{
    foreach (ProtocolInfo *info, protocols) {
        if (info->name() == protocolName) {
            return info;
        }
    }
    return NULL;
}

/**
 * \class ConnectionManager
 * \ingroup clientcm
 * \headerfile TelepathyQt4/connection-manager.h <TelepathyQt4/ConnectionManager>
 *
 * \brief The ConnectionManager class provides an object representing a
 * Telepathy connection manager.
 *
 * Connection managers allow connections to be made on one or more protocols.
 *
 * Most client applications should use this functionality via the
 * %AccountManager, to allow connections to be shared between client
 * applications.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * ConnectionManager object usable.
 *
 * Note that this feature must be enabled in order to use most ConnectionManager
 * methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature ConnectionManager::FeatureCore = Feature(QLatin1String(ConnectionManager::staticMetaObject.className()), 0, true);

/**
 * Create a new ConnectionManager object.
 *
 * \param name Name of the connection manager.
 * \return A ConnectionManagerPtr object pointing to the newly created
 *         ConnectionManager object.
 */
ConnectionManagerPtr ConnectionManager::create(const QString &name)
{
    return ConnectionManagerPtr(new ConnectionManager(name));
}

/**
 * Create a new ConnectionManager object using the given \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param name Name of the connection manager.
 * \return A ConnectionManagerPtr object pointing to the newly created
 *         ConnectionManager object.
 */
ConnectionManagerPtr ConnectionManager::create(const QDBusConnection &bus,
        const QString &name)
{
    return ConnectionManagerPtr(new ConnectionManager(bus, name));
}

/**
 * Construct a new ConnectionManager object.
 *
 * \param name Name of the connection manager.
 */
ConnectionManager::ConnectionManager(const QString &name)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            Private::makeBusName(name), Private::makeObjectPath(name)),
      OptionalInterfaceFactory<ConnectionManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, name))
{
}

/**
 * Construct a new ConnectionManager object using the given \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param name Name of the connection manager.
 */
ConnectionManager::ConnectionManager(const QDBusConnection &bus,
        const QString &name)
    : StatelessDBusProxy(bus, Private::makeBusName(name),
            Private::makeObjectPath(name)),
      OptionalInterfaceFactory<ConnectionManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, name))
{
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
    foreach (const ProtocolInfo *info, mPriv->protocols) {
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
 * Return whether this connection manager implements the protocol specified by
 * \a protocolName.
 *
 * This method requires ConnectionManager::FeatureCore to be enabled.
 *
 * \return \c true if the protocol is supported, \c false otherwise.
 * \sa protocol(), protocols()
 */
bool ConnectionManager::hasProtocol(const QString &protocolName) const
{
    foreach (const ProtocolInfo *info, mPriv->protocols) {
        if (info->name() == protocolName) {
            return true;
        }
    }
    return false;
}

/**
 * Return the ProtocolInfo object for the protocol specified by
 * \a protocolName.
 *
 * This method requires ConnectionManager::FeatureCore to be enabled.
 *
 * \return A ProtocolInfo object or 0 if the protocol specified by \a
 *         protocolName is not supported.
 * \sa hasProtocol()
 */
ProtocolInfo *ConnectionManager::protocol(const QString &protocolName) const
{
    foreach (ProtocolInfo *info, mPriv->protocols) {
        if (info->name() == protocolName) {
            return info;
        }
    }
    return 0;
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
    return new PendingConnection(ConnectionManagerPtr(this),
            protocol, parameters);
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
Client::ConnectionManagerInterface *ConnectionManager::baseInterface() const
{
    return mPriv->baseInterface;
}

/**** Private ****/
void ConnectionManager::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(ConnectionManager)";
        props = reply.value();

        // If Interfaces is not supported, the spec says to assume it's
        // empty, so keep the empty list mPriv was initialized with
        if (props.contains(QLatin1String("Interfaces"))) {
            setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
            mPriv->readinessHelper->setInterfaces(interfaces());
        }
    } else {
        warning().nospace() <<
            "Properties.GetAll(ConnectionManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();

        // FIXME shouldn't this invalidate the CM or fall back to calling the individual methods?
    }

    ProtocolPropertiesMap protocolsMap =
        qdbus_cast<ProtocolPropertiesMap>(props[QLatin1String("Protocols")]);
    if (!protocolsMap.isEmpty()) {
        ProtocolPropertiesMap::const_iterator i = protocolsMap.constBegin();
        ProtocolPropertiesMap::const_iterator end = protocolsMap.constEnd();
        while (i != end) {
            QString protocolPath = QString(
                    QLatin1String("%1/%2")).arg(objectPath()).arg(i.key());
            Private::ProtocolWrapper *wrapper = new Private::ProtocolWrapper(
                        dbusConnection(), busName(), protocolPath,
                        mPriv->name, i.key());
            connect(wrapper->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onProtocolReady(Tp::PendingOperation *)));
            mPriv->wrappers.insert(wrapper, wrapper);
            ++i;
        }
    } else {
        mPriv->introspectProtocolsLegacy();
    }

    watcher->deleteLater();
}

void ConnectionManager::gotProtocolsLegacy(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;
    QStringList protocolsNames;

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.ListProtocols";
        protocolsNames = reply.value();

        foreach (const QString &protocolName, protocolsNames) {
            mPriv->protocols.append(new ProtocolInfo(mPriv->name,
                        protocolName));
            mPriv->parametersQueue.enqueue(protocolName);
        }

        mPriv->introspectParametersLegacy();
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());

        warning().nospace() <<
            "ConnectionManager.ListProtocols failed: " <<
            reply.error().name() << ": " << reply.error().message();

        // FIXME shouldn't this invalidate the CM?
    }

    watcher->deleteLater();
}

void ConnectionManager::gotParametersLegacy(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ParamSpecList> reply = *watcher;
    QString protocolName = mPriv->parametersQueue.dequeue();
    ProtocolInfo *info = mPriv->protocol(protocolName);

    if (!reply.isError()) {
        debug() << QString(QLatin1String("Got reply to ConnectionManager.GetParameters(%1)")).arg(protocolName);
        ParamSpecList parameters = reply.value();
        foreach (const ParamSpec &spec, parameters) {
            debug() << "Parameter" << spec.name << "has flags" << spec.flags
                << "and signature" << spec.signature;

            info->addParameter(spec);
        }
    } else {
        // let's remove this protocol as we can't get the params
        mPriv->protocols.removeOne(info);

        warning().nospace() <<
            QString(QLatin1String("ConnectionManager.GetParameters(%1) failed: ")).arg(protocolName) <<
            reply.error().name() << ": " << reply.error().message();
    }

    if (mPriv->parametersQueue.isEmpty()) {
        if (!mPriv->protocols.isEmpty()) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        } else {
            // we could not retrieve the params for any protocol, fail core.
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());
        }

#if 0
        foreach (ProtocolInfo *info, mPriv->protocols) {
            qDebug() << "protocol name   :" << info->name();
            qDebug() << "protocol cn name:" << info->cmName();
            foreach (ProtocolParameter *param, info->parameters()) {
                qDebug() << "\tparam name:       " << param->name();
                qDebug() << "\tparam is required:" << param->isRequired();
                qDebug() << "\tparam is secret:  " << param->isSecret();
                qDebug() << "\tparam value:      " << param->defaultValue();
            }
        }
#endif
    }

    watcher->deleteLater();
}

void ConnectionManager::onProtocolReady(Tp::PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    Private::ProtocolWrapper *wrapper =
        qobject_cast<Private::ProtocolWrapper*>(pr->object());
    ProtocolInfo *info = wrapper->info();

    delete mPriv->wrappers.take(wrapper);

    if (!op->isError()) {
        mPriv->protocols.append(info);
    } else {
        delete info;
    }

    if (mPriv->wrappers.isEmpty()) {
        if (!mPriv->protocols.isEmpty()) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        } else {
            // we could not make any Protocol objects ready, fail core.
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                    op->errorName(), op->errorMessage());
        }
    }
}

} // Tp
