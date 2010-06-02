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

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QLocalServer>

namespace Tp
{

PendingOpenTube::Private::Private(PendingOpenTube* parent)
    : parent(parent)
{

}

PendingOpenTube::Private::~Private()
{

}

PendingOpenTube::PendingOpenTube(
        PendingVoid* offerOperation,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = OutgoingStreamTubeChannelPtr::dynamicCast(object);

    if (offerOperation->isFinished()) {
        mPriv->__k__onOfferFinished(offerOperation);
    } else {
        // Connect the pending void
        connect(offerOperation, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(__k__onOfferFinished(Tp::PendingOperation*)));
    }
}

PendingOpenTube::~PendingOpenTube()
{
    delete mPriv;
}

void PendingOpenTube::Private::__k__onOfferFinished(PendingOperation* op)
{
    if (op->isError()) {
        // Fail
        parent->setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Offer tube finished successfully";

    // It might have been already opened - check
    if (tube->tubeState() == TubeChannelStateOpen) {
        __k__onTubeStateChanged(tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        parent->connect(tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                        parent, SLOT(__k__onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingOpenTube::Private::__k__onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
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

void OutgoingStreamTubeChannelPrivate::__k__onNewRemoteConnection(
        uint contactId,
        const QDBusVariant &parameter,
        uint connectionId)
{
    Q_Q(OutgoingStreamTubeChannel);

    // Add the connection to our list
    connections << connectionId;

    emit q->newRemoteConnection(q->connection()->contactManager()->lookupContactByHandle(contactId),
            parameter.variant(),
            connectionId);
}

void OutgoingStreamTubeChannelPrivate::__k__onPendingOpenTubeFinished(Tp::PendingOperation* operation)
{
    // If the operation finished successfully, let's reintrospect the parameters
    if (!operation->isError()) {
        reintrospectParameters();
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
 * \fn void StreamTubeChannel::newRemoteConnection(const Tp::ContactPtr &contact, const QVariant &parameter, uint
connectionId)
 *
 * Emitted when a new participant opens a connection to this tube
 *
 * \param contact The participant who opened the new connection
 * \param parameter A parameter which can be used by the listening process to identify
 *                  the connection. Note that this parameter has a meaningful value
 *                  only if SocketAccessControl is \c Port or \c Credentials. In the
 *                  \c Port case, \p parameter is a QHostAddress. Otherwise, it is
 *                  a byte that has been sent with the credential.
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

    Q_D(OutgoingStreamTubeChannel);

    // In this specific overload, we're handling an IPv4/IPv6 socket
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        // IPv4 case
        // Check if the combination type/access control is supported
        if (!supportsIPv4SocketsOnLocalhost()) {
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
                        SocketAccessControlLocalhost,
                        parameters),
                OutgoingStreamTubeChannelPtr(this));
        PendingOpenTube *op = new PendingOpenTube(pv,
                OutgoingStreamTubeChannelPtr(this));
        connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(__k__onPendingOpenTubeFinished(Tp::PendingOperation*)));
        return op;
    } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        // IPv6 case
        // Check if the combination type/access control is supported
        if (!supportsIPv6SocketsOnLocalhost()) {
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
                        SocketAccessControlLocalhost,
                        parameters),
                OutgoingStreamTubeChannelPtr(this));
        PendingOpenTube *op = new PendingOpenTube(pv,
                OutgoingStreamTubeChannelPtr(this));
        connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(__k__onPendingOpenTubeFinished(Tp::PendingOperation*)));
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
        PendingOpenTube *op = new PendingOpenTube(pv,
                OutgoingStreamTubeChannelPtr(this));
        connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(__k__onPendingOpenTubeFinished(Tp::PendingOperation*)));
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
        PendingOpenTube *op = new PendingOpenTube(pv,
                OutgoingStreamTubeChannelPtr(this));
        connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(__k__onPendingOpenTubeFinished(Tp::PendingOperation*)));
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

void OutgoingStreamTubeChannel::connectNotify(const char* signal)
{
    if (QLatin1String(signal) == SIGNAL(newRemoteConnection(Tp::ContactPtr,QVariant,uint)) &&
        !isReady(FeatureConnectionMonitoring)) {
        warning() << "Connected to the signal newRemoteConnection, but FeatureConnectionMonitoring is "
            "not ready. The signal won't be emitted until the mentioned feature is ready.";
    }

    Tp::StreamTubeChannel::connectNotify(signal);
}

}

#include "TelepathyQt4/_gen/outgoing-stream-tube-channel.moc.hpp"
#include "TelepathyQt4/_gen/outgoing-stream-tube-channel-internal.moc.hpp"
