/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/ChannelDispatchOperation>
#include "TelepathyQt/channel-dispatch-operation-internal.h"

#include "TelepathyQt/_gen/cli-channel-dispatch-operation-body.hpp"
#include "TelepathyQt/_gen/cli-channel-dispatch-operation.moc.hpp"
#include "TelepathyQt/_gen/channel-dispatch-operation.moc.hpp"
#include "TelepathyQt/_gen/channel-dispatch-operation-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/fake-handler-manager-internal.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>

namespace Tp
{

struct TP_QT_NO_EXPORT ChannelDispatchOperation::Private
{
    Private(ChannelDispatchOperation *parent);
    ~Private();

    static void introspectMain(Private *self);

    void extractMainProps(const QVariantMap &props,
            bool immutableProperties);

    // Public object
    ChannelDispatchOperation *parent;

    // Context
    AccountFactoryConstPtr accFactory;
    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Instance of generated interface class
    Client::ChannelDispatchOperationInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    QVariantMap immutableProperties;
    ConnectionPtr connection;
    AccountPtr account;
    QList<ChannelPtr> channels;
    QStringList possibleHandlers;
    bool gotPossibleHandlers;
};

ChannelDispatchOperation::Private::Private(ChannelDispatchOperation *parent)
    : parent(parent),
      baseInterface(new Client::ChannelDispatchOperationInterface(parent)),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      gotPossibleHandlers(false)
{
    debug() << "Creating new ChannelDispatchOperation:" << parent->objectPath();

    parent->connect(baseInterface,
            SIGNAL(Finished()),
            SLOT(onFinished()));

    parent->connect(baseInterface,
            SIGNAL(ChannelLost(QDBusObjectPath,QString,QString)),
            SLOT(onChannelLost(QDBusObjectPath,QString,QString)));

    ReadinessHelper::Introspectables introspectables;

    // As ChannelDispatchOperation does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
}

ChannelDispatchOperation::Private::~Private()
{
}

void ChannelDispatchOperation::Private::introspectMain(ChannelDispatchOperation::Private *self)
{
    QVariantMap mainProps;
    foreach (QString key, self->immutableProperties.keys()) {
        if (key.startsWith(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String("."))) {
            QVariant value = self->immutableProperties.value(key);
            mainProps.insert(
                    key.remove(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".")),
                    value);
        }
    }

    if (!self->channels.isEmpty() && mainProps.contains(QLatin1String("Account"))
            && mainProps.contains(QLatin1String("Connection"))
            && mainProps.contains(QLatin1String("Interfaces"))
            && mainProps.contains(QLatin1String("PossibleHandlers"))) {
        debug() << "Supplied properties were sufficient, not introspecting"
            << self->parent->objectPath();
        self->extractMainProps(mainProps, true);
        return;
    }

    debug() << "Calling Properties::GetAll(ChannelDispatchOperation)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void ChannelDispatchOperation::Private::extractMainProps(const QVariantMap &props,
        bool immutableProperties)
{
    parent->setInterfaces(qdbus_cast<QStringList>(props.value(QLatin1String("Interfaces"))));

    QList<PendingOperation *> readyOps;

    if (!connection && props.contains(QLatin1String("Connection"))) {
        QDBusObjectPath connectionObjectPath =
            qdbus_cast<QDBusObjectPath>(props.value(QLatin1String("Connection")));
        QString connectionBusName =
            connectionObjectPath.path().mid(1).replace(QLatin1String("/"),
                    QLatin1String("."));

        PendingReady *readyOp =
            connFactory->proxy(connectionBusName, connectionObjectPath.path(),
                    chanFactory, contactFactory);
        connection = ConnectionPtr::qObjectCast(readyOp->proxy());
        readyOps.append(readyOp);
    }

    if (!account && props.contains(QLatin1String("Account"))) {
        QDBusObjectPath accountObjectPath =
            qdbus_cast<QDBusObjectPath>(props.value(QLatin1String("Account")));

        PendingReady *readyOp =
            accFactory->proxy(TP_QT_ACCOUNT_MANAGER_BUS_NAME,
                    accountObjectPath.path(), connFactory, chanFactory, contactFactory);
        account = AccountPtr::qObjectCast(readyOp->proxy());
        readyOps.append(readyOp);
    }

    if (!immutableProperties) {
        // If we're here, it means we had to introspect the object, and now for sure have the
        // correct channels list, so let's overwrite the initial channels - but keep the refs around
        // for a while as an optimization enabling the factory to still return the same ones instead
        // of constructing everything anew. Note that this is not done at all in the case the
        // immutable props and initial channels etc were sufficient.
        QList<ChannelPtr> saveChannels = channels;
        channels.clear();

        ChannelDetailsList channelDetailsList =
            qdbus_cast<ChannelDetailsList>(props.value(QLatin1String("Channels")));
        ChannelPtr channel;
        foreach (const ChannelDetails &channelDetails, channelDetailsList) {
            PendingReady *readyOp =
                chanFactory->proxy(connection,
                        channelDetails.channel.path(), channelDetails.properties);
            channels.append(ChannelPtr::qObjectCast(readyOp->proxy()));
            readyOps.append(readyOp);
        }

        // saveChannels goes out of scope now, so any initial channels which don't exist anymore are
        // freed
    }

    if (props.contains(QLatin1String("PossibleHandlers"))) {
        possibleHandlers = qdbus_cast<QStringList>(props.value(QLatin1String("PossibleHandlers")));
        gotPossibleHandlers = true;
    }

    if (readyOps.isEmpty()) {
        debug() << "No proxies to prepare for CDO" << parent->objectPath();
        readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        parent->connect(new PendingComposite(readyOps, ChannelDispatchOperationPtr(parent)),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onProxiesPrepared(Tp::PendingOperation*)));
    }
}

