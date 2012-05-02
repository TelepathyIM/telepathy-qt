/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt/StreamTubeClient>

#include "TelepathyQt/stream-tube-client-internal.h"
#include "TelepathyQt/_gen/stream-tube-client.moc.hpp"
#include "TelepathyQt/_gen/stream-tube-client-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/simple-stream-tube-handler.h"

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/IncomingStreamTubeChannel>
#include <TelepathyQt/PendingStreamTubeConnection>
#include <TelepathyQt/StreamTubeChannel>

#include <QAbstractSocket>
#include <QHash>

namespace Tp
{

/**
 * \class StreamTubeClient::TcpSourceAddressGenerator
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-client.h <TelepathyQt/StreamTubeClient>
 *
 * \brief The StreamTubeClient::TcpSourceAddressGenerator abstract interface allows using socket
 * source address/port based access control for connecting to tubes accepted as TCP sockets.
 *
 * By default, every application on the local computer is allowed to connect to the socket created
 * by the protocol backend as the local endpoint of the tube. This is not always desirable, as that
 * includes even other users.
 *
 * Note that since every TCP connection must have an unique source address, only one simultaneous
 * connection can be made through each tube for which this type of access control has been used.
 */

/**
 * \fn QPair<QHostAddress, quint16> StreamTubeClient::TcpSourceAddressGenerator::nextSourceAddress(const AccountPtr &, const
 * IncomingStreamTubeChannelPtr &)
 *
 * Return the source address from which connections will be allowed to the given \a tube once it has
 * been accepted.
 *
 * Returning the pair (QHostAddress::Any, 0) (or alternatively (QHostAddress::AnyIPv4, 0) in Qt5)
 * makes the protocol backend allow connections from any address on the local computer.
 * This can be used on a tube-by-tube basis if for some tubes its known that multiple connections
 * need to be made, so a single source address doesn't suffice.
 *
 * The \a account and \a tube parameters can be inspected to make the decision; typically by looking
 * at the tube's service type, parameters and/or initiator contact.
 *
 * The general pattern for implementing this method is:
 * <ol>
 * <li>Determine whether \a tube needs to allow multiple connections, and if so, skip source address
 * access control completely</li>
 * <li>Otherwise, create a socket and bind it to a free address</li>
 * <li>Return this socket's address</li>
 * <li>Keep the socket bound so that no other process can (accidentally or maliciously) take the
 * address until it's used to connect to the tube when StreamTubeClient::tubeAcceptedAsTcp() is
 * emitted for the tube</li>
 * </ol>
 *
 * \param account The account from which the tube originates.
 * \param tube The tube channel which is going to be accepted by the StreamTubeClient.
 *
 * \return A pair containing the host address and port allowed to connect.
 */

/**
 * \fn StreamTubeClient::TcpSourceAddressGenerator::~TcpSourceAddressGenerator
 *
 * Class destructor. Protected, because StreamTubeClient never deletes a TcpSourceAddressGenerator passed
 * to it.
 */

struct TP_QT_NO_EXPORT StreamTubeClient::Tube::Private : public QSharedData
{
    // empty placeholder for now
};

/**
 * \class StreamTubeClient::Tube
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-client.h <TelepathyQt/StreamTubeClient>
 *
 * \brief The StreamTubeClient::Tube class represents a tube being handled by the client.
 */

/**
 * Constructs a new invalid Tube instance.
 */
StreamTubeClient::Tube::Tube()
{
    // invalid instance
}

/**
 * Constructs a Tube instance for the given tube \a channel from the given \a account.
 *
 * \param account A pointer to the account the online connection of which the tube originates from.
 * \param channel A pointer to the tube channel object.
 */
StreamTubeClient::Tube::Tube(
        const AccountPtr &account,
        const IncomingStreamTubeChannelPtr &channel)
    : QPair<AccountPtr, IncomingStreamTubeChannelPtr>(account, channel), mPriv(new Private)
{
}

/**
 * Copy constructor.
 */
StreamTubeClient::Tube::Tube(
        const Tube &other)
    : QPair<AccountPtr, IncomingStreamTubeChannelPtr>(other.account(), other.channel()),
    mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
StreamTubeClient::Tube::~Tube()
{
    // mPriv deleted automatically
}

/**
 * Assignment operator.
 */
StreamTubeClient::Tube &StreamTubeClient::Tube::operator=(
        const Tube &other)
{
    if (&other == this) {
        return *this;
    }

    first = other.account();
    second = other.channel();
    mPriv = other.mPriv;

    return *this;
}

/**
 * \fn bool StreamTubeClient::Tube::isValid() const
 *
 * Return whether or not the tube is valid or is just the null object created using the default
 * constructor.
 *
 * \return \c true if valid, \c false otherwise.
 */

/**
 * \fn AccountPtr StreamTubeClient::Tube::account() const
 *
 * Return the account from which the tube originates.
 *
 * \return A pointer to the account object.
 */

/**
 * \fn IncomingStreamTubeChannelPtr StreamTubeClient::Tube::channel() const
 *
 * Return the actual tube channel.
 *
 * \return A pointer to the channel.
 */

struct TP_QT_NO_EXPORT StreamTubeClient::Private
{
    Private(const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices,
            const QString &maybeClientName,
            bool monitorConnections,
            bool bypassApproval)
        : registrar(registrar),
          handler(SimpleStreamTubeHandler::create(
                      p2pServices, roomServices, false, monitorConnections, bypassApproval)),
          clientName(maybeClientName),
          isRegistered(false),
          acceptsAsTcp(false), acceptsAsUnix(false),
          tcpGenerator(0), requireCredentials(false)
    {
        if (clientName.isEmpty()) {
            clientName = QString::fromLatin1("TpQtSTubeClient_%1_%2")
                .arg(registrar->dbusConnection().baseService()
                        .replace(QLatin1Char(':'), QLatin1Char('_'))
                        .replace(QLatin1Char('.'), QLatin1Char('_')))
                .arg((quintptr) this, 0, 16);
        }
    }

