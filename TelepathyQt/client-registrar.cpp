/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#include <TelepathyQt/ClientRegistrar>
#include "TelepathyQt/client-registrar-internal.h"

#include "TelepathyQt/_gen/client-registrar.moc.hpp"
#include "TelepathyQt/_gen/client-registrar-internal.moc.hpp"

#include "TelepathyQt/channel-factory.h"
#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/request-temporary-handler-internal.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/Connection>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>

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

    PendingReady *accReady = accFactory->proxy(TP_QT_ACCOUNT_MANAGER_BUS_NAME,
            accountPath.path(),
            connFactory,
            chanFactory,
            contactFactory);
    invocation->acc = AccountPtr::qObjectCast(accReady->proxy());
    readyOps.append(accReady);

    QString connectionBusName = connectionPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = connFactory->proxy(connectionBusName, connectionPath.path(),
            chanFactory, contactFactory);
    invocation->conn = ConnectionPtr::qObjectCast(connReady->proxy());
    readyOps.append(connReady);

    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        PendingReady *chanReady = chanFactory->proxy(invocation->conn,
                channelDetails.channel.path(), channelDetails.properties);
        ChannelPtr channel = ChannelPtr::qObjectCast(chanReady->proxy());
        invocation->chans.append(channel);
        readyOps.append(chanReady);
    }

    // Yes, we don't give the choice of making CDO and CR ready or not - however, readifying them is
    // 0-1 D-Bus calls each, for CR mostly 0 - and their constructors start making them ready
    // automatically, so we wouldn't save any D-Bus traffic anyway

    if (!dispatchOperationPath.path().isEmpty() && dispatchOperationPath.path() != QLatin1String("/")) {
        QVariantMap props;

        // TODO: push to tp spec having all of the CDO immutable props be contained in observerInfo
        // so we don't have to introspect the CDO either - then we can pretty much do:
        //
        // props = qdbus_cast<QVariantMap>(
        //         observerInfo.value(QLatin1String("dispatch-operation-properties")));
        //
        // Currently something like the following can be used for testing the CDO "we've got
        // everything we need" codepath:
        //
        // props.insert(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Account"),
        //        QVariant::fromValue(QDBusObjectPath(accountPath.path())));
        // props.insert(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Connection"),
        //      QVariant::fromValue(QDBusObjectPath(connectionPath.path())));
        // props.insert(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Interfaces"),
        //         QVariant::fromValue(QStringList()));
        // props.insert(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".PossibleHandlers"),
        //         QVariant::fromValue(QStringList()));

        invocation->dispatchOp = ChannelDispatchOperation::create(mBus, dispatchOperationPath.path(),
                props,
                invocation->chans,
                accFactory,
                connFactory,
                chanFactory,
                contactFactory);
        readyOps.append(invocation->dispatchOp->becomeReady());
    }

    invocation->observerInfo = AbstractClientObserver::ObserverInfo(observerInfo);

    ObjectImmutablePropertiesMap reqPropsMap = qdbus_cast<ObjectImmutablePropertiesMap>(
            observerInfo.value(QLatin1String("request-properties")));
    foreach (const QDBusObjectPath &reqPath, requestsSatisfied) {
        //don't load the channelRequest objects in requestsSatisfied if the properties are not supplied with the handler info
        //as the channelRequest is probably invalid
        //this works around https://bugs.freedesktop.org/show_bug.cgi?id=77986
        if (reqPropsMap.value(reqPath).isEmpty()) {
            continue;
        }
        ChannelRequestPtr channelRequest = ChannelRequest::create(invocation->acc,
                reqPath.path(), reqPropsMap.value(reqPath));
        invocation->chanReqs.append(channelRequest);
        readyOps.append(channelRequest->becomeReady());
    }

    invocation->ctx = MethodInvocationContextPtr<>(new MethodInvocationContext<>(mBus, message));

    invocation->readyOp = new PendingComposite(readyOps, invocation->ctx);
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    mInvocations.append(invocation);

    debug() << "Preparing proxies for ObserveChannels of" << channelDetailsList.size() << "channels"
        << "for client" << mClient;
}

