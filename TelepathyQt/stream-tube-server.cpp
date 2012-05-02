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

#include <TelepathyQt/StreamTubeServer>
#include "TelepathyQt/stream-tube-server-internal.h"

#include "TelepathyQt/_gen/stream-tube-server.moc.hpp"
#include "TelepathyQt/_gen/stream-tube-server-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/simple-stream-tube-handler.h"

#include <QScopedPointer>
#include <QSharedData>
#include <QTcpServer>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/OutgoingStreamTubeChannel>
#include <TelepathyQt/StreamTubeChannel>

namespace Tp
{

/**
 * \class StreamTubeServer::ParametersGenerator
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-server.h <TelepathyQt/StreamTubeServer>
 *
 * \brief The StreamTubeServer::ParametersGenerator abstract interface allows sending a different
 * set of parameters with each tube offer.
 *
 * %Tube parameters are arbitrary data sent with the tube offer, which can be retrieved in the
 * receiving end with IncomingStreamTubeChannel::parameters(). They can be used to transfer
 * e.g. session identification information, authentication credentials or alike, for bootstrapping
 * the protocol used for communicating over the tube.
 *
 * For usecases where the parameters don't need to change between each tube, just passing a fixed
 * set of parameters to a suitable StreamTubeServer::exportTcpSocket() overload is usually more
 * convenient than implementing a ParametersGenerator. Note that StreamTubeServer::exportTcpSocket()
 * can be called multiple times to change the parameters for future tubes when e.g. configuration
 * settings have been changed, so a ParametersGenerator only needs to be implemented if each and
 * every tube must have a different set of parameters.
 */

/**
 * \fn QVariantMap StreamTubeServer::ParametersGenerator::nextParameters(const AccountPtr &, const
 * OutgoingStreamTubeChannelPtr &, const ChannelRequestHints &)
 *
 * Return the parameters to send when offering the given \a tube.
 *
 * \param account The account from which the tube originates.
 * \param tube The tube channel which is going to be offered by the StreamTubeServer.
 * \param hints The hints associated with the request that led to the creation of this tube, if any.
 *
 * \return Parameters to send with the offer, or an empty QVariantMap if none are needed for this
 * tube.
 */

/**
 * \fn StreamTubeServer::ParametersGenerator::~ParametersGenerator
 *
 * Class destructor. Protected, because StreamTubeServer never deletes a ParametersGenerator passed
 * to it.
 */

class TP_QT_NO_EXPORT FixedParametersGenerator : public StreamTubeServer::ParametersGenerator
{
public:

    FixedParametersGenerator(const QVariantMap &params) : mParams(params) {}

    QVariantMap nextParameters(const AccountPtr &, const OutgoingStreamTubeChannelPtr &,
            const ChannelRequestHints &)
    {
        return mParams;
    }

private:

