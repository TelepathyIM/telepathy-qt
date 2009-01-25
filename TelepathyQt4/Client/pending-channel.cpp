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

#include <TelepathyQt4/Client/PendingChannel>

#include "TelepathyQt4/Client/_gen/pending-channel.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/Connection>

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

PendingChannel::PendingChannel(Connection* connection, const QString& errorName,
        const QString& errorMessage)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->handleType = 0;
    mPriv->handle = 0;

    setFinishedWithError(errorName, errorMessage);
}

PendingChannel::PendingChannel(Connection* connection, const QString& channelType, uint handleType, uint handle)
    : PendingOperation(connection), mPriv(new Private)
{
    mPriv->channelType = channelType;
    mPriv->handleType = handleType;
    mPriv->handle = handle;

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(connection->baseInterface()->RequestChannel(
                    channelType, handleType, handle, true), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onCallRequestChannelFinished(QDBusPendingCallWatcher *)));
}

PendingChannel::PendingChannel(Connection* connection, const QVariantMap& request, bool create)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->channelType = request.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
    mPriv->handleType = request.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")).toUInt();
    mPriv->handle = request.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle")).toUInt();

    if (create) {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                connection->requestsInterface()->CreateChannel(request), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onCallCreateChannelFinished(QDBusPendingCallWatcher *)));
    }
    else {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                connection->requestsInterface()->EnsureChannel(request), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onCallEnsureChannelFinished(QDBusPendingCallWatcher *)));
    }
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

void PendingChannel::onCallRequestChannelFinished(QDBusPendingCallWatcher* watcher)
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

void PendingChannel::onCallCreateChannelFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusObjectPath, QVariantMap> reply = *watcher;

    debug() << "Received reply to RequestChannel";

    if (!reply.isError()) {
        mPriv->objectPath = reply.argumentAt<0>();
        debug() << " Success: object path" << mPriv->objectPath.path();

        QVariantMap map = reply.argumentAt<1>();
        mPriv->channelType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        mPriv->handleType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")).toUInt();
        mPriv->handle = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle")).toUInt();

        setFinished();
    } else {
        debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingChannel::onCallEnsureChannelFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<bool, QDBusObjectPath, QVariantMap> reply = *watcher;

    debug() << "Received reply to RequestChannel";

    if (!reply.isError()) {
        mPriv->objectPath = reply.argumentAt<1>();
        debug() << " Success: object path" << mPriv->objectPath.path();

        QVariantMap map = reply.argumentAt<2>();
        mPriv->channelType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        mPriv->handleType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")).toUInt();
        mPriv->handle = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle")).toUInt();

        setFinished();
    } else {
        debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
