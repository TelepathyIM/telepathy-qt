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

#include "TelepathyQt4/outgoing-stream-tube-channel-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/Types>

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/types-internal.h"

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QLocalServer>

namespace Tp
{

PendingOpenTubePrivate::PendingOpenTubePrivate(const QVariantMap &parameters, PendingOpenTube* parent)
    : parent(parent)
    , parameters(parameters)
{

}

PendingOpenTubePrivate::~PendingOpenTubePrivate()
{

}

PendingOpenTube::PendingOpenTube(
        PendingVoid* offerOperation,
        const QVariantMap &parameters,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new PendingOpenTubePrivate(parameters, this))
{
    mPriv->tube = OutgoingStreamTubeChannelPtr::dynamicCast(object);

    if (offerOperation->isFinished()) {
        mPriv->onOfferFinished(offerOperation);
    } else {
        // Connect the pending void
        connect(offerOperation, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onOfferFinished(Tp::PendingOperation*)));
    }
}

PendingOpenTube::~PendingOpenTube()
{
    delete mPriv;
}

void PendingOpenTubePrivate::onOfferFinished(PendingOperation* op)
{
    if (op->isError()) {
        // Fail
        parent->setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Offer tube finished successfully";

    // It might have been already opened - check
    if (tube->tubeState() == TubeChannelStateOpen) {
        onTubeStateChanged(tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        parent->connect(tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                        parent, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingOpenTubePrivate::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // Inject the parameters into the tube
        tube->d_func()->parameters = parameters;
        // The tube is ready: let's notify
        parent->setFinished();
    } else if (state != TubeChannelStateRemotePending) {
        // Something happened
        parent->setFinishedWithError(QLatin1String("Connection refused"),
                QLatin1String("The connection to this tube was refused"));
    }
}


OutgoingStreamTubeChannelPrivate::OutgoingStreamTubeChannelPrivate(OutgoingStreamTubeChannel *parent)
    : StreamTubeChannelPrivate(parent)
{
    baseType = OutgoingTubeType;
}

OutgoingStreamTubeChannelPrivate::~OutgoingStreamTubeChannelPrivate()
{
}

void OutgoingStreamTubeChannelPrivate::onNewRemoteConnection(
        uint contactId,
        const QDBusVariant &parameter,
        uint connectionId)
{
    // Request the handles from our queued contact factory
    QUuid uuid = queuedContactFactory->appendNewRequest(UIntList() << contactId);

    // Add a pending connection
    pendingNewConnections.insert(uuid, qMakePair(connectionId, parameter));
}

void OutgoingStreamTubeChannelPrivate::onContactsRetrieved(
        const QUuid &uuid,
        const QList< Tp::ContactPtr > &contacts)
{
    // Retrieve our hash
    if (!pendingNewConnections.contains(uuid)) {
        warning() << "Contacts retrieved but no pending connections were found";
        return;
    }

    QPair< uint, QDBusVariant > connectionProperties = pendingNewConnections.take(uuid);

    // Add the connection to our list
    connections << connectionProperties.first;

    // Add it to our connections hash
    foreach (const Tp::ContactPtr &contact, contacts) {
        contactsForConnections.insertMulti(connectionProperties.first, contact);
    }

    QPair< QHostAddress, quint16 > address;
    address.first = QHostAddress::Null;

    // Now let's try to track the parameter
    if (addressType == SocketAddressTypeIPv4) {
        // Try a qdbus_cast to our address struct: we're shielded from crashes due to our specification
        SocketAddressIPv4 addr = qdbus_cast< Tp::SocketAddressIPv4 >(connectionProperties.second.variant());
        address.first = QHostAddress(addr.address);
        address.second = addr.port;
    } else if (addressType == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr = qdbus_cast< Tp::SocketAddressIPv6 >(connectionProperties.second.variant());
        address.first = QHostAddress(addr.address);
        address.second = addr.port;
    }

    if (address.first != QHostAddress::Null) {
        // We can map it to a source address as well
        connectionsForSourceAddresses.insertMulti(address, connectionProperties.first);
    }

    // Time for us to emit the signal
    Q_Q(OutgoingStreamTubeChannel);
    emit q->newConnection(connectionProperties.first);
}

void OutgoingStreamTubeChannelPrivate::onConnectionClosed(
        uint connectionId,
        const QString&,
        const QString&)
{
    // Remove stuff from our hashes
    contactsForConnections.remove(connectionId);

    QHash< QPair< QHostAddress, quint16 >, uint >::iterator i = connectionsForSourceAddresses.begin();

    while (i != connectionsForSourceAddresses.end()) {
        if (i.value() == connectionId) {
            i = connectionsForSourceAddresses.erase(i);
        } else {
            ++i;
        }
    }
}

/**
 * \class OutgoingStreamTubeChannel
 * \headerfile TelepathyQt4/stream-tube.h <TelepathyQt4/OutgoingStreamTubeChannel>
 *
 * \brief A class representing a Stream Tube
 *
 * \c OutgoingStreamTubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Type.StreamTubeChannel.
 * In particular, this class is meant to be used as a comfortable way for exposing new tubes.
 * It provides a set of overloads for exporting a variety of sockets over a stream tube.
 *
 * For more details, please refer to Telepathy spec.
 */


// Signals documentation
/**
 * \fn void StreamTubeChannel::newRemoteConnection(uint connectionId)
 *
 * Emitted when a new participant opens a connection to this tube
 *
 * \param connectionId The unique ID associated with this connection.
 */


/**
 * Create a new OutgoingStreamTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A OutgoingStreamTubeChannelPtr object pointing to the newly created
 *         OutgoingStreamTubeChannel object.
 */
OutgoingStreamTubeChannelPtr OutgoingStreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return OutgoingStreamTubeChannelPtr(new OutgoingStreamTubeChannel(connection, objectPath,
                immutableProperties, StreamTubeChannel::FeatureStreamTube));
}

/**
 * Construct a new OutgoingStreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
OutgoingStreamTubeChannel::OutgoingStreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : StreamTubeChannel(connection, objectPath, immutableProperties,
                        coreFeature, *new OutgoingStreamTubeChannelPrivate(this))
{
    Q_D(OutgoingStreamTubeChannel);

    connect(this, SIGNAL(connectionClosed(uint,QString,QString)),
            this, SLOT(onConnectionClosed(uint,QString,QString)));
    connect(d->queuedContactFactory, SIGNAL(contactsRetrieved(QUuid,QList<Tp::ContactPtr>)),
            this, SLOT(onContactsRetrieved(QUuid,QList<Tp::ContactPtr>)));
}

/**
 * Class destructor.
 */
OutgoingStreamTubeChannel::~OutgoingStreamTubeChannel()
{
}

/**
 * \brief Offer an IPv4/IPv6 socket over the tube
 *
 * This method offers an IPv4/IPv6 socket over this tube. The socket is represented through
 * a QHostAddress. If you are already handling a Tcp logic in your application, you can also
 * use an overload which accepts a QTcpServer.
 *
 * The %PendingOperation returned by this method will be completed as soon as the tube is
 * open and ready to be used.
 *
 * \param address A valid IPv4 or IPv6 address pointing to an existing socket
 * \param port The port the socket is listening for connections to
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 *
 * \returns A %PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \note The library will try to use Port access control whenever possible, as it allows to
 *       map connections to the socket's source address.
 *
 * \see StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress
 * \see StreamTubeChannel::supportsIPv4SocketsOnLocalhost
 * \see StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress
 * \see StreamTubeChannel::supportsIPv6SocketsOnLocalhost
 */
PendingOperation* OutgoingStreamTubeChannel::offerTcpSocket(
        const QHostAddress& address,
        quint16 port,
        const QVariantMap& parameters)
{
    if (!isReady(StreamTubeChannel::FeatureStreamTube)) {
        warning() << "StreamTubeChannel::FeatureStreamTube must be ready before "
                "calling offerTube";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                OutgoingStreamTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (tubeState() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a socket for each Stream Tube";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"),
                OutgoingStreamTubeChannelPtr(this));
    }

    SocketAccessControl accessControl = SocketAccessControlLocalhost;
    // Check if port is supported

    Q_D(OutgoingStreamTubeChannel);

    // In this specific overload, we're handling an IPv4/IPv6 socket
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        // IPv4 case
        SocketAccessControl accessControl;
        // Do some heuristics to find out the best access control. We always prefer port for tracking
        // connections and source addresses.
        if (supportsIPv4SocketsWithSpecifiedAddress()) {
            accessControl = SocketAccessControlPort;
        } else if (supportsIPv4SocketsOnLocalhost()) {
            accessControl = SocketAccessControlLocalhost;
        } else {
            // There are no combinations supported for this socket
            warning() << "You requested an address type/access control combination "
                    "not supported by this channel";
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        d->addressType = SocketAddressTypeIPv4;
        d->ipAddress = qMakePair< QHostAddress, quint16 >(address, port);

        SocketAddressIPv4 addr;
        addr.address = address.toString();
        addr.port = port;

        PendingVoid *pv = new PendingVoid(
                interface<Client::ChannelTypeStreamTubeInterface>()->Offer(
                        SocketAddressTypeIPv4,
                        QDBusVariant(QVariant::fromValue(addr)),
                        accessControl,
                        parameters),
                OutgoingStreamTubeChannelPtr(this));
        PendingOpenTube *op = new PendingOpenTube(pv, parameters,
                OutgoingStreamTubeChannelPtr(this));
        return op;
    } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        // IPv6 case
        // Do some heuristics to find out the best access control. We always prefer port for tracking
        // connections and source addresses.
        if (supportsIPv6SocketsWithSpecifiedAddress()) {
            accessControl = SocketAccessControlPort;
        } else if (supportsIPv6SocketsOnLocalhost()) {
            accessControl = SocketAccessControlLocalhost;
        } else {
            // There are no combinations supported for this socket
            warning() << "You requested an address type/access control combination "
                    "not supported by this channel";
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        d->addressType = SocketAddressTypeIPv6;
        d->ipAddress = qMakePair< QHostAddress, quint16 >(address, port);

        SocketAddressIPv6 addr;
        addr.address = address.toString();
        addr.port = port;

        PendingVoid *pv = new PendingVoid(
                interface<Client::ChannelTypeStreamTubeInterface>()->Offer(
                        SocketAddressTypeIPv6,
                        QDBusVariant(QVariant::fromValue(addr)),
                        accessControl,
                        parameters),
                OutgoingStreamTubeChannelPtr(this));
        PendingOpenTube *op = new PendingOpenTube(pv, parameters,
                OutgoingStreamTubeChannelPtr(this));
        return op;
    } else {
        // We're handling an IPv4/IPv6 socket only
        warning() << "offerTube can be called only with a QHostAddress representing "
                "an IPv4 or IPv6 address";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("Invalid host given"),
                OutgoingStreamTubeChannelPtr(this));
    }

}

