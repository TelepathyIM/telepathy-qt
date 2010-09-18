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
#include "TelepathyQt4/incoming-stream-tube-channel-internal.h"

#include "TelepathyQt4/_gen/incoming-stream-tube-channel.moc.hpp"

#include "TelepathyQt4/types-internal.h"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include <QHostAddress>
#include <QTcpSocket>

namespace Tp
{

IncomingStreamTubeChannel::Private::Private(IncomingStreamTubeChannel *parent)
    : parent(parent)
    , device(0)
{
}

IncomingStreamTubeChannel::Private::~Private()
{
}

void IncomingStreamTubeChannel::Private::onAcceptTubeFinished(PendingOperation* op)
{
    PendingStreamTubeConnection *pendingDevice = qobject_cast< PendingStreamTubeConnection* >(op);

    device = pendingDevice->device();
}

void IncomingStreamTubeChannel::Private::onNewLocalConnection(uint connectionId)
{
    // Add the connection to our list
    UIntList connections = parent->connections();
    connections << connectionId;
    parent->setConnections(connections);

    emit parent->newConnection(connectionId);
}

PendingStreamTubeConnection::Private::Private(PendingStreamTubeConnection* parent)
    : parent(parent)
    , device(0)
{

}

PendingStreamTubeConnection::Private::~Private()
{

}

PendingStreamTubeConnection::PendingStreamTubeConnection(
        PendingVariant* acceptOperation,
        SocketAddressType type,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = IncomingStreamTubeChannelPtr::dynamicCast(object);
    mPriv->type = type;

    // Connect the pending void
    connect(acceptOperation, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAcceptFinished(Tp::PendingOperation*)));
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
    , mPriv(new PendingStreamTubeConnection::Private(this))
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
 * \return The opened QIODevice if the operation is finished and successful, 0 otherwise.
 */
QIODevice* PendingStreamTubeConnection::device()
{
    return mPriv->device;
}

void PendingStreamTubeConnection::Private::onAcceptFinished(PendingOperation* op)
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
        onTubeStateChanged(tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        parent->connect(tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                        parent, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingStreamTubeConnection::Private::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready
        // Build the IO device
        if (type == SocketAddressTypeIPv4 || type == SocketAddressTypeIPv6) {
            tube->setIpAddress(qMakePair< QHostAddress, quint16 >(hostAddress, port));
            debug() << "Building a QTcpSocket " << hostAddress << port;

            QTcpSocket *socket = new QTcpSocket(tube.data());
            socket->connectToHost(hostAddress, port);
            device = socket;

            parent->connect(socket, SIGNAL(connected()),
                            parent, SLOT(onDeviceConnected()));
            parent->connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                            parent, SLOT(onAbstractSocketError(QAbstractSocket::SocketError)));
            debug() << "QTcpSocket built";
        } else {
            // Unix socket
            tube->setLocalAddress(socketPath);

            QLocalSocket *socket = new QLocalSocket(tube.data());
            socket->connectToServer(socketPath);
            device = socket;

            // The local socket might already be connected
            if (socket->state() == QLocalSocket::ConnectedState) {
                onDeviceConnected();
            } else {
                parent->connect(socket, SIGNAL(connected()),
                                parent, SLOT(onDeviceConnected()));
                parent->connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
                                parent, SLOT(onLocalSocketError(QLocalSocket::LocalSocketError)));
            }
        }
    } else if (state != TubeChannelStateLocalPending) {
        // Something happened
        parent->setFinishedWithError(QLatin1String("Connection refused"),
                      QLatin1String("The connection to this tube was refused"));
    }
}

void PendingStreamTubeConnection::Private::onAbstractSocketError(QAbstractSocket::SocketError error)
{
    // TODO: Use error to provide more detailed error messages
    Q_UNUSED(error)
    // Failure
    device = 0;
    parent->setFinishedWithError(QLatin1String("Error while creating TCP socket"),
                      QLatin1String("Could not connect to the new socket"));
}

