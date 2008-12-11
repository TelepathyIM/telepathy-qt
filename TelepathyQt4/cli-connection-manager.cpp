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

#include "cli-connection-manager.h"

#include <QtCore/QQueue>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/PendingSuccess>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include "TelepathyQt4/debug-internal.hpp"

namespace Telepathy
{
namespace Client
{


struct ProtocolInfo::Private
{
    QString cmName;
    QString protocolName;

    QMap<QString,QDBusSignature> parameters;
    QMap<QString,QVariant> defaults;
    QSet<QString> requiredParameters;
    QSet<QString> registerParameters;
    QSet<QString> propertyParameters;
    QSet<QString> secretParameters;

    Private(const QString& cmName, const QString& protocolName)
        : cmName(cmName), protocolName(protocolName)
    {
    }
};


ProtocolInfo::ProtocolInfo(const QString& cmName, const QString& protocol)
    : mPriv(new Private(cmName, protocol))
{
}


ProtocolInfo::~ProtocolInfo()
{
}


QString ProtocolInfo::cmName() const
{
    return mPriv->cmName;
}


QString ProtocolInfo::protocolName() const
{
    return mPriv->protocolName;
}


QStringList ProtocolInfo::parameters() const
{
    return mPriv->parameters.keys();
}


bool ProtocolInfo::hasParameter(const QString& param) const
{
    return mPriv->parameters.contains(param);
}


QDBusSignature ProtocolInfo::parameterDBusSignature(const QString& param) const
{
    return mPriv->parameters.value(param);
}


QVariant::Type ProtocolInfo::parameterType(const QString& param) const
{
    return QVariant::Invalid;
}


bool ProtocolInfo::parameterIsRequired(const QString& param,
        bool registering) const
{
    if (registering)
        return mPriv->registerParameters.contains(param);
    else
        return mPriv->requiredParameters.contains(param);
}


bool ProtocolInfo::parameterIsSecret(const QString& param) const
{
    return mPriv->secretParameters.contains(param);
}


bool ProtocolInfo::parameterIsDBusProperty(const QString& param) const
{
    return mPriv->propertyParameters.contains(param);
}


bool ProtocolInfo::parameterHasDefault(const QString& param) const
{
    return mPriv->defaults.contains(param);
}


QVariant ProtocolInfo::getParameterDefault(const QString& param) const
{
    return mPriv->defaults.value(param);
}


bool ProtocolInfo::canRegister() const
{
    return hasParameter(QLatin1String("register"));
}


struct ConnectionManager::Private
{
    ConnectionManager& parent;
    QString cmName;
    ConnectionManagerInterface* baseInterface;
    bool ready;
    QQueue<void (Private::*)()> introspectQueue;
    QQueue<QString> getParametersQueue;
    QQueue<QString> protocolQueue;

    QStringList interfaces;

    QMap<QString,ProtocolInfo*> protocols;

    static inline QString makeBusName(const QString& name)
    {
        return QString::fromAscii(
                TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE).append(name);
    }

    static inline QString makeObjectPath(const QString& name)
    {
        return QString::fromAscii(
                TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE).append(name);
    }

    // A PendingOperation on which the ConnectionManager can call
    // setFinished().
    class PendingReady : public PendingOperation
    {
        friend class ConnectionManager;
    public:
        PendingReady(ConnectionManager *parent)
            : PendingOperation(parent)
        {
        }
    };

    QList<PendingReady *> pendingReadyOperations;

    ~Private()
    {
        Q_FOREACH (ProtocolInfo* protocol, protocols) {
            delete protocol;
        }
    }

    void callGetAll()
    {
        debug() << "Calling Properties::GetAll(ConnectionManager)";
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
                parent.propertiesInterface()->GetAll(
                    TELEPATHY_INTERFACE_CONNECTION_MANAGER), &parent);
        parent.connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onGetAllConnectionManagerReturn(QDBusPendingCallWatcher*)));
    }

    void callGetParameters()
    {
        QString protocol = getParametersQueue.dequeue();
        protocolQueue.enqueue(protocol);
        debug() << "Calling ConnectionManager::GetParameters(" <<
            protocol << ")";
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
                baseInterface->GetParameters(protocol), &parent);
        parent.connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onGetParametersReturn(QDBusPendingCallWatcher*)));
    }

    void callListProtocols()
    {
        debug() << "Calling ConnectionManager::ListProtocols";
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
                baseInterface->ListProtocols(), &parent);
        parent.connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onListProtocolsReturn(QDBusPendingCallWatcher*)));
    }

    Private(ConnectionManager& parent, QString name)
        : parent(parent),
          cmName(name),
          baseInterface(new ConnectionManagerInterface(parent.dbusConnection(),
                      parent.busName(), parent.objectPath(), &parent)),
          ready(false)
    {
        debug() << "Creating new ConnectionManager:" << parent.busName();

        introspectQueue.enqueue(&Private::callGetAll);
        introspectQueue.enqueue(&Private::callListProtocols);
        QTimer::singleShot(0, &parent, SLOT(continueIntrospection()));
    }
};


