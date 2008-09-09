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

#include "cli-connection.h"

#include <TelepathyQt4/_gen/cli-connection-body.hpp>
#include <TelepathyQt4/_gen/cli-connection.moc.hpp>
#include <TelepathyQt4/cli-connection.moc.hpp>

#include <QMap>
#include <QQueue>
#include <QString>
#include <QtGlobal>

#include "debug-internal.hpp"

namespace Telepathy
{
namespace Client
{

struct Connection::Private
{
    // Public object
    Connection& parent;

    // Optional interface proxies
    ConnectionInterfaceAliasingInterface* aliasing;
    ConnectionInterfacePresenceInterface* presence;
    DBus::PropertiesInterface* properties;

    // Introspection
    bool initialIntrospection;
    Readiness readiness;
    QStringList interfaces;
    QQueue<void (Private::*)()> introspectQueue;

    // Introspected properties
    uint status;
    uint statusReason;
    uint aliasFlags;
    StatusSpecMap presenceStatuses;
    SimpleStatusSpecMap simplePresenceStatuses;

    Private(Connection &parent)
        : parent(parent)
    {
        aliasing = 0;
        presence = 0;
        properties = 0;

        initialIntrospection = false;
        readiness = ReadinessJustCreated;
        status = ConnectionStatusDisconnected;
        statusReason = ConnectionStatusReasonNoneSpecified;
        aliasFlags = 0;

        debug() << "Connecting to StatusChanged()";

        parent.connect(&parent,
                       SIGNAL(StatusChanged(uint, uint)),
                       SLOT(onStatusChanged(uint, uint)));

        debug() << "Calling GetStatus()";

        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetStatus(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotStatus(QDBusPendingCallWatcher*)));
    }

    void introspectAliasing()
    {
        // The Aliasing interface is not usable before the connection is established
        if (initialIntrospection) {
            continueIntrospection();
            return;
        }

        if (!aliasing) {
            aliasing = parent.aliasingInterface();
            Q_ASSERT(aliasing != 0);
        }

        debug() << "Calling GetAliasFlags()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(aliasing->GetAliasFlags(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotAliasFlags(QDBusPendingCallWatcher*)));
    }

    void introspectMain()
    {
        // Introspecting the main interface is currently just calling
        // GetInterfaces(), but it might include other stuff in the future if we
        // gain GetAll-able properties on the connection
        debug() << "Calling GetInterfaces()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetInterfaces(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
    }

    void introspectPresence()
    {
        // The Presence interface is not usable before the connection is established
        if (initialIntrospection) {
            continueIntrospection();
            return;
        }

        if (!presence) {
            presence = parent.presenceInterface();
            Q_ASSERT(presence != 0);
        }

        debug() << "Calling GetStatuses() (legacy)";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(presence->GetStatuses(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotStatuses(QDBusPendingCallWatcher*)));
    }

    void introspectSimplePresence()
    {
        if (!properties) {
            properties = parent.propertiesInterface();
            Q_ASSERT(properties != 0);
        }

        debug() << "Getting available SimplePresence statuses";
        QDBusPendingCall call =
            properties->Get(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
                           "Statuses");
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(call, &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotSimpleStatuses(QDBusPendingCallWatcher*)));
    }

    void continueIntrospection()
    {
        if (introspectQueue.isEmpty()) {
            if (initialIntrospection) {
                initialIntrospection = false;
                if (readiness < ReadinessNotYetConnected)
                    changeReadiness(ReadinessNotYetConnected);
            } else {
                if (readiness != ReadinessDead)
                    changeReadiness(ReadinessFull);
            }
        } else {
            (this->*introspectQueue.dequeue())();
        }
    }

    void changeReadiness(Readiness newReadiness)
    {
        Q_ASSERT(newReadiness != readiness);

        switch (readiness) {
            case ReadinessJustCreated:
                break;
            case ReadinessNotYetConnected:
                Q_ASSERT(newReadiness == ReadinessConnecting
                        || newReadiness == ReadinessDead);
                break;
            case ReadinessConnecting:
                Q_ASSERT(newReadiness == ReadinessFull
                        || newReadiness == ReadinessDead);
                break;
            case ReadinessFull:
                Q_ASSERT(newReadiness == ReadinessDead);
                break;
            case ReadinessDead:
            default:
                Q_ASSERT(false);
        }

        debug() << "Readiness changed from" << readiness << "to" << newReadiness;
        readiness = newReadiness;
        emit parent.readinessChanged(newReadiness);
    }
};

Connection::Connection(const QString& serviceName,
                       const QString& objectPath,
                       QObject* parent)
    : ConnectionInterface(serviceName, objectPath, parent),
      mPriv(new Private(*this))
{
}

Connection::Connection(const QDBusConnection& connection,
                       const QString& serviceName,
                       const QString& objectPath,
                       QObject* parent)
    : ConnectionInterface(connection, serviceName, objectPath, parent),
      mPriv(new Private(*this))
{
}

Connection::~Connection()
{
    delete mPriv;
}