/**
 * \overload
 * \brief Offer an IPv4/IPv6 socket over the tube
 *
 * This method offers an IPv4/IPv6 socket over this tube through a QTcpServer.
 *
 * The %PendingOperation returned by this method will be completed as soon as the tube is
 * open and ready to be used.
 *
 * \param server A valid QTcpServer already listening
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 *
 * \returns A %PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \note The library will try to use Port access control whenever possible, as it allows to
 *       map connections to the socket's source address.
 *
 * \see StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress
 * \see StreamTubeChannel::supportsIPv4SocketsOnLocalhost
 * \see StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress
 * \see StreamTubeChannel::supportsIPv6SocketsOnLocalhost
 */
PendingOperation* OutgoingStreamTubeChannel::offerTcpSocket(
        QTcpServer* server,
        const QVariantMap& parameters)
{
    // In this overload, we're handling a superset of QHostAddress.
    // Let's redirect the call to QHostAddress's overload
    return offerTcpSocket(server->serverAddress(), server->serverPort(),
            parameters);
}


/**
 * \brief Offer a Unix socket over the tube
 *
 * This method offers a Unix socket over this tube. The socket is represented through
 * a QString, which should contain the path to the socket. You can also expose an
 * abstract Unix socket, by including the leading null byte in the address
 *
 * If you are already handling a local socket logic in your application, you can also
 * use an overload which accepts a QLocalServer.
 *
 * The %PendingOperation returned by this method will be completed as soon as the tube is
 * open and ready to be used.
 *
 * \param address A valid path to an existing Unix socket or abstract Unix socket
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \returns A %PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \see StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost
 * \see StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials
 * \see StreamTubeChannel::supportsUnixSocketsOnLocalhost
 * \see StreamTubeChannel::supportsUnixSocketsWithCredentials
 */