void ClientObserverAdaptor::onReadyOpFinished(Tp::PendingOperation *op)
{
    Q_ASSERT(!mInvocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = mInvocations.begin();
            i != mInvocations.end(); ++i) {
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

    while (!mInvocations.isEmpty() && !mInvocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = mInvocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Invoking application observeChannels with" << invocation->chans.size()
            << "channels on" << mClient;

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
    AccountFactoryConstPtr accFactory = mRegistrar->accountFactory();
    ConnectionFactoryConstPtr connFactory = mRegistrar->connectionFactory();
    ChannelFactoryConstPtr chanFactory = mRegistrar->channelFactory();
    ContactFactoryConstPtr contactFactory = mRegistrar->contactFactory();

    QList<PendingOperation *> readyOps;

    QDBusObjectPath connectionPath = qdbus_cast<QDBusObjectPath>(
            properties.value(
                TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Connection")));
    debug() << "addDispatchOperation: connection:" << connectionPath.path();
    QString connectionBusName = connectionPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = connFactory->proxy(connectionBusName, connectionPath.path(), chanFactory,
                contactFactory);
    ConnectionPtr connection = ConnectionPtr::qObjectCast(connReady->proxy());
    readyOps.append(connReady);

    SharedPtr<InvocationData> invocation(new InvocationData);

    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        PendingReady *chanReady = chanFactory->proxy(connection, channelDetails.channel.path(),
                channelDetails.properties);
        invocation->chans.append(ChannelPtr::qObjectCast(chanReady->proxy()));
        readyOps.append(chanReady);
    }

    invocation->dispatchOp = ChannelDispatchOperation::create(mBus,
            dispatchOperationPath.path(), properties, invocation->chans, accFactory, connFactory,
            chanFactory, contactFactory);
    readyOps.append(invocation->dispatchOp->becomeReady());

    invocation->ctx = MethodInvocationContextPtr<>(new MethodInvocationContext<>(mBus, message));

    invocation->readyOp = new PendingComposite(readyOps, invocation->ctx);
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    mInvocations.append(invocation);
}

void ClientApproverAdaptor::onReadyOpFinished(Tp::PendingOperation *op)
{
    Q_ASSERT(!mInvocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = mInvocations.begin();
            i != mInvocations.end(); ++i) {
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

    while (!mInvocations.isEmpty() && !mInvocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = mInvocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Invoking application addDispatchOperation with CDO"
            << invocation->dispatchOp->objectPath() << "on" << mClient;

        mClient->addDispatchOperation(invocation->ctx, invocation->dispatchOp);
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

    RequestTemporaryHandler *tempHandler = dynamic_cast<RequestTemporaryHandler *>(mClient);
    if (tempHandler) {
        debug() << "  This is a temporary handler for the Request & Handle API,"
            << "giving an early signal of the invocation";
        tempHandler->setDBusHandlerInvoked();
    }

    PendingReady *accReady = accFactory->proxy(TP_QT_ACCOUNT_MANAGER_BUS_NAME,
            accountPath.path(),
            connFactory,
            chanFactory,
            contactFactory);
    invocation->acc = AccountPtr::qObjectCast(accReady->proxy());
    readyOps.append(accReady);

    QString connectionBusName = connectionPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = connFactory->proxy(connectionBusName, connectionPath.path(),
            chanFactory, contactFactory);
    invocation->conn = ConnectionPtr::qObjectCast(connReady->proxy());
    readyOps.append(connReady);

    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        PendingReady *chanReady = chanFactory->proxy(invocation->conn,
                channelDetails.channel.path(), channelDetails.properties);
        ChannelPtr channel = ChannelPtr::qObjectCast(chanReady->proxy());
        invocation->chans.append(channel);
        readyOps.append(chanReady);
    }

    invocation->handlerInfo = AbstractClientHandler::HandlerInfo(handlerInfo);

    ObjectImmutablePropertiesMap reqPropsMap = qdbus_cast<ObjectImmutablePropertiesMap>(
    handlerInfo.value(QLatin1String("request-properties")));
    foreach (const QDBusObjectPath &reqPath, requestsSatisfied) {
        //don't load the channelRequest objects in requestsSatisfied if the properties are not supplied with the handler info
        //as the channelRequest is probably invalid
        //this works around https://bugs.freedesktop.org/show_bug.cgi?id=77986
        if (reqPropsMap.value(reqPath).isEmpty()) {
            continue;
        }
        ChannelRequestPtr channelRequest = ChannelRequest::create(invocation->acc,
                reqPath.path(), reqPropsMap.value(reqPath));
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

    invocation->readyOp = new PendingComposite(readyOps, invocation->ctx);
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    mInvocations.append(invocation);

    debug() << "Preparing proxies for HandleChannels of" << channelDetailsList.size() << "channels"
        << "for client" << mClient;
}

void ClientHandlerAdaptor::onReadyOpFinished(Tp::PendingOperation *op)
{
    Q_ASSERT(!mInvocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = mInvocations.begin();
            i != mInvocations.end(); ++i) {
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

    while (!mInvocations.isEmpty() && !mInvocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = mInvocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            RequestTemporaryHandler *tempHandler = dynamic_cast<RequestTemporaryHandler *>(mClient);
            if (tempHandler) {
                debug() << "  This is a temporary handler for the Request & Handle API, indicating failure";
                tempHandler->setDBusHandlerErrored(invocation->error, invocation->message);
            }

            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Invoking application handleChannels with" << invocation->chans.size()
            << "channels on" << mClient;

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

        // register the channels in FakeHandlerManager so we report HandledChannels correctly
        FakeHandlerManager::instance()->registerChannels(channels);
    }
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
                request.path(), requestProperties,
                mRegistrar->accountFactory(),
                mRegistrar->connectionFactory(),
                mRegistrar->channelFactory(),
                mRegistrar->contactFactory()));
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
                request.path(), QVariantMap(),
                mRegistrar->accountFactory(),
                mRegistrar->connectionFactory(),
                mRegistrar->channelFactory(),
                mRegistrar->contactFactory()), errorName, errorMessage);
}

struct TP_QT_NO_EXPORT ClientRegistrar::Private
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
 * \headerfile TelepathyQt/client-registrar.h <TelepathyQt/ClientRegistrar>
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
 * To register a client, just call registerClient() with a given AbstractClientPtr
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
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Create a new client registrar object using the given \a bus.
 *
 * The instance will use an account factory creating Tp::Account objects with no features
 * ready, a connection factory creating Tp::Connection objects with no features ready, and a channel
 * factory creating stock Telepathy-Qt channel subclasses, as appropriate, with no features ready.
 *
 * \param bus QDBusConnection to use.
 * \return A ClientRegistrarPtr object pointing to the newly created ClientRegistrar object.
 */
ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus)
{
    return create(bus, AccountFactory::create(bus), ConnectionFactory::create(bus),
            ChannelFactory::create(bus), ContactFactory::create());
}

/**
 * Create a new client registrar object using QDBusConnection::sessionBus() and the given factories.
 *
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ClientRegistrarPtr object pointing to the newly created ClientRegistrar object.
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
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ClientRegistrarPtr object pointing to the newly created ClientRegistrar object.
 */
ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory)
{
    return ClientRegistrarPtr(new ClientRegistrar(bus, accountFactory, connectionFactory,
                channelFactory, contactFactory));
}

