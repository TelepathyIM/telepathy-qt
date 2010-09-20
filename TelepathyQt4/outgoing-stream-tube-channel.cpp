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

PendingOpenTube::Private::Private(const QVariantMap &parameters, PendingOpenTube* parent)
    : parent(parent)
    , parameters(parameters)
{

}

PendingOpenTube::Private::~Private()
{

}

PendingOpenTube::PendingOpenTube(
        PendingVoid* offerOperation,
        const QVariantMap &parameters,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new Private(parameters, this))
{
    mPriv->tube = OutgoingStreamTubeChannelPtr::dynamicCast(object);

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

void PendingOpenTube::onOfferFinished(PendingOperation* op)
{
    if (op->isError()) {
        // Fail
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Offer tube finished successfully";
    debug() << mPriv->tube->tubeState() << TubeChannelStateOpen;

    // It might have been already opened - check
    if (mPriv->tube->tubeState() == TubeChannelStateOpen) {
        onTubeStateChanged(mPriv->tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                this, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingOpenTube::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // Inject the parameters into the tube
        mPriv->tube->setParameters(mPriv->parameters);
        // The tube is ready: let's notify
        setFinished();
    } else if (state != TubeChannelStateRemotePending) {
        // Something happened
        setFinishedWithError(QLatin1String("Connection refused"),
                QLatin1String("The connection to this tube was refused"));
    }
}


QueuedContactFactory::QueuedContactFactory(Tp::ContactManagerPtr contactManager, QObject* parent)
    : QObject(parent)
    , m_isProcessing(false)
    , m_manager(contactManager)
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

    PendingContacts *pc = m_manager->contactsForHandles(entry.handles);
    pc->setProperty("__TpQt4__QueuedContactFactoryUuid", entry.uuid.toString());
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onPendingContactsFinished(Tp::PendingOperation*)));
}

QUuid QueuedContactFactory::appendNewRequest(const Tp::UIntList& handles)
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

void QueuedContactFactory::onPendingContactsFinished(PendingOperation* op)
{
    PendingContacts *pc = qobject_cast< PendingContacts* >(op);

    QUuid uuid = QUuid(pc->property("__TpQt4__QueuedContactFactoryUuid").toString());

    emit contactsRetrieved(uuid, pc->contacts());

    // No longer processing
    m_isProcessing = false;

    // Go for next one
    processNextRequest();
}


OutgoingStreamTubeChannel::Private::Private(OutgoingStreamTubeChannel *parent)
    : parent(parent)
    , queuedContactFactory(new QueuedContactFactory(parent->connection()->contactManager(), parent))
{
}

OutgoingStreamTubeChannel::Private::~Private()
{
}

/**
 * \class OutgoingStreamTubeChannel
 * \headerfile TelepathyQt4/outgoing-stream-tube-channel.h <TelepathyQt4/OutgoingStreamTubeChannel>
 *
 * \brief An high level wrapper for managing an outgoing stream tube
 *
 * \c OutgoingStreamTubeChannel is an high level wrapper for managing Telepathy interface
 * #TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE.
 * In particular, this class is meant to be used as a comfortable way for exposing new tubes.
 * It provides a set of overloads for exporting a variety of sockets over a stream tube.
 *
 * \section outgoing_stream_tube_usage_sec Usage
 *
 * \subsection outgoing_stream_tube_create_sec Creating an outgoing stream tube
 *
 * The easiest way to create account objects is through Account. One can
 * just use the Account convenience methods such as
 * Account::createStreamTube() to get a brand new stream tube channel ready to be used.
 *
 * To create such a channel, it is required to pass Account::createStreamTube() the contact identifier
 * and the service name which will be used over the tube.
 * For example:
 *
 * \code
 * AccountPtr myaccount = getMyAccountSomewhere();
 * ContactPtr myfriend = getMyFriendSomewhereElse();
 *
 * PendingChannelRequest *tube = myaccount->createStreamTube(myfriend, "rsync");
 * \endcode
 *
 * Be sure to track the pending request to retrieve your outgoing stream tube upon success.
 *
 * \subsection outgoing_stream_tube_offer_sec Offering the tube
 *
 * Before being ready to offer the tube, we must be sure the required features on our object
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
 * You can also enable StreamTubeChannel::FeatureConnectionMonitoring if the tube supports it. Have a look at
 * #StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress to learn more about this.
 *
 * Once your object is ready, you can use one of the overloads of #offerTcpSocket or #offerUnixSocket to offer an
 * existing socket over the tube. For example, if you wanted to offer an existing QTcpServer without applying any
 * restrictions on the access control, you would simply do
 *
 * \code
 * QTcpServer *server = getMyServer();
 *
 * PendingOperation *op = myTube->offerTcpSocket(server, QVariantMap());
 * \endcode
 *
 * You can now monitor the returned operation to know when the tube will be ready. It is guaranteed that when the
 * operation finishes, the tube will be already opened and ready to be used.
 *
 * See \ref async_model, \ref shared_ptr
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
    : StreamTubeChannel(connection, objectPath,
                        immutableProperties, coreFeature)
    , mPriv(new Private(this))
{
    setBaseTubeType(1);

    connect(this, SIGNAL(connectionClosed(uint,QString,QString)),
            this, SLOT(onConnectionClosed(uint,QString,QString)));
    connect(mPriv->queuedContactFactory, SIGNAL(contactsRetrieved(QUuid,QList<Tp::ContactPtr>)),
            this, SLOT(onContactsRetrieved(QUuid,QList<Tp::ContactPtr>)));
}

/**
 * Class destructor.
 */
