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

#include <TelepathyQt/IncomingStreamTubeChannel>

#include "TelepathyQt/_gen/incoming-stream-tube-channel.moc.hpp"

#include "TelepathyQt/types-internal.h"
#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingStreamTubeConnection>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/Types>

#include <QHostAddress>

namespace Tp
{

struct TP_QT_NO_EXPORT IncomingStreamTubeChannel::Private
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
 * \headerfile TelepathyQt/incoming-stream-tube-channel.h <TelepathyQt/IncomingStreamTubeChannel>
 *
 * \brief The IncomingStreamTubeChannel class represents an incoming Telepathy channel
 * of type StreamTube.
 *
 * In particular, this class is meant to be used as a comfortable way for
 * accepting incoming stream tubes. Tubes can be accepted as TCP and/or Unix sockets with various
 * access control methods depending on what the service supports using the various overloads of
 * acceptTubeAsTcpSocket() and acceptTubeAsUnixSocket().
 *
 * Once a tube is successfully accepted and open (the PendingStreamTubeConnection returned from the
 * accepting methods has finished), the application can connect to the socket the address of which
 * can be retrieved from PendingStreamTubeConnection::ipAddress() and/or
 * PendingStreamTubeConnection::localAddress() depending on which accepting method was used.
 * Connecting to this socket will open a tunneled connection to the service listening at the
 * offering end of the tube.
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
 * The connection manager will open a TCP socket for the application to connect to. The address of
 * the socket will be returned in PendingStreamTubeConnection::ipAddress() once the operation has
 * finished successfully.
 *
 * This overload lets you specify an allowed address/port combination for connecting to the CM
 * socket. Connections with other source addresses won't be accepted. The accessors
 * supportsIPv4SocketsWithSpecifiedAddress() and supportsIPv6SocketsWithSpecifiedAddress() can be
 * used to verify that the connection manager supports this kind of access control; otherwise, this
 * method will always fail unless QHostAddress::Any (or QHostAddress::AnyIPv4 in Qt5) or
 * QHostAddress::AnyIPv6 is passed, in which case the behavior is identical to the always supported
 * acceptTubeAsTcpSocket() overload.
 *
 * Note that when using QHostAddress::Any (or QHostAddress::AnyIPv4 in Qt5) or
 * QHostAddress::AnyIPv6, \a allowedPort is ignored.
 *
 * This method requires IncomingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param allowedAddress An allowed address for connecting to the socket.
 * \param allowedPort An allowed port for connecting to the socket.
 * \return A PendingStreamTubeConnection which will emit PendingStreamTubeConnection::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 */
PendingStreamTubeConnection *IncomingStreamTubeChannel::acceptTubeAsTcpSocket(
        const QHostAddress &allowedAddress,
        quint16 allowedPort)
{
    if (!isReady(IncomingStreamTubeChannel::FeatureCore)) {
        warning() << "IncomingStreamTubeChannel::FeatureCore must be ready before "
                "calling acceptTubeAsTcpSocket";
        return new PendingStreamTubeConnection(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingStreamTubeConnection(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    QVariant controlParameter;
    SocketAccessControl accessControl;
    QHostAddress hostAddress = allowedAddress;

#if QT_VERSION >= 0x050000
    if (hostAddress == QHostAddress::Any) {
        hostAddress = QHostAddress::AnyIPv4;
    }
#endif

    // Now, let's check what we need to do with accessControl. There is just one special case, Port.
    if (hostAddress != QHostAddress::Any &&
#if QT_VERSION >= 0x050000
        hostAddress != QHostAddress::AnyIPv4 &&
#endif
        hostAddress != QHostAddress::AnyIPv6) {
        // We need to have a valid QHostAddress AND Port.
        if (hostAddress.isNull() || allowedPort == 0) {
            warning() << "You have to set a valid allowed address+port to use Port access control";
            return new PendingStreamTubeConnection(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("The supplied allowed address and/or port was invalid"),
                    IncomingStreamTubeChannelPtr(this));
        }

        accessControl = SocketAccessControlPort;

        // IPv4 or IPv6?
        if (hostAddress.protocol() == QAbstractSocket::IPv4Protocol) {
            // IPv4 case
            SocketAddressIPv4 addr;
            addr.address = hostAddress.toString();
            addr.port = allowedPort;

            controlParameter = QVariant::fromValue(addr);
        } else if (hostAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            // IPv6 case
            SocketAddressIPv6 addr;
            addr.address = hostAddress.toString();
            addr.port = allowedPort;

            controlParameter = QVariant::fromValue(addr);
        } else {
            // We're handling an IPv4/IPv6 socket only
            warning() << "acceptTubeAsTcpSocket can be called only with a QHostAddress "
                    "representing an IPv4 or IPv6 address";
            return new PendingStreamTubeConnection(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("Invalid host given"),
                    IncomingStreamTubeChannelPtr(this));
        }
    } else {
        // We have to do no special stuff here
        accessControl = SocketAccessControlLocalhost;
        // Since QDBusMarshaller does not like null variants, just add an empty string.
        controlParameter = QVariant(QString());
    }

    // Set the correct address type and access control
    setAddressType(hostAddress.protocol() == QAbstractSocket::IPv4Protocol ?
            SocketAddressTypeIPv4 :
            SocketAddressTypeIPv6);
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
        return new PendingStreamTubeConnection(TP_QT_ERROR_NOT_IMPLEMENTED,
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
 * The connection manager will open a TCP socket for the application to connect to. The address of
 * the socket will be returned in PendingStreamTubeConnection::ipAddress() once the operation has
 * finished successfully.
 *
 * Using this overload, the connection manager will accept every incoming connection from localhost.
 *
 * This accept method must be supported by all connection managers adhering to the \telepathy_spec.
 *
 * This method requires IncomingStreamTubeChannel::FeatureCore to be ready.
 *
 * \return A PendingStreamTubeConnection which will emit PendingStreamTubeConnection::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
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
 * An Unix socket (can be used with QLocalSocket or alike) will be opened by the connection manager
 * as the local tube endpoint. This is only supported if supportsUnixSocketsOnLocalhost() is \c
 * true.
 *
 * You can also specify whether the CM should require an SCM_CREDS or SCM_CREDENTIALS message
 * upon connection instead of accepting every incoming connection from localhost. This provides
 * additional security, but requires sending the byte retrieved from
 * PendingStreamTubeConnection::credentialByte() in-line in the socket byte stream (in a credentials
 * message if available on the platform), which might not be compatible with all protocols or
 * libraries. Also, only connection managers for which supportsUnixSocketsWithCredentials() is \c
 * true support this type of access control.
 *
 * This method requires IncomingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param requireCredentials Whether the CM should require an SCM_CREDS or SCM_CREDENTIALS message
 *                           upon connection.
 * \return A PendingStreamTubeConnection which will emit PendingStreamTubeConnection::finished
 *         when the stream tube is ready to be used
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
        return new PendingStreamTubeConnection(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                IncomingStreamTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingStreamTubeConnection(TP_QT_ERROR_NOT_AVAILABLE,
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
        return new PendingStreamTubeConnection(TP_QT_ERROR_NOT_IMPLEMENTED,
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

void IncomingStreamTubeChannel::onNewLocalConnection(uint connectionId)
{
    addConnection(connectionId);
}

}
