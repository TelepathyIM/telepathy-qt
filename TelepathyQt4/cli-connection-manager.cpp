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
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include "TelepathyQt4/debug-internal.hpp"

namespace Telepathy
{
namespace Client
{


struct ConnectionManager::Private
{
    ConnectionManager& parent;
    ConnectionManagerInterface* baseInterface;
    bool ready;
    QQueue<void (Private::*)()> introspectQueue;
    QQueue<QString> protocolQueue;

    QStringList interfaces;
    QStringList protocols;

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

    void continueIntrospection()
    {
        if (!ready) {
            if (introspectQueue.isEmpty()) {
                // TODO: signal ready
                debug() << "ConnectionManager is ready";
            } else {
                (this->*introspectQueue.dequeue())();
            }
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
        QString protocol = protocolQueue.dequeue();
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

    Private(ConnectionManager& parent)
        : parent(parent),
          baseInterface(new ConnectionManagerInterface(parent.dbusConnection(),
                      parent.busName(), parent.objectPath(), &parent)),
          ready(false)
    {
        debug() << "Creating new ConnectionManager:" << parent.busName();

        introspectQueue.enqueue(&Private::callGetAll);
        introspectQueue.enqueue(&Private::callListProtocols);
        QTimer::singleShot(0, &parent, SLOT(onStartIntrospection()));
    }
};


ConnectionManager::ConnectionManager(const QString& name, QObject* parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            Private::makeBusName(name), Private::makeObjectPath(name),
            parent),
      mPriv(new Private(*this))
{
}


ConnectionManager::ConnectionManager(const QDBusConnection& bus,
        const QString& name, QObject* parent)
    : StatelessDBusProxy(bus, Private::makeBusName(name),
            Private::makeObjectPath(name), parent),
      mPriv(new Private(*this))
{
}


ConnectionManager::~ConnectionManager()
{
    delete mPriv;
}


QStringList ConnectionManager::interfaces() const
{
    return mPriv->interfaces;
}


QStringList ConnectionManager::supportedProtocols() const
{
    return mPriv->protocols;
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
    mPriv->continueIntrospection();
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

    mPriv->protocols = protocols;
    Q_FOREACH (const QString& protocol, protocols) {
        mPriv->protocolQueue.enqueue(protocol);
        mPriv->introspectQueue.enqueue(&Private::callGetParameters);
    }
    mPriv->continueIntrospection();
}


void ConnectionManager::onGetParametersReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<ParamSpecList> reply = *watcher;
    ParamSpecList parameters;

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
    }
    mPriv->continueIntrospection();
}

void ConnectionManager::onStartIntrospection()
{
    mPriv->continueIntrospection();
}

} // Telepathy::Client
} // Telepathy

#include <TelepathyQt4/_gen/cli-connection-manager-body.hpp>
#include <TelepathyQt4/_gen/cli-connection-manager.moc.hpp>
#include <TelepathyQt4/cli-connection-manager.moc.hpp>
