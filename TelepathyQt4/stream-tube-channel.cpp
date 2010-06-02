/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "TelepathyQt4/stream-tube-channel-internal.h"

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

StreamTubeChannelPrivate::StreamTubeChannelPrivate(StreamTubeChannel *parent)
    : TubeChannelPrivate(parent)
    , baseType(NoKnownType)
    , addressType(SocketAddressTypeUnix)
{
}

StreamTubeChannelPrivate::~StreamTubeChannelPrivate()
{
}

void StreamTubeChannelPrivate::init()
{
    TubeChannelPrivate::init();

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableStreamTube(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << TubeChannel::FeatureTube,                                 // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &StreamTubeChannelPrivate::introspectStreamTube,
        this);
    introspectables[StreamTubeChannel::FeatureStreamTube] = introspectableStreamTube;

    ReadinessHelper::Introspectable introspectableConnectionMonitoring(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << StreamTubeChannel::FeatureStreamTube,                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &StreamTubeChannelPrivate::introspectConnectionMonitoring,
        this);
    introspectables[StreamTubeChannel::FeatureConnectionMonitoring] = introspectableConnectionMonitoring;

    readinessHelper->addIntrospectables(introspectables);
}

void StreamTubeChannelPrivate::__k__onConnectionClosed(
        uint connectionId,
        const QString& error,
        const QString& message)
{
    Q_Q(StreamTubeChannel);
    emit q->connectionClosed(connectionId, error, message);
}

void StreamTubeChannelPrivate::extractStreamTubeProperties(const QVariantMap& props)
{
    serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
    socketTypes = qdbus_cast<SupportedSocketMap>(props[QLatin1String("SupportedSocketTypes")]);
}

void StreamTubeChannelPrivate::__k__gotStreamTubeProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        extractStreamTubeProperties(props);
        debug() << "Got reply to Properties::GetAll(StreamTubeChannel)";
        readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureStreamTube, true);
    }
    else {
        warning().nospace() << "Properties::GetAll(StreamTubeChannel) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureStreamTube, false,
                reply.error());
    }
}

void StreamTubeChannelPrivate::introspectConnectionMonitoring(
        StreamTubeChannelPrivate* self)
{
    StreamTubeChannel *parent = self->q_func();

    Client::ChannelTypeStreamTubeInterface *streamTubeInterface =
            parent->interface<Client::ChannelTypeStreamTubeInterface>();

    // It must be present
    Q_ASSERT(streamTubeInterface);

    parent->connect(streamTubeInterface, SIGNAL(ConnectionClosed(uint,QString,QString)),
                    parent, SLOT(__k__onConnectionClosed(uint,QString,QString)));

    // Depending on the base type given by the inheriter, let's connect to some additional signals
    if (self->baseType == OutgoingTubeType) {
        parent->connect(streamTubeInterface, SIGNAL(NewRemoteConnection(uint,QDBusVariant,uint)),
                        parent, SLOT(__k__onNewRemoteConnection(uint,QDBusVariant,uint)));
    } else if (self->baseType == IncomingTubeType) {
        parent->connect(streamTubeInterface, SIGNAL(NewLocalConnection(uint)),
                        parent, SLOT(__k__onNewLocalConnection(uint)));
    }

    self->readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureConnectionMonitoring, true);
}

void StreamTubeChannelPrivate::introspectStreamTube(
        StreamTubeChannelPrivate* self)
{
    StreamTubeChannel *parent = self->q_func();

    debug() << "Introspect stream tube properties";

    Client::DBus::PropertiesInterface *properties =
            parent->interface<Client::DBus::PropertiesInterface>();

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE)),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            parent,
            SLOT(__k__gotStreamTubeProperties(QDBusPendingCallWatcher *)));
}