void PendingStreamTubeConnection::Private::onLocalSocketError(QLocalSocket::LocalSocketError error)
{
    // TODO: Use error to provide more detailed error messages
    Q_UNUSED(error)
    // Failure
    device = 0;
    parent->setFinishedWithError(QLatin1String("Error while creating Local socket"),
                      QLatin1String("Could not connect to the new socket"));
}

void PendingStreamTubeConnection::Private::onDeviceConnected()
{
    // Our IODevice is ready! Let's just set finished
    debug() << "Device connected!";
    parent->setFinished();
}


/**
 * \return The type of socket this PendingStreamTubeConnection has created
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
 * \return The local address obtained from this PendingStreamTubeConnection as a QByteArray,
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
 * \return The IP address and port obtained from this PendingStreamTubeConnection as a QHostAddress,
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
 * \headerfile TelepathyQt4/incoming-stream-tube-channel.h <TelepathyQt4/IncomingStreamTubeChannel>
 *
 * \brief An high level wrapper for managing an incoming stream tube
 *
 * \c IncomingStreamTubeChannel is an high level wrapper for managing Telepathy interface
 * #TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE.
 * In particular, this class is meant to be used as a comfortable way for accepting incoming.
 * It provides an easy way for obtaining a QIODevice which can be used for communication.
 *
 * \section incoming_stream_tube_usage_sec Usage
 *
 * \subsection incoming_stream_tube_create_sec Obtaining an incoming stream tube
 *
 * Whenever a contact invites you to open an incoming stream tube, if you are registered as a channel handler
 * for the channel type #TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE, you will be notified of the offer and
 * you will be able to handle the channel. Please refer to the documentation of #AbstractClientHandler for more
 * details on channel handling.
 *
 * Supposing your channel handler has been created correctly, you would do:
 *
 * \code
 * void MyChannelManager::handleChannels(const Tp::MethodInvocationContextPtr<>& context,
 *                               const Tp::AccountPtr& account,
 *                               const Tp::ConnectionPtr& connection,
 *                               const QList< Tp::ChannelPtr >& channels,
 *                               const QList< Tp::ChannelRequestPtr >& requestsSatisfied,
 *                               const QDateTime& userActionTime,
 *                               const QVariantMap& handlerInfo)
 * {
 *     foreach(const Tp::ChannelPtr &channel, channels) {
 *         QVariantMap properties = channel->immutableProperties();
 *
 *         if (properties[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] ==
 *                        TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE) {
 *
 *             // Handle the channel
 *             Tp::IncomingStreamTubeChannelPtr myTube = Tp::IncomingStreamTubeChannelPtr::dynamicCast(channel);
 *
 *          }
 *     }
 *
 *     context->setFinished();
 * }
 * \endcode
 *
 * \subsection incoming_stream_tube_accept_sec Accepting the tube
 *
 * Before being ready to accept the tube, we must be sure the required features on our object
 * are ready. In this case, we need to enable TubeChannel::FeatureTube and StreamTubeChannel::FeatureStreamTube.
 *
 * \code
 *
 * Features features = Features() << TubeChannel::FeatureTube
 *                                << StreamTubeChannel::FeatureStreamTube;
 * connect(myTube->becomeReady(features),
 *         SIGNAL(finished(Tp::PendingOperation *)),
 *         SLOT(onStreamTubeChannelReady(Tp::PendingOperation *)));
 *
 * \endcode
 *
 * To learn more on how to use introspectable and features, please see \ref account_ready_sec.
 *
 * Once your object is ready, you can use one of the overloads of #acceptTubeAsTcpSocket or
 * #acceptTubeAsUnixSocket to accept a socket over the tube. Please note that the socket type is \b not dependent on
 * what has been offered on the other end: choosing whether you want to accept a TCP socket or an Unix socket boils
 * down to your personal preference only and does not affect in any way the way the communication with the other end
 * will be handled.
 *
 * Let's consider the case of accepting an Unix socket without applying any access control restriction.
 * You would simply do:
 *
 * \code
 * PendingStreamTubeConnection *pendingConnection = myTube->acceptTubeAsUnixSocket();
 * \endcode
 *
 * The returned PendingOperation serves both for monitoring the state of the tube and for obtaining, upon success,
 * either the address of the new socket or a QIODevice ready to be used. When the operation finishes, you can do:
 *
 * \code
 * void MyTubeReceiver::onStreamTubeAccepted(PendingOperation* op)
 * {
 *     if (op->isError()) {
 *        return;
 *     }
 *
 *     PendingStreamTubeConnection *pendingConnection = qobject_cast< PendingStreamTubeConnection* >(op);
 *
 *     QIODevice *mySocketReadyToBeUsed = pendingConnection->device();
 *     // Do some stuff here
 * }
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
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
    : StreamTubeChannel(connection, objectPath,
            immutableProperties, coreFeature)
    , mPriv(new Private(this))
{
    setBaseTubeType(2);
}

/**
 * Class destructor.
 */