    void ensureRegistered()
    {
        if (isRegistered) {
            return;
        }

        debug() << "Register StreamTubeClient with name " << clientName;

        if (registrar->registerClient(handler, clientName)) {
            isRegistered = true;
        } else {
            warning() << "StreamTubeClient" << clientName
                << "registration failed";
        }
    }

    ClientRegistrarPtr registrar;
    SharedPtr<SimpleStreamTubeHandler> handler;
    QString clientName;
    bool isRegistered;

    bool acceptsAsTcp, acceptsAsUnix;
    TcpSourceAddressGenerator *tcpGenerator;
    bool requireCredentials;

    QHash<StreamTubeChannelPtr, TubeWrapper *> tubes;
};

StreamTubeClient::TubeWrapper::TubeWrapper(
        const AccountPtr &acc,
        const IncomingStreamTubeChannelPtr &tube,
        const QHostAddress &sourceAddress,
        quint16 sourcePort,
        StreamTubeClient *parent)
    : QObject(parent), mAcc(acc), mTube(tube), mSourceAddress(sourceAddress), mSourcePort(sourcePort)
{
    QHostAddress hostAddress = sourceAddress;

    if (sourcePort != 0) {
#if QT_VERSION >= 0x050000
        if (hostAddress == QHostAddress::Any ||
            hostAddress == QHostAddress::LocalHost) {
            hostAddress = QHostAddress::AnyIPv4;
        }
#endif

        if ((hostAddress.protocol() == QAbstractSocket::IPv4Protocol &&
                    !tube->supportsIPv4SocketsWithSpecifiedAddress()) ||
                (hostAddress.protocol() == QAbstractSocket::IPv6Protocol &&
                 !tube->supportsIPv6SocketsWithSpecifiedAddress())) {
            debug() << "StreamTubeClient falling back to Localhost AC for tube" <<
                tube->objectPath();
            mSourceAddress = sourceAddress.protocol() == QAbstractSocket::IPv4Protocol ?
                 QHostAddress::Any : QHostAddress::AnyIPv6;
            mSourcePort = 0;
        }
    }

    connect(tube->acceptTubeAsTcpSocket(mSourceAddress, mSourcePort),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeAccepted(Tp::PendingOperation*)));
    connect(tube.data(),
            SIGNAL(newConnection(uint)),
            SLOT(onNewConnection(uint)));
    connect(tube.data(),
            SIGNAL(connectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));
}

StreamTubeClient::TubeWrapper::TubeWrapper(
        const AccountPtr &acc,
        const IncomingStreamTubeChannelPtr &tube,
        bool requireCredentials,
        StreamTubeClient *parent)
    : QObject(parent), mAcc(acc), mTube(tube), mSourcePort(0)
{
    if (requireCredentials && !tube->supportsUnixSocketsWithCredentials()) {
        debug() << "StreamTubeClient falling back to Localhost AC for tube" << tube->objectPath();
        requireCredentials = false;
    }

    connect(tube->acceptTubeAsUnixSocket(requireCredentials),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeAccepted(Tp::PendingOperation*)));
    connect(tube.data(),
            SIGNAL(newConnection(uint)),
            SLOT(onNewConnection(uint)));
    connect(tube.data(),
            SIGNAL(connectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));
}

