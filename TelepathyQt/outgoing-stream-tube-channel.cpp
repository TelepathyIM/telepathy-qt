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

#include <TelepathyQt/OutgoingStreamTubeChannel>
#include "TelepathyQt/outgoing-stream-tube-channel-internal.h"

#include "TelepathyQt/_gen/outgoing-stream-tube-channel.moc.hpp"
#include "TelepathyQt/_gen/outgoing-stream-tube-channel-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/types-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/Types>

#include <QHostAddress>
#include <QTcpServer>
#include <QLocalServer>

namespace Tp
{

PendingOpenTube::Private::Private(const QVariantMap &parameters, PendingOpenTube *parent)
    : parent(parent),
      parameters(parameters)
{
}

PendingOpenTube::PendingOpenTube(
        PendingVoid *offerOperation,
        const QVariantMap &parameters,
        const OutgoingStreamTubeChannelPtr &object)
    : PendingOperation(object),
      mPriv(new Private(parameters, this))
{
    mPriv->tube = object;

    // FIXME: connect to channel invalidation here also

    debug() << "Calling StreamTube.Offer";
    if (offerOperation->isFinished()) {
        onOfferFinished(offerOperation);
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

void PendingOpenTube::onOfferFinished(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "StreamTube.Offer failed with " <<
            op->errorName() << ": " << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "StreamTube.Offer returned successfully";

    // It might have been already opened - check
    if (mPriv->tube->state() != TubeChannelStateOpen) {
        debug() << "Awaiting tube to be opened";
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(),
                SIGNAL(stateChanged(Tp::TubeChannelState)),
                SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }

    onTubeStateChanged(mPriv->tube->state());
}

void PendingOpenTube::onTubeStateChanged(TubeChannelState state)
{
    if (state == TubeChannelStateOpen) {
        debug() << "Tube is now opened";
        // Inject the parameters into the tube
        mPriv->tube->setParameters(mPriv->parameters);
        // The tube is ready: let's notify
        setFinished();
    } else {
        if (state != TubeChannelStateRemotePending) {
            warning() << "Offering tube failed with" << TP_QT_ERROR_CONNECTION_REFUSED;
            // Something happened
            setFinishedWithError(TP_QT_ERROR_CONNECTION_REFUSED,
                    QLatin1String("The connection to this tube was refused"));
        } else {
            debug() << "Awaiting remote to accept the tube";
        }
    }
}

QueuedContactFactory::QueuedContactFactory(Tp::ContactManagerPtr contactManager, QObject* parent)
    : QObject(parent),
      m_isProcessing(false),
      m_manager(contactManager)
{
}

QueuedContactFactory::~QueuedContactFactory()
{
}

void QueuedContactFactory::processNextRequest()
{
    if (m_isProcessing) {
        // Return, nothing to do
        return;
    }

    if (m_queue.isEmpty()) {
        // Queue completed, notify and return
        emit queueCompleted();
        return;
    }

    m_isProcessing = true;

    Entry entry = m_queue.dequeue();

    // TODO: pass id hints to ContactManager if we ever gain support to retrieve contact ids
    //       from NewRemoteConnection.
    PendingContacts *pc = m_manager->contactsForHandles(entry.handles);
    pc->setProperty("__TpQt__QueuedContactFactoryUuid", entry.uuid.toString());
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onPendingContactsFinished(Tp::PendingOperation*)));
}

QUuid QueuedContactFactory::appendNewRequest(const Tp::UIntList &handles)
{
    // Create a new entry
    Entry entry;
    entry.uuid = QUuid::createUuid();
    entry.handles = handles;
    m_queue.enqueue(entry);

    // Enqueue a process request in the event loop
    QTimer::singleShot(0, this, SLOT(processNextRequest()));

    // Return the UUID
    return entry.uuid;
}

void QueuedContactFactory::onPendingContactsFinished(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts*>(op);

    QUuid uuid = QUuid(pc->property("__TpQt__QueuedContactFactoryUuid").toString());

    emit contactsRetrieved(uuid, pc->contacts());

    // No longer processing
    m_isProcessing = false;

    // Go for next one
    processNextRequest();
}

OutgoingStreamTubeChannel::Private::Private(OutgoingStreamTubeChannel *parent)
    : parent(parent),
      queuedContactFactory(new QueuedContactFactory(parent->connection()->contactManager(), parent))
{
}

/**
 * \class OutgoingStreamTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/outgoing-stream-tube-channel.h <TelepathyQt/OutgoingStreamTubeChannel>
 *
 * \brief The OutgoingStreamTubeChannel class represents an outgoing Telepathy channel
 * of type StreamTube.
 *
 * Outgoing (locally initiated/requested) tubes are initially in the #TubeChannelStateNotOffered state. The
 * various offer methods in this class can be used to offer a local listening TCP or Unix socket for
 * the tube's target to connect to, at which point the tube becomes #TubeChannelStateRemotePending.
 * If the target accepts the connection request, the state goes #TubeChannelStateOpen and the
 * connection manager will start tunneling any incoming connections from the recipient side to the
 * local service.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * OutgoingStreamTubeChannel object usable.
 *
 * This is currently the same as StreamTubeChannel::FeatureCore, but may change to include more.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature OutgoingStreamTubeChannel::FeatureCore =
    Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 0); // ST::FeatureCore

/**
 * Create a new OutgoingStreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A OutgoingStreamTubeChannelPtr object pointing to the newly created
 *         OutgoingStreamTubeChannel object.
 */
OutgoingStreamTubeChannelPtr OutgoingStreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return OutgoingStreamTubeChannelPtr(new OutgoingStreamTubeChannel(connection, objectPath,
            immutableProperties, OutgoingStreamTubeChannel::FeatureCore));
}

