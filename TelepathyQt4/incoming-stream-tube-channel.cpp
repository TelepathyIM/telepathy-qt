/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt4/IncomingStreamTubeChannel>

#include "TelepathyQt4/_gen/incoming-stream-tube-channel.moc.hpp"

#include "TelepathyQt4/types-internal.h"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingStreamTubeConnection>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include <QHostAddress>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT IncomingStreamTubeChannel::Private
{
    Private(IncomingStreamTubeChannel *parent);
    ~Private();

    // Public object
    IncomingStreamTubeChannel *parent;
};

IncomingStreamTubeChannel::Private::Private(IncomingStreamTubeChannel *parent)
    : parent(parent)
{
}

IncomingStreamTubeChannel::Private::~Private()
{
}

/**
 * \class IncomingStreamTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/incoming-stream-tube-channel.h <TelepathyQt4/IncomingStreamTubeChannel>
 *
 * \brief The IncomingStreamTubeChannel class represents a Telepathy channel
 * of type StreamTube for incoming stream tubes.
 *
 * In particular, this class is meant to be used as a comfortable way for
 * accepting incoming stream tubes.
 * It provides an easy way for obtaining a QIODevice which can be used for
 * communication.
 *
 * \section incoming_stream_tube_usage_sec Usage
 *
 * \subsection incoming_stream_tube_create_sec Obtaining an incoming stream tube
 *
 * Whenever a contact invites you to open an incoming stream tube, if you are registered
 * as a channel handler for the channel type #TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE,
 * you will be notified of the offer and you will be able to handle the channel.
 * Please refer to the documentation of #AbstractClientHandler for more
 * details on channel handling.
 *
 * Supposing your channel handler has been created correctly, you would do:
 *
 * \code
 * void MyChannelManager::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
 *                               const Tp::AccountPtr &account,
 *                               const Tp::ConnectionPtr &connection,
 *                               const QList<Tp::ChannelPtr> &channels,
 *                               const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
 *                               const QDateTime &userActionTime,
 *                               const QVariantMap &handlerInfo)
 * {
 *     foreach(const Tp::ChannelPtr &channel, channels) {
 *         QVariantMap properties = channel->immutableProperties();
 *
 *         if (properties[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] ==
 *                        TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE) {
 *
 *             // Handle the channel
 *             Tp::IncomingStreamTubeChannelPtr myTube =
 *                      Tp::IncomingStreamTubeChannelPtr::dynamicCast(channel);
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
 * are ready. In this case, we need to enable TubeChannel::FeatureTube and
 * StreamTubeChannel::FeatureStreamTube.
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
 * #acceptTubeAsUnixSocket to accept a socket over the tube. Please note that the socket type
 * is \b not dependent on what has been offered on the other end: choosing whether you want
 * to accept a TCP socket or an Unix socket boils down to your personal preference only
 * and does not affect in any way the way the communication with the other end will be handled.
 *
 * Let's consider the case of accepting an Unix socket without applying
 * any access control restriction. You would simply do:
 *
 * \code
 * PendingStreamTubeConnection *pendingConnection = myTube->acceptTubeAsUnixSocket();
 * \endcode
 *
 * The returned PendingOperation serves both for monitoring the state of the tube and for
 * obtaining, upon success, either the address of the new socket or a QIODevice ready to be used.
 * When the operation finishes, you can do:
 *
 * \code
 * void MyTubeReceiver::onStreamTubeAccepted(PendingOperation *op)
 * {
 *     if (op->isError()) {
 *        return;
 *     }
 *
 *     PendingStreamTubeConnection *pendingConnection =
 *                                              qobject_cast<PendingStreamTubeConnection*>(op);
 *
 *     QIODevice *mySocketReadyToBeUsed = pendingConnection->device();
 *     // Do some stuff here
 * }
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
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
            immutableProperties, coreFeature),
      mPriv(new Private(this))
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
 * Accept an incoming Stream tube as a TCP socket.
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
 * Otherwise, you can specify QHostAddress::Any to accept every incoming connection from localhost,
 * or use the other overload.
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
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsTcpSocket(
        const QHostAddress &allowedAddress,
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
            warning() << "acceptTubeAsTcpSocket can be called only with a QHostAddress "
                    "representing an IPv4 or IPv6 address";
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
    if ((accessControl == SocketAccessControlLocalhost &&
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
                QLatin1String("The requested address type/access control "
                              "combination is not supported"),
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
    return op;
}

/**
 * Accept an incoming Stream tube as a TCP socket.
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the \c LocalPending state.
 *
 * Once called, this method will try opening the tube, and will create a QIODevice which
 * can be used to communicate with the other end. The device is guaranteed to be opened
 * read/write when the resulting \c PendingStreamTubeConnection finishes successfully. You can then
 * access the device right from \c PendingStreamTubeConnection or from %device().
 *
 * This overload will open a tube which accepts every incoming connection from Localhost.
 * Please note that this is the equivalent of calling acceptTubeAsTcpSocket(QHostAddress::Any, 0).
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
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsTcpSocket()
{
    return acceptTubeAsTcpSocket(QHostAddress::Any, 0);
}

/**
 * Accept an incoming Stream tube as a Unix socket.
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
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsUnixSocket(
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
    if ((accessControl == SocketAccessControlLocalhost &&
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
                QLatin1String("The requested address type/access control "
                        "combination is not supported"),
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
 * Return the local address of the opened tube.
 *
 * Calling this method when the tube has not been opened will cause it
 * to return an unmeaningful value. The same will happen if the tube has been accepted as a TCP
 * socket. Use #ipAddress if that is the case.
 *
 * \return The local address representing this opened tube as a QString,
 *         if the tube has been accepted as an Unix socket.
 *
 * \note This function will return a meaningful value only if the tube has already been opened.
 */
QString IncomingStreamTubeChannel::localAddress() const
{
    return StreamTubeChannel::localAddress();
}

/**
 * Return the IP address/port combination of the opened tube.
 *
 * Calling this method when the tube has not been opened will cause it
 * to return an unmeaningful value. The same will happen if the tube has been accepted as an Unix
 * socket. Use #localAddress if that is the case.
 *
 * \return The IP address/port combination representing this opened tube,
 *         if the tube has been accepted as a TCP socket.
 *
 * \note This function will return a meaningful value only if the tube has already been opened.
 */
QPair<QHostAddress, quint16> IncomingStreamTubeChannel::ipAddress() const
{
    return StreamTubeChannel::ipAddress();
}

void IncomingStreamTubeChannel::onNewLocalConnection(uint connectionId)
{
    // Add the connection to our list
    UIntList currentConnections = connections();
    currentConnections << connectionId;
    setConnections(currentConnections);

    emit newConnection(connectionId);
}

// Signals documentation
/**
 * \fn void StreamTubeChannel::newLocalConnection(uint connectionId)
 *
 * Emitted when the tube application connects to ConnectionManager's socket
 *
 * \param connectionId The unique ID associated with this connection.
 */

}