void StreamTubeClient::TubeWrapper::onTubeAccepted(Tp::PendingOperation *op)
{
    emit acceptFinished(this, qobject_cast<Tp::PendingStreamTubeConnection *>(op));
}

void StreamTubeClient::TubeWrapper::onNewConnection(uint conn)
{
    emit newConnection(this, conn);
}

void StreamTubeClient::TubeWrapper::onConnectionClosed(uint conn, const QString &error,
        const QString &message)
{
    emit connectionClosed(this, conn, error, message);
}

/**
 * \class StreamTubeClient
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-client.h <TelepathyQt/StreamTubeClient>
 *
 * \brief The StreamTubeClient class is a Handler implementation for incoming %Stream %Tube channels,
 * allowing an application to easily get notified about services they can connect to offered to them
 * over Telepathy Tubes without worrying about the channel dispatching details.
 *
 * Telepathy Tubes is a technology for connecting arbitrary applications together through the IM
 * network (and sometimes with direct peer-to-peer connections), such that issues like firewall/NAT
 * traversal are automatically handled. Stream Tubes in particular offer properties similar to
 * SOCK_STREAM sockets. The StreamTubeClient class negotiates tubes offered to us so that an
 * application can connect such bytestream sockets of theirs to them. The StreamTubeServer class is
 * the counterpart, offering services from a bytestream socket server to tubes requested to be
 * initiated.
 *
 * Both peer-to-peer (\c TargetHandleType == \ref HandleTypeContact) and group (\c TargetHandleType
 * == \ref HandleTypeRoom) channels are supported, and it's possible to specify the tube services to
 * handle for each separately. There must be at least one service in total declared, as it never
 * makes sense to handle stream tubes without considering the protocol of the service offered
 * through them.
 *
 * %Connection monitoring allows fine-grained error reporting for connections made through tubes,
 * and observing connections being made and broken even if the application code running
 * StreamTubeClient can't easily get this information from the code actually connecting through it.
 * Such a setting might occur e.g. when a wrapper application is developed to connect some existing
 * "black box" networked application through TCP tubes by launching it with the appropriate command
 * line arguments or alike for accepted tubes.
 *
 * Enabling connection monitoring adds a small overhead and latency to handling each incoming tube
 * and signaling each new incoming connection over them, though, so use it only when needed.
 *
 * A service activated Handler can be implemented using StreamTubeClient by passing a predefined \a
 * clientName manually to the chosen create() method, and installing Telepathy \c .client and D-Bus
 * \c .service files declaring the implemented tube services as channel classes and a path to the
 * executable. If this is not needed, the \a clientName can be omitted, in which case a random
 * unique client name is generated and used instead. However, then the tube client application must
 * already be running for remote contacts to be able to offer services to us over tubes.
 *
 * Whether the Handler application implemented using StreamTubeClient is service activatable or not,
 * incoming channels will typically first be given to an Approver, if there is one for tube services
 * corresponding to the tube in question. Only if the Approver decides that the tube communication
 * should be allowed (usually by asking the user), or if there is no matching Approver at all, is
 * the channel given to the actual Handler tube client. This can be overridden by setting \a
 * bypassApproval to \c true, which skips approval for the given services completely and directs
 * them straight to the Handler.
 *
 * StreamTubeClient shares Account, Connection and Channel proxies and Contact objects with the
 * rest of the application as long as a reference to the AccountManager, ClientRegistrar, or the
 * factories used elsewhere is passed to the create() method. A stand-alone tube client Handler can
 * get away without passing these however, or just passing select factories to make the desired
 * features prepared and subclasses employed for these objects for their own convenience.
 *
 * Whichever method is used, the ChannelFactory (perhaps indirectly) given must construct
 * IncomingStreamTubeChannel instances or subclasses thereof for all channel classes corresponding
 * to the tube services the client should be able to connect to. This is the default; overriding it
 * without obeying these constraints using ChannelFactory::setSubclassForIncomingStreamTubes() or
 * the related methods for room tubes prevents StreamTubeClient from operating correctly.
 *
 * \todo Coin up a small Python script or alike to easily generate the .client and .service files.
 * (fd.o #41614)
 */

/**
 * Create a new StreamTubeClient, which will register itself on the session bus using an internal
 * ClientRegistrar and use the given factories.
 *
 * \param p2pServices Names of the tube services to accept on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to accept on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 * \param bypassApproval \c true to skip approval, \c false to invoke an Approver for incoming
 * channels if there is one.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 */
StreamTubeClientPtr StreamTubeClient::create(
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return create(
            QDBusConnection::sessionBus(),
            accountFactory,
            connectionFactory,
            channelFactory,
            contactFactory,
            p2pServices,
            roomServices,
            clientName,
            monitorConnections,
            bypassApproval);
}