/**
 * Create a new client registrar object using the bus and factories of the given Account \a manager.
 *
 * Using this create method will enable (like any other way of passing the same factories to an AM
 * and a registrar) getting the same Account/Connection etc. proxy instances from both
 * AccountManager and AbstractClient implementations.
 *
 * \param manager The AccountManager the bus and factories of which should be used.
 * \return A ClientRegistrarPtr object pointing to the newly ClientRegistrar object.
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
    : Object(),
      mPriv(new Private(bus, accountFactory, connectionFactory, channelFactory, contactFactory))
{
}

/**
 * Class destructor.
 */
ClientRegistrar::~ClientRegistrar()
{
    unregisterClients();
    delete mPriv;
}

/**
 * Return the D-Bus connection being used by this client registrar.
 *
 * \return A QDBusConnection object.
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
 * \return A read-only pointer to the AccountFactory object.
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
 * \return A read-only pointer to the ConnectionFactory object.
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
 * \return A read-only pointer to the ChannelFactory object.
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
 * \return A read-only pointer to the ContactFactory object.
 */
ContactFactoryConstPtr ClientRegistrar::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return the list of clients registered using registerClient() on this client
 * registrar.
 *
 * \return A list of pointers to AbstractClient objects.
 * \sa registerClient(), unregisterClient()
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
 * \return \c true if \a client was successfully registered, \c false otherwise.
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
        // o.f.T.Client.clientName.<unique_bus_name>_<pointer> should be enough to identify
        // an unique identifier
        busName.append(QString(QLatin1String(".%1_%2"))
                .arg(mPriv->bus.baseService()
                    .replace(QLatin1String(":"), QLatin1String("_"))
                    .replace(QLatin1String("."), QLatin1String("_")))
                .arg((quintptr) client.data(), 0, 16));
    }

    if (mPriv->services.contains(busName)) {
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
        return false;
    }

    if (!mPriv->bus.registerService(busName)) {
        warning() << "Unable to register service: busName" <<
            busName << "already registered";
        mPriv->bus.unregisterObject(objectPath, QDBusConnection::UnregisterTree);
        delete object;
        return false;
    }


    if (handler) {
        handler->setRegistered(true);
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
 * \return \c true if \a client was successfully unregistered, \c false otherwise.
 * \sa registeredClients(), registerClient()
 */
bool ClientRegistrar::unregisterClient(const AbstractClientPtr &client)
{
    if (!mPriv->clients.contains(client)) {
        warning() << "Trying to unregister an unregistered client";
        return false;
    }

    AbstractClientHandler *handler =
        dynamic_cast<AbstractClientHandler*>(client.data());
    if (handler) {
        handler->setRegistered(false);
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
