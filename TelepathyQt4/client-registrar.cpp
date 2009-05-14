/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/ClientRegistrar>
#include "TelepathyQt4/client-registrar-internal.h"

#include "TelepathyQt4/_gen/client-registrar.moc.hpp"
#include "TelepathyQt4/_gen/client-registrar-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/MethodInvocationContext>
#include <TelepathyQt4/PendingReady>

namespace Tp
{

class HandleChannelsInvocationContext : public MethodInvocationContext<>
{
    Q_DISABLE_COPY(HandleChannelsInvocationContext)

public:
    typedef void (*FinishedCb)(const MethodInvocationContextPtr<> &context,
                               const QList<ChannelPtr> &channels,
                               void *data);

    static MethodInvocationContextPtr<> create(const QDBusConnection &bus,
            const QDBusMessage &message, const QList<ChannelPtr> &channels,
            FinishedCb finishedCb, void *finishedCbData)
    {
        return MethodInvocationContextPtr<>::dynamicCast(
                SharedPtr<HandleChannelsInvocationContext>(
                    new HandleChannelsInvocationContext(bus, message, channels,
                        finishedCb, finishedCbData)));
    }

private:
    HandleChannelsInvocationContext(const QDBusConnection &connection,
            const QDBusMessage &message, const QList<ChannelPtr> &channels,
            FinishedCb finishedCb, void *finishedCbData)
        : MethodInvocationContext<>(connection, message),
          mChannels(channels),
          mFinishedCb(finishedCb),
          mFinishedCbData(finishedCbData)
    {
    }

    void onFinished()
    {
        if (mFinishedCb) {
            mFinishedCb(MethodInvocationContextPtr<>(this), mChannels, mFinishedCbData);
        }
    }