ChannelDispatchOperation::PendingClaim::PendingClaim(const ChannelDispatchOperationPtr &op,
        const AbstractClientHandlerPtr &handler)
    : PendingOperation(op),
      mDispatchOp(op),
      mHandler(handler)
{
    debug() << "Invoking CDO.Claim";
    connect(new PendingVoid(op->baseInterface()->Claim(), op),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onClaimFinished(Tp::PendingOperation*)));
}

ChannelDispatchOperation::PendingClaim::~PendingClaim()
{
}

void ChannelDispatchOperation::PendingClaim::onClaimFinished(
        PendingOperation *op)
{
    if (!op->isError()) {
        debug() << "CDO.Claim returned successfully, updating HandledChannels";
        if (mHandler) {
            // register the channels in HandledChannels
            FakeHandlerManager::instance()->registerChannels(
                    mDispatchOp->channels());
        }
        setFinished();
    } else {
        warning() << "CDO.Claim failed with" << op->errorName() << "-" << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
    }
}

/**
 * \class ChannelDispatchOperation
 * \ingroup clientchanneldispatchoperation
 * \headerfile TelepathyQt/channel-dispatch-operation.h <TelepathyQt/ChannelDispatchOperation>
 *
 * \brief The ChannelDispatchOperation class represents a Telepathy channel
 * dispatch operation.
 *
 * One of the channel dispatcher's functions is to offer incoming channels to
 * Approver clients for approval. An approver should generally ask the user
 * whether they want to participate in the requested communication channels
 * (join the chat or chatroom, answer the call, accept the file transfer, or
 * whatever is appropriate). A collection of channels offered in this way
 * is represented by a ChannelDispatchOperation object.
 *
 * If the user wishes to accept the communication channels, the approver
 * should call handleWith() to indicate the user's or approver's preferred
 * handler for the channels (the empty string indicates no particular
 * preference, and will cause any suitable handler to be used).
 *
 * If the user wishes to reject the communication channels, or if the user
 * accepts the channels and the approver will handle them itself, the approver
 * should call claim(). If the resulting PendingOperation succeeds, the approver
 * immediately has control over the channels as their primary handler,
 * and may do anything with them (in particular, it may close them in whatever
 * way seems most appropriate).
 *
 * There are various situations in which the channel dispatch operation will
 * be closed, causing the DBusProxy::invalidated() signal to be emitted. If this
 * happens, the approver should stop prompting the user.
 *
 * Because all approvers are launched simultaneously, the user might respond
 * to another approver; if this happens, the invalidated signal will be
 * emitted with the error code #TP_QT_ERROR_OBJECT_REMOVED.
 *
 * If a channel closes, the signal channelLost() is emitted. If all channels
 * close, there is nothing more to dispatch, so the invalidated signal will be
 * emitted with the error code #TP_QT_ERROR_OBJECT_REMOVED.
 *
 * If the channel dispatcher crashes or exits, the invalidated
 * signal will be emitted with the error code
 * #TP_QT_DBUS_ERROR_NAME_HAS_NO_OWNER. In a high-quality implementation,
 * the dispatcher should be restarted, at which point it will create new
 * channel dispatch operations for any undispatched channels, and the approver
 * will be notified again.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * ChannelDispatchOperation object usable.
 *
 * Note that this feature must be enabled in order to use most
 * ChannelDispatchOperation methods.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature ChannelDispatchOperation::FeatureCore = Feature(QLatin1String(ChannelDispatchOperation::staticMetaObject.className()), 0, true);

/**
 * Create a new channel dispatch operation object using the given \a bus, the given factories and
 * the given initial channels.
 *
 * \param bus QDBusConnection to use.
 * \param objectPath The channel dispatch operation object path.
 * \param immutableProperties The channel dispatch operation immutable properties.
 * \param initialChannels The channels this CDO has initially (further tracking is done internally).
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ChannelDispatchOperationPtr object pointing to the newly created
 *         ChannelDispatchOperation object.
 */
