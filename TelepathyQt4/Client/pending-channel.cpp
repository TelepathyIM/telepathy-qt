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
#include <TelepathyQt4/Client/FileTransfer>
#include <TelepathyQt4/Client/RoomList>
#include <TelepathyQt4/Client/StreamedMediaChannel>
#include <TelepathyQt4/Client/TextChannel>
#include <TelepathyQt4/Constants>

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

namespace Telepathy
{
namespace Client
{

struct PendingChannel::Private
{
    bool yours;
    QString channelType;
    uint handleType;
    uint handle;
    QDBusObjectPath objectPath;
};

/**
 * \class PendingChannel
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/Client/pending-channel.h> <TelepathyQt4/Client/PendingChannel>
 *
 * Class containing the parameters of and the reply to an asynchronous channel
 * request. Instances of this class cannot be constructed directly; the only way
 * to get one is trough Connection.
 */

/**
 * Construct a new PendingChannel object that will fail.
 *
 * \param connection Connection to use.
 * \param errorName The error name.
 * \param errorMessage The error message.
 */
PendingChannel::PendingChannel(Connection *connection, const QString &errorName,
        const QString &errorMessage)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->yours = false;
    mPriv->handleType = 0;
    mPriv->handle = 0;

    setFinishedWithError(errorName, errorMessage);
}

/**
 * Construct a new PendingChannel object.
 *
 * \param connection Connection to use.
 * \param request A dictionary containing the desirable properties.
 * \param create Whether createChannel or ensureChannel should be called.
 */
PendingChannel::PendingChannel(Connection *connection,
        const QVariantMap &request, bool create)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->yours = create;
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

/**
 * Class destructor.
 */
PendingChannel::~PendingChannel()
{
    delete mPriv;
}

/**
 * Return the Connection object through which the channel request was made.
 *
 * \return Pointer to the Connection.
 */
Connection *PendingChannel::connection() const
{
    return qobject_cast<Connection *>(parent());
}

/**
 * Return whether this channel belongs to this process.
 *
 * If false, the caller MUST assume that some other process is
 * handling this channel; if true, the caller SHOULD handle it
 * themselves or delegate it to another client.
 *
 * Note that the value is undefined until the operation finishes.
 *
 * \return Boolean indicating whether this channel belongs to this process.
 */
bool PendingChannel::yours() const
{
    if (!isFinished()) {
        warning() << "PendingChannel::yours called before finished, returning undefined value";
    }
    else if (!isValid()) {
        warning() << "PendingChannel::yours called when not valid, returning undefined value";
    }

    return mPriv->yours;
}

/**
 * Return the channel type specified in the channel request.
 *
 * \return The D-Bus interface name of the interface specific to the
 *         requested channel type.
 */
const QString &PendingChannel::channelType() const
{
    return mPriv->channelType;
}

/**
 * Return the handle type specified in the channel request.
 *
 * \return The handle type, as specified in #HandleType.
 */
uint PendingChannel::handleType() const
{
    return mPriv->handleType;
}

/**
 * Return the handle specified in the channel request.
 *
 * \return The handle.
 */
uint PendingChannel::handle() const
{
    return mPriv->handle;
}

/**
 * Returns a newly constructed Channel high-level proxy object associated
 * with the remote channel resulting from the channel request. If isValid()
 * returns <code>false</code>, the request has not (at least yet) completed
 * successfully, and 0 will be returned.
 *
 * \param parent Passed to the Channel constructor.
 * \return Pointer to the new Channel object, 0 if an error occurred.
 */
Channel *PendingChannel::channel(QObject *parent) const
{
    if (!isFinished()) {
        warning() << "PendingChannel::channel called before finished, returning 0";
        return 0;
    }
    else if (!isValid()) {
        warning() << "PendingChannel::channel called when not valid, returning 0";
        return 0;
    }

    Channel *channel;

    if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) {
        channel = new TextChannel(connection(), mPriv->objectPath.path(),
                parent);
    }
    else if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) {
        channel = new StreamedMediaChannel(connection(),
                mPriv->objectPath.path(), parent);
    }
    else if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_ROOM_LIST) {
        channel = new RoomList(connection(), mPriv->objectPath.path(),
                parent);
    }
    else if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER) {
        channel = new FileTransfer(connection(), mPriv->objectPath.path(),
                parent);
    }
    else {
        // ContactList, old-style Tubes, or a future channel type
        channel = new Channel(connection(), mPriv->objectPath.path(), parent);
    }
    return channel;
}

void PendingChannel::onCallCreateChannelFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath, QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        mPriv->objectPath = reply.argumentAt<0>();
        debug() << "Got reply to Connection.CreateChannel - object path:" <<
            mPriv->objectPath.path();

        QVariantMap map = reply.argumentAt<1>();
        mPriv->channelType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        mPriv->handleType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")).toUInt();
        mPriv->handle = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle")).toUInt();

        setFinished();
    } else {
        debug().nospace() << "CreateChannel failed:" <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingChannel::onCallEnsureChannelFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<bool, QDBusObjectPath, QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        mPriv->yours = reply.argumentAt<0>();

        mPriv->objectPath = reply.argumentAt<1>();
        debug() << "Got reply to Connection.EnsureChannel - object path:" <<
            mPriv->objectPath.path();

        QVariantMap map = reply.argumentAt<2>();
        mPriv->channelType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        mPriv->handleType = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")).toUInt();
        mPriv->handle = map.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle")).toUInt();

        setFinished();
    } else {
        debug().nospace() << "EnsureChannel failed:" <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