OutgoingStreamTubeChannel::~OutgoingStreamTubeChannel()
{
    delete mPriv;
}

/**
 * \brief Offer an IPv4/IPv6 socket over the tube
 *
 * This method offers an IPv4/IPv6 socket over this tube. The socket is represented through
 * a QHostAddress. If you are already handling a Tcp logic in your application, you can also
 * use an overload which accepts a QTcpServer.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed, the tube will be
 * open and ready to be used.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \param address A valid IPv4 or IPv6 address pointing to an existing socket
 * \param port The port the socket is listening for connections to
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 *
 * \return A PendingOperation which will finish as soon as the tube is ready to be used
 *         (hence in the Open state)
 *
 * \note The library will try to use Port access control whenever possible, as it allows to
 *       map connections to the socket's source address. This means that if
 *       #StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress (or
 *       #StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress, depending on the type of the offered socket)
 *       returns true, this method will automatically enable the connection tracking feature, as long as
 *       StreamTubeChannel::FeatureConnectionMonitoring has been enabled.
 *
 * \sa StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress
 * \sa StreamTubeChannel::supportsIPv4SocketsOnLocalhost
 * \sa StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress
 * \sa StreamTubeChannel::supportsIPv6SocketsOnLocalhost
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

        setAddressType(SocketAddressTypeIPv4);
        setIpAddress(qMakePair< QHostAddress, quint16 >(address, port));

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

        setAddressType(SocketAddressTypeIPv6);
        setIpAddress(qMakePair< QHostAddress, quint16 >(address, port));

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
 * It is guaranteed that when the PendingOperation returned by this method will be completed, the tube will be
 * open and ready to be used.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \param server A valid QTcpServer, which should be already listening for incoming connections.
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 *
 * \return A PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \note The library will try to use Port access control whenever possible, as it allows to
 *       map connections to the socket's source address. This means that if
 *       #StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress (or
 *       #StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress, depending on the type of the offered socket)
 *       returns true, this method will automatically enable the connection tracking feature, as long as
 *       StreamTubeChannel::FeatureConnectionMonitoring has been enabled.
 *
 * \sa StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress
 * \sa StreamTubeChannel::supportsIPv4SocketsOnLocalhost
 * \sa StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress
 * \sa StreamTubeChannel::supportsIPv6SocketsOnLocalhost
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
 * It is guaranteed that when the PendingOperation returned by this method will be completed, the tube will be
 * open and ready to be used.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \param address A valid path to an existing Unix socket or abstract Unix socket
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \return A PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \sa StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost
 * \sa StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials
 * \sa StreamTubeChannel::supportsUnixSocketsOnLocalhost
 * \sa StreamTubeChannel::supportsUnixSocketsWithCredentials
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

        setAddressType(SocketAddressTypeAbstractUnix);
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
        if ( (accessControl == SocketAccessControlLocalhost && !supportsUnixSocketsOnLocalhost()) ||
             (accessControl == SocketAccessControlCredentials && !supportsUnixSocketsWithCredentials()) ||
             (accessControl != SocketAccessControlLocalhost && accessControl != SocketAccessControlCredentials) ) {
            warning() << "You requested an address type/access control combination "
                "not supported by this channel";
            return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                    QLatin1String("The requested address type/access control combination is not supported"),
                    OutgoingStreamTubeChannelPtr(this));
        }

        setAddressType(SocketAddressTypeUnix);
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
 * \overload
 * \brief Offer a Unix socket over the tube
 *
 * This method offers a Unix socket over this tube through a QLocalServer.
 *
 * It is guaranteed that when the PendingOperation returned by this method will be completed, the tube will be
 * open and ready to be used.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \param server A valid QLocalServer, which should be already listening for incoming connections.
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \return A PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 *
 * \sa StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost
 * \sa StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials
 * \sa StreamTubeChannel::supportsUnixSocketsOnLocalhost
 * \sa StreamTubeChannel::supportsUnixSocketsWithCredentials
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
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be enabled.
 *
 * \return an hash mapping a source address to its connection ID
 *
 * \note The tube has to be open for calling this function
 *
 * \sa contactsForConnections
 * \sa StreamTubeChannel::addressType
 * \sa StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress
 * \sa StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress
 */