/**
 * \class StreamTubeChannel
 * \headerfile TelepathyQt4/stream-tube-channel.h <TelepathyQt4/StreamTubeChannel>
 *
 * \brief A class representing a Stream Tube
 *
 * \c StreamTubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Type.StreamTubeChannel.
 * It provides a transport for reliable and ordered data transfer, similar to SOCK_STREAM sockets.
 *
 * For more details, please refer to Telepathy spec.
 */

// Features declaration and documentation
/**
 * Feature representing the core that needs to become ready to make the
 * StreamTubeChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * StreamTubeChannel methods.
 * See specific methods documentation for more details.
 */
const Feature StreamTubeChannel::FeatureStreamTube = Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 0);
/**
 * Feature used in order to monitor connections to this tube.
 *
 * %connectionClosed will be emitted when an existing connection gets closed
 */
const Feature StreamTubeChannel::FeatureConnectionMonitoring = Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 1);

// Signals documentation
/**
 * \fn void StreamTubeChannel::connectionClosed(uint connectionId, const QString &error, const QString &message)
 *
 * Emitted when a connection has been closed.
 *
 * \param connectionId The unique ID associated with this connection.
 * \param error The error occurred
 * \param message A debug message
 */

/**
 * Create a new StreamTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A StreamTubeChannelPtr object pointing to the newly created
 *         StreamTubeChannel object.
 */
StreamTubeChannelPtr StreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return StreamTubeChannelPtr(new StreamTubeChannel(connection, objectPath,
                immutableProperties, StreamTubeChannel::FeatureStreamTube));
}

/**
 * Construct a new StreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
StreamTubeChannel::StreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : TubeChannel(connection, objectPath, immutableProperties,
            coreFeature, *new StreamTubeChannelPrivate(this))
{
}

StreamTubeChannel::StreamTubeChannel(const ConnectionPtr& connection,
        const QString& objectPath,
        const QVariantMap& immutableProperties,
        const Feature &coreFeature,
        StreamTubeChannelPrivate& dd)
    : TubeChannel(connection, objectPath, immutableProperties, coreFeature, dd)
{
}


/**
 * Class destructor.
 */
StreamTubeChannel::~StreamTubeChannel()
{
}

/**
 * \returns the service name that will be used over the tube
 */
QString StreamTubeChannel::service() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::service() used with "
            "FeatureStreamTube not ready";
        return QString();
    }

    Q_D(const StreamTubeChannel);

    return d->serviceName;
}


/**
 * \returns Whether this stream tube is capable to accept or offer an IPv4 socket
 *          accepting all incoming connections coming from localhost.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsTcpSocket
 * \see OutgoingStreamTubeChannel::offerTcpSocket
 * \see supportsIPv4SocketsWithAllowedAddress
 */
bool StreamTubeChannel::supportsIPv4SocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportedIPv4SocketsOnLocalhost() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlLocalhost);
}


/**
 * \returns Whether this stream tube is capable to accept or offer an IPv4 socket
 *          when specifying an allowed address for connecting to the socket.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsTcpSocket
 * \see OutgoingStreamTubeChannel::offerTcpSocket
 * \see supportsIPv4SocketsOnLocalhost
 */
bool StreamTubeChannel::supportsIPv4SocketsWithAllowedAddress() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv4SocketsWithAllowedAddress() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlPort);
}


/**
 * \returns Whether this stream tube is capable to accept or offer an IPv6 socket
 *          accepting all incoming connections coming from localhost.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsTcpSocket
 * \see OutgoingStreamTubeChannel::offerTcpSocket
 * \see supportsIPv6SocketsWithAllowedAddress
 */
bool StreamTubeChannel::supportsIPv6SocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv6SocketsOnLocalhost() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlLocalhost);
}


/**
 * \returns Whether this stream tube is capable to accept or offer an IPv6 socket
 *          when specifying an allowed address for connecting to the socket.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsTcpSocket
 * \see OutgoingStreamTubeChannel::offerTcpSocket
 * \see supportsIPv6SocketsOnLocalhost
 */
bool StreamTubeChannel::supportsIPv6SocketsWithAllowedAddress() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv6SocketsWithAllowedAddress() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlPort);
}


