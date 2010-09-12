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

#include "TelepathyQt4/channel-factory.h"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelDispatchOperation>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/MethodInvocationContext>
#include <TelepathyQt4/PendingComposite>
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
        return SharedPtr<MethodInvocationContext<> >(
                    new HandleChannelsInvocationContext(bus, message, channels,
                        finishedCb, finishedCbData));
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

ClientAdaptor::ClientAdaptor(ClientRegistrar *registrar, const QStringList &interfaces,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mRegistrar(registrar),
      mInterfaces(interfaces)
{
}

ClientAdaptor::~ClientAdaptor()
{
}

ClientObserverAdaptor::ClientObserverAdaptor(ClientRegistrar *registrar,
        AbstractClientObserver *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mRegistrar(registrar),
      mBus(registrar->dbusConnection()),
      mClient(client)
{
}

ClientObserverAdaptor::~ClientObserverAdaptor()
{
}

void ClientObserverAdaptor::ObserveChannels(const QDBusObjectPath &accountPath,
        const QDBusObjectPath &connectionPath,
        const Tp::ChannelDetailsList &channelDetailsList,
        const QDBusObjectPath &dispatchOperationPath,
        const Tp::ObjectPathList &requestsSatisfied,
        const QVariantMap &observerInfo,
        const QDBusMessage &message)
{
    debug() << "ObserveChannels: account:" << accountPath.path() <<
        ", connection:" << connectionPath.path();

    AccountFactoryConstPtr accFactory = mRegistrar->accountFactory();
    ConnectionFactoryConstPtr connFactory = mRegistrar->connectionFactory();
    ChannelFactoryConstPtr chanFactory = mRegistrar->channelFactory();
    ContactFactoryConstPtr contactFactory = mRegistrar->contactFactory();

    SharedPtr<InvocationData> invocation(new InvocationData());

    QList<PendingOperation *> readyOps;

    PendingReady *accReady = accFactory->proxy(QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            accountPath.path(),
            connFactory,
            chanFactory,
            contactFactory);
    invocation->acc = AccountPtr::dynamicCast(accReady->proxy());
    readyOps.append(accReady);

    QString connectionBusName = connectionPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = connFactory->proxy(connectionBusName, connectionPath.path(),
            chanFactory, contactFactory);
    invocation->conn = ConnectionPtr::dynamicCast(connReady->proxy());
    readyOps.append(connReady);

    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        PendingReady *chanReady = chanFactory->proxy(invocation->conn,
                channelDetails.channel.path(), channelDetails.properties);
        ChannelPtr channel = ChannelPtr::dynamicCast(chanReady->proxy());
        invocation->chans.append(channel);
        readyOps.append(chanReady);
    }

    // Yes, we don't give the choice of making CDO and CR ready or not - however, readifying them is
    // 0-1 D-Bus calls each, for CR mostly 0 - and their constructors start making them ready
    // automatically, so we wouldn't save any D-Bus traffic anyway

    if (!dispatchOperationPath.path().isEmpty() && dispatchOperationPath.path() != QLatin1String("/")) {
        invocation->dispatchOp = ChannelDispatchOperation::create(mBus, dispatchOperationPath.path(),
                QVariantMap());
        readyOps.append(invocation->dispatchOp->becomeReady());
    }

    // TODO make observer info nicer to use...
    invocation->observerInfo = observerInfo;

    /*
     * Uncomment this and the below one when we have spec 0.19.12
     *
     * ObjectImmutablePropertiesMap propMap = qdbus_cast<ObjectImmutablePropertiesMap>(
     * observerInfo.value(QLatin1String("request-properties")));
     */
    foreach (const QDBusObjectPath &path, requestsSatisfied) {
        ChannelRequestPtr channelRequest = ChannelRequest::create(mBus,
                path.path(), QVariantMap() /* propMap.value(path.path()) */);
        invocation->chanReqs.append(channelRequest);
        readyOps.append(channelRequest->becomeReady());
    }

    invocation->ctx = MethodInvocationContextPtr<>(new MethodInvocationContext<>(mBus, message));

    invocation->readyOp = new PendingComposite(readyOps, this);
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    invocations.append(invocation);

    debug() << "Preparing proxies for ObserveChannels of" << channelDetailsList.size() << "channels"
        << "for client" << mClient;
}

void ClientObserverAdaptor::onReadyOpFinished(Tp::PendingOperation *op) {
    Q_ASSERT(!invocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = invocations.begin();
            i != invocations.end(); ++i) {
        if ((*i)->readyOp != op) {
            continue;
        }

        (*i)->readyOp = 0;

        if (op->isError()) {
            warning() << "Preparing proxies for ObserveChannels failed with" << op->errorName()
                << op->errorMessage();
            (*i)->error = op->errorName();
            (*i)->message = op->errorMessage();
        }

        break;
    }

    while (!invocations.isEmpty() && !invocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = invocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Invoking application observeChannels with" << invocation->chans.size()
            << "channels on" << mClient;

        // API/ABI break TODO: make observerInfo a friendly high-level variantmap wrapper similar to
        // Connection::ErrorDetails
        mClient->observeChannels(invocation->ctx, invocation->acc, invocation->conn,
                invocation->chans, invocation->dispatchOp, invocation->chanReqs,
                invocation->observerInfo);
    }
}

ClientApproverAdaptor::ClientApproverAdaptor(ClientRegistrar *registrar,
        AbstractClientApprover *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mRegistrar(registrar),
      mBus(registrar->dbusConnection()),
      mClient(client)
{
}

ClientApproverAdaptor::~ClientApproverAdaptor()
{
}

void ClientApproverAdaptor::AddDispatchOperation(const Tp::ChannelDetailsList &channelDetailsList,
        const QDBusObjectPath &dispatchOperationPath,
        const QVariantMap &properties,
        const QDBusMessage &message)
{
    ConnectionFactoryConstPtr connFactory = mRegistrar->connectionFactory();
    ChannelFactoryConstPtr chanFactory = mRegistrar->channelFactory();
    ContactFactoryConstPtr contactFactory = mRegistrar->contactFactory();

    QList<PendingOperation *> readyOps;

    QDBusObjectPath connectionPath = qdbus_cast<QDBusObjectPath>(
            properties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_DISPATCH_OPERATION ".Connection")));
    debug() << "addDispatchOperation: connection:" << connectionPath.path();
    QString connectionBusName = connectionPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = connFactory->proxy(connectionBusName, connectionPath.path(), chanFactory,
                contactFactory);
    ConnectionPtr connection = ConnectionPtr::dynamicCast(connReady->proxy());
    readyOps.append(connReady);

    SharedPtr<InvocationData> invocation(new InvocationData);

    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        PendingReady *chanReady = chanFactory->proxy(connection, channelDetails.channel.path(),
                channelDetails.properties);
        invocation->chans.append(ChannelPtr::dynamicCast(chanReady->proxy()));
        readyOps.append(chanReady);
    }

    invocation->dispatchOp = ChannelDispatchOperation::create(dispatchOperationPath.path(),
            properties);
    readyOps.append(invocation->dispatchOp->becomeReady());

    invocation->ctx = MethodInvocationContextPtr<>(new MethodInvocationContext<>(mBus, message));

    invocation->readyOp = new PendingComposite(readyOps, this);
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    invocations.append(invocation);
}