QHash< QPair< QHostAddress, quint16 >, uint > OutgoingStreamTubeChannel::connectionsForSourceAddresses() const
{
    if (addressType() != SocketAddressTypeIPv4 && addressType() != SocketAddressTypeIPv6) {
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

    return mPriv->connectionsForSourceAddresses;
}


/**
 * If StreamTubeChannel::FeatureConnectionMonitoring has been enabled, this function
 * returns a map from a connection ID to the associated contact.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be enabled.
 *
 * \return an hash mapping a connection ID to the associated contact.
 *
 * \note The tube has to be open for calling this function
 *
 * \sa connectionsForSourceAddresses
 * \sa StreamTubeChannel::addressType
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
        const QList< Tp::ContactPtr > &contacts)
{
    // Retrieve our hash
    if (!mPriv->pendingNewConnections.contains(uuid)) {
        warning() << "Contacts retrieved but no pending connections were found";
        return;
    }

    QPair< uint, QDBusVariant > connectionProperties = mPriv->pendingNewConnections.take(uuid);

    // Add the connection to our list
    UIntList connections;
    connections << connectionProperties.first;
    setConnections(connections);

    // Add it to our connections hash
    foreach (const Tp::ContactPtr &contact, contacts) {
        mPriv->contactsForConnections.insertMulti(connectionProperties.first, contact);
    }

    QPair< QHostAddress, quint16 > address;
    address.first = QHostAddress::Null;

    // Now let's try to track the parameter
    if (addressType() == SocketAddressTypeIPv4) {
        // Try a qdbus_cast to our address struct: we're shielded from crashes due to our specification
        SocketAddressIPv4 addr = qdbus_cast< Tp::SocketAddressIPv4 >(connectionProperties.second.variant());
        address.first = QHostAddress(addr.address);
        address.second = addr.port;
    } else if (addressType() == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr = qdbus_cast< Tp::SocketAddressIPv6 >(connectionProperties.second.variant());
        address.first = QHostAddress(addr.address);
        address.second = addr.port;
    }

    if (address.first != QHostAddress::Null) {
        // We can map it to a source address as well
        mPriv->connectionsForSourceAddresses.insertMulti(address, connectionProperties.first);
    }

    // Time for us to emit the signal
    emit newConnection(connectionProperties.first);
}

void OutgoingStreamTubeChannel::onConnectionClosed(
        uint connectionId,
        const QString&,
        const QString&)
{
    // Remove stuff from our hashes
    mPriv->contactsForConnections.remove(connectionId);

    QHash< QPair< QHostAddress, quint16 >, uint >::iterator i =
                                    mPriv->connectionsForSourceAddresses.begin();

    while (i != mPriv->connectionsForSourceAddresses.end()) {
        if (i.value() == connectionId) {
            i = mPriv->connectionsForSourceAddresses.erase(i);
        } else {
            ++i;
        }
    }
}

}