ChannelDispatchOperationPtr ChannelDispatchOperation::create(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties,
        const QList<ChannelPtr> &initialChannels,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return ChannelDispatchOperationPtr(new ChannelDispatchOperation(
                bus, objectPath, immutableProperties, initialChannels, accountFactory,
                connectionFactory, channelFactory, contactFactory));
}

/**
 * Construct a new channel dispatch operation object using the given \a bus, the given factories and
 * the given initial channels.
 *
 * \param bus QDBusConnection to use
 * \param objectPath The channel dispatch operation object path.
 * \param immutableProperties The channel dispatch operation immutable properties.
 * \param initialChannels The channels this CDO has initially (further tracking is done internally).
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 */
ChannelDispatchOperation::ChannelDispatchOperation(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties,
        const QList<ChannelPtr> &initialChannels,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
    : StatefulDBusProxy(bus,
            TP_QT_IFACE_CHANNEL_DISPATCHER,
            objectPath, FeatureCore),
      OptionalInterfaceFactory<ChannelDispatchOperation>(this),
      mPriv(new Private(this))
{
    if (accountFactory->dbusConnection().name() != bus.name()) {
        warning() << "  The D-Bus connection in the account factory is not the proxy connection";
    }

    if (connectionFactory->dbusConnection().name() != bus.name()) {
        warning() << "  The D-Bus connection in the connection factory is not the proxy connection";
    }

    if (channelFactory->dbusConnection().name() != bus.name()) {
        warning() << "  The D-Bus connection in the channel factory is not the proxy connection";
    }

    mPriv->channels = initialChannels;

    mPriv->accFactory = accountFactory;
    mPriv->connFactory = connectionFactory;
    mPriv->chanFactory = channelFactory;
    mPriv->contactFactory = contactFactory;

    mPriv->immutableProperties = immutableProperties;
}

/**
 * Class destructor.
 */
ChannelDispatchOperation::~ChannelDispatchOperation()
{
    delete mPriv;
}

/**
 * Return the connection with which the channels for this dispatch
 * operation are associated.
 *
 * This method requires ChannelDispatchOperation::FeatureCore to be ready.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr ChannelDispatchOperation::connection() const
{
    return mPriv->connection;
}

/**
 * Return the account with which the connection and channels for this dispatch
 * operation are associated.
 *
 * This method requires ChannelDispatchOperation::FeatureCore to be ready.
 *
 * \return A pointer to the Account object.
 */
AccountPtr ChannelDispatchOperation::account() const
{
    return mPriv->account;
}

/**
 * Return the channels to be dispatched.
 *
 * This method requires ChannelDispatchOperation::FeatureCore to be ready.
 *
 * \return A list of pointers to Channel objects.
 */
QList<ChannelPtr> ChannelDispatchOperation::channels() const
{
    if (!isReady()) {
        warning() << "ChannelDispatchOperation::channels called with channel "
            "not ready";
    }
    return mPriv->channels;
}

/**
 * Return the well known bus names (starting with
 * org.freedesktop.Telepathy.Client.) of the possible Handlers for this
 * dispatch operation channels with the preferred handlers first.
 *
 * As a result, approvers should use the first handler by default, unless they
 * have a reason to do otherwise.
 *
 * This method requires ChannelDispatchOperation::FeatureCore to be ready.
 *
 * \return List of possible handlers names.
 */
QStringList ChannelDispatchOperation::possibleHandlers() const
{
    return mPriv->possibleHandlers;
}

/**
 * Called by an approver to accept a channel bundle and request that the given
 * handler be used to handle it.
 *
 * If successful, this method will cause the ChannelDispatchOperation object to
 * disappear, emitting invalidated with error
 * #TP_QT_ERROR_OBJECT_REMOVED.
 *
 * However, this method may fail because the dispatch has already been completed
 * and the object has already gone. If this occurs, it indicates that another
 * approver has asked for the bundle to be handled by a particular handler. The
 * approver must not attempt to interact with the channels further in this case,
 * unless it is separately invoked as the handler.
 *
 * Approvers which are also channel handlers should use claim() instead of
 * this method to request that they can handle a channel bundle themselves.
 *
 * \param handler The well-known bus name (starting with
 *                org.freedesktop.Telepathy.Client.) of the channel handler that
 *                should handle the channel, or an empty string if
 *                the client has no preferred channel handler.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *ChannelDispatchOperation::handleWith(const QString &handler)
{
    return new PendingVoid(
            mPriv->baseInterface->HandleWith(handler),
            ChannelDispatchOperationPtr(this));
}

/**
 * Called by an approver to claim channels for closing them.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *ChannelDispatchOperation::claim()
{
    return new PendingClaim(ChannelDispatchOperationPtr(this));
}

/**
 * Called by an approver to claim channels for handling internally. If this
 * method is called successfully, the \a handler becomes the
 * handler for the channel, but does not have the
 * AbstractClientHandler::handleChannels() method called on it.
 *
 * Approvers wishing to reject channels must call this method to claim ownership
 * of them, and must not call requestClose() on the channels unless/until this
 * method returns successfully.
 *
 * The channel dispatcher can't know how best to close arbitrary channel types,
 * so it leaves it up to the approver to do so. For instance, for text channels
 * it is necessary to acknowledge any messages that have already been displayed
 * to the user first - ideally, the approver would display and then acknowledge
 * the messages - or to call Channel::requestClose() if the destructive
 * behaviour of that method is desired.
 *
 * Similarly, an approver for streamed media channels can close the channel with
 * a reason (e.g. "busy") if desired. The channel dispatcher, which is designed
 * to have no specific knowledge of particular channel types, can't do that.
 *
 * If successful, this method will cause the ChannelDispatchOperation object to
 * disappear, emitting Finished, in the same way as for handleWith().
 *
 * This method may fail because the dispatch operation has already been
 * completed. Again, see handleWith() for more details. The approver must not
 * attempt to interact with the channels further in this case.
 *
 * \param handler The channel handler, that should remain registered during the
 *                lifetime of channels(), otherwise dispatching will fail if the
 *                channel dispatcher restarts.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa claim(), handleWith()
 */
PendingOperation *ChannelDispatchOperation::claim(const AbstractClientHandlerPtr &handler)
{
    if (!handler->isRegistered()) {
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Handler must be registered for using claim(handler)"),
                ChannelDispatchOperationPtr(this));
    }

    return new PendingClaim(ChannelDispatchOperationPtr(this),
           handler);
}