IncomingStreamTubeChannel::~IncomingStreamTubeChannel()
{
    delete mPriv;
}


/**
 * \brief Accept an incoming Stream tube as a TCP socket
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the \c LocalPending state.
 *
 * Once called, this method will try opening the tube, and will create a QIODevice which
 * can be used to communicate with the other end. The device is guaranteed to be opened
 * read/write when the resulting \c PendingStreamTubeConnection finishes successfully. You can then
 * access the device right from \c PendingStreamTubeConnection or from %device().
 *
 * This overload lets you specify an allowed address/port combination for connecting to this socket.
 * Otherwise, you can specify QHostAddress::Any to accept every incoming connection from localhost, or
 * use the other overload.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \note When using QHostAddress::Any, the allowedPort parameter is ignored.
 *
 * \param allowedAddress an allowed address for connecting to this socket, or QHostAddress::Any
 * \param allowedPort an allowed port for connecting to this socket
 *
 * \return A %PendingStreamTubeConnection which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \see StreamTubeChannel::supportsIPv4SocketsWithAllowedAddress
 * \see StreamTubeChannel::supportsIPv4SocketsOnLocalhost
 * \see StreamTubeChannel::supportsIPv6SocketsWithAllowedAddress
 * \see StreamTubeChannel::supportsIPv6SocketsOnLocalhost
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
        // We need to have a valid QHostAddress AND Port.
        if (allowedAddress.isNull() || allowedPort == 0) {
            warning() << "You have to set a valid allowed address+port to use Port access control";
            return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                    QLatin1String("The supplied allowed address and/or port was invalid"),
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

    // Set the correct address type
    if (allowedAddress == QHostAddress::Any) {
        // Use IPv4 by default
        setAddressType(SocketAddressTypeIPv4);
    } else {
        setAddressType(allowedAddress.protocol() == QAbstractSocket::IPv4Protocol ?
                       SocketAddressTypeIPv4 :
                       SocketAddressTypeIPv6);
    }

    // Fail early if the combination is not supported
    if ( (accessControl == SocketAccessControlLocalhost &&
          addressType() == SocketAddressTypeIPv4 &&
          !supportsIPv4SocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlPort &&
          addressType() == SocketAddressTypeIPv4 &&
          !supportsIPv4SocketsWithSpecifiedAddress()) ||
         (accessControl == SocketAccessControlLocalhost &&
          addressType() == SocketAddressTypeIPv6 &&
          !supportsIPv6SocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlPort &&
          addressType() == SocketAddressTypeIPv6 &&
          !supportsIPv6SocketsWithSpecifiedAddress())) {
        warning() << "You requested an address type/access control combination "
                "not supported by this channel";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("The requested address type/access control combination is not supported"),
                IncomingStreamTubeChannelPtr(this));
    }

    // Perform the actual call
    PendingVariant *pv = new PendingVariant(
            interface<Client::ChannelTypeStreamTubeInterface>()->Accept(
                    addressType(),
                    accessControl,
                    QDBusVariant(controlParameter)),
            IncomingStreamTubeChannelPtr(this));

    PendingStreamTubeConnection *op = new PendingStreamTubeConnection(pv, addressType(),
            IncomingStreamTubeChannelPtr(this));
    connect(op, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAcceptTubeFinished(Tp::PendingOperation*)));
    return op;
}


/**
 * \brief Accept an incoming Stream tube as a TCP socket
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the \c LocalPending state.
 *
 * Once called, this method will try opening the tube, and will create a QIODevice which
 * can be used to communicate with the other end. The device is guaranteed to be opened
 * read/write when the resulting \c PendingStreamTubeConnection finishes successfully. You can then
 * access the device right from \c PendingStreamTubeConnection or from %device().
 *
 * This overload will open a tube which accepts every incoming connection from Localhost. Please note
 * that this is the equivalent of calling acceptTubeAsTcpSocket(QHostAddress::Any, 0).
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return A %PendingStreamTubeConnection which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \see StreamTubeChannel::supportsIPv4SocketsWithAllowedAddress
 * \see StreamTubeChannel::supportsIPv4SocketsOnLocalhost
 * \see StreamTubeChannel::supportsIPv6SocketsWithAllowedAddress
 * \see StreamTubeChannel::supportsIPv6SocketsOnLocalhost
 */
