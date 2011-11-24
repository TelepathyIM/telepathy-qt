/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @license LGPL 2.1
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

#include <TelepathyQt/StreamTubeChannel>

#include "TelepathyQt/_gen/stream-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingVariantMap>

#include <QHostAddress>

namespace Tp
{

struct TP_QT_NO_EXPORT StreamTubeChannel::Private
{
    Private(StreamTubeChannel *parent);

    static void introspectStreamTube(Private *self);
    static void introspectConnectionMonitoring(Private *self);

    void extractStreamTubeProperties(const QVariantMap &props);

    // Public object
    StreamTubeChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection
    SupportedSocketMap socketTypes;
    QString serviceName;

    QSet<uint> connections;
    QPair<QHostAddress, quint16> ipAddress;
    QString unixAddress;
    SocketAddressType addressType;
    SocketAccessControl accessControl;
    bool droppingConnections;
};

StreamTubeChannel::Private::Private(StreamTubeChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      addressType(SocketAddressTypeUnix),
      accessControl(SocketAccessControlLocalhost),
      droppingConnections(false)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableStreamTube(
            QSet<uint>() << 0,                                                      // makesSenseForStatuses
            Features() << TubeChannel::FeatureCore,                                 // dependsOnFeatures (core)
            QStringList(),                                                          // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &StreamTubeChannel::Private::introspectStreamTube,
            this);
    introspectables[StreamTubeChannel::FeatureCore] = introspectableStreamTube;

    ReadinessHelper::Introspectable introspectableConnectionMonitoring(
            QSet<uint>() << 0,                                                            // makesSenseForStatuses
            Features() << StreamTubeChannel::FeatureCore,                                 // dependsOnFeatures (core)
            QStringList(),                                                                // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc)
                    &StreamTubeChannel::Private::introspectConnectionMonitoring,
            this);
    introspectables[StreamTubeChannel::FeatureConnectionMonitoring] =
            introspectableConnectionMonitoring;

    parent->connect(
            parent,
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(dropConnections()));

    readinessHelper->addIntrospectables(introspectables);
}

void StreamTubeChannel::Private::introspectStreamTube(
        StreamTubeChannel::Private *self)
{
    StreamTubeChannel *parent = self->parent;

    debug() << "Introspecting stream tube properties";
    Client::ChannelTypeStreamTubeInterface *streamTubeInterface =
            parent->interface<Client::ChannelTypeStreamTubeInterface>();

    PendingVariantMap *pvm = streamTubeInterface->requestAllProperties();
    parent->connect(pvm,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(gotStreamTubeProperties(Tp::PendingOperation *)));
}

void StreamTubeChannel::Private::introspectConnectionMonitoring(
        StreamTubeChannel::Private *self)
{
    StreamTubeChannel *parent = self->parent;

    Client::ChannelTypeStreamTubeInterface *streamTubeInterface =
            parent->interface<Client::ChannelTypeStreamTubeInterface>();

    parent->connect(streamTubeInterface,
            SIGNAL(ConnectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));

    if (parent->isRequested()) {
        parent->connect(streamTubeInterface,
                SIGNAL(NewRemoteConnection(uint,QDBusVariant,uint)),
                SLOT(onNewRemoteConnection(uint,QDBusVariant,uint)));
    } else {
        parent->connect(streamTubeInterface,
                SIGNAL(NewLocalConnection(uint)),
                SLOT(onNewLocalConnection(uint)));
    }

    self->readinessHelper->setIntrospectCompleted(
            StreamTubeChannel::FeatureConnectionMonitoring, true);
}

void StreamTubeChannel::Private::extractStreamTubeProperties(const QVariantMap &props)
{
    serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
    socketTypes = qdbus_cast<SupportedSocketMap>(props[QLatin1String("SupportedSocketTypes")]);
}

/**
 * \class StreamTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/stream-tube-channel.h <TelepathyQt/StreamTubeChannel>
 *
 * \brief The StreamTubeChannel class represents a Telepathy channel of type StreamTube.
 *
 * It provides a transport for reliable and ordered data transfer, similar to SOCK_STREAM sockets.
 *
 * StreamTubeChannel is an intermediate base class; OutgoingStreamTubeChannel and
 * IncomingStreamTubeChannel are the specialized classes used for locally and remotely initiated
 * tubes respectively.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * StreamTubeChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * StreamTubeChannel methods.
 * See specific methods documentation for more details.
 */
const Feature StreamTubeChannel::FeatureCore =
        Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to monitor connections to this stream tube.
 *
 * See connection monitoring specific methods' documentation for more details.
 *
 * \sa newConnection(), connectionClosed()
 */
const Feature StreamTubeChannel::FeatureConnectionMonitoring =
        Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 1);

