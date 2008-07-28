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

#include <QQueue>
#include <QtGlobal>

#include "cli-dbus.h"
#include "debug.hpp"

namespace Telepathy
{
namespace Client
{

struct Connection::Private
{
    Connection& parent;
    ConnectionInterfaceAliasingInterface aliasing;
    ConnectionInterfacePresenceInterface presence;
    ConnectionInterfaceSimplePresenceInterface simplePresence;
    DBus::PropertiesInterface properties;
    QQueue<void (Private::*)()> introspectQueue;

    bool ready;
    long status;
    uint statusReason;
    QStringList interfaces;
    ConnectionAliasFlags aliasFlags;
    StatusSpecMap presenceStatuses;
    SimpleStatusSpecMap simplePresenceStatuses;

    Private(Connection &parent)
        : parent(parent),
          aliasing(parent),
          presence(parent),
          simplePresence(parent),
          properties(parent)
    {
        ready = false;
        status = -1;
        statusReason = ConnectionStatusReasonNoneSpecified;

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
        debug() << "Calling GetAliasFlags()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(aliasing.GetAliasFlags(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotAliasFlags(QDBusPendingCallWatcher*)));
    }

    void introspectPresence()
    {
        debug() << "Calling GetStatuses() (legacy)";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(presence.GetStatuses(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotStatuses(QDBusPendingCallWatcher*)));
    }

    void introspectSimplePresence()
    {
        debug() << "Getting available SimplePresence statuses";
        QDBusPendingCall call =
            properties.Get(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
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
            debug() << "Connection ready";
            ready = true;
            emit parent.nowReady();
        } else {
            (this->*introspectQueue.dequeue())();
        }
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

bool Connection::ready() const
{
    return mPriv->ready;
}

long Connection::status() const
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

#if 0

ConnectionAliasFlags Connection::aliasFlags() const
{
    return mPriv->aliasFlags;
}

StatusSpecMap Connection::presenceStatuses() const
{
    return mPriv->presenceStatuses;
}


SimpleStatusSpecMap Connection::simplePresenceStatuses() const
{
    if (!ready() && mPriv->simplePresenceStatuses.isEmpty()) {
        debug() << "Getting the simple presence statuses available before connecting";
        mPriv->simplePresenceStatuses = mPriv->simplePresence.statuses();
    }

    return mPriv->simplePresenceStatuses;
}
#endif

void Connection::onStatusChanged(uint status, uint reason)
{
    if (mPriv->status == -1) {
        // We've got a StatusChanged before the initial GetStatus reply, ignore it
        return;
    }

    debug().nospace() << "New status (" << status << ", " << reason << ')';

    mPriv->status = status;
    mPriv->statusReason = reason;

    if (status == ConnectionStatusConnected) {
        debug() << "Calling GetInterfaces()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(GetInterfaces(), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
    }
}

void Connection::gotStatus(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to initial GetStatus()";
        // Avoid early return in onStatusChanged()
        mPriv->status = reply.argumentAt<0>();
        onStatusChanged(reply.argumentAt<0>(), ConnectionStatusReasonNoneSpecified);
    } else {
        warning().nospace() << "GetStatus() failed with " << reply.error().name() << ": " << reply.error().message();
    }
}

void Connection::gotInterfaces(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        mPriv->interfaces = reply.value();
        debug() << "Got reply to initial GetInterfaces():";
        debug() << mPriv->interfaces;
    } else {
        warning().nospace() << "GetInterfaces() failed with " << reply.error().name() << ": " << reply.error().message() << " - assuming no interfaces";
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
        debug().nospace() << "Got initial alias flags 0x" << hex << mPriv->aliasFlags;
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
        debug() << "Got initial legacy presence statuses";
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
        debug() << "Got initial simple presence statuses";
    } else {
        warning().nospace() << "Getting initial simple presence statuses failed with " << reply.error().name() << ": " << reply.error().message();
    }

    mPriv->continueIntrospection();
}

}
}
