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

#include <TelepathyQt4/IncomingStreamTubeChannel>
#include "TelepathyQt4/stream-tube-channel-internal.h"

#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include "TelepathyQt4/debug-internal.h"

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalSocket>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT IncomingStreamTubeChannelPrivate : public StreamTubeChannelPrivate
{
    Q_DECLARE_PUBLIC(IncomingStreamTubeChannel)
public:
    IncomingStreamTubeChannelPrivate(IncomingStreamTubeChannel *parent);
    virtual ~IncomingStreamTubeChannelPrivate();

    // Public object
    IncomingStreamTubeChannel *parent;

    // Properties
    QIODevice *device;

    // Private slots
    void __k__onAcceptTubeFinished(Tp::PendingOperation* op);
    void __k__onNewLocalConnection(uint connectionId);
};

IncomingStreamTubeChannelPrivate::IncomingStreamTubeChannelPrivate(IncomingStreamTubeChannel *parent)
    : StreamTubeChannelPrivate(parent)
    , device(0)
{
    baseType = IncomingTubeType;
}

IncomingStreamTubeChannelPrivate::~IncomingStreamTubeChannelPrivate()
{
}

void IncomingStreamTubeChannelPrivate::__k__onAcceptTubeFinished(PendingOperation* op)
{
    PendingStreamTubeConnection *pendingDevice = qobject_cast< PendingStreamTubeConnection* >(op);

    device = pendingDevice->device();
}

void IncomingStreamTubeChannelPrivate::__k__onNewLocalConnection(uint connectionId)
{
    Q_Q(IncomingStreamTubeChannel);

    // Add the connection to our list
    connections << connectionId;

    emit q->newLocalConnection(connectionId);
}

class TELEPATHY_QT4_NO_EXPORT PendingStreamTubeConnectionPrivate
{
public:
    PendingStreamTubeConnectionPrivate(PendingStreamTubeConnection *parent);
    virtual ~PendingStreamTubeConnectionPrivate();

    // Public object
    PendingStreamTubeConnection *parent;

    IncomingStreamTubeChannelPtr tube;
    SocketAddressType type;
    QHostAddress hostAddress;
    quint16 port;
    QString socketPath;

    QIODevice *device;

    // Private slots
    void __k__onAcceptFinished(Tp::PendingOperation* op);
    void __k__onTubeStateChanged(Tp::TubeChannelState state);
    void __k__onDeviceConnected();
    void __k__onAbstractSocketError(QAbstractSocket::SocketError error);
    void __k__onLocalSocketError(QLocalSocket::LocalSocketError error);
};

PendingStreamTubeConnectionPrivate::PendingStreamTubeConnectionPrivate(PendingStreamTubeConnection *parent)
    : parent(parent)
    , device(0)
{

}

PendingStreamTubeConnectionPrivate::~PendingStreamTubeConnectionPrivate()
{

}

PendingStreamTubeConnection::PendingStreamTubeConnection(
        PendingVariant* acceptOperation,
        SocketAddressType type,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new PendingStreamTubeConnectionPrivate(this))
{
    mPriv->tube = IncomingStreamTubeChannelPtr::dynamicCast(object);
    mPriv->type = type;

    // Connect the pending void
    connect(acceptOperation, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(__k__onAcceptFinished(Tp::PendingOperation*)));
}

/**
 * \class PendingStreamTubeConnection
 * \headerfile TelepathyQt4/incoming-stream-tube-channel.h <TelepathyQt4/IncomingStreamTubeChannel>
 *
 * \brief A pending operation for accepting a stream tube
 *
 * This class represents an asynchronous operation for accepting a stream tube.
 * When the operation is finished, you can access the resulting device
 * through #device().
 * Otherwise, you can access the bare address through either #tcpAddress or #localAddress.
 */