void ClientApproverAdaptor::onReadyOpFinished(Tp::PendingOperation *op) {
    Q_ASSERT(!invocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = invocations.begin();
            i != invocations.end(); ++i) {
        if ((*i)->readyOp != op) {
            continue;
        }

        (*i)->readyOp = 0;

        if (op->isError()) {
            warning() << "Preparing proxies for AddDispatchOperation failed with" << op->errorName()
                << op->errorMessage();
            (*i)->error = op->errorName();
            (*i)->message = op->errorMessage();
        }

        break;
    }

    while (!invocations.isEmpty() && !invocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = invocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Invoking application addDispatchOperation with CDO"
            << invocation->dispatchOp->objectPath() << "on" << mClient;

        // API/ABI break FIXME: Don't pass the same channels as the channels arg and (separately
        // constructed) in dispatchOp->channels() !!!
        mClient->addDispatchOperation(invocation->ctx, invocation->chans, invocation->dispatchOp);
    }
}

QHash<QPair<QString, QString>, QList<ClientHandlerAdaptor *> > ClientHandlerAdaptor::mAdaptorsForConnection;

ClientHandlerAdaptor::ClientHandlerAdaptor(ClientRegistrar *registrar,
        AbstractClientHandler *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mRegistrar(registrar),
      mBus(registrar->dbusConnection()),
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

    AccountFactoryConstPtr accFactory = mRegistrar->accountFactory();
    ConnectionFactoryConstPtr connFactory = mRegistrar->connectionFactory();
    ChannelFactoryConstPtr chanFactory = mRegistrar->channelFactory();
    ContactFactoryConstPtr contactFactory = mRegistrar->contactFactory();

    SharedPtr<InvocationData> invocation(new InvocationData());
    QList<PendingOperation *> readyOps;

    PendingReady *accReady = accFactory->proxy(QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            accountPath.path(),
            connFactory,
            chanFactory,
            contactFactory);
    invocation->acc = AccountPtr::dynamicCast(accReady->proxy());
    readyOps.append(accReady);

    QString connectionBusName = connectionPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = connFactory->proxy(connectionBusName, connectionPath.path(),
            chanFactory, contactFactory);
    invocation->conn = ConnectionPtr::dynamicCast(connReady->proxy());
    readyOps.append(connReady);

    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        PendingReady *chanReady = chanFactory->proxy(invocation->conn,
                channelDetails.channel.path(), channelDetails.properties);
        ChannelPtr channel = ChannelPtr::dynamicCast(chanReady->proxy());
        invocation->chans.append(channel);
        readyOps.append(chanReady);
    }

    // API/ABI break TODO: make handler info nicer to use
    invocation->handlerInfo = handlerInfo;

    /*
     * Uncomment this and the below one when we have spec 0.19.12
     *
     * ObjectImmutablePropertiesMap propMap = qdbus_cast<ObjectImmutablePropertiesMap>(
     * handlerInfo.value(QLatin1String("request-properties")));
     */
    foreach (const QDBusObjectPath &path, requestsSatisfied) {
        ChannelRequestPtr channelRequest = ChannelRequest::create(mBus,
                path.path(), QVariantMap() /* propMap.value(path.path()) */);
        invocation->chanReqs.append(channelRequest);
        readyOps.append(channelRequest->becomeReady());
    }

    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    if (userActionTime_t != 0) {
        invocation->time = QDateTime::fromTime_t((uint) userActionTime_t);
    }

    invocation->ctx = HandleChannelsInvocationContext::create(mBus, message,
                invocation->chans,
                reinterpret_cast<HandleChannelsInvocationContext::FinishedCb>(
                    &ClientHandlerAdaptor::onContextFinished),
                this);

    invocation->readyOp = new PendingComposite(readyOps, this);
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    invocations.append(invocation);

    debug() << "Preparing proxies for HandleChannels of" << channelDetailsList.size() << "channels"
        << "for client" << mClient;
}