/**
 * \returns Whether this stream tube is capable to accept or offer an Unix socket
 *          accepting all incoming connections coming from localhost.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingStreamTubeChannel::offerUnixSocket
 * \see supportsUnixSocketsWithCredentials
 */
bool StreamTubeChannel::supportsUnixSocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsOnLocalhost() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeUnix).contains(SocketAccessControlLocalhost);
}

/**
 * \returns Whether this stream tube is capable to accept or offer an Unix socket
 *          requiring credentials for connecting to it.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingStreamTubeChannel::offerUnixSocket
 * \see supportsUnixSocketsOnLocalhost
 */
bool StreamTubeChannel::supportsUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsWithCredentials() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeAbstractUnix).contains(SocketAccessControlCredentials);
}


/**
 * \returns Whether this stream tube is capable to accept or offer an Abstract Unix socket
 *          accepting all incoming connections coming from localhost.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingStreamTubeChannel::offerUnixSocket
 * \see supportsUnixSocketsWithCredentials
 */
bool StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeAbstractUnix).contains(SocketAccessControlLocalhost);
}


/**
 * \returns Whether this Stream tube supports offering or accepting it as an
 *          Abstract Unix socket and requiring credentials for connecting to it.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingStreamTubeChannel::offerUnixSocket
 * \see supportsUnixSockets
 */
bool StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeAbstractUnix).contains(SocketAccessControlCredentials);
}


void StreamTubeChannel::connectNotify(const char* signal)
{
    if (QLatin1String(signal) == SIGNAL(connectionClosed(uint,QString,QString)) &&
        !isReady(FeatureConnectionMonitoring)) {
        warning() << "Connected to the signal connectionClosed, but FeatureConnectionMonitoring is "
            "not ready. The signal won't be emitted until the mentioned feature is ready.";
    }

    TubeChannel::connectNotify(signal);
}

/**
 * This function returns all the known active connections since FeatureConnectionMonitoring has
 * been enabled. For this method to return all known connections, you need to make
 * FeatureConnectionMonitoring ready before accepting or offering the tube.
 *
 * \returns A list of active connection ids known to this tube
 */
UIntList StreamTubeChannel::connections() const
{
    if (!isReady(FeatureConnectionMonitoring)) {
        warning() << "StreamTubeChannel::connections() used with "
            "FeatureConnectionMonitoring not ready";
        return UIntList();
    }

    Q_D(const StreamTubeChannel);

    return d->connections;
}


/**
 * \returns The local address used by this StreamTube as a QByteArray, if this tube is using
 *          a SocketAddressTypeUnix or SocketAddressTypeAbstractUnix.
 *
 * \note This function will return a valid value only after the tube has been opened
 *
 * \see addressType
 */
QString StreamTubeChannel::localAddress() const
{
    if (tubeState() != TubeChannelStateOpen) {
        return QString();
    }

    Q_D(const StreamTubeChannel);

    return d->unixAddress;
}


/**
 * \returns The IP address and port used by this StreamTube as a QHostAddress, if this tube is using
 *          a SocketAddressTypeIPv4 or SocketAddressTypeIPv6.
 *
 * \note This function will return a valid value only after the tube has been opened
 *
 * \see addressType
 */
QPair< QHostAddress, quint16 > StreamTubeChannel::ipAddress() const
{
    if (tubeState() != TubeChannelStateOpen) {
        return qMakePair< QHostAddress, quint16 >(QHostAddress::Null, 0);
    }

    Q_D(const StreamTubeChannel);

    return d->ipAddress;
}


/**
 * \returns The type of socket this StreamTube is using
 *
 * \note This function will return a valid value only after the tube has been opened
 *
 * \see localAddress
 * \see tcpAddress
 */
SocketAddressType StreamTubeChannel::addressType() const
{
    Q_D(const StreamTubeChannel);

    return d->addressType;
}

}

#include "TelepathyQt4/_gen/stream-tube-channel.moc.hpp"