    QVariantMap mParams;
};

struct TP_QT_NO_EXPORT StreamTubeServer::RemoteContact::Private : public QSharedData
{
    // empty placeholder for now
};

/**
 * \class StreamTubeServer::RemoteContact
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-server.h <TelepathyQt/StreamTubeServer>
 *
 * \brief The StreamTubeServer::RemoteContact class represents a contact from which a socket
 * connection to our exported socket originates.
 */

/**
 * Constructs a new invalid RemoteContact instance.
 */
StreamTubeServer::RemoteContact::RemoteContact()
{
    // invalid instance
}

/**
 * Constructs a new RemoteContact for the given \a contact object from the given \a account.
 *
 * \param account A pointer to the account which this contact can be reached through.
 * \param contact A pointer to the contact object.
 */
StreamTubeServer::RemoteContact::RemoteContact(
        const AccountPtr &account,
        const ContactPtr &contact)
    : QPair<AccountPtr, ContactPtr>(account, contact), mPriv(new Private)
{
}

/**
 * Copy constructor.
 */
StreamTubeServer::RemoteContact::RemoteContact(
        const RemoteContact &other)
    : QPair<AccountPtr, ContactPtr>(other.account(), other.contact()), mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
StreamTubeServer::RemoteContact::~RemoteContact()
{
    // mPriv deleted automatically
}

/**
 * Assignment operator.
 */
StreamTubeServer::RemoteContact &StreamTubeServer::RemoteContact::operator=(
        const RemoteContact &other)
{
    if (&other == this) {
        return *this;
    }

    first = other.account();
    second = other.contact();
    mPriv = other.mPriv;

    return *this;
}

/**
 * \fn bool StreamTubeServer::RemoteContact::isValid() const
 *
 * Return whether or not the contact is valid or is just the null object created using the default
 * constructor.
 *
 * \return \c true if valid, \c false otherwise.
 */

/**
 * \fn AccountPtr StreamTubeServer::RemoteContact::account() const
 *
 * Return the account through which the contact can be reached.
 *
 * \return A pointer to the account object.
 */

/**
 * \fn ContactPtr StreamTubeServer::RemoteContact::contact() const
 *
 * Return the actual contact object.
 *
 * \return A pointer to the object.
 */

struct TP_QT_NO_EXPORT StreamTubeServer::Tube::Private : public QSharedData
{
    // empty placeholder for now
};

/**
 * \class StreamTubeServer::Tube
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-server.h <TelepathyQt/StreamTubeServer>
 *
 * \brief The StreamTubeServer::Tube class represents a tube being handled by the server.
 */

/**
 * Constructs a new invalid Tube instance.
 */
StreamTubeServer::Tube::Tube()
{
    // invalid instance
}

/**
 * Constructs a Tube instance for the given tube \a channel originating from the given \a account.
 *
 * \param account A pointer to the account object.
 * \param channel A pointer to the tube channel object.
 */
StreamTubeServer::Tube::Tube(
        const AccountPtr &account,
        const OutgoingStreamTubeChannelPtr &channel)
    : QPair<AccountPtr, OutgoingStreamTubeChannelPtr>(account, channel), mPriv(new Private)
{
}

/**
 * Copy constructor.
 */
StreamTubeServer::Tube::Tube(
        const Tube &other)
    : QPair<AccountPtr, OutgoingStreamTubeChannelPtr>(other.account(), other.channel()),
    mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
StreamTubeServer::Tube::~Tube()
{
    // mPriv deleted automatically
}

/**
 * Assignment operator.
 */
StreamTubeServer::Tube &StreamTubeServer::Tube::operator=(
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
 * \fn bool StreamTubeServer::Tube::isValid() const
 *
 * Return whether or not the tube is valid or is just the null object created using the default
 * constructor.
 *
 * \return \c true if valid, \c false otherwise.
 */

/**
 * \fn AccountPtr StreamTubeServer::Tube::account() const
 *
 * Return the account from which the tube originates.
 *
 * \return A pointer to the account object.
 */

/**
 * \fn OutgoingStreamTubeChannelPtr StreamTubeServer::Tube::channel() const
 *
 * Return the actual tube channel.
 *
 * \return A pointer to the channel.
 */

struct StreamTubeServer::Private
{
    Private(const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices,
            const QString &maybeClientName,
            bool monitorConnections)
        : registrar(registrar),
          handler(SimpleStreamTubeHandler::create(p2pServices, roomServices, true, monitorConnections)),
          clientName(maybeClientName),
          isRegistered(false),
          exportedPort(0),
          generator(0)
    {
        if (clientName.isEmpty()) {
            clientName = QString::fromLatin1("TpQtSTubeServer_%1_%2")
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

        debug() << "Register StreamTubeServer with name " << clientName;

        if (registrar->registerClient(handler, clientName)) {
            isRegistered = true;
        } else {
            warning() << "StreamTubeServer" << clientName
                << "registration failed";
        }
    }

    ClientRegistrarPtr registrar;
    SharedPtr<SimpleStreamTubeHandler> handler;
    QString clientName;
    bool isRegistered;

    QHostAddress exportedAddr;
    quint16 exportedPort;
    ParametersGenerator *generator;
    QScopedPointer<FixedParametersGenerator> fixedGenerator;

    QHash<StreamTubeChannelPtr, TubeWrapper *> tubes;

};

StreamTubeServer::TubeWrapper::TubeWrapper(const AccountPtr &acc,
        const OutgoingStreamTubeChannelPtr &tube, const QHostAddress &exportedAddr,
        quint16 exportedPort, const QVariantMap &params, StreamTubeServer *parent)
    : QObject(parent), mAcc(acc), mTube(tube)
{
    connect(tube->offerTcpSocket(exportedAddr, exportedPort, params),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeOffered(Tp::PendingOperation*)));
    connect(tube.data(),
            SIGNAL(newConnection(uint)),
            SLOT(onNewConnection(uint)));
    connect(tube.data(),
            SIGNAL(connectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));
}

void StreamTubeServer::TubeWrapper::onTubeOffered(Tp::PendingOperation *op)
{
    emit offerFinished(this, op);
}

void StreamTubeServer::TubeWrapper::onNewConnection(uint conn)
{
    emit newConnection(this, conn);
}

void StreamTubeServer::TubeWrapper::onConnectionClosed(uint conn, const QString &error,
        const QString &message)
{
    emit connectionClosed(this, conn, error, message);
}

/**
 * \class StreamTubeServer
 * \ingroup serverclient
 * \headerfile TelepathyQt/stream-tube-server.h <TelepathyQt/StreamTubeServer>
 *
 * \brief The StreamTubeServer class is a Handler implementation for outgoing %Stream %Tube channels,
 * allowing an application to easily export a TCP network server over Telepathy Tubes without
 * worrying about the channel dispatching details.
 *
 * Telepathy Tubes is a technology for connecting arbitrary applications together through the IM
 * network (and sometimes with direct peer-to-peer connections), such that issues like firewall/NAT
 * traversal are automatically handled. Stream Tubes in particular offer properties similar to
 * SOCK_STREAM sockets. The StreamTubeServer class exports such a bytestream socket \b server over
 * the tubes it \em handles as a Telepathy Handler %Client; the StreamTubeClient class is the
 * counterpart, enabling TCP/UNIX socket clients to connect to services from such exported servers
 * offered to them via tubes.
 *
 * Both peer-to-peer (\c TargetHandleType == \ref HandleTypeContact) and group (\c TargetHandleType
 * == \ref HandleTypeRoom) channels are supported, and it's possible to specify the tube services to
 * handle for each separately. It is also possible to not advertise handling capability for ANY tube
 * service; instead just using the StreamTubeServer to handle tubes on an one-off basis by passing
 * its corresponding %Client service name as the \a preferredHandler when requesting tubes via the
 * Account::createStreamTube() methods (or equivalent).
 *
 * %Connection monitoring allows associating incoming connections on the exported server socket with
 * the corresponding remote contacts. This allows an application to show the details of and/or
 * initiate further communication with the remote contacts, without considering the actual tube
 * channels the connections are being made through at all (in particular, their
 * Channel::targetContact() accessor for peer-to-peer and the
 * OutgoingStreamTubeChannel::connectionsForSourceAddresses() accessor for group tubes).
 *
 * Enabling connection monitoring adds a small overhead and latency to handling each incoming tube
 * and signaling each new incoming connection over them, though, so use it only when needed.
 * Additionally, some protocol backends or environments they're running in might not support the
 * ::SocketAccessControlPort mechanism, in which case the source address won't be reported for
 * connections through them. Even in this case, the remote contacts can be associated by accepting
 * one incoming socket connection at a time, and waiting for the corresponding contact to be
 * signaled (although its source address will be invalid, it's the only possibility given its the
 * only accepted connection). However, it's not necessary to do this e.g. with the Gabble XMPP
 * backend, because it fully supports the required mechanism.
 *
 * A service activated Handler can be implemented using StreamTubeServer by passing a predefined \a
 * clientName manually to the chosen create() method, and installing Telepathy \c .client and D-Bus
 * \c .service files declaring the implemented tube services as channel classes and a path to the
 * executable. If this is not needed, the \a clientName can be omitted, in which case a random
 * unique client name is generated and used instead.
 *
 * StreamTubeServer shares Account, Connection and Channel proxies and Contact objects with the
 * rest of the application as long as a reference to the AccountManager, ClientRegistrar, or the
 * factories used elsewhere is passed to the create() method. A stand-alone tube service Handler can
 * get away without passing these however, or just passing select factories to make the desired
 * features prepared and subclasses employed for these objects for their own convenience.
 *
 * Whichever method is used, the ChannelFactory (perhaps indirectly) given must construct
 * OutgoingStreamTubeChannel instances or subclasses thereof for all channel classes corresponding
 * to the tube services to handle. This is the default; overriding it without obeying these
 * constraints using ChannelFactory::setSubclassForOutgoingStreamTubes() or the related methods
 * for room tubes prevents StreamTubeServer from operating correctly.
 *
 * \todo Coin up a small Python script or alike to easily generate the .client and .service files.
 * (fd.o #41614)
 * \todo Support exporting Unix sockets as well. (fd.o #41615)
 */

/**
 * Create a new StreamTubeServer, which will register itself on the session bus using an internal
 * ClientRegistrar and use the given factories.
 *
 * \param p2pServices Names of the tube services to handle on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to handle on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 */
StreamTubeServerPtr StreamTubeServer::create(
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
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
            monitorConnections);
}

/**
 * Create a new StreamTubeServer, which will register itself on the given \a bus using an internal
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
 */
StreamTubeServerPtr StreamTubeServer::create(
        const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
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
            monitorConnections);
}