void ClientHandlerAdaptor::onReadyOpFinished(Tp::PendingOperation *op) {
    Q_ASSERT(!invocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = invocations.begin();
            i != invocations.end(); ++i) {
        if ((*i)->readyOp != op) {
            continue;
        }

        (*i)->readyOp = 0;

        if (op->isError()) {
            warning() << "Preparing proxies for HandleChannels failed with" << op->errorName()
                << op->errorMessage();
            (*i)->error = op->errorName();
            (*i)->message = op->errorMessage();
        }

        break;
    }

    while (!invocations.isEmpty() && !invocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = invocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Invoking application observeChannels with" << invocation->chans.size()
            << "channels on" << mClient;

        // API/ABI break TODO: make handlerInfo a friendly high-level variantmap wrapper similar to
        // Connection::ErrorDetails
        mClient->handleChannels(invocation->ctx, invocation->acc, invocation->conn,
                invocation->chans, invocation->chanReqs, invocation->time, invocation->handlerInfo);
    }
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
        ClientRegistrar *registrar,
        AbstractClientHandler *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mRegistrar(registrar),
      mBus(registrar->dbusConnection()),
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
    message.setDelayedReply(true);
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
    message.setDelayedReply(true);
    mBus.send(message.createReply());
    mClient->removeRequest(ChannelRequest::create(mBus,
                request.path(), QVariantMap()), errorName, errorMessage);
}

