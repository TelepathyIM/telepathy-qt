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

#include <TelepathyQt4/Client/Connection>

#include <TelepathyQt4/_gen/cli-connection-body.hpp>
#include <TelepathyQt4/_gen/cli-connection.moc.hpp>

#include "debug.hpp"

namespace Telepathy
{
namespace Client
{

struct Connection::Private
{
    Connection* parent;
    ConnectionInterface interface;

    bool ready;
    long status;
    uint statusReason;
    QStringList interfaces;

    Private(Connection *parent,
            const QDBusConnection& connection,
            const QString& serviceName,
            const QString& objectPath)
        : parent(parent),
          interface(connection, serviceName, objectPath, parent)
    {
        ready = false;
        status = -1;
        statusReason = ConnectionStatusReasonNoneSpecified;

        parent->connect(&interface,
                        SIGNAL(statusChanged(uint, uint)),
                        SLOT(statusChanged(uint, uint)));

        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(interface.getStatus(), parent);
        parent->connect(watcher,
                        SIGNAL(finished(QDBusPendingCallWatcher*)),
                        SLOT(gotStatus(QDBusPendingCallWatcher*)));
    }
};

Connection::Connection(const QString& serviceName,
                       const QString& objectPath,
                       QObject* parent)
    : QObject(parent),
      mPriv(new Private(this, QDBusConnection::sessionBus(), serviceName, objectPath))
{
}

Connection::Connection(const QDBusConnection& connection,
                       const QString& serviceName,
                       const QString& objectPath,
                       QObject* parent)
    : QObject(parent),
      mPriv(new Private(this, connection, serviceName, objectPath))
{
}

Connection::~Connection()
{
    delete mPriv;
}

ConnectionInterface& Connection::interface()
{
    return mPriv->interface;
}

const ConnectionInterface& Connection::interface() const
{
    return mPriv->interface;
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

void Connection::statusChanged(uint status, uint reason)
{
    if (mPriv->status == -1) {
        // We've got a StatusChanged before the initial GetStatus reply, ignore it
        return;
    }

    mPriv->status = status;
    mPriv->statusReason = reason;

    emit statusChanged(status, reason);

    if (status == ConnectionStatusConnected) {
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(interface().getInterfaces(), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
    }
}

void Connection::gotStatus(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;

    if (!reply.isError()) {
        // Avoid early return in statusChanged()
        mPriv->status = reply.argumentAt<0>();
        statusChanged(reply.argumentAt<0>(), reply.argumentAt<1>());
    } else {
        warning().nospace() << "GetStatus() failed with " << reply.error().name() << ": " << reply.error().message();
    }
}

void Connection::gotInterfaces(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError())
        mPriv->interfaces = reply.value();
    else
        warning().nospace() << "GetInterfaces() failed with " << reply.error().name() << ": " << reply.error().message() << " - assuming no interfaces";

    // TODO introspect interfaces

    mPriv->ready = true;
    emit ready();
}

}
}
