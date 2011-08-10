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

#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include "TelepathyQt4/outgoing-stream-tube-channel-internal.h"

#include "TelepathyQt4/_gen/outgoing-stream-tube-channel.moc.hpp"
#include "TelepathyQt4/_gen/outgoing-stream-tube-channel-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/types-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/Types>

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
            warning() << "Offering tube failed with" << TP_QT4_ERROR_CONNECTION_REFUSED;
            // Something happened
            setFinishedWithError(TP_QT4_ERROR_CONNECTION_REFUSED,
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
    if (m_isProcessing || m_queue.isEmpty()) {
        // Return, nothing to do
        return;
    }

    m_isProcessing = true;

    Entry entry = m_queue.dequeue();

    // TODO: pass id hints to ContactManager if we ever gain support to retrieve contact ids
    //       from NewRemoteConnection.
    PendingContacts *pc = m_manager->contactsForHandles(entry.handles);
    pc->setProperty("__TpQt4__QueuedContactFactoryUuid", entry.uuid.toString());
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

    // Check if we can process a request
    processNextRequest();

    // Return the UUID
    return entry.uuid;
}

void QueuedContactFactory::onPendingContactsFinished(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts*>(op);

    QUuid uuid = QUuid(pc->property("__TpQt4__QueuedContactFactoryUuid").toString());

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
 * \headerfile TelepathyQt4/outgoing-stream-tube-channel.h <TelepathyQt4/OutgoingStreamTubeChannel>
 *
 * \brief The IncomingStreamTubeChannel class represents a Telepathy channel
 * of type StreamTube for outgoing stream tubes.
 *
 * In particular, this class is meant to be used as a comfortable way for
 * exposing new stream tubes.
 * It provides a set of overloads for exporting a variety of sockets over
 * a stream tube.
 *
 * For more details, please refer to \telepathy_spec
 *
 * See \ref async_model, \ref shared_ptr
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
    connect(this, SIGNAL(connectionClosed(uint,QString,QString)),
            this, SLOT(onConnectionClosed(uint,QString,QString)),
            Qt::QueuedConnection);

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
 * This method offers a TCP socket over this tube. The socket is represented through
 * a QHostAddress.
 *
 * If you are already handling a TCP logic in your application, you can also
 * use an overload which accepts a QTcpServer.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed,
 * the tube will be opened and ready to be used.
 *
 * Note that the library will try to use #SocketAccessControlPort access control whenever possible,
 * as it allows to map connections to the socket's source address. This means that if
 * StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress() (or
 * StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress(), depending on
 * the type of the offered socket) returns \c true, this method will automatically
 * enable the connection tracking feature, as long as
 * StreamTubeChannel::FeatureConnectionMonitoring has been enabled.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param address A valid IPv4 or IPv6 address pointing to an existing socket.
 * \param port The port the socket is listening for connections to.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 *                   For more details, please refer to \telepathy_spec.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 * \sa StreamTubeChannel::supportsIPv4SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress(),
 *     StreamTubeChannel::supportsIPv6SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress()
 */
PendingOperation* OutgoingStreamTubeChannel::offerTcpSocket(
        const QHostAddress &address,
        quint16 port,
        const QVariantMap &parameters)
{
    if (!isReady(OutgoingStreamTubeChannel::FeatureCore)) {
        warning() << "OutgoingStreamTubeChannel::FeatureCore must be ready before "
                "calling offerTube";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                OutgoingStreamTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (state() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a socket for each Stream Tube";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"),
                OutgoingStreamTubeChannelPtr(this));
    }

    SocketAccessControl accessControl = SocketAccessControlLocalhost;
    // Check if port is supported

    // In this specific overload, we're handling an IPv4/IPv6 socket
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
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
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control "
                            "combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeIPv4);
        setAccessControl(accessControl);
        setIpAddress(qMakePair<QHostAddress, quint16>(address, port));

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
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control "
                            "combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeIPv6);
        setAccessControl(accessControl);
        setIpAddress(qMakePair<QHostAddress, quint16>(address, port));

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
 * Offer a TCP socket over this stream tube.
 *
 * This method offers a TCP socket over this tube. The socket is represented through
 * a QTcpServer.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed,
 * the tube will be opened and ready to be used.
 *
 * Note that the library will try to use #SocketAccessControlPort access control whenever possible,
 * as it allows to map connections to the socket's source address. This means that if
 * StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress() (or
 * StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress(), depending on
 * the type of the offered socket) returns \c true, this method will automatically
 * enable the connection tracking feature, as long as
 * StreamTubeChannel::FeatureConnectionMonitoring has been enabled.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param server A valid QTcpServer, which should be already listening for incoming connections.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 *                   For more details, please refer to \telepathy_spec.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the stream tube is ready to be used
 *         (hence in the #TubeStateOpen state).
 * \sa StreamTubeChannel::supportsIPv4SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress(),
 *     StreamTubeChannel::supportsIPv6SocketsOnLocalhost(),
 *     StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress()
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
 * This method offers an Unix socket over this stream tube. The socket is represented through
 * a QString, which should contain the path to the socket. You can also expose an
 * abstract Unix socket, by including the leading null byte in the address.
 *
 * If you are already handling a local socket logic in your application, you can also
 * use an overload which accepts a QLocalServer.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed,
 * the tube will be opened and ready to be used.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param address A valid path to an existing Unix socket or abstract Unix socket.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 *                   For more details, please refer to \telepathy_spec.
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
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"),
                OutgoingStreamTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (state() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a socket for each Stream Tube";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
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
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
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
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
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
 * Offer a Unix socket over the tube.
 *
 * This method offers an Unix socket over this stream tube. The socket is represented through
 * a QLocalServer.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed,
 * the tube will be opened and ready to be used.
 *
 * This method requires OutgoingStreamTubeChannel::FeatureCore to be ready.
 *
 * \param server A valid QLocalServer, which should be already listening for incoming connections.
 * \param parameters A dictionary of arbitrary parameters to send with the tube offer.
 *                   For more details, please refer to \telepathy_spec.
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
 * This method is only useful if this tube was offered using a TCP socket with
 * #SocketAccessControlPort access control.
 *
 * The connection ids retrieved here can be used to track an address
 * which connected to your socket to a contact (by using contactsForConnections()).
 *
 * Note that this function will only return valid data after the tube has been opened.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be ready.
 *
 * \return The map from source addresses as QHostAddress to the corresponding connection ids.
 * \sa contactsForConnections(), connectionsForCredentials(),
 *     TubeChannel::addressType(),
 *     StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress(),
 *     StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress()
 */
QHash<QPair<QHostAddress, quint16>, uint> OutgoingStreamTubeChannel::connectionsForSourceAddresses() const
{
    if (addressType() != SocketAddressTypeIPv4 && addressType() != SocketAddressTypeIPv6) {
        warning() << "OutgoingStreamTubeChannel::connectionsForSourceAddresses() makes sense "
                "just when offering a TCP socket";
        return QHash<QPair<QHostAddress, quint16>, uint>();
    }

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

    return mPriv->connectionsForSourceAddresses;
}

/**
 * Return a map from a credential byte to the corresponding connections ids.
 *
 * This method is only useful if this tube was offered using an Unix socket requiring credentials.
 *
 * The connection ids retrieved here can be used to track an address
 * which connected to your socket to a contact (by using contactsForConnections()).
 *
 * Note that this function will only return valid data after the tube has been opened.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be ready.
 *
 * \return The map from credential bytes to the corresponding connection ids.
 * \sa contactsForConnections(), connectionsForSourceAddresses(),
 *     TubeChannel::addressType(),
 *     supportsUnixSocketsWithCredentials(),
 *     supportsAbstractUnixSocketsWithCredentials()
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
    // Retrieve our hash
    if (!mPriv->pendingNewConnections.contains(uuid)) {
        warning() << "Contacts retrieved but no pending connections were found";
        return;
    }

    QPair<uint, QDBusVariant> connectionProperties = mPriv->pendingNewConnections.take(uuid);

    // Add the connection to our list
    UIntList connections;
    connections << connectionProperties.first;
    setConnections(connections);

    // Add it to our connections hash
    foreach (const Tp::ContactPtr &contact, contacts) {
        mPriv->contactsForConnections.insertMulti(connectionProperties.first, contact);
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
    emit newConnection(connectionProperties.first);
}

void OutgoingStreamTubeChannel::onConnectionClosed(uint connectionId,
        const QString &errorName, const QString &errorMessage)
{
    // Remove stuff from our hashes
    mPriv->contactsForConnections.remove(connectionId);

    {
        QHash<QPair<QHostAddress, quint16>, uint>::iterator i =
            mPriv->connectionsForSourceAddresses.begin();
        while (i != mPriv->connectionsForSourceAddresses.end()) {
            if (i.value() == connectionId) {
                i = mPriv->connectionsForSourceAddresses.erase(i);
            } else {
                ++i;
            }
        }
    }

    {
        QHash<uchar, uint>::iterator i = mPriv->connectionsForCredentials.begin();
        while (i != mPriv->connectionsForCredentials.end()) {
            if (i.value() == connectionId) {
                i = mPriv->connectionsForCredentials.erase(i);
            } else {
                ++i;
            }
        }
    }
}

}