struct TELEPATHY_QT4_NO_EXPORT ClientRegistrar::Private
{
    Private(const QDBusConnection &bus, const AccountFactoryConstPtr &accFactory,
            const ConnectionFactoryConstPtr &connFactory, const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory)
        : bus(bus), accFactory(accFactory), connFactory(connFactory), chanFactory(chanFactory),
        contactFactory(contactFactory)
    {
        if (accFactory->dbusConnection().name() != bus.name()) {
            warning() << "  The D-Bus connection in the account factory is not the proxy connection";
        }

        if (connFactory->dbusConnection().name() != bus.name()) {
            warning() << "  The D-Bus connection in the connection factory is not the proxy connection";
        }

        if (chanFactory->dbusConnection().name() != bus.name()) {
            warning() << "  The D-Bus connection in the channel factory is not the proxy connection";
        }
    }

    QDBusConnection bus;

    AccountFactoryConstPtr accFactory;
    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    QHash<AbstractClientPtr, QString> clients;
    QHash<AbstractClientPtr, QObject*> clientObjects;
    QSet<QString> services;
};

/**
 * \class ClientRegistrar
 * \ingroup serverclient
 * \headerfile TelepathyQt4/client-registrar.h <TelepathyQt4/ClientRegistrar>
 *
 * \brief The ClientRegistrar class is responsible for registering Telepathy
 * clients (Observer, Approver, Handler).
 *
 * Clients should inherit AbstractClientObserver, AbstractClientApprover,
 * AbstractClientHandler or some combination of these, by using multiple
 * inheritance, and register themselves using registerClient().
 *
 * See the individual classes descriptions for more details.
 *
 * \section cr_usage_sec Usage
 *
 * \subsection cr_create_sec Creating a client registrar object
 *
 * One way to create a ClientRegistrar object is to just call the create method.
 * For example:
 *
 * \code ClientRegistrarPtr cr = ClientRegistrar::create(); \endcode
 *
 * You can also provide a D-Bus connection as a QDBusConnection:
 *
 * \code ClientRegistrarPtr cr = ClientRegistrar::create(QDBusConnection::systemBus()); \endcode
 *
 * \subsection cr_registering_sec Registering a client
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
 * Create a new client registrar object using the given \a bus.
 *
 * ClientRegistrar instances are unique per D-Bus connection. The returned
 * ClientRegistrarPtr will point to the same ClientRegistrar instance on
 * successive calls with the same \a bus, unless the instance
 * had already been destroyed, in which case a new instance will be returned.
 *
 * The resulting instance will use factories from a previous ClientRegistrar with the same \a bus,
 * if any, otherwise factories returning stock TpQt4 subclasses as approriate, with no features
 * prepared. This gives fully backwards compatible behavior for this function if the factory
 * variants are never used.
 *
 * \todo API/ABI break: Drop the bus-wide singleton guarantee, it's awkward and the associated name
 * registration checks have always been implemented incorrectly anyway. We need this for the account
 * friendly channel request and handle API at least.
 *
 * \param bus QDBusConnection to use.
 * \return A ClientRegistrarPtr object pointing to the ClientRegistrar.
 */
ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus)
{
    return create(bus, AccountFactory::create(bus, Features()), ConnectionFactory::create(bus),
            ChannelFactory::create(bus), ContactFactory::create());
}

