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

#define IN_TELEPATHY_QT4_HEADER
#include "pending-channel.h"
#include "pending-channel.moc.hpp"

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/debug-internal.h>

namespace Telepathy
{
namespace Client
{

struct PendingChannel::Private
{
    QString channelType;
    uint handleType;
    uint handle;
    QDBusObjectPath objectPath;
};

PendingChannel::PendingChannel(Connection* connection, const QString& channelType, uint handleType, uint handle)
    : PendingOperation(connection), mPriv(new Private)
{
    mPriv->channelType = channelType;
    mPriv->handleType = handleType;
    mPriv->handle = handle;
}

PendingChannel::~PendingChannel()
{
    delete mPriv;
}

Connection* PendingChannel::connection() const
{
    return qobject_cast<Connection*>(parent());
}

const QString& PendingChannel::channelType() const
{
    return mPriv->channelType;
}

uint PendingChannel::handleType() const
{
    return mPriv->handleType;
}

uint PendingChannel::handle() const
{
    return mPriv->handle;
}

Channel* PendingChannel::channel(QObject* parent) const
{
    if (!isFinished()) {
        warning() << "PendingChannel::channel called before finished, returning 0";
        return 0;
    } else if (!isValid()) {
        warning() << "PendingChannel::channel called when not valid, returning 0";
        return 0;
    }

    Channel* channel =
        new Channel(connection(),
                    mPriv->objectPath.path(),
                    parent);
    return channel;
}

void PendingChannel::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    debug() << "Received reply to RequestChannel";

    if (!reply.isError()) {
        debug() << " Success: object path" << reply.value().path();
        mPriv->objectPath = reply.value();
        setFinished();
    } else {
        debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