PendingStreamTubeConnection::PendingStreamTubeConnection(
        const QString& errorName,
        const QString& errorMessage,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new PendingStreamTubeConnectionPrivate(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingStreamTubeConnection::~PendingStreamTubeConnection()
{
    delete mPriv;
}

/**
 * \returns The opened QIODevice if the operation is finished and successful, 0 otherwise.
 */
QIODevice* PendingStreamTubeConnection::device()
{
    return mPriv->device;
}

void PendingStreamTubeConnectionPrivate::__k__onAcceptFinished(PendingOperation* op)
{
    if (op->isError()) {
        parent->setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Accept tube finished successfully";

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    // Build the address
    if (type == SocketAddressTypeIPv4) {
        SocketAddressIPv4 addr = qdbus_cast<SocketAddressIPv4>(pv->result());
        debug().nospace() << "Got address " << addr.address <<
                ":" << addr.port;
        hostAddress = QHostAddress(addr.address);
        port = addr.port;
    } else if (type == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr = qdbus_cast<SocketAddressIPv6>(pv->result());
        debug().nospace() << "Got address " << addr.address <<
                ":" << addr.port;
        hostAddress = QHostAddress(addr.address);
        port = addr.port;
    } else {
        // Unix socket
        socketPath = QLatin1String(qdbus_cast<QByteArray>(pv->result()));
        debug() << "Got socket " << socketPath;
    }

    // It might have been already opened - check
    if (tube->tubeState() == TubeChannelStateOpen) {
        __k__onTubeStateChanged(tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        parent->connect(tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                        parent, SLOT(__k__onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingStreamTubeConnectionPrivate::__k__onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready
        // Build the IO device
        if (type == SocketAddressTypeIPv4 || type == SocketAddressTypeIPv6) {
            tube->d_func()->ipAddress = qMakePair< QHostAddress, quint16 >(hostAddress, port);
            debug() << "Building a QTcpSocket " << hostAddress << port;

            QTcpSocket *socket = new QTcpSocket(tube.data());
            socket->connectToHost(hostAddress, port);
            device = socket;

            parent->connect(socket, SIGNAL(connected()),
                            parent, SLOT(__k__onDeviceConnected()));
            parent->connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                            parent, SLOT(__k__onAbstractSocketError(QAbstractSocket::SocketError)));
            debug() << "QTcpSocket built";
        } else {
            // Unix socket
            tube->d_func()->unixAddress = socketPath;

            QLocalSocket *socket = new QLocalSocket(tube.data());
            socket->connectToServer(socketPath);
            device = socket;

            // The local socket might already be connected
            if (socket->state() == QLocalSocket::ConnectedState) {
                __k__onDeviceConnected();
            } else {
                parent->connect(socket, SIGNAL(connected()),
                                parent, SLOT(__k__onDeviceConnected()));
                parent->connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
                                parent, SLOT(__k__onLocalSocketError(QLocalSocket::LocalSocketError)));
            }
        }
    } else if (state != TubeChannelStateLocalPending) {
        // Something happened
        parent->setFinishedWithError(QLatin1String("Connection refused"),
                      QLatin1String("The connection to this tube was refused"));
    }
}

void PendingStreamTubeConnectionPrivate::__k__onAbstractSocketError(QAbstractSocket::SocketError error)
{
    // TODO: Use error to provide more detailed error messages
    Q_UNUSED(error)
    // Failure
    device = 0;
    parent->setFinishedWithError(QLatin1String("Error while creating TCP socket"),
                      QLatin1String("Could not connect to the new socket"));
}

void PendingStreamTubeConnectionPrivate::__k__onLocalSocketError(QLocalSocket::LocalSocketError error)
{
    // TODO: Use error to provide more detailed error messages
    Q_UNUSED(error)
    // Failure
    device = 0;
    parent->setFinishedWithError(QLatin1String("Error while creating Local socket"),
                      QLatin1String("Could not connect to the new socket"));
}

void PendingStreamTubeConnectionPrivate::__k__onDeviceConnected()
{
    // Our IODevice is ready! Let's just set finished
    debug() << "Device connected!";
    parent->setFinished();
}


/**
 * \returns The type of socket this PendingStreamTubeConnection has created
 *
 * \note This function will return a valid value only after the operation has been finished successfully
 *
 * \see localAddress
 * \see tcpAddress
 */
SocketAddressType PendingStreamTubeConnection::addressType() const
{
    return mPriv->tube->addressType();
}


/**
 * \returns The local address obtained from this PendingStreamTubeConnection as a QByteArray,
 *          if the connection has been estabilished through a SocketAddressTypeUnix or
 *          a SocketAddressTypeAbstractUnix.
 *
 * \note This function will return a valid value only after the operation has been finished successfully
 *
 * \see addressType
 */
QString PendingStreamTubeConnection::localAddress() const
{
    return mPriv->tube->localAddress();
}


/**
 * \returns The IP address and port obtained from this PendingStreamTubeConnection as a QHostAddress,
 *          if the connection has been estabilished through a SocketAddressTypeIpv4 or
 *          a SocketAddressTypeIPv6.
 *
 * \note This function will return a valid value only after the operation has been finished successfully
 *
 * \see addressType
 */
QPair< QHostAddress, quint16 > PendingStreamTubeConnection::ipAddress() const
{
    return mPriv->tube->ipAddress();
}



/**
 * \class IncomingStreamTubeChannel
 * \headerfile TelepathyQt4/stream-tube.h <TelepathyQt4/IncomingStreamTubeChannel>
 *
 * \brief A class representing a Stream Tube
 *
 * \c IncomingStreamTubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Type.StreamTubeChannel.
 * In particular, this class is meant to be used as a comfortable way for accepting incoming.
 * It provides an easy way for obtaining a QIODevice which can be used for communication.
 *
 * For more details, please refer to Telepathy spec.
 */


// Signals documentation
/**
 * \fn void StreamTubeChannel::newLocalConnection(uint connectionId)
 *
 * Emitted when the tube application connects to ConnectionManager's socket
 *
 * \param connectionId The unique ID associated with this connection.
 */


/**
 * Create a new IncomingStreamTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A IncomingStreamTubeChannelPtr object pointing to the newly created
 *         IncomingStreamTubeChannel object.
 */
IncomingStreamTubeChannelPtr IncomingStreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return IncomingStreamTubeChannelPtr(new IncomingStreamTubeChannel(connection, objectPath,
                immutableProperties, StreamTubeChannel::FeatureStreamTube));
}

/**
 * Construct a new IncomingStreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
IncomingStreamTubeChannel::IncomingStreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : StreamTubeChannel(connection, objectPath, immutableProperties,
            coreFeature, *new IncomingStreamTubeChannelPrivate(this))
{
}

/**
 * Class destructor.
 */
IncomingStreamTubeChannel::~IncomingStreamTubeChannel()
{
}


/**
 * \brief Accept an incoming Stream tube
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the \c LocalPending state.
 *
 * Once called, this method will try opening the tube, and will create a QIODevice which
 * can be used to communicate with the other end. The device is guaranteed to be opened
 * read/write when the resulting \c PendingStreamTubeConnection finishes successfully. You can then
 * access the device right from \c PendingStreamTubeConnection or from %device().
 *
 * If you want to specify a different access control method than \c LocalHost, you will
 * have to satisfy some specific requirements: in the \c Port case, you will need to
 * call %setAllowedAddress first to set the allowed address/port combination.
 * Please refer to the spec for more details.
 *
 * \note If you plan to establish more than one connection through the tube,
 *       the \c Port access control can't be used as you can't connect more
 *       than once from the same port.
 *
 * \param accessControl The type of access control the connection manager should apply to the socket.
 *
 */
PendingStreamTubeConnection* IncomingStreamTubeChannel::acceptTubeAsTcpSocket(
        const QHostAddress& allowedAddress,
        quint16 allowedPort)
{
    if (!isReady(StreamTubeChannel::FeatureStreamTube)) {
        warning() << "StreamTubeChannel::FeatureStreamTube must be ready before "
                "calling acceptTubeAsTcpSocket";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (tubeState() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    QVariant controlParameter;
    SocketAccessControl accessControl;

    // Now, let's check what we need to do with accessControl. There is just one special case, Port.
    if (allowedAddress != QHostAddress::Any) {
        // We need to have a valid QHostAddress
        if (allowedAddress.isNull()) {
            warning() << "You have to set a valid allowed address to use Port access control";
            return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                    QLatin1String("The supplied allowed address was invalid"),
                    IncomingStreamTubeChannelPtr(this));
        }

        accessControl = SocketAccessControlPort;

        // IPv4 or IPv6?
        if (allowedAddress.protocol() == QAbstractSocket::IPv4Protocol) {
            // IPv4 case
            SocketAddressIPv4 addr;
            addr.address = allowedAddress.toString();
            addr.port = allowedPort;

            controlParameter = QVariant::fromValue(addr);
        } else if (allowedAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            // IPv6 case
            SocketAddressIPv6 addr;
            addr.address = allowedAddress.toString();
            addr.port = allowedPort;

            controlParameter = QVariant::fromValue(addr);
        } else {
            // We're handling an IPv4/IPv6 socket only
            warning() << "acceptTubeAsTcpSocket can be called only with a QHostAddress representing "
                    "an IPv4 or IPv6 address";
            return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                    QLatin1String("Invalid host given"),
                    IncomingStreamTubeChannelPtr(this));
        }
    } else {
        // We have to do no special stuff here
        accessControl = SocketAccessControlLocalhost;
        // Since QDBusMarshaller does not like null variants, just add an empty string.
        controlParameter = QVariant(QString());
    }

    Q_D(IncomingStreamTubeChannel);

    // Set the correct address type
    if (allowedAddress == QHostAddress::Any) {
        // Use IPv4 by default
        d->addressType = SocketAddressTypeIPv4;
    } else {
        d->addressType = allowedAddress.protocol() == QAbstractSocket::IPv4Protocol ?
                         SocketAddressTypeIPv4 :
                         SocketAddressTypeIPv6;
    }

    // Fail early if the combination is not supported
    if ( (accessControl == SocketAccessControlLocalhost &&
          d->addressType == SocketAddressTypeIPv4 &&
          !supportsIPv4SocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlPort &&
          d->addressType == SocketAddressTypeIPv4 &&
          !supportsIPv4SocketsWithAllowedAddress()) ||
         (accessControl == SocketAccessControlLocalhost &&
          d->addressType == SocketAddressTypeIPv6 &&
          !supportsIPv6SocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlPort &&
          d->addressType == SocketAddressTypeIPv6 &&
          !supportsIPv6SocketsWithAllowedAddress())) {
        warning() << "You requested an address type/access control combination "
                "not supported by this channel";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("The requested address type/access control combination is not supported"),
                IncomingStreamTubeChannelPtr(this));
    }

    // Perform the actual call
    PendingVariant *pv = new PendingVariant(
            interface<Client::ChannelTypeStreamTubeInterface>()->Accept(
                    d->addressType,
                    accessControl,
                    QDBusVariant(controlParameter)),
            IncomingStreamTubeChannelPtr(this));

    PendingStreamTubeConnection *op = new PendingStreamTubeConnection(pv, d->addressType,
            IncomingStreamTubeChannelPtr(this));
    connect(op, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(__k__onAcceptTubeFinished(Tp::PendingOperation*)));
    return op;
}

PendingStreamTubeConnection* IncomingStreamTubeChannel::acceptTubeAsUnixSocket(
        bool requireCredentials)
{
    if (!isReady(StreamTubeChannel::FeatureStreamTube)) {
        warning() << "StreamTubeChannel::FeatureStreamTube must be ready before "
                "calling acceptTubeAsUnixSocket";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (tubeState() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    Q_D(IncomingStreamTubeChannel);

    SocketAccessControl accessControl = requireCredentials ?
            SocketAccessControlCredentials :
            SocketAccessControlLocalhost;
    d->addressType = SocketAddressTypeUnix;

    // Fail early if the combination is not supported
    if ( (accessControl == SocketAccessControlLocalhost &&
          d->addressType == SocketAddressTypeUnix &&
          !supportsUnixSocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlCredentials &&
          d->addressType == SocketAddressTypeUnix &&
          !supportsUnixSocketsWithCredentials()) ||
         (accessControl == SocketAccessControlLocalhost &&
          d->addressType == SocketAddressTypeAbstractUnix &&
          !supportsAbstractUnixSocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlCredentials &&
          d->addressType == SocketAddressTypeAbstractUnix &&
          !supportsAbstractUnixSocketsWithCredentials())) {
        warning() << "You requested an address type/access control combination "
                "not supported by this channel";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("The requested address type/access control combination is not supported"),
                IncomingStreamTubeChannelPtr(this));
    }

    // Perform the actual call
    PendingVariant *pv = new PendingVariant(
            interface<Client::ChannelTypeStreamTubeInterface>()->Accept(
                    d->addressType,
                    accessControl,
                    QDBusVariant(QVariant(QString()))),
            IncomingStreamTubeChannelPtr(this));

    PendingStreamTubeConnection *op = new PendingStreamTubeConnection(pv, d->addressType,
            IncomingStreamTubeChannelPtr(this));
    connect(op, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(__k__onAcceptTubeFinished(Tp::PendingOperation*)));
    return op;
}


/**
 * \returns A valid QIODevice if the tube is in the \c Open state, 0 otherwise.
 */
QIODevice* IncomingStreamTubeChannel::device()
{
    if (tubeState() == TubeChannelStateOpen) {
        Q_D(IncomingStreamTubeChannel);
        return d->device;
    } else {
        return 0;
    }
}

void IncomingStreamTubeChannel::connectNotify(const char* signal)
{
    if (QLatin1String(signal) == SIGNAL(newLocalConnection(uint)) &&
        !isReady(FeatureConnectionMonitoring)) {
        warning() << "Connected to the signal newLocalConnection, but FeatureConnectionMonitoring is "
            "not ready. The signal won't be emitted until the mentioned feature is ready.";
    }

    Tp::StreamTubeChannel::connectNotify(signal);
}

}

#include "TelepathyQt4/_gen/incoming-stream-tube-channel.moc.hpp"