/**
 * Create a new client registrar object using QDBusConnection::sessionBus() and the given factories.
 *
 * ClientRegistrar instances are unique per D-Bus connection. The returned ClientRegistrarPtr will
 * point to the same ClientRegistrar instance on successive calls, unless the instance for
 * QDBusConnection::sessionBus() had already been destroyed, in which case a new instance will be
 * returned. Therefore, the factory settings have no effect if there already is a ClientRegistrar
 * for QDBusConnection::sessionBus().
 *
 * \todo API/ABI break: Drop the bus-wide singleton guarantee, it's awkward and the associated name
 * registration checks have always been implemented incorrectly anyway. We need this for the account
 * friendly channel request and handle API at least.
 *
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ClientRegistrarPtr object pointing to the ClientRegistrar.
 */
ClientRegistrarPtr ClientRegistrar::create(
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory)
{
    return create(QDBusConnection::sessionBus(), accountFactory, connectionFactory, channelFactory,
            contactFactory);
}

/**
 * Create a new client registrar object using the given \a bus and the given factories.
 *
 * ClientRegistrar instances are unique per D-Bus connection. The returned
 * ClientRegistrarPtr will point to the same ClientRegistrar instance on
 * successive calls with the same \a bus, unless the instance
 * had already been destroyed, in which case a new instance will be returned. Therefore, the factory
 * settings have no effect if there already is a ClientRegistrar for the given \a bus.
 *
 * \todo API/ABI break: Drop the bus-wide singleton guarantee, it's awkward and the associated name
 * registration checks have always been implemented incorrectly anyway. We need this for the account
 * friendly channel request and handle API at least.
 *
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ClientRegistrarPtr object pointing to the ClientRegistrar.
 */
ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory)
{
    QPair<QString, QString> busId =
        qMakePair(bus.name(), bus.baseService());
    if (registrarForConnection.contains(busId)) {
        return ClientRegistrarPtr(
                registrarForConnection.value(busId));
    }
    return ClientRegistrarPtr(new ClientRegistrar(bus, accountFactory, connectionFactory,
                channelFactory, contactFactory));
}

/**
 * Create a new client registrar object using the bus and factories of the given Account \a manager.
 *
 * ClientRegistrar instances are unique per D-Bus connection. The returned ClientRegistrarPtr will
 * point to the same ClientRegistrar instance on successive calls with the same bus, unless the
 * instance had already been destroyed, in which case a new instance will be returned. Therefore,
 * the factory settings have no effect if there already is a ClientRegistrar for the bus of the
 * given \a manager.
 *
 * Using this create() method will enable (like any other way of passing the same factories to an AM
 * and a registrar) getting the same Account/Connection etc. proxy instances from both
 * AccountManager and AbstractClient implementations.
 *
 * \todo API/ABI break: Drop the bus-wide singleton guarantee, it's awkward and the associated name
 * registration checks have always been implemented incorrectly anyway. We need this for the account
 * friendly channel request and handle API at least.
 *
 * \param manager The AccountManager the bus and factories of which should be used.
 * \return A ClientRegistrarPtr object pointing to the ClientRegistrar.
 */
ClientRegistrarPtr ClientRegistrar::create(const AccountManagerPtr &manager)
{
    if (!manager) {
        return ClientRegistrarPtr();
    }

    return create(manager->dbusConnection(), manager->accountFactory(),
            manager->connectionFactory(), manager->channelFactory(), manager->contactFactory());
}