/**
 * Create a new StreamTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A StreamTubeChannelPtr object pointing to the newly created
 *         StreamTubeChannel object.
 */
StreamTubeChannelPtr StreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return StreamTubeChannelPtr(new StreamTubeChannel(connection, objectPath,
            immutableProperties, StreamTubeChannel::FeatureCore));
}

/**
 * Construct a new StreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on StreamTubeChannel::FeatureCore.
 */
StreamTubeChannel::StreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : TubeChannel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
StreamTubeChannel::~StreamTubeChannel()
{
    delete mPriv;
}

/**
 * Return the service name which will be used over this stream tube. This should be a
 * well-known TCP service name, for instance "rsync" or "daap".
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return The service name.
 */
QString StreamTubeChannel::service() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::service() used with "
                "FeatureCore not ready";
        return QString();
    }

    return mPriv->serviceName;
}

/**
 * Return whether this stream tube is capable to accept or offer an IPv4 socket accepting all
 * incoming connections coming from localhost.
 *
 * Note that the \telepathy_spec implies that any connection manager, if capable of providing
 * stream tubes, must at least support IPv4 sockets with localhost access control.
 * For this reason, this method should always return \c true.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept or offer an IPv4 socket
 *         accepting all incoming connections coming from localhost, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     supportsIPv4SocketsWithSpecifiedAddress()
 */
bool StreamTubeChannel::supportsIPv4SocketsOnLocalhost() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsIPv4SocketsOnLocalhost() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept an IPv4 socket accepting all
 * incoming connections coming from a specific address for incoming tubes or whether
 * this stream tube is capable of mapping connections to the socket's source address for outgoing
 * tubes.
 *
 * For incoming tubes, when this capability is available, the stream tube can be accepted specifying
 * an IPv4 address. Every connection coming from any other address than the specified one will be
 * rejected.
 *
 * For outgoing tubes, when this capability is available, one can keep track of incoming connections
 * by enabling StreamTubeChannel::FeatureConnectionMonitoring (possibly before
 * opening the stream tube itself), and checking OutgoingStreamTubeChannel::contactsForConnections()
 * or OutgoingStreamTubeChannel::connectionsForSourceAddresses().
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsTcpSocket() or
 * OutgoingStreamTubeChannel::offerTcpSocket() with a specified address to prevent failures,
 * as the spec implies this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept an IPv4 socket accepting all
 *         incoming connections coming from a specific address for incoming tubes or
 *         the stream tube is capable of mapping connections to the socket's source address for
 *         outgoing tubes, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     OutgoingStreamTubeChannel::connectionsForSourceAddresses(),
 *     OutgoingStreamTubeChannel::contactsForConnections(),
 *     supportsIPv4SocketsOnLocalhost()
 */
bool StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlPort);
}

/**
 * Return whether this stream tube is capable to accept or offer an IPv6 socket accepting all
 * incoming connections coming from localhost.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsTcpSocket() or
 * OutgoingStreamTubeChannel::offerTcpSocket() with a specified address to prevent failures,
 * as the spec implies this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept or offer an IPv6 socket
 *         accepting all incoming connections coming from localhost, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     supportsIPv6SocketsWithSpecifiedAddress()
 */
bool StreamTubeChannel::supportsIPv6SocketsOnLocalhost() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsIPv6SocketsOnLocalhost() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept an IPv6 socket accepting all
 * incoming connections coming from a specific address for incoming tubes or whether
 * this stream tube is capable of mapping connections to the socket's source address for outgoing
 * tubes.
 *
 * For incoming tubes, when this capability is available, the stream tube can be accepted specifying
 * an IPv6 address. Every connection coming from any other address than the specified one will be
 * rejected.
 *
 * For outgoing tubes, when this capability is available, one can keep track of incoming connections
 * by enabling StreamTubeChannel::FeatureConnectionMonitoring (possibly before
 * opening the stream tube itself), and checking OutgoingStreamTubeChannel::contactsForConnections()
 * or OutgoingStreamTubeChannel::connectionsForSourceAddresses().
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsTcpSocket() or
 * OutgoingStreamTubeChannel::offerTcpSocket() with a specified address to prevent failures,
 * as the spec implies this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept an IPv6 socket accepting all
 *         incoming connections coming from a specific address for incoming tubes or
 *         the stream tube is capable of mapping connections to the socket's source address for
 *         outgoing tubes, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     OutgoingStreamTubeChannel::connectionsForSourceAddresses(),
 *     OutgoingStreamTubeChannel::contactsForConnections(),
 *     supportsIPv6SocketsOnLocalhost()
 */