/**
 * Create a new StreamTubeServer, which will register itself on the bus of and share objects with
 * the given \a accountManager, creating an internal ClientRegistrar.
 *
 * \param accountManager A pointer to the account manager to link up with.
 * \param p2pServices Names of the tube services to handle on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to handle on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 */
StreamTubeServerPtr StreamTubeServer::create(
        const AccountManagerPtr &accountManager,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
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
            monitorConnections);
}

/**
 * Create a new StreamTubeServer, which will register itself on the bus of and using the given
 * client \a registrar, and share objects with it.
 *
 * \param registrar The client registrar to use.
 * \param p2pServices Names of the tube services to handle on peer-to-peer tube channels.
 * \param roomServices Names of the tube services to handle on room/group tube channels.
 * \param clientName The client name (without the \c org.freedesktop.Telepathy.Client. prefix).
 * \param monitorConnections Whether to enable connection monitoring or not.
 */
StreamTubeServerPtr StreamTubeServer::create(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
{
    return StreamTubeServerPtr(
            new StreamTubeServer(registrar, p2pServices, roomServices, clientName,
                monitorConnections));
}

StreamTubeServer::StreamTubeServer(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
    : mPriv(new Private(registrar, p2pServices, roomServices, clientName, monitorConnections))
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
StreamTubeServer::~StreamTubeServer()
{
    if (isRegistered()) {
        mPriv->registrar->unregisterClient(mPriv->handler);
    }

    delete mPriv;
}

/**
 * Return the client registrar used by the server to register itself as a Handler client.
 *
 * This is the registrar originally passed to
 * create(const ClientRegistrarPtr &, const QStringList &, const QStringList &, const QString &, bool)
 * if that was used, and an internally constructed one otherwise. In any case, it can be used to
 * e.g. register further clients like any other ClientRegistrar.
 *
 * \return A pointer to the registrar.
 */
ClientRegistrarPtr StreamTubeServer::registrar() const
{
    return mPriv->registrar;
}

/**
 * Return the Telepathy %Client name of the server.
 *
 * \return The name, without the \c org.freedesktop.Telepathy.Client. prefix of the full D-Bus service name.
 */
QString StreamTubeServer::clientName() const
{
    return mPriv->clientName;
}

/**
 * Return whether the server has been successfully registered or not.
 *
 * Registration is attempted, at the latest, when a socket is first exported using exportTcpSocket().
 * It can fail e.g. because the connection to the bus has failed, or a predefined \a clientName has
 * been passed to create(), and a %Client with the same name is already registered. Typically, failure
 * registering would be a fatal error for a stand-alone tube handler, but only a warning event for
 * an application serving other purposes. In any case, a high-quality user of the API will check the
 * return value of this accessor after exporting their socket.
 *
 * \return \c true if the server has been successfully registered, \c false if not.
 */
bool StreamTubeServer::isRegistered() const
{
    return mPriv->isRegistered;
}

/**
 * Return whether connection monitoring is enabled on this server.
 *
 * For technical reasons, connection monitoring can't be enabled when the server is already running,
 * so there is no corresponding setter method. It has to be enabled by passing \c true as the \a
 * monitorConnections parameter to the create() method.
 *
 * If connection monitoring isn't enabled, newTcpConnection() and tcpConnectionClosed() won't be
 * emitted and tcpConnections() won't be populated.
 *
 * \return \c true if monitoring is enabled, \c false if not.
 */
bool StreamTubeServer::monitorsConnections() const
{
    return mPriv->handler->monitorsConnections();
}

/**
 * Return the host address and port of the currently exported TCP socket, if any.
 *
 * QHostAddress::Null is reported as the address and 0 as the port if no TCP socket has yet been
 * successfully exported.
 *
 * \return The host address and port values in a pair structure.
 */
QPair<QHostAddress, quint16> StreamTubeServer::exportedTcpSocketAddress() const
{
    return qMakePair(mPriv->exportedAddr, mPriv->exportedPort);
}

/**
 * Return the fixed parameters, if any, which are sent along when offering the exported socket on
 * all handled tubes.
 *
 * To prevent accidentally leaving the current parameters to be sent when offering a different
 * socket, or vice versa, the parameters can only be set together with the socket using
 * exportTcpSocket(). Parameters often contain sensitive information such as session identifiers or
 * authentication credentials, which could then be used to maliciously access the service listening
 * on the other socket.
 *
 * If a custom dynamic ParametersGenerator was passed to exportTcpSocket() instead of a set of fixed
 * parameters, an empty set of parameters is returned.
 *
 * \return The parameters in a string-variant map.
 */
QVariantMap StreamTubeServer::exportedParameters() const
{
    if (!mPriv->generator) {
        return QVariantMap();
    }

    FixedParametersGenerator *generator =
        dynamic_cast<FixedParametersGenerator *>(mPriv->generator);

    if (generator) {
        return generator->nextParameters(AccountPtr(), OutgoingStreamTubeChannelPtr(),
                ChannelRequestHints());
    } else {
        return QVariantMap();
    }
}

/**
 * Set the server to offer the socket listening at the given (\a address, \a port) combination as the
 * local endpoint of tubes handled in the future.
 *
 * A fixed set of protocol bootstrapping \a parameters can optionally be set to be sent along with all
 * tube offers until the next call to exportTcpSocket(). See the ParametersGenerator documentation
 * for an in-depth description of the parameter transfer mechanism, and a more flexible way to vary
 * the parameters between each handled tube.
 *
 * The handler is registered on the bus at the latest when this method or another exportTcpSocket()
 * overload is called for the first time, so one should check the return value of isRegistered() at
 * that point to verify that was successful.
 *
 * \param address The listen address of the socket.
 * \param port The port of the socket.
 * \param parameters The bootstrapping parameters in a string-value map.
 */
void StreamTubeServer::exportTcpSocket(
        const QHostAddress &address,
        quint16 port,
        const QVariantMap &parameters)
{
    if (address.isNull() || port == 0) {
        warning() << "Attempted to export null TCP socket address or zero port, ignoring";
        return;
    }

    mPriv->exportedAddr = address;
    mPriv->exportedPort = port;

    mPriv->generator = 0;
    if (!parameters.isEmpty()) {
        mPriv->fixedGenerator.reset(new FixedParametersGenerator(parameters));
        mPriv->generator = mPriv->fixedGenerator.data();
    }

    mPriv->ensureRegistered();
}

/**
 * Set the StreamTubeServer to offer the already listening TCP \a server as the local endpoint of tubes
 * handled in the future.
 *
 * This is just a convenience wrapper around
 * exportTcpSocket(const QHostAddress &, quint16, const QVariantMap &) to be used when the TCP
 * server code is implemented using the QtNetwork facilities.
 *
 * A fixed set of protocol bootstrapping \a parameters can optionally be set to be sent along with all
 * tube offers until the next call to exportTcpSocket(). See the ParametersGenerator documentation
 * for an in-depth description of the parameter transfer mechanism, and a more flexible way to vary
 * the parameters between each handled tube.
 *
 * \param server A pointer to the TCP server.
 * \param parameters The bootstrapping parameters in a string-value map.
 */
void StreamTubeServer::exportTcpSocket(
        const QTcpServer *server,
        const QVariantMap &parameters)
{
    if (!server->isListening()) {
        warning() << "Attempted to export non-listening QTcpServer, ignoring";
        return;
    }

    if (server->serverAddress() == QHostAddress::Any
#if QT_VERSION >= 0x050000
        || server->serverAddress() == QHostAddress::AnyIPv4
#endif
       ) {
        return exportTcpSocket(QHostAddress::LocalHost, server->serverPort(), parameters);
    } else if (server->serverAddress() == QHostAddress::AnyIPv6) {
        return exportTcpSocket(QHostAddress::LocalHostIPv6, server->serverPort(), parameters);
    } else {
        return exportTcpSocket(server->serverAddress(), server->serverPort(), parameters);
    }
}

/**
 * Set the server to offer the socket listening at the given \a address - \a port combination as the
 * local endpoint of tubes handled in the future, sending the parameters from the given \a generator
 * along with the offers.
 *
 * The handler is registered on the bus at the latest when this method or another exportTcpSocket()
 * overload is called for the first time, so one should check the return value of isRegistered() at
 * that point to verify that was successful.
 *
 * \param address The listen address of the socket.
 * \param port The port of the socket.
 * \param generator A pointer to the bootstrapping parameters generator.
 */
void StreamTubeServer::exportTcpSocket(
        const QHostAddress &address,
        quint16 port,
        ParametersGenerator *generator)
{
    if (address.isNull() || port == 0) {
        warning() << "Attempted to export null TCP socket address or zero port, ignoring";
        return;
    }

    mPriv->exportedAddr = address;
    mPriv->exportedPort = port;
    mPriv->generator = generator;

    mPriv->ensureRegistered();
}

/**
 * Set the server to offer the already listening TCP \a server as the local endpoint of tubes
 * handled in the future, sending the parameters from the given \a generator along with the offers.
 *
 * This is just a convenience wrapper around
 * exportTcpSocket(const QHostAddress &, quint16, ParametersGenerator *) to be used when the TCP
 * server code is implemented using the QtNetwork facilities.
 *
 * \param server A pointer to the TCP server.
 * \param generator A pointer to the bootstrapping parameters generator.
 */
void StreamTubeServer::exportTcpSocket(
        const QTcpServer *server,
        ParametersGenerator *generator)
{
    if (!server->isListening()) {
        warning() << "Attempted to export non-listening QTcpServer, ignoring";
        return;
    }

    if (server->serverAddress() == QHostAddress::Any
#if QT_VERSION >= 0x050000
        || server->serverAddress() == QHostAddress::AnyIPv4
#endif
       ) {
        return exportTcpSocket(QHostAddress::LocalHost, server->serverPort(), generator);
    } else if (server->serverAddress() == QHostAddress::AnyIPv6) {
        return exportTcpSocket(QHostAddress::LocalHostIPv6, server->serverPort(), generator);
    } else {
        return exportTcpSocket(server->serverAddress(), server->serverPort(), generator);
    }
}

/**
 * Return the tubes currently handled by the server.
 *
 * \return A list of Tube structures containing pointers to the account and tube channel for each
 * tube.
 */
QList<StreamTubeServer::Tube> StreamTubeServer::tubes() const
{
    QList<Tube> tubes;

    foreach (TubeWrapper *wrapper, mPriv->tubes.values()) {
        tubes.push_back(Tube(wrapper->mAcc, wrapper->mTube));
    }

    return tubes;
}

/**
 * Return the ongoing TCP connections over tubes handled by this server.
 *
 * The returned mapping has the connection source addresses as keys and the contacts along with the
 * accounts which can be used to reach them as values. Connections through protocol backends which
 * don't support SocketAccessControlPort will be included as the potentially many values for the
 * null source address key, the pair (\c QHostAddress::Null, 0).
 *
 * This is effectively a state recovery accessor corresponding to the change notification signals
 * newTcpConnection() and tcpConnectionClosed().
 *
 * The mapping is only populated if connection monitoring was requested when creating the server (so
 * monitorsConnections() returns \c true).
 *
 * \return The connections in a mapping with pairs of their source host addresses and ports as keys
 * and structures containing pointers to the account and remote contacts they're from as values.
 */
QHash<QPair<QHostAddress, quint16>,
    StreamTubeServer::RemoteContact>
    StreamTubeServer::tcpConnections() const
{
    QHash<QPair<QHostAddress /* sourceAddress */, quint16 /* sourcePort */>, RemoteContact> conns;
    if (!monitorsConnections()) {
        warning() << "StreamTubeServer::tcpConnections() used, but connection monitoring is disabled";
        return conns;
    }

    foreach (const Tube &tube, tubes()) {
        // Ignore invalid and non-Open tubes to prevent a few useless warnings in corner cases where
        // a tube is still being opened, or has been invalidated but we haven't processed that event
        // yet.
        if (!tube.channel()->isValid() || tube.channel()->state() != TubeChannelStateOpen) {
            continue;
        }

        if (tube.channel()->addressType() != SocketAddressTypeIPv4 &&
                tube.channel()->addressType() != SocketAddressTypeIPv6) {
            continue;
        }

        QHash<QPair<QHostAddress,quint16>, uint> srcAddrConns =
            tube.channel()->connectionsForSourceAddresses();
        QHash<uint, ContactPtr> connContacts =
            tube.channel()->contactsForConnections();

        QPair<QHostAddress, quint16> srcAddr;
        foreach (srcAddr, srcAddrConns.keys()) {
            ContactPtr contact = connContacts.take(srcAddrConns.value(srcAddr));
            conns.insert(srcAddr, RemoteContact(tube.account(), contact));
        }

        // The remaining values in our copy of connContacts are those which didn't have a
        // corresponding source address, probably because the service doesn't properly implement
        // Port AC
        foreach (const ContactPtr &contact, connContacts.values()) {
            // Insert them with an invalid source address as the key
            conns.insertMulti(qMakePair(QHostAddress(QHostAddress::Null), quint16(0)),
                    RemoteContact(tube.account(), contact));
        }
    }

    return conns;
}

void StreamTubeServer::onInvokedForTube(
        const AccountPtr &acc,
        const StreamTubeChannelPtr &tube,
        const QDateTime &time,
        const ChannelRequestHints &hints)
{
    Q_ASSERT(isRegistered()); // our SSTH shouldn't be receiving any channels unless it's registered
    Q_ASSERT(tube->isRequested());
    Q_ASSERT(tube->isValid()); // SSTH won't emit invalid tubes

    OutgoingStreamTubeChannelPtr outgoing = OutgoingStreamTubeChannelPtr::qObjectCast(tube);

    if (outgoing) {
        emit tubeRequested(acc, outgoing, time, hints);
    } else {
        warning() << "The ChannelFactory used by StreamTubeServer must construct" <<
            "OutgoingStreamTubeChannel subclasses for Requested=true StreamTubes";
        tube->requestClose();
        return;
    }

    if (!mPriv->tubes.contains(tube)) {
        debug().nospace() << "Offering socket " << mPriv->exportedAddr << ":" << mPriv->exportedPort
            << " on tube " << tube->objectPath();

        QVariantMap params;
        if (mPriv->generator) {
            params = mPriv->generator->nextParameters(acc, outgoing, hints);
        }

        Q_ASSERT(!mPriv->exportedAddr.isNull() && mPriv->exportedPort != 0);

        TubeWrapper *wrapper =
            new TubeWrapper(acc, outgoing, mPriv->exportedAddr, mPriv->exportedPort, params, this);

        connect(wrapper,
                SIGNAL(offerFinished(TubeWrapper*,Tp::PendingOperation*)),
                SLOT(onOfferFinished(TubeWrapper*,Tp::PendingOperation*)));
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

        mPriv->tubes.insert(outgoing, wrapper);
    }
}

void StreamTubeServer::onOfferFinished(
        TubeWrapper *wrapper,
        Tp::PendingOperation *op)
{
    OutgoingStreamTubeChannelPtr tube = wrapper->mTube;

    if (op->isError()) {
        warning() << "Offer() failed, closing tube" << tube->objectPath() << '-' <<
            op->errorName() << ':' << op->errorMessage();

        if (wrapper->mTube->isValid()) {
            wrapper->mTube->requestClose();
        }

        wrapper->mTube->disconnect(this);
        emit tubeClosed(wrapper->mAcc, wrapper->mTube, op->errorName(), op->errorMessage());
        mPriv->tubes.remove(wrapper->mTube);
        wrapper->deleteLater();
    } else {
        debug() << "Tube" << tube->objectPath() << "offered successfully";
    }
}

void StreamTubeServer::onTubeInvalidated(
        Tp::DBusProxy *proxy,
        const QString &error,
        const QString &message)
{
    OutgoingStreamTubeChannelPtr tube(qobject_cast<OutgoingStreamTubeChannel *>(proxy));
    Q_ASSERT(!tube.isNull());

    TubeWrapper *wrapper = mPriv->tubes.value(tube);
    if (!wrapper) {
        // Offer finish with error already removed it
        return;
    }

    debug() << "Tube" << tube->objectPath() << "invalidated with" << error << ':' << message;

    emit tubeClosed(wrapper->mAcc, wrapper->mTube, error, message);
    mPriv->tubes.remove(tube);
    delete wrapper;
}

void StreamTubeServer::onNewConnection(
        TubeWrapper *wrapper,
        uint conn)
{
    Q_ASSERT(monitorsConnections());

    if (wrapper->mTube->addressType() == SocketAddressTypeIPv4
            || wrapper->mTube->addressType() == SocketAddressTypeIPv6) {
        QHash<QPair<QHostAddress,quint16>, uint> srcAddrConns =
            wrapper->mTube->connectionsForSourceAddresses();
        QHash<uint, Tp::ContactPtr> connContacts =
            wrapper->mTube->contactsForConnections();

        QPair<QHostAddress, quint16> srcAddr = srcAddrConns.key(conn);
        emit newTcpConnection(srcAddr.first, srcAddr.second, wrapper->mAcc,
                connContacts.value(conn), wrapper->mTube);
    } else {
        // No UNIX socket should ever have been offered yet
        Q_ASSERT(false);
    }
}

void StreamTubeServer::onConnectionClosed(
        TubeWrapper *wrapper,
        uint conn,
        const QString &error,
        const QString &message)
{
    Q_ASSERT(monitorsConnections());

    if (wrapper->mTube->addressType() == SocketAddressTypeIPv4
            || wrapper->mTube->addressType() == SocketAddressTypeIPv6) {
        QHash<QPair<QHostAddress,quint16>, uint> srcAddrConns =
            wrapper->mTube->connectionsForSourceAddresses();
        QHash<uint, Tp::ContactPtr> connContacts =
            wrapper->mTube->contactsForConnections();

        QPair<QHostAddress, quint16> srcAddr = srcAddrConns.key(conn);
        emit tcpConnectionClosed(srcAddr.first, srcAddr.second, wrapper->mAcc,
                connContacts.value(conn), error, message, wrapper->mTube);
    } else {
        // No UNIX socket should ever have been offered yet
        Q_ASSERT(false);
    }
}

/**
 * \fn void StreamTubeServer::tubeRequested(const AccountPtr &account, const
 * OutgoingStreamTubeChannelPtr &tube, const QDateTime &userActionTime, const ChannelRequestHints
 * &hints)
 *
 * Emitted when a tube has been requested for one of our services, and we've began handling it.
 *
 * This is emitted before invoking the ParametersGenerator, if any, for the tube.
 *
 * \param account A pointer to the account from which the tube was requested from.
 * \param tube A pointer to the actual tube channel.
 * \param userActionTime The time the request occurred at, if it was an user action. Should be used
 * for focus stealing prevention.
 * \param hints The hints passed to the request, if any.
 */

/**
 * \fn void StreamTubeServer::tubeClosed(const AccountPtr &account, const
 * OutgoingStreamTubeChannelPtr &tube, const QString &error, const QString &message)
 *
 * Emitted when a tube we've been handling (previously announced with tubeRequested()) has
 * encountered an error or has otherwise been closed from further communication.
 *
 * \param account A pointer to the account from which the tube was requested from.
 * \param tube A pointer to the actual tube channel.
 * \param error The D-Bus error name corresponding to the reason for the closure.
 * \param message A freeform debug message associated with the error.
 */

/**
 * \fn void StreamTubeServer::newTcpConnection(const QHostAddress &sourceAddress, quint16
 * sourcePort, const AccountPtr &account, const ContactPtr &contact, const
 * OutgoingStreamTubeChannelPtr &tube)
 *
 * Emitted when we have picked up a new TCP connection to the (current or previous) exported server
 * socket. This can be used to associate connections the protocol backend relays to the exported
 * socket with the remote contact who originally initiated them in the other end of the tube.
 *
 * This is only emitted if connection monitoring was enabled when creating the StreamTubeServer.
 * Additionally, if the protocol backend the connection is from doesn't support the
 * ::SocketAccessControlPort mechanism, the source address and port will always be invalid.
 *
 * \param sourceAddress The source address of the connection, or QHostAddress::Null if it can't be
 * resolved.
 * \param sourcePort The source port of the connection, or 0 if it can't be resolved.
 * \param account A pointer to the account through which the remote contact can be reached.
 * \param contact A pointer to the remote contact object.
 * \param tube A pointer to the tube channel through which the connection has been made.
 */

/**
 * \fn void StreamTubeServer::tcpConnectionClosed(const QHostAddress &sourceAddress, quint16
 * sourcePort, const AccountPtr &account, const ContactPtr &contact, conts QString &error, const
 * QString &message, const OutgoingStreamTubeChannelPtr &tube)
 *
 * Emitted when a TCP connection (previously announced with newTcpConnection()) through one of our
 * handled tubes has been closed due to an error or by a graceful disconnect (in which case the
 * error is ::TP_QT_ERROR_DISCONNECTED).
 *
 * This is only emitted if connection monitoring was enabled when creating the StreamTubeServer.
 * Additionally, if the protocol backend the connection is from doesn't support the
 * ::SocketAccessControlPort mechanism, the source address and port will always be invalid.
 *
 * \param sourceAddress The source address of the connection, or QHostAddress::Null if it couldn't
 * be resolved.
 * \param sourcePort The source port of the connection, or 0 if it couldn't be resolved.
 * \param account A pointer to the account through which the remote contact can be reached.
 * \param contact A pointer to the remote contact object.
 * \param error The D-Bus error name corresponding to the reason for the closure.
 * \param message A freeform debug message associated with the error.
 * \param tube A pointer to the tube channel through which the connection has been made.
 */

} // Tp