/**
 * Create a new StreamTubeClient, which will register itself on the given \a bus using an internal
 * ClientRegistrar and use the given factories.
 *
 * The factories must all be created for the given \a bus.
 *
 * \param bus Connection to the bus to register on.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \param p2pServices Names of the tube services to handle on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to handle on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 * \param bypassApproval \c true to skip approval, \c false to invoke an Approver for incoming
 * channels if there is one.
 */
StreamTubeClientPtr StreamTubeClient::create(
        const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
{
    return create(
            ClientRegistrar::create(
                bus,
                accountFactory,
                connectionFactory,
                channelFactory,
                contactFactory),
            p2pServices,
            roomServices,
            clientName,
            monitorConnections,
            bypassApproval);
}

/**
 * Create a new StreamTubeClient, which will register itself on the bus of and share objects with
 * the given \a accountManager, creating an internal ClientRegistrar.
 *
 * \param accountManager A pointer to the account manager to link up with.
 * \param p2pServices Names of the tube services to handle on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to handle on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 * \param bypassApproval \c true to skip approval, \c false to invoke an Approver for incoming
 * channels if there is one.
 */
StreamTubeClientPtr StreamTubeClient::create(
        const AccountManagerPtr &accountManager,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
{
    return create(
            accountManager->dbusConnection(),
            accountManager->accountFactory(),
            accountManager->connectionFactory(),
            accountManager->channelFactory(),
            accountManager->contactFactory(),
            p2pServices,
            roomServices,
            clientName,
            monitorConnections,
            bypassApproval);
}

/**
 * Create a new StreamTubeClient, which will register itself on the bus of and using the given
 * client \a registrar, and share objects with it.
 *
 * \param registrar The client registrar to use.
 * \param p2pServices Names of the tube services to handle on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to handle on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 * \param bypassApproval \c true to skip approval, \c false to invoke an Approver for incoming
 * channels if there is one.
 */
StreamTubeClientPtr StreamTubeClient::create(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
{
    if (p2pServices.isEmpty() && roomServices.isEmpty()) {
        warning() << "Tried to create a StreamTubeClient with no services, returning NULL";
        return StreamTubeClientPtr();
    }

    return StreamTubeClientPtr(
            new StreamTubeClient(registrar, p2pServices, roomServices, clientName,
                monitorConnections, bypassApproval));
}

StreamTubeClient::StreamTubeClient(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
    : mPriv(new Private(registrar, p2pServices, roomServices, clientName, monitorConnections, bypassApproval))
{
    connect(mPriv->handler.data(),
            SIGNAL(invokedForTube(
                    Tp::AccountPtr,
                    Tp::StreamTubeChannelPtr,
                    QDateTime,
                    Tp::ChannelRequestHints)),
            SLOT(onInvokedForTube(
                    Tp::AccountPtr,
                    Tp::StreamTubeChannelPtr,
                    QDateTime,
                    Tp::ChannelRequestHints)));
}

/**
 * Class destructor.
 */
StreamTubeClient::~StreamTubeClient()
{
    if (isRegistered()) {
        mPriv->registrar->unregisterClient(mPriv->handler);
    }

    delete mPriv;
}

/**
 * Return the client registrar used by the client to register itself as a Telepathy channel Handler
 * %Client.
 *
 * This is the registrar originally passed to
 * create(const ClientRegistrarPtr &, const QStringList &, const QStringList &, const QString &, bool, bool)
 * if that was used, and an internally constructed one otherwise. In any case, it can be used to
 * e.g. register further clients, just like any other ClientRegistrar.
 *
 * \return A pointer to the registrar.
 */
ClientRegistrarPtr StreamTubeClient::registrar() const
{
    return mPriv->registrar;
}

/**
 * Return the Telepathy %Client name of the client.
 *
 * \return The name, without the \c org.freedesktop.Telepathy.Client. prefix of the full D-Bus service name.
 */
QString StreamTubeClient::clientName() const
{
    return mPriv->clientName;
}

/**
 * Return whether the client has been successfully registered or not.
 *
 * Registration is attempted, at the latest, when the client is first set to accept incoming tubes,
 * either as TCP sockets (setToAcceptAsTcp()) or Unix ones (setToAcceptAsUnix()).  It can fail e.g.
 * because the connection to the bus has failed, or a predefined \a clientName has been passed to
 * create(), and a %Client with the same name is already registered. Typically, failure registering
 * would be a fatal error for a stand-alone tube handler, but only a warning event for an
 * application serving other purposes. In any case, a high-quality user of the API will check the
 * return value of this accessor after choosing the desired address family.
 *
 * \return \c true if the client has been successfully registered, \c false if not.
 */
bool StreamTubeClient::isRegistered() const
{
    return mPriv->isRegistered;
}

/**
 * Return whether connection monitoring is enabled on this client.
 *
 * For technical reasons, connection monitoring can't be enabled when the client is already running,
 * so there is no corresponding setter method. It has to be enabled by passing \c true as the \a
 * monitorConnections parameter to the create() method.
 *
 * If connection monitoring isn't enabled, newConnection() and connectionClosed() won't be
 * emitted and connections() won't be populated.
 *
 * \return \c true if monitoring is enabled, \c false if not.
 */
bool StreamTubeClient::monitorsConnections() const
{
    return mPriv->handler->monitorsConnections();
}

/**
 * Return whether the client is currently set to accept incoming tubes as TCP sockets.
 *
 * \return \c true if the client will accept tubes as TCP sockets, \c false if it will accept them
 * as Unix ones or hasn't been set to accept at all yet.
 */
bool StreamTubeClient::acceptsAsTcp() const
{
    return mPriv->acceptsAsTcp;
}

/**
 * Return the TCP source address generator, if any, set by setToAcceptAsTcp() previously.
 *
 * \return A pointer to the generator instance.
 */
StreamTubeClient::TcpSourceAddressGenerator *StreamTubeClient::tcpGenerator() const
{
    if (!acceptsAsTcp()) {
        warning() << "StreamTubeClient::tcpGenerator() used, but not accepting as TCP, returning 0";
        return 0;
    }

    return mPriv->tcpGenerator;
}

/**
 * Return whether the client is currently set to accept incoming tubes as Unix sockets.
 *
 * \return \c true if the client will accept tubes as Unix sockets, \c false if it will accept them
 * as TCP ones or hasn't been set to accept at all yet.
 */
bool StreamTubeClient::acceptsAsUnix() const
{
    return mPriv->acceptsAsUnix;
}

/**
 * Set the client to accept tubes received to handle in the future in a fashion which will yield a
 * TCP socket as the local endpoint to connect to.
 *
 * A source address generator can optionally be set. If non-null, it will be invoked for each new
 * tube received to handle and an attempt is made to restrict connections to the tube's local socket
 * endpoint to those from that source address.
 *
 * However, if the protocol backend doesn't actually support source address based access control,
 * tubeAcceptedAsTcp() will be emitted with QHostAddress::Any
 * as the allowed source address to signal that it doesn't matter where we connect from, but more
 * importantly, that anybody else on the same host could have, and can, connect to the tube.
 * The tube can be closed at this point if this would be unacceptable security-wise.
 * To totally prevent the tube from being accepted in the first place, one can close it already when
 * tubeOffered() is emitted for it - support for the needed security mechanism can be queried using
 * its supportsIPv4SocketsWithSpecifiedAddress() accessor.
 *
 * The handler is registered on the bus at the latest when this method or setToAcceptAsUnix() is
 * called for the first time, so one should check the return value of isRegistered() at that point
 * to verify that was successful.
 *
 * \param generator A pointer to the source address generator to use, or 0 to allow all
 * connections from the local host.
 *
 * \todo Make it possible to set the tube client to auto-close tubes if the desired access control
 * level is not achieved, as an alternative to the current best-effort behavior.
 */
void StreamTubeClient::setToAcceptAsTcp(TcpSourceAddressGenerator *generator)
{
    mPriv->tcpGenerator = generator;
    mPriv->acceptsAsTcp = true;
    mPriv->acceptsAsUnix = false;

    mPriv->ensureRegistered();
}

/**
 * Set the client to accept tubes received to handle in the future in a fashion which will yield a
 * Unix socket as the local endpoint to connect to.
 *
 * If that doesn't cause problems for the payload protocol, it's possible to increase security by
 * restricting the processes allowed to connect to the local endpoint socket to those from the same
 * user ID as the protocol backend is running as by setting \a requireCredentials to \c true. This
 * requires transmitting a single byte, signaled as the \a credentialByte parameter to the
 * tubeAcceptedAsUnix() signal, in a \c SCM_CREDS or SCM_CREDENTIALS message, whichever is supported
 * by the platform, as the first thing after having connected to the socket. Even if the platform
 * doesn't implement either concept, the byte must still be sent.
 *
 * However, not all protocol backends support the credential passing based access control on all the
 * platforms they can run on. If a tube is offered through such a backend, tubeAcceptedAsUnix() will
 * be emitted with \a requiresCredentials set to \c false, to signal that a credential byte should
 * NOT be sent for that tube, and that any local process can or could have connected to the tube
 * already. The tube can be closed at this point if this would be unacceptable security-wise. To
 * totally prevent the tube from being accepted in the first place, one can close it already when
 * tubeOffered() is emitted for it - support for the needed security mechanism can be queried using
 * its supportsIPv4SocketsWithSpecifiedAddress()
 * accessor.
 *
 * The handler is registered on the bus at the latest when this method or setToAcceptAsTcp() is
 * called for the first time, so one should check the return value of isRegistered() at that point
 * to verify that was successful.
 *
 * \param requireCredentials \c true to try and restrict connecting by UID, \c false to allow all
 * connections.
 *
 * \todo Make it possible to set the tube client to auto-close tubes if the desired access control
 * level is not achieved, as an alternative to the current best-effort behavior.
 */
void StreamTubeClient::setToAcceptAsUnix(bool requireCredentials)
{
    mPriv->tcpGenerator = 0;
    mPriv->acceptsAsTcp = false;
    mPriv->acceptsAsUnix = true;
    mPriv->requireCredentials = requireCredentials;

    mPriv->ensureRegistered();
}

/**
 * Return the tubes currently handled by the client.
 *
 * \return A list of Tube structures containing pointers to the account and tube channel for each
 * tube.
 */
QList<StreamTubeClient::Tube> StreamTubeClient::tubes() const
{
    QList<Tube> tubes;

    foreach (TubeWrapper *wrapper, mPriv->tubes.values()) {
        tubes.push_back(Tube(wrapper->mAcc, wrapper->mTube));
    }

    return tubes;
}

/**
 * Return the ongoing connections established through tubes signaled by this client.
 *
 * The returned mapping has for each Tube a structure containing pointers to the account and tube
 * channel objects as keys, with the integer identifiers for the current connections on them as the
 * values. The IDs are unique amongst the connections active on a single tube at any given time, but
 * not globally.
 *
 * This is effectively a state recovery accessor corresponding to the change notification signals
 * newConnection() and connectionClosed().
 *
 * The mapping is only populated if connection monitoring was requested when creating the client (so
 * monitorsConnections() returns \c true).
 *
 * \return The connections in a mapping with Tube structures containing pointers to the account and
 * channel objects for each tube as keys, and the sets of numerical IDs as values.
 */
QHash<StreamTubeClient::Tube, QSet<uint> > StreamTubeClient::connections() const
{
    QHash<Tube, QSet<uint> > conns;
    if (!monitorsConnections()) {
        warning() << "StreamTubeClient::connections() used, but connection monitoring is disabled";
        return conns;
    }

    foreach (const Tube &tube, tubes()) {
        if (!tube.channel()->isValid()) {
            // The tube has been invalidated, so skip it to avoid warnings
            // We're going to get rid of the wrapper in the next mainloop iteration when we get the
            // invalidation signal
            continue;
        }

        QSet<uint> tubeConns = tube.channel()->connections();
        if (!tubeConns.empty()) {
            conns.insert(tube, tubeConns);
        }
    }

    return conns;
}

void StreamTubeClient::onInvokedForTube(
        const AccountPtr &acc,
        const StreamTubeChannelPtr &tube,
        const QDateTime &time,
        const ChannelRequestHints &hints)
{
    Q_ASSERT(isRegistered()); // our SSTH shouldn't be receiving any channels unless it's registered
    Q_ASSERT(!tube->isRequested());
    Q_ASSERT(tube->isValid()); // SSTH won't emit invalid tubes

    if (mPriv->tubes.contains(tube)) {
        debug() << "Ignoring StreamTubeClient reinvocation for tube" << tube->objectPath();
        return;
    }

    IncomingStreamTubeChannelPtr incoming = IncomingStreamTubeChannelPtr::qObjectCast(tube);

    if (!incoming) {
        warning() << "The ChannelFactory used by StreamTubeClient must construct" <<
            "IncomingStreamTubeChannel subclasses for Requested=false StreamTubes";
        tube->requestClose();
        return;
    }

    TubeWrapper *wrapper = 0;

    if (mPriv->acceptsAsTcp) {
        QPair<QHostAddress, quint16> srcAddr =
            qMakePair(QHostAddress(QHostAddress::Any), quint16(0));

        if (mPriv->tcpGenerator) {
            srcAddr = mPriv->tcpGenerator->nextSourceAddress(acc, incoming);
        }

        wrapper = new TubeWrapper(acc, incoming, srcAddr.first, srcAddr.second, this);
    } else {
        Q_ASSERT(mPriv->acceptsAsUnix); // we should only be registered when we're set to accept as either TCP or Unix
        wrapper = new TubeWrapper(acc, incoming, mPriv->requireCredentials, this);
    }

    connect(wrapper,
            SIGNAL(acceptFinished(TubeWrapper*,Tp::PendingStreamTubeConnection*)),
            SLOT(onAcceptFinished(TubeWrapper*,Tp::PendingStreamTubeConnection*)));
    connect(tube.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onTubeInvalidated(Tp::DBusProxy*,QString,QString)));

    if (monitorsConnections()) {
        connect(wrapper,
                SIGNAL(newConnection(TubeWrapper*,uint)),
                SLOT(onNewConnection(TubeWrapper*,uint)));
        connect(wrapper,
                SIGNAL(connectionClosed(TubeWrapper*,uint,QString,QString)),
                SLOT(onConnectionClosed(TubeWrapper*,uint,QString,QString)));
    }

    mPriv->tubes.insert(tube, wrapper);

    emit tubeOffered(acc, incoming);
}

void StreamTubeClient::onAcceptFinished(TubeWrapper *wrapper, PendingStreamTubeConnection *conn)
{
    Q_ASSERT(wrapper != NULL);
    Q_ASSERT(conn != NULL);

    if (!mPriv->tubes.contains(wrapper->mTube)) {
        debug() << "StreamTubeClient ignoring Accept result for invalidated tube"
            << wrapper->mTube->objectPath();
        return;
    }

    if (conn->isError()) {
        warning() << "StreamTubeClient couldn't accept tube" << wrapper->mTube->objectPath() << '-'
            << conn->errorName() << ':' << conn->errorMessage();

        if (wrapper->mTube->isValid()) {
            wrapper->mTube->requestClose();
        }

        wrapper->mTube->disconnect(this);
        emit tubeClosed(wrapper->mAcc, wrapper->mTube, conn->errorName(), conn->errorMessage());
        mPriv->tubes.remove(wrapper->mTube);
        wrapper->deleteLater();
        return;
    }

    debug() << "StreamTubeClient accepted tube" << wrapper->mTube->objectPath();

    if (conn->addressType() == SocketAddressTypeIPv4
            || conn->addressType() == SocketAddressTypeIPv6) {
        QPair<QHostAddress, quint16> addr = conn->ipAddress();
        emit tubeAcceptedAsTcp(addr.first, addr.second, wrapper->mSourceAddress,
                wrapper->mSourcePort, wrapper->mAcc, wrapper->mTube);
    } else {
        emit tubeAcceptedAsUnix(conn->localAddress(), conn->requiresCredentials(),
                conn->credentialByte(), wrapper->mAcc, wrapper->mTube);
    }
}

void StreamTubeClient::onTubeInvalidated(Tp::DBusProxy *proxy, const QString &error,
        const QString &message)
{
    StreamTubeChannelPtr tube(qobject_cast<StreamTubeChannel *>(proxy));
    Q_ASSERT(!tube.isNull());

    TubeWrapper *wrapper = mPriv->tubes.value(tube);
    if (!wrapper) {
        // Accept finish with error already removed it
        return;
    }

    debug() << "Client StreamTube" << tube->objectPath() << "invalidated - " << error << ':'
        << message;

    emit tubeClosed(wrapper->mAcc, wrapper->mTube, error, message);
    mPriv->tubes.remove(tube);
    delete wrapper;
}

void StreamTubeClient::onNewConnection(
        TubeWrapper *wrapper,
        uint conn)
{
    Q_ASSERT(monitorsConnections());
    emit newConnection(wrapper->mAcc, wrapper->mTube, conn);
}

void StreamTubeClient::onConnectionClosed(
        TubeWrapper *wrapper,
        uint conn,
        const QString &error,
        const QString &message)
{
    Q_ASSERT(monitorsConnections());
    emit connectionClosed(wrapper->mAcc, wrapper->mTube, conn, error, message);
}

/**
 * \fn void StreamTubeClient::tubeOffered(const AccountPtr &account, const
 * IncomingStreamTubeChannelPtr &tube)
 *
 * Emitted when one of the services we're interested in connecting to has been offered by us as a
 * tube, which we've began handling.
 *
 * This is emitted before invoking the TcpSourceAddressGenerator, if any, for the tube.
 *
 * \param account A pointer to the account through which the tube was offered.
 * \param tube A pointer to the actual tube channel.
 */

/**
 * \fn void StreamTubeClient::tubeAcceptedAsTcp(const QHostAddress &listenAddress, quint16
 * listenPort, const QHostAddress &sourceAddress, quint16 sourcePort, const AccountPtr &account,
 * const IncomingStreamTubeChannelPtr &tube)
 *
 * Emitted when a tube offered to us (previously announced with tubeOffered()) has been successfully
 * accepted and a TCP socket established as the local endpoint, as specified by setToAcceptAsTcp().
 *
 * The allowed source address and port are signaled here if there was a TcpSourceAddressGenerator set
 * at the time of accepting this tube, it yielded a non-zero address, and the protocol backend
 * supports source address based access control. This enables the application to use the correct one
 * of the sockets it currently has bound to the generated addresses.
 *
 * \param listenAddress The listen address of the local endpoint socket.
 * \param listenPort The listen port of the local endpoint socket.
 * \param sourceAddress The host address allowed to connect to the tube, or QHostAddress::Any
 * if source address based access control is not in use.
 * \param sourcePort The port from which connections are allowed to the tube, or 0 if source address
 * based access control is not in use.
 * \param account A pointer to the account object through which the tube was offered.
 * \param tube A pointer to the actual tube channel object.
 */

// Explicitly specifying Tp:: for this signal's argument types works around a doxygen bug causing it
// to confuse the doc here being for StreamTubeServer::tubeClosed :/
/**
 * \fn void StreamTubeClient::tubeClosed(const Tp::AccountPtr &account, const
 * Tp::IncomingStreamTubeChannelPtr &tube, const QString &error, const QString &message)
 *
 * Emitted when a tube we've been handling (previously announced with tubeOffered()) has
 * encountered an error or has otherwise been closed from further communication.
 *
 * \param account A pointer to the account through which the tube was offered.
 * \param tube A pointer to the actual tube channel.
 * \param error The D-Bus error name corresponding to the reason for the closure.
 * \param message A freeform debug message associated with the error.
 */

/**
 * \fn void StreamTubeClient::tubeAcceptedAsUnix(const QHostAddress &listenAddress, quint16
 * listenPort, const QHostAddress &sourceAddress, quint16 sourcePort, const AccountPtr &account,
 * const IncomingStreamTubeChannelPtr &tube)
 *
 * Emitted when a tube offered to us (previously announced with tubeOffered()) has been successfully
 * accepted and a Unix socket established as the local endpoint, as specified by setToAcceptAsUnix().
 *
 * The credential byte which should be sent after connecting to the tube (in a SCM_CREDENTIALS or
 * SCM_CREDS message if supported by the platform) will be signaled here if the client was set to
 * attempt requiring credentials at the time of accepting this tube, and the protocol backend
 * supports credential passing based access control. Otherwise, \a requiresCredentials will be false
 * and no byte or associated out-of-band credentials metadata should be sent.
 *
 * \param listenAddress The listen address of the local endpoint socket.
 * \param requiresCredentials \c true if \a credentialByte should be sent after connecting to the
 * socket, \c false if not.
 * \param credentialByte The byte to send if \a requiresCredentials is \c true.
 * \param account A pointer to the account object through which the tube was offered.
 * \param tube A pointer to the actual tube channel object.
 */

/**
 * \fn void StreamTubeClient::newConnection(const AccountPtr &account, const
 * IncomingStreamTubeChannelPtr &tube, uint connectionId)
 *
 * Emitted when a new connection has been made to the local endpoint socket for \a tube.
 *
 * This can be used to later associate connection errors reported by connectionClosed() with the
 * corresponding application sockets. However, establishing the association generally requires
 * connecting only one socket at a time, waiting for newConnection() to be emitted, and only then
 * proceeding, as there is no identification for the connections unlike the incoming connections in
 * StreamTubeServer.
 *
 * Note that the connection IDs are only unique within a given tube, so identification of the tube
 * channel must also be recorded together with the ID to establish global uniqueness. Even then, the
 * a connection ID can be reused after the previous connection identified by it having been
 * signaled as closed with connectionClosed().
 *
 * This is only emitted if connection monitoring was enabled when creating the StreamTubeClient.
 *
 * \param account A pointer to the account through which the tube was offered.
 * \param tube A pointer to the tube channel through which the connection has been made.
 * \param connectionId The integer ID of the new connection.
 */

/**
 * \fn void StreamTubeClient::connectionClosed(const AccountPtr &account, const
 * IncomingStreamTubeChannelPtr &tube, uint connectionId, const QString &error, const
 * QString &message)
 *
 * Emitted when a connection (previously announced with newConnection()) through one of our
 * handled tubes has been closed due to an error or by a graceful disconnect (in which case the
 * error is ::TP_QT_ERROR_CANCELLED).
 *
 * This is only emitted if connection monitoring was enabled when creating the StreamTubeClient.
 *
 * \param account A pointer to the account through which the tube was offered.
 * \param tube A pointer to the tube channel through which the connection had been made.
 * \param connectionId The integer ID of the connection closed.
 * \param error The D-Bus error name corresponding to the reason for the closure.
 * \param message A freeform debug message associated with the error.
 */

} // Tp