/**
 * \fn void ChannelDispatchOperation::channelLost(const ChannelPtr &channel,
 *             const QString &errorName, const QString &errorMessage);
 *
 * Emitted when a channel has closed before it could be claimed or handled. If this is
 * emitted for the last remaining channel in a channel dispatch operation, it
 * will immediately be followed by invalidated() with error
 * #TP_QT_ERROR_OBJECT_REMOVED.
 *
 * \param channel The channel that was closed.
 * \param error The name of a D-Bus error indicating why the channel closed.
 * \param errorMessage The error message.
 */

/**
 * Return the ChannelDispatchOperationInterface for this ChannelDispatchOperation
 * class. This method is protected since the convenience methods provided by
 * this class should always be used instead of the interface by users of the
 * class.
 *
 * \return A pointer to the existing Client::ChannelDispatchOperationInterface object for this
 *         ChannelDispatchOperation object.
 */
Client::ChannelDispatchOperationInterface *ChannelDispatchOperation::baseInterface() const
{
    return mPriv->baseInterface;
}

void ChannelDispatchOperation::onFinished()
{
    debug() << "ChannelDispatchOperation finished and was removed";
    invalidate(TP_QT_ERROR_OBJECT_REMOVED,
               QLatin1String("ChannelDispatchOperation finished and was removed"));
}

void ChannelDispatchOperation::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    // Watcher is NULL if we didn't have to introspect at all
    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(ChannelDispatchOperation)";
        mPriv->extractMainProps(reply.value(), false);
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        warning().nospace() << "Properties::GetAll(ChannelDispatchOperation) failed with "
            << reply.error().name() << ": " << reply.error().message();
    }
}

void ChannelDispatchOperation::onChannelLost(
        const QDBusObjectPath &channelObjectPath,
        const QString &errorName, const QString &errorMessage)
{
    foreach (const ChannelPtr &channel, mPriv->channels) {
        if (channel->objectPath() == channelObjectPath.path()) {
            emit channelLost(channel, errorName, errorMessage);
            mPriv->channels.removeOne(channel);
            return;
        }
    }
}

void ChannelDispatchOperation::onProxiesPrepared(Tp::PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Preparing proxies for CDO" << objectPath() << "failed with"
            << op->errorName() << ":" << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false);
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

} // Tp
