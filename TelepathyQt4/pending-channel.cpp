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

#include <TelepathyQt4/PendingChannel>

#include "TelepathyQt4/_gen/pending-channel.moc.hpp"

#include "TelepathyQt4/channel-factory.h"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Constants>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingChannel::Private
{
    Private(const ConnectionPtr &connection) :
        connection(connection)
    {
    }

    WeakPtr<Connection> connection;
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
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/pending-channel.h <TelepathyQt4/PendingChannel>
 *
 * \brief Class containing the parameters of and the reply to an asynchronous
 * channel request. Instances of this class cannot be constructed directly; the
 * only way to get one is trough Connection.
 */

/**
 * Construct a new PendingChannel object that will fail.
 *
 * \param connection Connection to use.
 * \param errorName The error name.
 * \param errorMessage The error message.
 */
PendingChannel::PendingChannel(const ConnectionPtr &connection, const QString &errorName,
        const QString &errorMessage)
    : PendingOperation(connection.data()),
      mPriv(new Private(connection))
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
PendingChannel::PendingChannel(const ConnectionPtr &connection,
        const QVariantMap &request, bool create)
    : PendingOperation(connection.data()),
      mPriv(new Private(connection))
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
ConnectionPtr PendingChannel::connection() const
{
    return ConnectionPtr(mPriv->connection);
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
uint PendingChannel::targetHandleType() const
{
    return mPriv->handleType;
}

/**
 * If the channel request has finished, return the target handle of the
 * resulting channel. Otherwise, return the target handle that was requested
 * (which might be different in some situations - see targetHandleType).
 *
 * \return The handle.
 */
uint PendingChannel::targetHandle() const
{
    return mPriv->handle;
}

/**
 * If this channel request has finished, return the immutable properties of
 * the resulting channel. Otherwise, return an empty map.
 *
 * The keys and values in this map are defined by the Telepathy D-Bus
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
    QVariantMap props = mPriv->immutableProperties;

    // This is a reasonable guess - if it's Yours it's guaranteedly Requested by us, and if it's not
    // it could be either Requested by somebody else but also an incoming channel just as well.
    if (!props.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested"))) {
        debug() << "CM didn't provide Requested in channel immutable props, guessing"
            << mPriv->yours;
        props[QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")] =
            mPriv->yours;
    }

    // Also, the spec says that if the channel was Requested by the local user, InitiatorHandle must
    // be the Connection's self handle
    if (!props.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitiatorHandle"))) {
        if (qdbus_cast<bool>(props.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")))) {
            ConnectionPtr conn(mPriv->connection);
            if (conn && conn->isReady()) {
                debug() << "CM didn't provide InitiatorHandle in channel immutable props, but we "
                    "know it's the conn's self handle (and have it)";
                props[QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitiatorHandle")] =
                    conn->selfHandle();
            }
        }
    }

    return props;
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

    SharedPtr<Connection> conn(mPriv->connection);
    mPriv->channel = ChannelFactory::create(conn,
            mPriv->objectPath.path(), immutableProperties());
    return mPriv->channel;
}

/**
 * Returns the channel object path or an empty string on error.
 *
 * This method is useful for creating custom Channel objects, so instead of using
 * PendingChannel::channel, one could construct a new custom channel with
 * the object path.
 *
 * \return Channel object path.
 */
QString PendingChannel::objectPath() const
{
    if (!isFinished()) {
        warning() << "PendingChannel::channel called before finished";
    } else if (!isValid()) {
        warning() << "PendingChannel::channel called when not valid";
    }

    return mPriv->objectPath.path();
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

} // Tp