/**
 * Construct a new OutgoingStreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on OutgoingStreamTubeChannel::FeatureCore.
 */
OutgoingStreamTubeChannel::OutgoingStreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : StreamTubeChannel(connection, objectPath,
                        immutableProperties, coreFeature),
      mPriv(new Private(this))
{
    connect(mPriv->queuedContactFactory,
            SIGNAL(contactsRetrieved(QUuid,QList<Tp::ContactPtr>)),
            this,
            SLOT(onContactsRetrieved(QUuid,QList<Tp::ContactPtr>)));
}

/**
 * Class destructor.
 */
OutgoingStreamTubeChannel::~OutgoingStreamTubeChannel()
{
    delete mPriv;
}

/**
 * Offer a TCP socket over this stream tube.
 *
 * This method offers a TCP socket over this tube. The socket's address is given as
 * a QHostAddress and a numerical port in native byte order.
 *
 * If your application uses QTcpServer as the local TCP server implementation, you can use the
 * offerTcpSocket(const QTcpServer *, const QVariantMap &) overload instead to more easily pass the
 * server's listen address.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed,
 * the tube will be opened and ready to be used.
 *
 * Connection managers adhering to the \telepathy_spec should always support offering IPv4 TCP
 * sockets. IPv6 sockets are only supported if supportsIPv6SocketsOnLocalhost() is \c true.
 *
 * Note that the library will try to use #SocketAccessControlPort access control whenever possible,
 * as it allows to map connections to users based on their source addresses. If
 * supportsIPv4SocketsWithSpecifiedAddress() or supportsIPv6SocketsWithSpecifiedAddress() for IPv4
 * and IPv6 sockets respectively is \c false, this feature is not available, and the
 * connectionsForSourceAddresses() map won't contain useful distinct keys.
 *
 * Arbitrary parameters can be associated with the offer to bootstrap legacy protocols; these will
 * in particular be available as IncomingStreamTubeChannel::parameters() for a tube receiver
 * implemented using TelepathyQt in the other end.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param address A valid IPv4 or IPv6 address pointing to an existing socket.
 * \param port The port the socket is listening for connections to.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 */
PendingOperation *OutgoingStreamTubeChannel::offerTcpSocket(
        const QHostAddress &address,
        quint16 port,
        const QVariantMap &parameters)
{
    if (!isReady(OutgoingStreamTubeChannel::FeatureCore)) {
        warning() << "OutgoingStreamTubeChannel::FeatureCore must be ready before "
                "calling offerTube";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                OutgoingStreamTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (state() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a socket for each Stream Tube";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel busy"),
                OutgoingStreamTubeChannelPtr(this));
    }

    SocketAccessControl accessControl = SocketAccessControlLocalhost;
    // Check if port is supported

    QHostAddress hostAddress = address;
