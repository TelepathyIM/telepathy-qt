/**
 * This file is part of TelepathyQt4
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

    // Public object
    IncomingStreamTubeChannel *parent;
    static bool initRandom;
};

bool IncomingStreamTubeChannel::Private::initRandom = true;

IncomingStreamTubeChannel::Private::Private(IncomingStreamTubeChannel *parent)
    : parent(parent)
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
 * It provides a set of overloads for accepting a variety of sockets over
 * a stream tube.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * IncomingStreamTubeChannel object usable.
 *
 * This is currently the same as StreamTubeChannel::FeatureCore, but may change to include more.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature IncomingStreamTubeChannel::FeatureCore =
    Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 0); // ST::FeatureCore

/**
 * Create a new IncomingStreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A IncomingStreamTubeChannelPtr object pointing to the newly created
 *         IncomingStreamTubeChannel object.
 */
IncomingStreamTubeChannelPtr IncomingStreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return IncomingStreamTubeChannelPtr(new IncomingStreamTubeChannel(connection, objectPath,
            immutableProperties, IncomingStreamTubeChannel::FeatureCore));
}

/**
 * Construct a new IncomingStreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on IncomingStreamTubeChannel::FeatureCore.
 */
IncomingStreamTubeChannel::IncomingStreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : StreamTubeChannel(connection, objectPath,
            immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
IncomingStreamTubeChannel::~IncomingStreamTubeChannel()
{
    delete mPriv;
}

/**
 * Accept an incoming stream tube as a TCP socket.
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the #TubeStateLocalPending state.
 *
 * This overload lets you specify an allowed address/port combination for connecting to the socket.
 * Otherwise, you can specify QHostAddress::Any to accept every incoming connection from localhost,
 * or use the other overload.
 *
 * Note that when using QHostAddress::Any, \a allowedPort is ignored.
 *
 * This method requires IncomingStreamTubeChannel::FeatureCore to be enabled.
 *
 * \param allowedAddress An allowed address for connecting to the socket, or QHostAddress::Any
 * \param allowedPort An allowed port for connecting to the socket.
 * \return A PendingStreamTubeConnection which will finish as soon as the tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 * \sa StreamTubeChannel::supportsIPv4SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress(),
 *     StreamTubeChannel::supportsIPv6SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress()
 */
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsTcpSocket(
        const QHostAddress &allowedAddress,
        quint16 allowedPort)
{
    if (!isReady(IncomingStreamTubeChannel::FeatureCore)) {
        warning() << "IncomingStreamTubeChannel::FeatureCore must be ready before "
                "calling acceptTubeAsTcpSocket";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
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

    setAccessControl(accessControl);

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
            false, 0, IncomingStreamTubeChannelPtr(this));
    return op;
}

/**
 * Accept an incoming stream tube as a TCP socket.
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the #TubeStateLocalPending state.
 *
 * This overload will open a tube which accepts every incoming connection from localhost.
 *
 * Note that this is the equivalent of calling acceptTubeAsTcpSocket(QHostAddress::Any, 0).
 *
 * This method requires IncomingStreamTubeChannel::FeatureCore to be enabled.
 *
 * \return A PendingStreamTubeConnection which will finish as soon as the tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 * \sa StreamTubeChannel::supportsIPv4SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress(),
 *     StreamTubeChannel::supportsIPv6SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress()
 */
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsTcpSocket()
{
    return acceptTubeAsTcpSocket(QHostAddress::Any, 0);
}

/**
 * Accept an incoming stream tube as a Unix socket.
 *
 * This method accepts an incoming connection request for a stream tube. It can be called
 * only if the tube is in the #TubeStateLocalPending state.
 *
 * You can also specify whether the server should require an SCM_CRED or SCM_CREDENTIALS message
 * upon connection instead of accepting every incoming connection from localhost.
 *
 * This method requires IncomingStreamTubeChannel::FeatureCore to be enabled.
 *
 * \param requireCredentials Whether the server should require an SCM_CRED or SCM_CREDENTIALS message
 *                           upon connection.
 * \return A PendingStreamTubeConnection which will finish as soon as the tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 * \sa StreamTubeChannel::supportsUnixSocketsOnLocalhost(),
 *     StreamTubeChannel::supportsUnixSocketsWithCredentials(),
 *     StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost(),
 *     StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials()
 */
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsUnixSocket(
        bool requireCredentials)
{
    if (!isReady(IncomingStreamTubeChannel::FeatureCore)) {
        warning() << "IncomingStreamTubeChannel::FeatureCore must be ready before "
                "calling acceptTubeAsUnixSocket";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingStreamTubeConnection(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    SocketAccessControl accessControl = requireCredentials ?
            SocketAccessControlCredentials :
            SocketAccessControlLocalhost;
    setAddressType(SocketAddressTypeUnix);
    setAccessControl(accessControl);

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

    QDBusVariant accessControlParam;
    uchar credentialByte = 0;
    if (accessControl == SocketAccessControlLocalhost) {
        accessControlParam.setVariant(qVariantFromValue(static_cast<uint>(0)));
    } else if (accessControl == SocketAccessControlCredentials) {
        if (mPriv->initRandom) {
            qsrand(QTime::currentTime().msec());
            mPriv->initRandom = false;
        }
        credentialByte = static_cast<uchar>(qrand());
        accessControlParam.setVariant(qVariantFromValue(credentialByte));
    } else {
        Q_ASSERT(false);
    }

    // Perform the actual call
    PendingVariant *pv = new PendingVariant(
            interface<Client::ChannelTypeStreamTubeInterface>()->Accept(
                    addressType(),
                    accessControl,
                    accessControlParam),
            IncomingStreamTubeChannelPtr(this));

    PendingStreamTubeConnection *op = new PendingStreamTubeConnection(pv, addressType(),
            requireCredentials, credentialByte, IncomingStreamTubeChannelPtr(this));
    return op;
}

/**
 * Return the local address of the opened tube.
 *
 * Calling this method when the tube has not been opened will cause it
 * to return an undefined value. The same will happen if the tube has been accepted as a TCP
 * socket. Use ipAddress() if that is the case.
 *
 * \return The local address representing this opened tube as a QString
 *         if the tube has been accepted as an Unix socket, or an undefined value otherwise.
 * \sa acceptTubeAsUnixSocket(), ipAddress()
 */
QString IncomingStreamTubeChannel::localAddress() const
{
    return StreamTubeChannel::localAddress();
}

/**
 * Return the IP address/port combination of the opened tube.
 *
 * Calling this method when the tube has not been opened will cause it
 * to return an undefined value. The same will happen if the tube has been accepted as an Unix
 * socket. Use localAddress() if that is the case.
 *
 * \return The IP address/port combination representing this opened tube
 *         if the tube has been accepted as a TCP socket, or an undefined value otherwise.
 * \sa acceptTubeAsTcpSocket(), localAddress()
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

}