Connection::Readiness Connection::readiness() const
{
    return mPriv->readiness;
}

uint Connection::status() const
{
    return mPriv->status;
}

uint Connection::statusReason() const
{
    return mPriv->statusReason;
}

QStringList Connection::interfaces() const
{
    return mPriv->interfaces;
}

uint Connection::aliasFlags() const
{
    return mPriv->aliasFlags;
}

StatusSpecMap Connection::presenceStatuses() const
{
    return mPriv->presenceStatuses;
}

SimpleStatusSpecMap Connection::simplePresenceStatuses() const
{
    return mPriv->simplePresenceStatuses;
}

void Connection::onStatusChanged(uint status, uint reason)
{
    debug() << "Status changed from" << mPriv->status << "to" << status << "because of" << reason;

    mPriv->status = status;
    mPriv->statusReason = reason;

    switch (status) {
        case ConnectionStatusConnected:
            debug() << "Performing introspection for the Connected status";
            mPriv->introspectQueue.enqueue(&Private::introspectMain);
            mPriv->continueIntrospection();
            break;

        case ConnectionStatusConnecting:
            if (mPriv->readiness < ReadinessConnecting)
                mPriv->changeReadiness(ReadinessConnecting);
            else
                warning() << "Got unexpected status change to Connecting";
            break;

        case ConnectionStatusDisconnected:
            if (mPriv->readiness != ReadinessDead)
                mPriv->changeReadiness(ReadinessDead);
            else
                warning() << "Got unexpected status change to Disconnected";
            break;

        default:
            warning() << "Unknown connection status" << status;
            break;
    }
}

void Connection::gotStatus(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "GetStatus() failed with " << reply.error().name() << ": " << reply.error().message();
        mPriv->changeReadiness(ReadinessDead);
        return;
    }

    uint status = reply.value();

    debug() << "Got connection status" << status;
    mPriv->status = status;

    // Don't do any introspection yet if the connection is in the Connecting
    // state; the StatusChanged handler will take care of doing that, if the
    // connection ever gets to the Connected state.
    if (status == ConnectionStatusConnecting) {
        debug() << "Not introspecting yet because the connection is currently Connecting";
        mPriv->changeReadiness(ReadinessConnecting);
        return;
    }

    if (status == ConnectionStatusDisconnected) {
        debug() << "Performing introspection for the Disconnected status";
        mPriv->initialIntrospection = true;
    } else {
        if (status != ConnectionStatusConnected) {
            warning() << "Not performing introspection for unknown status" << status;
            return;
        } else {
            debug() << "Performing introspection for the Connected status";
        }
    }

    mPriv->introspectQueue.enqueue(&Private::introspectMain);
    mPriv->continueIntrospection();
}

void Connection::gotInterfaces(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        mPriv->interfaces = reply.value();
        debug() << "Got reply to GetInterfaces():" << mPriv->interfaces;
    } else {
        warning().nospace() << "GetInterfaces() failed with " << reply.error().name() << ": " << reply.error().message() << " - assuming no new interfaces";
    }

    for (QStringList::const_iterator i  = mPriv->interfaces.constBegin();
                                     i != mPriv->interfaces.constEnd();
                                     ++i) {
        void (Private::*introspectFunc)() = 0;

        if (*i == TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING)
            introspectFunc = &Private::introspectAliasing;
        else if (*i == TELEPATHY_INTERFACE_CONNECTION_INTERFACE_PRESENCE)
            introspectFunc = &Private::introspectPresence;
        else if (*i == TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)
            introspectFunc = &Private::introspectSimplePresence;

        if (introspectFunc)
            mPriv->introspectQueue.enqueue(introspectFunc);
    }

    mPriv->continueIntrospection();
}

void Connection::gotAliasFlags(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (!reply.isError()) {
        mPriv->aliasFlags = static_cast<ConnectionAliasFlag>(reply.value());
        debug().nospace() << "Got alias flags 0x" << hex << mPriv->aliasFlags;
    } else {
        warning().nospace() << "GetAliasFlags() failed with " << reply.error().name() << ": " << reply.error().message();
    }

    mPriv->continueIntrospection();
}

void Connection::gotStatuses(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<StatusSpecMap> reply = *watcher;

    if (!reply.isError()) {
        mPriv->presenceStatuses = reply.value();
        debug() << "Got" << mPriv->presenceStatuses.size() << "legacy presence statuses";
    } else {
        warning().nospace() << "GetStatuses() failed with " << reply.error().name() << ": " << reply.error().message();
    }

    mPriv->continueIntrospection();
}

void Connection::gotSimpleStatuses(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        mPriv->simplePresenceStatuses = qdbus_cast<SimpleStatusSpecMap>(reply.value().variant());
        debug() << "Got" << mPriv->simplePresenceStatuses.size() << "simple presence statuses";
    } else {
        warning().nospace() << "Getting simple presence statuses failed with " << reply.error().name() << ": " << reply.error().message();
    }

    mPriv->continueIntrospection();
}

}
}