PendingStreamTubeConnection* IncomingStreamTubeChannel::acceptTubeAsTcpSocket()
{
    return acceptTubeAsTcpSocket(QHostAddress::Any, 0);
}


/**
 * \brief Accept an incoming Stream tube as a Unix socket
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the \c LocalPending state.
 *
 * Once called, this method will try opening the tube, and will create a QIODevice which
 * can be used to communicate with the other end. The device is guaranteed to be opened
 * read/write when the resulting \c PendingStreamTubeConnection finishes successfully. You can then
 * access the device right from \c PendingStreamTubeConnection or from %device().
 *
 * You can also specify whether the server should require an SCM_CREDENTIALS message
 * upon connection instead of accepting every incoming connection from localhost.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \return A %PendingStreamTubeConnection which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \see StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost
 * \see StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials
 * \see StreamTubeChannel::supportsUnixSocketsOnLocalhost
 * \see StreamTubeChannel::supportsUnixSocketsWithCredentials
 */
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

    SocketAccessControl accessControl = requireCredentials ?
            SocketAccessControlCredentials :
            SocketAccessControlLocalhost;
    setAddressType(SocketAddressTypeUnix);

    // Fail early if the combination is not supported
    if ( (accessControl == SocketAccessControlLocalhost &&
          addressType() == SocketAddressTypeUnix &&
          !supportsUnixSocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlCredentials &&
          addressType() == SocketAddressTypeUnix &&
          !supportsUnixSocketsWithCredentials()) ||
         (accessControl == SocketAccessControlLocalhost &&
          addressType() == SocketAddressTypeAbstractUnix &&
          !supportsAbstractUnixSocketsOnLocalhost()) ||
         (accessControl == SocketAccessControlCredentials &&
          addressType() == SocketAddressTypeAbstractUnix &&
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
                    addressType(),
                    accessControl,
                    QDBusVariant(QVariant(QString()))),
            IncomingStreamTubeChannelPtr(this));

    PendingStreamTubeConnection *op = new PendingStreamTubeConnection(pv, addressType(),
            IncomingStreamTubeChannelPtr(this));
    connect(op, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAcceptTubeFinished(Tp::PendingOperation*)));
    return op;
}


/**
 * \return A valid QIODevice if the tube is in the \c Open state, 0 otherwise.
 */
QIODevice* IncomingStreamTubeChannel::device()
{
    if (tubeState() == TubeChannelStateOpen) {
        return mPriv->device;
    } else {
        return 0;
    }
}

}
