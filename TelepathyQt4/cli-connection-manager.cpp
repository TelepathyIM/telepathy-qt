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
#include <TelepathyQt4/Types>

#include "TelepathyQt4/debug-internal.hpp"

namespace Telepathy
{
namespace Client
{


// FIXME proper map dbusSignature to QVariant on mType
ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QDBusSignature &dbusSignature,
                                     QVariant defaultValue,
                                     Telepathy::ConnMgrParamFlag flags)
    : mName(name),
      mDBusSignature(dbusSignature),
      mType(QVariant::Invalid),
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


ProtocolInfo::ProtocolInfo(const QString &cmName, const QString &protocolName)
    : mPriv(new Private()),
      mCmName(cmName),
      mProtocolName(protocolName)
{
}


ProtocolInfo::~ProtocolInfo()
{
    Q_FOREACH (ProtocolParameter *param, mPriv->params) {
        delete param;
    }
}


const ProtocolParameterList &ProtocolInfo::parameters() const
{
    return mPriv->params;
}


bool ProtocolInfo::hasParameter(const QString &name) const
{
    Q_FOREACH (ProtocolParameter *param, mPriv->params) {
        if (param->name() == name) {
            return true;
        }
    }
    return false;
}


bool ProtocolInfo::canRegister() const
{
    return hasParameter(QLatin1String("register"));
}


void ProtocolInfo::addParameter(const ParamSpec &spec)
{
    QVariant defaultValue;
    if (spec.flags & ConnMgrParamFlagHasDefault)
        defaultValue = spec.defaultValue.variant();

    uint flags = spec.flags;
    if (spec.name.endsWith("password"))
        flags |= Telepathy::ConnMgrParamFlagSecret;

    ProtocolParameter *param = new ProtocolParameter(spec.name,
            QDBusSignature(spec.signature),
            defaultValue,
            (Telepathy::ConnMgrParamFlag) flags);
    mPriv->params.append(param);
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

        info->addParameter(spec);
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