    QList<ChannelPtr> mChannels;
    FinishedCb mFinishedCb;
    void *mFinishedCbData;
};

ClientAdaptor::ClientAdaptor(const QStringList &interfaces,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mInterfaces(interfaces)
{
}

ClientAdaptor::~ClientAdaptor()
{
}

QHash<QPair<QString, QString>, QList<ClientHandlerAdaptor *> > ClientHandlerAdaptor::mAdaptorsForConnection;

ClientHandlerAdaptor::ClientHandlerAdaptor(const QDBusConnection &bus,
        AbstractClientHandler *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mBus(bus),
      mClient(client)
{
    QList<ClientHandlerAdaptor *> &handlerAdaptors =
        mAdaptorsForConnection[qMakePair(mBus.name(), mBus.baseService())];
    handlerAdaptors.append(this);
}

ClientHandlerAdaptor::~ClientHandlerAdaptor()
{
    QPair<QString, QString> busId = qMakePair(mBus.name(), mBus.baseService());
    QList<ClientHandlerAdaptor *> &handlerAdaptors =
        mAdaptorsForConnection[busId];
    handlerAdaptors.removeOne(this);
    if (handlerAdaptors.isEmpty()) {
        mAdaptorsForConnection.remove(busId);
    }
}

void ClientHandlerAdaptor::HandleChannels(const QDBusObjectPath &accountPath,
        const QDBusObjectPath &connectionPath,
        const Tp::ChannelDetailsList &channelDetailsList,
        const Tp::ObjectPathList &requestsSatisfied,
        qulonglong userActionTime_t,
        const QVariantMap &handlerInfo,
        const QDBusMessage &message)
{
    debug() << "HandleChannels: account:" << accountPath.path() <<
        ", connection:" << connectionPath.path();

    AccountPtr account = Account::create(mBus,
            TELEPATHY_ACCOUNT_MANAGER_BUS_NAME,
            accountPath.path());

    QString connectionBusName = connectionPath.path().mid(1).replace('/', '.');
    ConnectionPtr connection = Connection::create(mBus, connectionBusName,
            connectionPath.path());

    QList<ChannelPtr> channels;
    ChannelPtr channel;
    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        channel = Channel::create(connection, channelDetails.channel.path(),
                channelDetails.properties);
        channels.append(channel);
    }

    QList<ChannelRequestPtr> channelRequests;
    ChannelRequestPtr channelRequest;
    foreach (const QDBusObjectPath &path, requestsSatisfied) {
        channelRequest = ChannelRequest::create(mBus,
                path.path(), QVariantMap());
        channelRequests.append(channelRequest);
    }

    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    QDateTime userActionTime;
    if (userActionTime_t != 0) {
        userActionTime = QDateTime::fromTime_t((uint) userActionTime_t);
    }

    MethodInvocationContextPtr<> context =
        HandleChannelsInvocationContext::create(mBus, message,
                channels,
                (HandleChannelsInvocationContext::FinishedCb) &ClientHandlerAdaptor::onContextFinished,
                this);

    mClient->handleChannels(context, account, connection, channels,
            channelRequests, userActionTime, handlerInfo);
}

void ClientHandlerAdaptor::onContextFinished(
        const MethodInvocationContextPtr<> &context,
        const QList<ChannelPtr> &channels, ClientHandlerAdaptor *self)
{
    if (!context->isError()) {
        debug() << "HandleChannels context finished successfully, "
            "updating handled channels";
        foreach (const ChannelPtr &channel, channels) {
            self->mHandledChannels.insert(channel);
            self->connect(channel.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SLOT(onChannelInvalidated(Tp::DBusProxy *)));
        }
    }
}

void ClientHandlerAdaptor::onChannelInvalidated(DBusProxy *proxy)
{
    ChannelPtr channel(dynamic_cast<Channel*>(proxy));
    mHandledChannels.remove(channel);
}

ClientHandlerRequestsAdaptor::ClientHandlerRequestsAdaptor(
        const QDBusConnection &bus,
        AbstractClientHandler *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mBus(bus),
      mClient(client)
{
}

ClientHandlerRequestsAdaptor::~ClientHandlerRequestsAdaptor()
{
}

void ClientHandlerRequestsAdaptor::AddRequest(
        const QDBusObjectPath &request,
        const QVariantMap &requestProperties,
        const QDBusMessage &message)
{
    debug() << "AddRequest:" << request.path();
    mBus.send(message.createReply());
    mClient->addRequest(ChannelRequest::create(mBus,
                request.path(), requestProperties));
}

void ClientHandlerRequestsAdaptor::RemoveRequest(
        const QDBusObjectPath &request,
        const QString &errorName, const QString &errorMessage,
        const QDBusMessage &message)
{
    debug() << "RemoveRequest:" << request.path() << "-" << errorName
        << "-" << errorMessage;
    mBus.send(message.createReply());
    mClient->removeRequest(ChannelRequest::create(mBus,
                request.path(), QVariantMap()), errorName, errorMessage);
}

struct ClientRegistrar::Private
{
    Private(const QDBusConnection &bus)
        : bus(bus)
    {
    }

    QDBusConnection bus;
    QHash<AbstractClientPtr, QString> clients;
    QHash<AbstractClientPtr, QObject*> clientObjects;
    QSet<QString> services;
};

/**
 * \class ClientRegistrar
 * \ingroup clientclient
 * \headerfile <TelepathyQt4/client-registrar.h> <TelepathyQt4/ClientRegistrar>
 *
 * Object responsible for registering
 * <a href="http://telepathy.freedesktop.org">Telepathy</a>
 * clients (Observer, Approver, Handler).
 *
 * Clients should inherit AbstractClientObserver, AbstractClientApprover,
 * AbstractClientHandler or some combination of these, by using multiple
 * inheritance, and register themselves using registerClient().
 *
 * See the individual classes descriptions for more details.
 *
 * \section usage_sec Usage
 *
 * \subsection create_sec Creating a client registrar object
 *
 * One way to create a ClientRegistrar object is to just call the create method.
 * For example:
 *
 * \code ClientRegistrarPtr cr = ClientRegistrar::create(); \endcode
 *
 * You can also provide a D-Bus connection as a QDBusConnection:
 *
 * \code ClientRegistrarPtr cr = ClientRegistrar::create(QDBusConnection::sessionBus()); \endcode
 *
 * \subsection registering_sec Registering a client
 *
 * To register a client, just call registerClient with a given AbstractClientPtr
 * pointing to a valid AbstractClient instance.
 *
 * \code
 *
 * class MyClient : public AbstractClientObserver, public AbstractClientHandler
 * {
 *     ...
 * };
 *
 * ...
 *
 * ClientRegistrarPtr cr = ClientRegistrar::create();
 * SharedPtr<MyClient> client = SharedPtr<MyClient>(new MyClient(...));
 * cr->registerClient(AbstractClientPtr::dynamicCast(client), "myclient");
 *
 * \endcode
 *
 * \sa AbstractClientObserver, AbstractClientApprover, AbstractClientHandler
 */

QHash<QPair<QString, QString>, ClientRegistrar*> ClientRegistrar::registrarForConnection;

/**
 * Create a new client registrar object using QDBusConnection::sessionBus().
 *
 * ClientRegistrar instances are unique per D-Bus connection. The returned
 * ClientRegistrarPtr will point to the same ClientRegistrar instance on
 * successive calls, unless the instance had already been destroyed, in which
 * case a new instance will be returned.
 *
 * \return A ClientRegistrarPtr object pointing to the ClientRegistrar.
 */
ClientRegistrarPtr ClientRegistrar::create()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    return create(bus);
}

/**
 * Create a new client registrar object using the given \a bus.
 *
 * ClientRegistrar instances are unique per D-Bus connection. The returned
 * ClientRegistrarPtr will point to the same ClientRegistrar instance on
 * successive calls with the same \a bus, unless the instance
 * had already been destroyed, in which case a new instance will be returned.
 *
 * \param bus QDBusConnection to use.
 * \return A ClientRegistrarPtr object pointing to the ClientRegistrar.
 */
ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus)
{
    QPair<QString, QString> busId =
        qMakePair(bus.name(), bus.baseService());
    if (registrarForConnection.contains(busId)) {
        return ClientRegistrarPtr(
                registrarForConnection.value(busId));
    }
    return ClientRegistrarPtr(new ClientRegistrar(bus));
}

/**
 * Construct a new client registrar object  using the given \a bus.
 *
 * \param bus QDBusConnection to use.
 */
ClientRegistrar::ClientRegistrar(const QDBusConnection &bus)
    : mPriv(new Private(bus))
{
    registrarForConnection.insert(qMakePair(bus.name(),
                bus.baseService()), this);
}

/**
 * Class destructor.
 */
ClientRegistrar::~ClientRegistrar()
{
    registrarForConnection.remove(qMakePair(mPriv->bus.name(),
                mPriv->bus.baseService()));
    unregisterClients();
    delete mPriv;
}

/**
 * Return the D-Bus connection being used by this client registrar.
 *
 * \return QDBusConnection being used.
 */
QDBusConnection ClientRegistrar::dbusConnection() const
{
    return mPriv->bus;
}

/**
 * Return a list of clients registered using registerClient on this client registrar.
 *
 * \return A list of registered clients.
 * \sa registerClient()
 */
QList<AbstractClientPtr> ClientRegistrar::registeredClients() const
{
    return mPriv->clients.keys();
}

/**
 * Register a client on D-Bus.
 *
 * The client registrar will be responsible for proper exporting the client
 * D-Bus interfaces, based on inheritance.
 *
 * If each of a client instance should be able to manipulate channels
 * separately, set unique to true.
 *
 * The client name MUST be a non-empty string of ASCII digits, letters, dots
 * and/or underscores, starting with a letter, and without sets of
 * two consecutive dots or a dot followed by a digit.
 *
 * This method will do nothing if the client is already registered, and \c true
 * will be returned.
 *
 * To unregister a client use unregisterClient().
 *
 * \param client The client to register.
 * \param clientName The client name used to register.
 * \param unique Whether each of a client instance is able to manipulate
 *               channels separately.
 * \return \c true if client was successfully registered, \c false otherwise.
 * \sa registeredClients(), unregisterClient()
 */
bool ClientRegistrar::registerClient(const AbstractClientPtr &client,
        const QString &clientName, bool unique)
{
    if (!client) {
        warning() << "Unable to register a null client";
        return false;
    }

    if (mPriv->clients.contains(client)) {
        debug() << "Client already registered";
        return true;
    }

    QString busName = QLatin1String("org.freedesktop.Telepathy.Client.");
    busName.append(clientName);
    if (unique) {
        // o.f.T.Client.<unique_bus_name>_<pointer> should be enough to identify
        // an unique identifier
        busName.append(QString(".%1._%2")
                .arg(mPriv->bus.baseService()
                    .replace(':', '_')
                    .replace('.', "._"))
                .arg((intptr_t) client.data()));
    }

    if (mPriv->services.contains(busName) ||
        !mPriv->bus.registerService(busName)) {
        warning() << "Unable to register client: busName" <<
            busName << "already registered";
        return false;
    }

    QObject *object = new QObject(this);
    QStringList interfaces;

    AbstractClientHandler *handler =
        dynamic_cast<AbstractClientHandler*>(client.data());

    if (handler) {
        // export o.f.T.Client.Handler
        new ClientHandlerAdaptor(mPriv->bus, handler, object);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Handler"));
        if (handler->wantsRequestNotification()) {
            // export o.f.T.Client.Interface.Requests
            new ClientHandlerRequestsAdaptor(mPriv->bus, handler, object);
            interfaces.append(
                    QLatin1String(
                        "org.freedesktop.Telepathy.Client.Interface.Requests"));
        }
    }

    // TODO add more adaptors when they exist

    if (interfaces.isEmpty()) {
        warning() << "Client does not implement any known interface";
        // cleanup
        mPriv->bus.unregisterService(busName);
        return false;
    }

    // export o.f,T,Client interface
    new ClientAdaptor(interfaces, object);

    QString objectPath = QString("/%1").arg(busName);
    objectPath.replace('.', '/');
    if (!mPriv->bus.registerObject(objectPath, object)) {
        // this shouldn't happen, but let's make sure
        warning() << "Unable to register client: objectPath" <<
            objectPath << "already registered";
        // cleanup
        delete object;
        mPriv->bus.unregisterService(busName);
        return false;
    }

    debug() << "Client registered - busName:" << busName <<
        "objectPath:" << objectPath << "interfaces:" << interfaces;

    mPriv->services.insert(busName);
    mPriv->clients.insert(client, objectPath);
    mPriv->clientObjects.insert(client, object);

    return true;
}

/**
 * Unregister a client registered using registerClient on this client registrar.
 *
 * If \a client was not registered previously, \c false will be returned.
 *
 * \param client The client to unregister.
 * \return \c true if client was successfully unregistered, \c false otherwise.
 * \sa registeredClients(), registerClient()
 */
bool ClientRegistrar::unregisterClient(const AbstractClientPtr &client)
{
    if (!mPriv->clients.contains(client)) {
        warning() << "Trying to unregister an unregistered client";
        return false;
    }

    QString objectPath = mPriv->clients.value(client);
    mPriv->bus.unregisterObject(objectPath);
    mPriv->clients.remove(client);
    QObject *object = mPriv->clientObjects.value(client);
    // delete object here and it's children (adaptors), to make sure if adaptor
    // is keeping a static list of adaptors per connection, the list is updated.
    delete object;
    mPriv->clientObjects.remove(client);

    QString busName = objectPath.mid(1).replace('/', '.');
    mPriv->bus.unregisterService(busName);
    mPriv->services.remove(busName);

    debug() << "Client unregistered - busName:" << busName <<
        "objectPath:" << objectPath;

    return true;
}

/**
 * Unregister all clients registered using registerClient on this client registrar.
 *
 * \sa registeredClients(), registerClient(), unregisterClient()
 */
void ClientRegistrar::unregisterClients()
{
    // copy the hash as it will be modified
    QHash<AbstractClientPtr, QString> clients = mPriv->clients;

    QHash<AbstractClientPtr, QString>::const_iterator end =
        clients.constEnd();
    QHash<AbstractClientPtr, QString>::const_iterator it =
        clients.constBegin();
    while (it != end) {
        unregisterClient(it.key());
        ++it;
    }
}

} // Tp