#if QT_VERSION >= 0x050000
    if (hostAddress == QHostAddress::Any) {
        hostAddress = QHostAddress::AnyIPv4;
    }
#endif

    // In this specific overload, we're handling an IPv4/IPv6 socket
    if (hostAddress.protocol() == QAbstractSocket::IPv4Protocol) {
        // IPv4 case
        SocketAccessControl accessControl;
        // Do some heuristics to find out the best access control.We always prefer port for tracking
        // connections and source addresses.
        if (supportsIPv4SocketsWithSpecifiedAddress()) {
            accessControl = SocketAccessControlPort;
        } else if (supportsIPv4SocketsOnLocalhost()) {
            accessControl = SocketAccessControlLocalhost;
        } else {
            // There are no combinations supported for this socket
            warning() << "You requested an address type/access control combination "
                    "not supported by this channel";
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("The requested address type/access control "
                            "combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeIPv4);
        setAccessControl(accessControl);
        setIpAddress(qMakePair<QHostAddress, quint16>(hostAddress, port));

        SocketAddressIPv4 addr;
        addr.address = hostAddress.toString();
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
    } else if (hostAddress.protocol() == QAbstractSocket::IPv6Protocol) {
        // IPv6 case
        // Do some heuristics to find out the best access control.We always prefer port for tracking
        // connections and source addresses.
        if (supportsIPv6SocketsWithSpecifiedAddress()) {
            accessControl = SocketAccessControlPort;
        } else if (supportsIPv6SocketsOnLocalhost()) {
            accessControl = SocketAccessControlLocalhost;
        } else {
            // There are no combinations supported for this socket
            warning() << "You requested an address type/access control combination "
                    "not supported by this channel";
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("The requested address type/access control "
                            "combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeIPv6);
        setAccessControl(accessControl);
        setIpAddress(qMakePair<QHostAddress, quint16>(hostAddress, port));

        SocketAddressIPv6 addr;
        addr.address = hostAddress.toString();
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
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Invalid host given"),
                OutgoingStreamTubeChannelPtr(this));
    }

}

/**
 * Offer a TCP socket over this stream tube.
 *
 * Otherwise identical to offerTcpSocket(const QHostAddress &, quint16, const QVariantMap &), but
 * allows passing the local service's address in an already listening QTcpServer.
 *
 * \param server A valid QTcpServer, which should be already listening for incoming connections.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 */
PendingOperation *OutgoingStreamTubeChannel::offerTcpSocket(
        const QTcpServer *server,
        const QVariantMap &parameters)
{
    // In this overload, we're handling a superset of QHostAddress.
    // Let's redirect the call to QHostAddress's overload
    return offerTcpSocket(server->serverAddress(), server->serverPort(),
            parameters);
}

/**
 * Offer an Unix socket over this stream tube.
 *
 * This method offers an Unix socket over this stream tube. The socket address is given as a
 * a QString, which should contain the path to the socket. Abstract Unix sockets are also supported,
 * and are given as addresses prefixed with a \c NUL byte.
 *
 * If your application uses QLocalServer as the local Unix server implementation, you can use the
 * offerUnixSocket(const QLocalServer *, const QVariantMap &, bool) overload instead to more easily
 * pass the server's listen address.
 *
 * Note that only connection managers for which supportsUnixSocketsOnLocalhost() or
 * supportsAbstractUnixSocketsOnLocalhost() is \c true support exporting Unix sockets.
 *
 * If supportsUnixSocketsWithCredentials() or supportsAbstractUnixSocketsWithCredentials(), as
 * appropriate, returns \c true, the \c requireCredentials parameter can be set to \c true to make
 * the connection manager pass an SCM_CREDS or SCM_CREDENTIALS message as supported by the platform
 * when making a new connection. This enables preventing other local users from connecting to the
 * service, but might not be possible to use with all protocols as the message is in-band in the
 * data stream.
 *
 * Arbitrary parameters can be associated with the offer to bootstrap legacy protocols; these will
 * in particular be available as IncomingStreamTubeChannel::parameters() for a tube receiver
 * implemented using TelepathyQt in the other end.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param socketAddress A valid path to an existing Unix socket or abstract Unix socket.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 * \param requireCredentials Whether the server requires a SCM_CREDS or SCM_CREDENTIALS message
 *                           upon connection.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 */
PendingOperation *OutgoingStreamTubeChannel::offerUnixSocket(
        const QString &socketAddress,
        const QVariantMap &parameters,
        bool requireCredentials)
{
    SocketAccessControl accessControl = requireCredentials ?
            SocketAccessControlCredentials :
            SocketAccessControlLocalhost;

    if (!isReady(OutgoingStreamTubeChannel::FeatureCore)) {
        warning() << "OutgoingStreamTubeChannel::FeatureCore must be ready before "
                "calling offerTube";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                OutgoingStreamTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (state() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a socket for each Stream Tube";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel busy"), OutgoingStreamTubeChannelPtr(this));
    }

    // In this specific overload, we're handling an Unix/AbstractUnix socket
    if (socketAddress.startsWith(QLatin1Char('\0'))) {
        // Abstract Unix socket case
        // Check if the combination type/access control is supported
        if ((accessControl == SocketAccessControlLocalhost &&
                !supportsAbstractUnixSocketsOnLocalhost()) ||
            (accessControl == SocketAccessControlCredentials &&
                !supportsAbstractUnixSocketsWithCredentials()) ) {
            warning() << "You requested an address type/access control combination "
                    "not supported by this channel";
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("The requested address type/access control "
                            "combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeAbstractUnix);
        setAccessControl(accessControl);
        setLocalAddress(socketAddress);

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
        if ((accessControl == SocketAccessControlLocalhost &&
                !supportsUnixSocketsOnLocalhost()) ||
            (accessControl == SocketAccessControlCredentials &&
                !supportsUnixSocketsWithCredentials()) ||
            (accessControl != SocketAccessControlLocalhost &&
                accessControl != SocketAccessControlCredentials) ) {
            warning() << "You requested an address type/access control combination "
                "not supported by this channel";
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("The requested address type/access control "
                            "combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeUnix);
        setAccessControl(accessControl);
        setLocalAddress(socketAddress);

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
 * Offer an Unix socket over the tube.
 *
 * Otherwise identical to offerUnixSocket(const QString &, const QVariantMap &, bool), but allows
 * passing the local service's address as an already listening QLocalServer.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param server A valid QLocalServer, which should be already listening for incoming connections.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 * \param requireCredentials Whether the server should require a SCM_CRED or SCM_CREDENTIALS message
 *                           upon connection.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 * \sa StreamTubeChannel::supportsUnixSocketsOnLocalhost(),
 *     StreamTubeChannel::supportsUnixSocketsWithCredentials(),
 *     StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost(),
 *     StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials()
 */
PendingOperation *OutgoingStreamTubeChannel::offerUnixSocket(
        const QLocalServer *server,
        const QVariantMap &parameters,
        bool requireCredentials)
{
    // In this overload, we're handling a superset of a local socket
    // Let's redirect the call to QString's overload
    return offerUnixSocket(server->fullServerName(), parameters, requireCredentials);
}

/**
 * Return a map from a source address to the corresponding connections ids.
 *
 * The connection ids retrieved here can be used to map a source address
 * which connected to your socket to a connection ID (for error reporting) and further, to a contact
 * (by using contactsForConnections()).
 *
 * This method is only useful if a TCP socket was offered on this tube and the connection manager
 * supports #SocketAccessControlPort, which can be discovered using
 * supportsIPv4SocketsWithSpecifiedAddress() and supportsIPv6SocketsWithSpecifiedAddress() for IPv4
 * and IPv6 sockets respectively.
 *
 * Note that this function will only return valid data after the tube has been opened.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be ready.
 *
 * \return The map from source addresses as (QHostAddress, port in native byte order) pairs to the
 *     corresponding connection ids.
 * \sa connectionsForCredentials()
 */
QHash<QPair<QHostAddress, quint16>, uint> OutgoingStreamTubeChannel::connectionsForSourceAddresses() const
{
    if (addressType() != SocketAddressTypeIPv4 && addressType() != SocketAddressTypeIPv6) {
        warning() << "OutgoingStreamTubeChannel::connectionsForSourceAddresses() makes sense "
                "just when offering a TCP socket";
        return QHash<QPair<QHostAddress, quint16>, uint>();
    }

    if (isValid() || !isDroppingConnections() ||
            !requestedFeatures().contains(StreamTubeChannel::FeatureConnectionMonitoring)) {
        if (!isReady(StreamTubeChannel::FeatureConnectionMonitoring)) {
            warning() << "StreamTubeChannel::FeatureConnectionMonitoring must be ready before "
                "   calling connectionsForSourceAddresses";
            return QHash<QPair<QHostAddress, quint16>, uint>();
        }

        if (state() != TubeChannelStateOpen) {
            warning() << "OutgoingStreamTubeChannel::connectionsForSourceAddresses() makes sense "
                "just when the tube is open";
            return QHash<QPair<QHostAddress, quint16>, uint>();
        }
    }

    return mPriv->connectionsForSourceAddresses;
}

/**
 * Return a map from a credential byte to the corresponding connections ids.
 *
 * The connection ids retrieved here can be used to map a source address
 * which connected to your socket to a connection ID (for error reporting) and further, to a contact
 * (by using contactsForConnections()).
 *
 * This method is only useful if this tube was offered using an Unix socket and passing credential
 * bytes was enabled (\c requireCredentials == true).
 *
 * Note that this function will only return valid data after the tube has been opened.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be ready.
 *
 * \return The map from credential bytes to the corresponding connection ids.
 * \sa connectionsForSourceAddresses()
 */
QHash<uchar, uint> OutgoingStreamTubeChannel::connectionsForCredentials() const
{
    if (addressType() != SocketAddressTypeUnix && addressType() != SocketAddressTypeAbstractUnix) {
        warning() << "OutgoingStreamTubeChannel::connectionsForCredentials() makes sense "
                "just when offering an Unix socket";
        return QHash<uchar, uint>();
    }

    if (accessControl() != SocketAccessControlCredentials) {
        warning() << "OutgoingStreamTubeChannel::connectionsForCredentials() makes sense "
                "just when offering an Unix socket requiring credentials";
        return QHash<uchar, uint>();
    }

    if (isValid() || !isDroppingConnections() ||
            !requestedFeatures().contains(StreamTubeChannel::FeatureConnectionMonitoring)) {
        if (!isReady(StreamTubeChannel::FeatureConnectionMonitoring)) {
            warning() << "StreamTubeChannel::FeatureConnectionMonitoring must be ready before "
                "calling OutgoingStreamTubeChannel::connectionsForCredentials()";
            return QHash<uchar, uint>();
        }

        if (state() != TubeChannelStateOpen) {
            warning() << "OutgoingStreamTubeChannel::connectionsForCredentials() makes sense "
                "just when the tube is opened";
            return QHash<uchar, uint>();
        }
    }

    return mPriv->connectionsForCredentials;
}

/**
 * Return a map from connection ids to the associated contact.
 *
 * Note that this function will only return valid data after the tube has been opened.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be ready.

 * \return The map from connection ids to pointer to Contact objects.
 * \sa connectionsForSourceAddresses(), connectionsForCredentials(),
 *     StreamTubeChannel::addressType()
 */
QHash<uint, ContactPtr> OutgoingStreamTubeChannel::contactsForConnections() const
{
    if (isValid() || !isDroppingConnections() ||
            !requestedFeatures().contains(StreamTubeChannel::FeatureConnectionMonitoring)) {
        if (!isReady(StreamTubeChannel::FeatureConnectionMonitoring)) {
            warning() << "StreamTubeChannel::FeatureConnectionMonitoring must be ready before "
                "calling contactsForConnections";
            return QHash<uint, ContactPtr>();
        }

        if (state() != TubeChannelStateOpen) {
            warning() << "OutgoingStreamTubeChannel::contactsForConnections() makes sense "
                "just when the tube is open";
            return QHash<uint, ContactPtr>();
        }
    }

    return mPriv->contactsForConnections;
}

void OutgoingStreamTubeChannel::onNewRemoteConnection(
        uint contactId,
        const QDBusVariant &parameter,
        uint connectionId)
{
    // Request the handles from our queued contact factory
    QUuid uuid = mPriv->queuedContactFactory->appendNewRequest(UIntList() << contactId);

    // Add a pending connection
    mPriv->pendingNewConnections.insert(uuid, qMakePair(connectionId, parameter));
}

void OutgoingStreamTubeChannel::onContactsRetrieved(
        const QUuid &uuid,
        const QList<Tp::ContactPtr> &contacts)
{
    if (!isValid()) {
        debug() << "Invalidated OutgoingStreamTubeChannel not emitting queued connection event";
        return;
    }

    if (!mPriv->pendingNewConnections.contains(uuid)) {
        if (mPriv->pendingClosedConnections.contains(uuid)) {
            // closed connection
            Private::ClosedConnection conn = mPriv->pendingClosedConnections.take(uuid);

            // First, do removeConnection() so connectionClosed is emitted, and anybody connected to it
            // (like StreamTubeServer) has a chance to recover the source address / contact
            removeConnection(conn.id, conn.error, conn.message);

            // Remove stuff from our hashes
            mPriv->contactsForConnections.remove(conn.id);

            QHash<QPair<QHostAddress, quint16>, uint>::iterator srcAddrIter =
                mPriv->connectionsForSourceAddresses.begin();
            while (srcAddrIter != mPriv->connectionsForSourceAddresses.end()) {
                if (srcAddrIter.value() == conn.id) {
                    srcAddrIter = mPriv->connectionsForSourceAddresses.erase(srcAddrIter);
                } else {
                    ++srcAddrIter;
                }
            }

            QHash<uchar, uint>::iterator credIter = mPriv->connectionsForCredentials.begin();
            while (credIter != mPriv->connectionsForCredentials.end()) {
                if (credIter.value() == conn.id) {
                    credIter = mPriv->connectionsForCredentials.erase(credIter);
                } else {
                    ++credIter;
                }
            }
        } else {
            warning() << "No pending connections found in OSTC" << objectPath() << "for contacts"
                << contacts;
        }

        return;
    }

    // new connection
    QPair<uint, QDBusVariant> connectionProperties = mPriv->pendingNewConnections.take(uuid);

    // Add it to our connections hash
    foreach (const Tp::ContactPtr &contact, contacts) {
        mPriv->contactsForConnections.insert(connectionProperties.first, contact);
    }

    QPair<QHostAddress, quint16> address;
    address.first = QHostAddress::Null;

    // Now let's try to track the parameter
    if (addressType() == SocketAddressTypeIPv4) {
        // Try a qdbus_cast to our address struct: we're shielded from crashes
        // thanks to our specification
        SocketAddressIPv4 addr =
                qdbus_cast<Tp::SocketAddressIPv4>(connectionProperties.second.variant());
        address.first = QHostAddress(addr.address);
        address.second = addr.port;
    } else if (addressType() == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr =
                qdbus_cast<Tp::SocketAddressIPv6>(connectionProperties.second.variant());
        address.first = QHostAddress(addr.address);
        address.second = addr.port;
    } else if (addressType() == SocketAddressTypeUnix ||
               addressType() == SocketAddressTypeAbstractUnix) {
        if (accessControl() == SocketAccessControlCredentials) {
            uchar credentialByte = qdbus_cast<uchar>(connectionProperties.second.variant());
            mPriv->connectionsForCredentials.insertMulti(credentialByte, connectionProperties.first);
        }
    }

    if (address.first != QHostAddress::Null) {
        // We can map it to a source address as well
        mPriv->connectionsForSourceAddresses.insertMulti(address, connectionProperties.first);
    }

    // Time for us to emit the signal
    addConnection(connectionProperties.first);
}

// This replaces the base class onConnectionClosed() slot, but unlike a virtual function, is ABI
// compatible
void OutgoingStreamTubeChannel::onConnectionClosed(uint connectionId,
        const QString &errorName, const QString &errorMessage)
{
    // Insert a fake request to our queued contact factory to make the close events properly ordered
    // with new connection events
    QUuid uuid = mPriv->queuedContactFactory->appendNewRequest(UIntList());

    // Add a pending connection close
    mPriv->pendingClosedConnections.insert(uuid,
            Private::ClosedConnection(connectionId, errorName, errorMessage));
}

}