ConnectionManager::ConnectionManager(const QString& name, QObject* parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            Private::makeBusName(name), Private::makeObjectPath(name),
            parent),
      mPriv(new Private(*this, name))
{
}


ConnectionManager::ConnectionManager(const QDBusConnection& bus,
        const QString& name, QObject* parent)
    : StatelessDBusProxy(bus, Private::makeBusName(name),
            Private::makeObjectPath(name), parent),
      mPriv(new Private(*this, name))
{
}


ConnectionManager::~ConnectionManager()
{
    delete mPriv;
}


QString ConnectionManager::cmName() const
{
    return mPriv->cmName;
}


QStringList ConnectionManager::interfaces() const
{
    return mPriv->interfaces;
}


QStringList ConnectionManager::supportedProtocols() const
{
    return mPriv->protocols.keys();
}


const ProtocolInfo* ConnectionManager::protocolInfo(
        const QString& protocol) const
{
    return mPriv->protocols.value(protocol);
}


bool ConnectionManager::isReady() const
{
    return mPriv->ready;
}


// TODO: We don't actually consider anything during initial setup to be
// fatal, so the documentation isn't completely true.
PendingOperation *ConnectionManager::becomeReady()
{
    if (mPriv->ready)
        return new PendingSuccess(this);

    Private::PendingReady *pendingReady = new Private::PendingReady(this);
    mPriv->pendingReadyOperations << pendingReady;
    return pendingReady;
}


ConnectionManagerInterface* ConnectionManager::baseInterface() const
{
    return mPriv->baseInterface;
}


void ConnectionManager::onGetAllConnectionManagerReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(ConnectionManager)";
        props = reply.value();
    } else {
        warning().nospace() <<
            "Properties.GetAll(ConnectionManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    // If Interfaces is not supported, the spec says to assume it's
    // empty, so keep the empty list mPriv was initialized with
    if (props.contains("Interfaces")) {
        mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    }
    continueIntrospection();
}


void ConnectionManager::onListProtocolsReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;
    QStringList protocols;

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.ListProtocols";
        protocols = reply.value();
    } else {
        warning().nospace() <<
            "ConnectionManager.ListProtocols failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    Q_FOREACH (const QString& protocol, protocols) {
        mPriv->protocols.insert(protocol, new ProtocolInfo(mPriv->cmName,
                    protocol));

        mPriv->getParametersQueue.enqueue(protocol);
        mPriv->introspectQueue.enqueue(&Private::callGetParameters);
    }
    continueIntrospection();
}


void ConnectionManager::onGetParametersReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<ParamSpecList> reply = *watcher;
    ParamSpecList parameters;
    QString protocol = mPriv->protocolQueue.dequeue();
    ProtocolInfo* info = mPriv->protocols.value(protocol);

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.GetParameters";
        parameters = reply.value();
    } else {
        warning().nospace() <<
            "ConnectionManager.GetParameters failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    Q_FOREACH (const ParamSpec& spec, parameters) {
        debug() << "Parameter" << spec.name << "has flags" << spec.flags
            << "and signature" << spec.signature;
        info->mPriv->parameters.insert(spec.name,
                QDBusSignature(spec.signature));

        if (spec.flags & ConnMgrParamFlagHasDefault)
            info->mPriv->defaults.insert(spec.name,
                    spec.defaultValue.variant());

        if (spec.flags & ConnMgrParamFlagRequired)
            info->mPriv->requiredParameters.insert(spec.name);

        if (spec.flags & ConnMgrParamFlagRegister)
            info->mPriv->registerParameters.insert(spec.name);

        if ((spec.flags & ConnMgrParamFlagSecret)
                || spec.name.endsWith("password"))
            info->mPriv->secretParameters.insert(spec.name);

#if 0
        // enable when we merge the new telepathy-spec
        if (spec.flags & ConnMgrParamFlag)
            info->mPriv->propertyParameters.insert(spec.name);
#endif
    }
    continueIntrospection();
}

void ConnectionManager::continueIntrospection()
{
    if (!mPriv->ready) {
        if (mPriv->introspectQueue.isEmpty()) {
            debug() << "ConnectionManager is ready";
            mPriv->ready = true;
            Q_EMIT ready(this);

            while (!mPriv->pendingReadyOperations.isEmpty()) {
                debug() << "Finishing one";
                mPriv->pendingReadyOperations.takeFirst()->setFinished();
            }
        } else {
            (mPriv->*(mPriv->introspectQueue.dequeue()))();
        }
    }
}

} // Telepathy::Client
} // Telepathy

#include <TelepathyQt4/_gen/cli-connection-manager-body.hpp>
#include <TelepathyQt4/_gen/cli-connection-manager.moc.hpp>
#include <TelepathyQt4/cli-connection-manager.moc.hpp>