/**
 * Construct a new client registrar object using the given \a bus.
 *
 * The resulting registrar will use factories constructing stock TpQt4 classes with no features
 * prepared. This is equivalent to the old pre-factory behavior of ClientRegistrar.
 *
 * \param bus QDBusConnection to use.
 */
ClientRegistrar::ClientRegistrar(const QDBusConnection &bus)
    : mPriv(new Private(bus, AccountFactory::create(bus, Features()),
                ConnectionFactory::create(bus),   ChannelFactory::create(bus),
                ContactFactory::create()))
{
    registrarForConnection.insert(qMakePair(bus.name(),
                bus.baseService()), this);
}

/**
 * Construct a new client registrar object using the given \a bus and the given factories.
 *
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 */
ClientRegistrar::ClientRegistrar(const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
    : mPriv(new Private(bus, accountFactory, connectionFactory, channelFactory, contactFactory))
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
 * Get the account factory used by this client registrar.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the registrar would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
AccountFactoryConstPtr ClientRegistrar::accountFactory() const
{
    return mPriv->accFactory;
}

/**
 * Get the connection factory used by this client registrar.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the registrar would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ConnectionFactoryConstPtr ClientRegistrar::connectionFactory() const
{
    return mPriv->connFactory;
}

/**
 * Get the channel factory used by this client registrar.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the registrar would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ChannelFactoryConstPtr ClientRegistrar::channelFactory() const
{
    return mPriv->chanFactory;
}

/**
 * Get the contact factory used by this client registrar.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the registrar would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ContactFactoryConstPtr ClientRegistrar::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return a list of clients registered using registerClient() on this client
 * registrar.
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
 * The client registrar will export the appropriate D-Bus interfaces,
 * based on the abstract classes subclassed by \param client.
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
        busName.append(QString(QLatin1String(".%1.x%2"))
                .arg(mPriv->bus.baseService()
                    .replace(QLatin1String(":"), QLatin1String("_"))
                    .replace(QLatin1String("."), QLatin1String("._")))
                .arg((intptr_t) client.data(), 0, 16));
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
        new ClientHandlerAdaptor(this, handler, object);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Handler"));
        if (handler->wantsRequestNotification()) {
            // export o.f.T.Client.Interface.Requests
            new ClientHandlerRequestsAdaptor(this, handler, object);
            interfaces.append(
                    QLatin1String(
                        "org.freedesktop.Telepathy.Client.Interface.Requests"));
        }
    }

    AbstractClientObserver *observer =
        dynamic_cast<AbstractClientObserver*>(client.data());
    if (observer) {
        // export o.f.T.Client.Observer
        new ClientObserverAdaptor(this, observer, object);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Observer"));
    }

    AbstractClientApprover *approver =
        dynamic_cast<AbstractClientApprover*>(client.data());
    if (approver) {
        // export o.f.T.Client.Approver
        new ClientApproverAdaptor(this, approver, object);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Approver"));
    }

    if (interfaces.isEmpty()) {
        warning() << "Client does not implement any known interface";
        // cleanup
        mPriv->bus.unregisterService(busName);
        return false;
    }

    // export o.f.T.Client interface
    new ClientAdaptor(this, interfaces, object);

    QString objectPath = QString(QLatin1String("/%1")).arg(busName);
    objectPath.replace(QLatin1String("."), QLatin1String("/"));
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
 * Unregister a client registered using registerClient() on this client
 * registrar.
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

    QString busName = objectPath.mid(1).replace(QLatin1String("/"),
            QLatin1String("."));
    mPriv->bus.unregisterService(busName);
    mPriv->services.remove(busName);

    debug() << "Client unregistered - busName:" << busName <<
        "objectPath:" << objectPath;

    return true;
}

/**
 * Unregister all clients registered using registerClient() on this client
 * registrar.
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
