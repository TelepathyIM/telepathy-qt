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
    QVariantMap immutableProperties;
    ChannelPtr channel;
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
 * If the channel request has finished, return the handle type of the resulting
 * channel. Otherwise, return the handle type that was requested.
 *
 * (One example of a request producing a different target handle type is that
 * on protocols like MSN, one-to-one conversations don't really exist, and if
 * you request a text channel with handle type HandleTypeContact, what you
 * will actually get is a text channel with handle type HandleTypeNone, with
 * the requested contact as a member.)
 *
 * \return The handle type, as specified in #HandleType.
 */
uint PendingChannel::handleType() const
{
    return mPriv->handleType;
}

/**
 * If the channel request has finished, return the target handle of the
 * resulting channel. Otherwise, return the target handle that was requested
 * (which might be different in some situations - see handleType).
 *
 * \return The handle.
 */
uint PendingChannel::handle() const
{
    return mPriv->handle;
}

/**
 * If this channel request has finished, return the immutable properties of
 * the resulting channel. Otherwise, return an empty map.
 *
 * The keys and values in this map are defined by the Telepathy D-Bus API
 * specification, or by third-party extensions to that specification.
 * These are the properties that cannot change over the lifetime of the
 * channel; they're announced in the result of the request, for efficiency.
 * This map should be passed to the constructor of Channel or its subclasses
 * (such as TextChannel).
 *
 * These properties can also be used to process channels in a way that does
 * not require the creation of a Channel object - for instance, a
 * ChannelDispatcher implementation should be able to classify and process
 * channels based on their immutable properties, without needing to create
 * Channel objects.
 *
 * \return A map in which the keys are D-Bus property names and the values
 *         are the corresponding values.
 */
QVariantMap PendingChannel::immutableProperties() const
{
    return mPriv->immutableProperties;
}

/**
 * Returns a shared pointer to a Channel high-level proxy object associated
 * with the remote channel resulting from the channel request. If isValid()
 * returns <code>false</code>, the request has not (at least yet) completed
 * successfully, and a null ChannelPtr will be returned.
 *
 * \return Shared pointer to the new Channel object, 0 if an error occurred.
 */
ChannelPtr PendingChannel::channel() const
{
    if (!isFinished()) {
        warning() << "PendingChannel::channel called before finished, returning 0";
        return ChannelPtr();
    } else if (!isValid()) {
        warning() << "PendingChannel::channel called when not valid, returning 0";
        return ChannelPtr();
    }

    if (mPriv->channel) {
        return mPriv->channel;
    }

    if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) {
        mPriv->channel = ChannelPtr(
                new TextChannel(connection(), mPriv->objectPath.path(),
                    mPriv->immutableProperties));
    }
    else if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) {
        mPriv->channel = ChannelPtr(
                new StreamedMediaChannel(connection(), mPriv->objectPath.path(),
                    mPriv->immutableProperties));
    }
    else if (channelType() == TELEPATHY_INTERFACE_CHANNEL_TYPE_ROOM_LIST) {
        mPriv->channel = ChannelPtr(
                new RoomList(connection(), mPriv->objectPath.path(),
                    mPriv->immutableProperties));
    }
    // FIXME: update spec so we can do this properly
    else if (channelType() == "org.freedesktop.Telepathy.Channel.Type.FileTransfer") {
        mPriv->channel = ChannelPtr(
                new FileTransfer(connection(), mPriv->objectPath.path(),
                    mPriv->immutableProperties));
    }
    else {
        // ContactList, old-style Tubes, or a future channel type
        mPriv->channel = ChannelPtr(
                new Channel(connection(), mPriv->objectPath.path(),
                    mPriv->immutableProperties));
    }
    return mPriv->channel;
}

void PendingChannel::onCallCreateChannelFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath, QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        mPriv->objectPath = reply.argumentAt<0>();
        debug() << "Got reply to Connection.CreateChannel - object path:" <<
            mPriv->objectPath.path();

        QVariantMap map = reply.argumentAt<1>();
        mPriv->immutableProperties = map;
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
        mPriv->immutableProperties = map;
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