bool StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlPort);
}

/**
 * Return whether this stream tube is capable to accept or offer an Unix socket accepting all
 * incoming connections coming from localhost.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() without credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept or offer an Unix socket
 *         accepting all incoming connections coming from localhost, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsWithCredentials()
 *     supportsAbstractUnixSocketsOnLocalhost(),
 *     supportsAbstractUnixSocketsWithCredentials(),
 */
bool StreamTubeChannel::supportsUnixSocketsOnLocalhost() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsOnLocalhost() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeUnix).contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept or offer an Unix socket which will require
 * credentials upon connection.
 *
 * When this capability is available and enabled, the connecting process must send a byte when
 * it first connects, which is not considered to be part of the data stream.
 * If the operating system uses sendmsg() with SCM_CREDS or SCM_CREDENTIALS to pass
 * credentials over sockets, the connecting process must do so if possible;
 * if not, it must still send the byte.
 *
 * The listening process will disconnect the connection unless it can determine
 * by OS-specific means that the connecting process has the same user ID as the listening process.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() with credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept or offer an Unix socket
 *         which will require credentials upon connection, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsOnLocalhost(),
 *     supportsAbstractUnixSocketsOnLocalhost(),
 *     supportsAbstractUnixSocketsWithCredentials(),
 */
bool StreamTubeChannel::supportsUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsWithCredentials() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes[SocketAddressTypeUnix].contains(SocketAccessControlCredentials);
}

/**
 * Return whether this stream tube is capable to accept or offer an abstract Unix socket accepting
 * all incoming connections coming from localhost.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() without credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept or offer an abstract Unix socket
 *         accepting all incoming connections coming from localhost, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsOnLocalhost(),
 *     supportsUnixSocketsWithCredentials(),
 *     supportsAbstractUnixSocketsWithCredentials()
 */
bool StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes[SocketAddressTypeAbstractUnix].contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept or offer an abstract Unix socket which will
 * require credentials upon connection.
 *
 * When this capability is available and enabled, the connecting process must send a byte when
 * it first connects, which is not considered to be part of the data stream.
 * If the operating system uses sendmsg() with SCM_CREDS or SCM_CREDENTIALS to pass
 * credentials over sockets, the connecting process must do so if possible;
 * if not, it must still send the byte.
 *
 * The listening process will disconnect the connection unless it can determine
 * by OS-specific means that the connecting process has the same user ID as the listening process.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() with credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureCore to be ready.
 *
 * \return \c true if the stream tube is capable to accept or offer an abstract Unix socket
 *         which will require credentials upon connection, \c false otherwise.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsOnLocalhost(),
 *     supportsUnixSocketsWithCredentials(),
 *     supportsAbstractUnixSocketsOnLocalhost()
 */
bool StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureCore)) {
        warning() << "StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials() used with "
                "FeatureCore not ready";
        return false;
    }

    return mPriv->socketTypes[SocketAddressTypeAbstractUnix].contains(SocketAccessControlCredentials);
}

/**
 * Return all the known active connections since StreamTubeChannel::FeatureConnectionMonitoring has
 * been enabled.
 *
 * For this method to return all known connections, you need to make
 * StreamTubeChannel::FeatureConnectionMonitoring ready before accepting or offering the stream
 * tube.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be ready.
 *
 * \return The list of active connection ids.
 * \sa newConnection(), connectionClosed()
 */
QSet<uint> StreamTubeChannel::connections() const
{
    if (!isReady(FeatureConnectionMonitoring)) {
        warning() << "StreamTubeChannel::connections() used with "
                "FeatureConnectionMonitoring not ready";
        return QSet<uint>();
    }

    return mPriv->connections;
}

/**
 * Return the type of the tube's local endpoint socket.
 *
 * Note that this function will return a valid value only after state() has gone #TubeStateOpen.
 *
 * \return The socket type as #SocketAddressType.
 * \sa localAddress(), ipAddress()
 */
SocketAddressType StreamTubeChannel::addressType() const
{
    return mPriv->addressType;
}

/**
 * Return the access control used by this stream tube.
 *
 * Note that this function will only return a valid value after state() has gone #TubeStateOpen.
 *
 * \return The access control as #SocketAccessControl.
 * \sa addressType()
 */
SocketAccessControl StreamTubeChannel::accessControl() const
{
    return mPriv->accessControl;
}