PendingOperation* OutgoingStreamTubeChannel::offerUnixSocket(
        const QString& socketAddress,
        const QVariantMap& parameters,
        bool requireCredentials)
{
    SocketAccessControl accessControl = requireCredentials ?
            SocketAccessControlCredentials :
            SocketAccessControlLocalhost;

    if (!isReady(StreamTubeChannel::FeatureStreamTube)) {
        warning() << "StreamTubeChannel::FeatureStreamTube must be ready before "
                "calling offerTube";
    return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                OutgoingStreamTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (tubeState() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a socket for each Stream Tube";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"), OutgoingStreamTubeChannelPtr(this));
    }

    Q_D(OutgoingStreamTubeChannel);

    // In this specific overload, we're handling an Unix/AbstractUnix socket
    if (socketAddress.startsWith(QLatin1Char('\0'))) {
        // Abstract Unix socket case
        // Check if the combination type/access control is supported
        if ( (accessControl == SocketAccessControlLocalhost && !supportsAbstractUnixSocketsOnLocalhost()) ||
             (accessControl == SocketAccessControlCredentials && !supportsAbstractUnixSocketsWithCredentials()) ) {
            warning() << "You requested an address type/access control combination "
                    "not supported by this channel";
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        d->addressType = SocketAddressTypeAbstractUnix;
        d->unixAddress = socketAddress;

        PendingVoid *pv = new PendingVoid(
                interface<Client::ChannelTypeStreamTubeInterface>()->Offer(
                        SocketAddressTypeAbstractUnix,
                        QDBusVariant(QVariant(socketAddress.toLatin1())),
                        accessControl,
                        parameters),
                OutgoingStreamTubeChannelPtr(this));
        PendingOpenTube *op = new PendingOpenTube(pv, parameters,
                OutgoingStreamTubeChannelPtr(this));
        return op;
    } else {
        // Unix socket case
        // Check if the combination type/access control is supported
        if ( (accessControl == SocketAccessControlLocalhost && !supportsUnixSocketsOnLocalhost()) ||
             (accessControl == SocketAccessControlCredentials && !supportsUnixSocketsWithCredentials()) ||
             (accessControl != SocketAccessControlLocalhost && accessControl != SocketAccessControlCredentials) ) {
            warning() << "You requested an address type/access control combination "
                "not supported by this channel";
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        d->addressType = SocketAddressTypeUnix;
        d->unixAddress = socketAddress;

        PendingVoid *pv = new PendingVoid(
                interface<Client::ChannelTypeStreamTubeInterface>()->Offer(
                        SocketAddressTypeUnix,
                        QDBusVariant(QVariant(socketAddress.toLatin1())),
                        accessControl,
                        parameters),
                OutgoingStreamTubeChannelPtr(this));
        PendingOpenTube *op = new PendingOpenTube(pv, parameters,
                OutgoingStreamTubeChannelPtr(this));
        return op;
    }
}

/**
 * \overload
 * \brief Offer a Unix socket over the tube
 *
 * This method offers a Unix socket over this tube through a QLocalServer.
 *
 * The %PendingOperation returned by this method will be completed as soon as the tube is
 * open and ready to be used.
 *
 * \param server A valid QLocalServer, already listening
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \returns A %PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \see StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost
 * \see StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials
 * \see StreamTubeChannel::supportsUnixSocketsOnLocalhost
 * \see StreamTubeChannel::supportsUnixSocketsWithCredentials
 */
PendingOperation* OutgoingStreamTubeChannel::offerUnixSocket(
        QLocalServer* server,
        const QVariantMap& parameters,
        bool requireCredentials)
{
    // In this overload, we're handling a superset of a local socket
    // Let's redirect the call to QString's overload
    return offerUnixSocket(server->fullServerName(), parameters, requireCredentials);
}


/**
 * If StreamTubeChannel::FeatureConnectionMonitoring has been enabled, the socket address type of this
 * tube is IPv4 or IPv6, and if the tube supports connection with an specified address, this function
 * returns a map from a source address to its connection ID. It is useful to track an address
 * which connected to your socket to a contact (by using contactsForConnections).
 *
 * \returns an hash mapping a source address to its connection ID
 *
 * \note The tube has to be open for calling this function
 *
 * \see contactsForConnections
 * \see StreamTubeChannel::addressType
 * \see StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress
 * \see StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress
 */
QHash< QPair< QHostAddress, quint16 >, uint > OutgoingStreamTubeChannel::connectionsForSourceAddresses() const
{
    Q_D(const OutgoingStreamTubeChannel);

    if (d->addressType != SocketAddressTypeIPv4 && d->addressType != SocketAddressTypeIPv6) {
        warning() << "OutgoingStreamTubeChannel::connectionsForSourceAddresses() makes sense "
            "just when offering a TCP socket";
        return QHash< QPair< QHostAddress, quint16 >, uint >();
    }

    if (!isReady(StreamTubeChannel::FeatureConnectionMonitoring)) {
        warning() << "StreamTubeChannel::FeatureConnectionMonitoring must be ready before "
            "calling connectionsForSourceAddresses";
        return QHash< QPair< QHostAddress, quint16 >, uint >();
    }

    if (tubeState() != TubeChannelStateOpen) {
        warning() << "OutgoingStreamTubeChannel::connectionsForSourceAddresses() makes sense "
            "just when the tube is open";
        return QHash< QPair< QHostAddress, quint16 >, uint >();
    }

    return d->connectionsForSourceAddresses;
}


/**
 * If StreamTubeChannel::FeatureConnectionMonitoring has been enabled, this function
 * returns a map from a connection ID to the associated contact.
 *
 * \returns an hash mapping a connection ID to the associated contact.
 *
 * \note The tube has to be open for calling this function
 *
 * \see connectionsForSourceAddresses
 * \see StreamTubeChannel::addressType
 */
QHash< uint, ContactPtr > OutgoingStreamTubeChannel::contactsForConnections() const
{
    if (!isReady(StreamTubeChannel::FeatureConnectionMonitoring)) {
        warning() << "StreamTubeChannel::FeatureConnectionMonitoring must be ready before "
            "calling contactsForConnections";
        return QHash< uint, ContactPtr >();
    }

    if (tubeState() != TubeChannelStateOpen) {
        warning() << "OutgoingStreamTubeChannel::contactsForConnections() makes sense "
            "just when the tube is open";
        return QHash< uint, ContactPtr >();
    }

    Q_D(const OutgoingStreamTubeChannel);

    return d->contactsForConnections;
}

}

#include "TelepathyQt4/_gen/outgoing-stream-tube-channel.moc.hpp"
#include "TelepathyQt4/_gen/outgoing-stream-tube-channel-internal.moc.hpp"
