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
    : q_ptr(parent)
    , readinessHelper(parent->readinessHelper())
    , baseType(NoKnownType)
    , addressType(SocketAddressTypeUnix)
{
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

StreamTubeChannelPrivate::~StreamTubeChannelPrivate()
{
}

void StreamTubeChannelPrivate::__k__onConnectionClosed(
        uint connectionId,
        const QString& error,
        const QString& message)
{
    Q_Q(StreamTubeChannel);
    emit q->connectionClosed(connectionId, error, message);
}

void StreamTubeChannelPrivate::extractProperties(const QVariantMap& props)
{
    serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
    socketTypes = qdbus_cast<SupportedSocketMap>(props[QLatin1String("SupportedSocketTypes")]);
}

void StreamTubeChannelPrivate::__k__gotStreamTubeProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        extractProperties(props);
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
    : TubeChannel(connection, objectPath, immutableProperties, coreFeature),
      d_ptr(new StreamTubeChannelPrivate(this))
{
}

StreamTubeChannel::StreamTubeChannel(const ConnectionPtr& connection,
        const QString& objectPath,
        const QVariantMap& immutableProperties,
        const Feature &coreFeature,
        StreamTubeChannelPrivate& dd)
    : TubeChannel(connection, objectPath, immutableProperties, coreFeature)
    , d_ptr(&dd)
{
}


/**
 * Class destructor.
 */
StreamTubeChannel::~StreamTubeChannel()
{
    delete d_ptr;
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
 * \returns The IP protocols this stream tube is capable of handling.
 *
 * \note You will be able to call either IncomingStreamTubeChannel::acceptTubeAsTcpSocket or
 *       OutgoingStreamTubeChannel::offerTubeAsTcpSocket with an address belonging to one
 *       of the protocols returned.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsTcpSocket
 * \see OutgoingStreamTubeChannel::offerTubeAsTcpSocket
 * \see supportedTcpProtocolsWithAllowedAddress
 */
StreamTubeChannel::TcpProtocols StreamTubeChannel::supportedTcpProtocols() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportedTcpProtocols() used with "
            "FeatureStreamTube not ready";
        return NoProtocol;
    }

    Q_D(const StreamTubeChannel);

    TcpProtocols protocols = NoProtocol;
    if (d->socketTypes.contains(SocketAddressTypeIPv4)) {
        protocols |= StreamTubeChannel::IPv4Protocol;
    }
    if (d->socketTypes.contains(SocketAddressTypeIPv6)) {
        protocols |= StreamTubeChannel::IPv6Protocol;
    }
    return protocols;
}

/**
 * \returns The IP protocols this stream tube is capable of handling when specifying an allowed
 *          address for connecting to the socket.
 *
 * \note You will be able to call OutgoingStreamTubeChannel::offerTubeAsTcpSocket with an address
 *       and an allowed address+port belonging to one of the protocols returned.
 *
 * \see OutgoingStreamTubeChannel::offerTubeAsTcpSocket
 * \see supportedTcpProtocols
 */
StreamTubeChannel::TcpProtocols StreamTubeChannel::supportedTcpProtocolsWithAllowedAddress() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportedTcpProtocolsWithAllowedAddress() used with "
            "FeatureStreamTube not ready";
        return NoProtocol;
    }

    Q_D(const StreamTubeChannel);

    TcpProtocols protocols = NoProtocol;
    if (d->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlPort)) {
        protocols |= StreamTubeChannel::IPv4Protocol;
    }
    if (d->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlPort)) {
        protocols |= StreamTubeChannel::IPv6Protocol;
    }
    return protocols;
}

/**
 * \returns Whether this Stream tube supports offering or accepting it as an Unix socket.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingStreamTubeChannel::offerTubeAsUnixSocket
 * \see supportsUnixSocketsWithCredentials
 */
bool StreamTubeChannel::supportsUnixSockets() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsUnixSockets() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.contains(SocketAddressTypeUnix);
}

/**
 * \returns Whether this Stream tube supports offering or accepting it as an Unix socket and requiring
 *          credentials for connecting to it.
 *
 * \see IncomingStreamTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingStreamTubeChannel::offerTubeAsUnixSocket
 * \see supportsUnixSockets
 */
bool StreamTubeChannel::supportsUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsWithCredentials() used with "
            "FeatureStreamTube not ready";
        return false;
    }

    Q_D(const StreamTubeChannel);

    return d->socketTypes.value(SocketAddressTypeUnix).contains(SocketAccessControlCredentials);
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
QByteArray StreamTubeChannel::localAddress() const
{
    if (tubeState() != TubeChannelStateOpen) {
        return QByteArray();
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