/**
 * Return the IP address/port combination used by this stream tube.
 *
 * This method will return a meaningful value only if the local endpoint socket for the tube is a
 * TCP socket, i.e. addressType() is #SocketAddressTypeIPv4 or #SocketAddressTypeIPv6.
 *
 * Note that this function will return a valid value only after state() has gone #TubeStateOpen.
 *
 * \return Pair of IP address as QHostAddress and port if using a TCP socket,
 *         or an undefined value otherwise.
 * \sa localAddress()
 */
QPair<QHostAddress, quint16> StreamTubeChannel::ipAddress() const
{
    if (state() != TubeChannelStateOpen) {
        warning() << "Tube not open, returning invalid IP address";
        return qMakePair<QHostAddress, quint16>(QHostAddress::Null, 0);
    }

    return mPriv->ipAddress;
}

/**
 * Return the local address used by this stream tube.
 *
 * This method will return a meaningful value only if the local endpoint socket for the tube is an
 * UNIX socket, i.e. addressType() is #SocketAddressTypeUnix or #SocketAddressTypeAbstractUnix.
 *
 * Note that this function will return a valid value only after state() has gone #TubeStateOpen.
 *
 * \return Unix socket address if using an Unix socket,
 *         or an undefined value otherwise.
 * \sa ipAddress()
 */
QString StreamTubeChannel::localAddress() const
{
    if (state() != TubeChannelStateOpen) {
        warning() << "Tube not open, returning invalid local socket address";
        return QString();
    }

    return mPriv->unixAddress;
}

void StreamTubeChannel::addConnection(uint connection)
{
    if (!mPriv->connections.contains(connection)) {
        mPriv->connections.insert(connection);
        emit newConnection(connection);
    } else {
        warning() << "Tried to add connection" << connection << "on StreamTube" << objectPath()
            << "but it already was there";
    }
}

void StreamTubeChannel::removeConnection(uint connection, const QString &error,
        const QString &message)
{
    if (mPriv->connections.contains(connection)) {
        mPriv->connections.remove(connection);
        emit connectionClosed(connection, error, message);
    } else {
        warning() << "Tried to remove connection" << connection << "from StreamTube" << objectPath()
            << "but it wasn't there";
    }
}

void StreamTubeChannel::setAddressType(SocketAddressType type)
{
    mPriv->addressType = type;
}

void StreamTubeChannel::setAccessControl(SocketAccessControl accessControl)
{
    mPriv->accessControl = accessControl;
}

void StreamTubeChannel::setIpAddress(const QPair<QHostAddress, quint16> &address)
{
    mPriv->ipAddress = address;
}

void StreamTubeChannel::setLocalAddress(const QString &address)
{
    mPriv->unixAddress = address;
}

bool StreamTubeChannel::isDroppingConnections() const
{
    return mPriv->droppingConnections;
}

void StreamTubeChannel::gotStreamTubeProperties(PendingOperation *op)
{
    if (!op->isError()) {
        PendingVariantMap *pvm = qobject_cast<PendingVariantMap *>(op);

        mPriv->extractStreamTubeProperties(pvm->result());

        debug() << "Got reply to Properties::GetAll(StreamTubeChannel)";
        mPriv->readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureCore, true);
    }
    else {
        warning().nospace() << "Properties::GetAll(StreamTubeChannel) failed "
                "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureCore, false,
                op->errorName(), op->errorMessage());
    }
}

void StreamTubeChannel::onConnectionClosed(uint connId, const QString &error,
        const QString &message)
{
    removeConnection(connId, error, message);
}

void StreamTubeChannel::dropConnections()
{
    if (!mPriv->connections.isEmpty()) {
        debug() << "StreamTubeChannel invalidated with" << mPriv->connections.size()
            << "connections remaining, synthesizing close events";
        mPriv->droppingConnections = true;
        foreach (uint connId, mPriv->connections) {
            removeConnection(connId, TP_QT_ERROR_ORPHANED,
                    QLatin1String("parent tube invalidated, streams closing"));
        }
        mPriv->droppingConnections = false;
    }
}

/**
 * \fn void StreamTubeChannel::connectionClosed(uint connectionId,
 *             const QString &errorName, const QString &errorMessage)
 *
 * Emitted when a connection on this stream tube has been closed.
 *
 * \param connectionId The unique ID associated with the connection that was closed.
 * \param errorName The name of a D-Bus error describing the error that occurred.
 * \param errorMessage A debugging message associated with the error.
 * \sa newConnection(), connections()
 */

} // Tp
