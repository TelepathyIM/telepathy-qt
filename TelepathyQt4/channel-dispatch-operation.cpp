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

#include <TelepathyQt4/ChannelDispatchOperation>

#include "TelepathyQt4/_gen/cli-channel-dispatch-operation-body.hpp"
#include "TelepathyQt4/_gen/cli-channel-dispatch-operation.moc.hpp"
#include "TelepathyQt4/_gen/channel-dispatch-operation.moc.hpp"

#include "TelepathyQt4/channel-factory.h"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingVoid>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ChannelDispatchOperation::Private
{
    Private(ChannelDispatchOperation *parent);
    ~Private();

    static void introspectMain(Private *self);

    void extractMainProps(const QVariantMap &props,
            bool immutableProperties);
    void checkReady();

    // Public object
    ChannelDispatchOperation *parent;

    // Instance of generated interface class
    Client::ChannelDispatchOperationInterface *baseInterface;

    // Optional interface proxies
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    ConnectionPtr connection;
    AccountPtr account;
    QList<ChannelPtr> channels;
    QStringList possibleHandlers;
};

ChannelDispatchOperation::Private::Private(ChannelDispatchOperation *parent)
    : parent(parent),
      baseInterface(new Client::ChannelDispatchOperationInterface(
                    parent->dbusConnection(), parent->busName(),
                    parent->objectPath(), parent)),
      properties(0),
      readinessHelper(parent->readinessHelper())
{
    debug() << "Creating new ChannelDispatchOperation:" << parent->objectPath();

    parent->connect(baseInterface,
            SIGNAL(Finished()),
            SLOT(onFinished()));

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
    if (!self->properties) {
        self->properties = self->parent->propertiesInterface();
        Q_ASSERT(self->properties != 0);
    }

    self->parent->connect(self->baseInterface,
            SIGNAL(ChannelLost(const QDBusObjectPath &, const QString &, const QString &)),
            SLOT(onChannelLost(const QDBusObjectPath &, const QString &, const QString &)));

    debug() << "Calling Properties::GetAll(ChannelDispatchOperation)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_DISPATCH_OPERATION)),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void ChannelDispatchOperation::Private::extractMainProps(const QVariantMap &props,
        bool immutableProperties)
{
    parent->setInterfaces(qdbus_cast<QStringList>(props.value(QLatin1String("Interfaces"))));

    if (!connection && props.contains(QLatin1String("Connection"))) {
        QDBusObjectPath connectionObjectPath =
            qdbus_cast<QDBusObjectPath>(props.value(QLatin1String("Connection")));
        QString connectionBusName =
            connectionObjectPath.path().mid(1).replace(QLatin1String("/"),
                    QLatin1String("."));
        connection = Connection::create(
                connectionBusName,
                connectionObjectPath.path());
    }

    if (!account && props.contains(QLatin1String("Account"))) {
        QDBusObjectPath accountObjectPath =
            qdbus_cast<QDBusObjectPath>(props.value(QLatin1String("Account")));
        account = Account::create(
                QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
                accountObjectPath.path());
    }

    if (!immutableProperties) {
        ChannelDetailsList channelDetailsList =
            qdbus_cast<ChannelDetailsList>(props.value(QLatin1String("Channels")));
        ChannelPtr channel;
        foreach (const ChannelDetails &channelDetails, channelDetailsList) {
            channel = ChannelFactory::create(connection,
                    channelDetails.channel.path(),
                    channelDetails.properties);
            channels.append(channel);
        }
    }

    possibleHandlers = qdbus_cast<QStringList>(props.value(QLatin1String("PossibleHandlers")));
}

void ChannelDispatchOperation::Private::checkReady()
{
    if ((connection && !connection->isReady()) ||
        (account && !account->isReady())) {
        return;
    }

    foreach (const ChannelPtr &channel, channels) {
        if (!channel->isReady()) {
            return;
        }
    }

    readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

/**
 * \class ChannelDispatchOperation
 * \ingroup clientchanneldispatchoperation
 * \headerfile TelepathyQt4/channel-dispatch-operation.h <TelepathyQt4/ChannelDispatchOperation>
 *
 * \brief The ChannelDispatchOperation class provides an object representing a
 * Telepathy channel dispatch operation.
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
 * emitted with the error code #TELEPATHY_QT4_ERROR_OBJECT_REMOVED.
 *
 * If a channel closes, the signal channelLost() is emitted. If all channels
 * close, there is nothing more to dispatch, so the invalidated signal will be
 * emitted with the error code #TELEPATHY_QT4_ERROR_OBJECT_REMOVED.
 *
 * If the channel dispatcher crashes or exits, the invalidated
 * signal will be emitted with the error code
 * #TELEPATHY_DBUS_ERROR_NAME_HAS_NO_OWNER. In a high-quality implementation,
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
 * Create a new channel dispatch operation object using
 * QDBusConnection::sessionBus().
 *
 * \param objectPath The channel dispatch operation object path.
 * \param immutableProperties The immutable properties of the channel dispatch
 *        operation.
 * \return A ChannelDispatchOperationPtr pointing to the newly created
 *         ChannelDispatchOperation.
 */
ChannelDispatchOperationPtr ChannelDispatchOperation::create(const QString &objectPath,
        const QVariantMap &immutableProperties)
{
    return ChannelDispatchOperationPtr(new ChannelDispatchOperation(
                QDBusConnection::sessionBus(), objectPath,
                immutableProperties));
}

/**
 * Create a new channel dispatch operation object using the given \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param objectPath The channel dispatch operation object path.
 * \param immutableProperties The immutable properties of the channel dispatch
 *        operation.
 * \return A ChannelDispatchOperationPtr pointing to the newly created
 *         ChannelDispatchOperation.
 */
ChannelDispatchOperationPtr ChannelDispatchOperation::create(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ChannelDispatchOperationPtr(new ChannelDispatchOperation(
                bus, objectPath, immutableProperties));
}

/**
 * Construct a new channel dispatch operation object using the given \a bus.
 *
 * \param bus QDBusConnection to use
 * \param objectPath The channel dispatch operation object path.
 * \param immutableProperties The immutable properties of the channel dispatch
 *        operation.
 */
ChannelDispatchOperation::ChannelDispatchOperation(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties)
    : StatefulDBusProxy(bus,
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_DISPATCHER),
            objectPath),
      OptionalInterfaceFactory<ChannelDispatchOperation>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
    mPriv->extractMainProps(immutableProperties, true);
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
 * This method requires ChannelDispatchOperation::FeatureCore to be enabled.
 *
 * \return Connection with which the channels are associated.
 */
ConnectionPtr ChannelDispatchOperation::connection() const
{
    return mPriv->connection;
}

/**
 * Return the account with which the connection and channels for this dispatch
 * operation are associated.
 *
 * This method requires ChannelDispatchOperation::FeatureCore to be enabled.
 *
 * \return Account with which the connection and channels are associated.
 */
AccountPtr ChannelDispatchOperation::account() const
{
    return mPriv->account;
}

/**
 * Return the channels to be dispatched.
 *
 * This method requires ChannelDispatchOperation::FeatureCore to be enabled.
 *
 * \return The channels to be dispatched.
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
 * This method requires ChannelDispatchOperation::FeatureCore to be enabled.
 *
 * \return Possible handlers for this dispatch operation channels.
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
 * #TELEPATHY_QT4_ERROR_OBJECT_REMOVED.
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
            this);
}

/**
 * Called by an approver to claim channels for handling internally. If this
 * method is called successfully, the process calling this method becomes the
 * handler for the channel, but does not have the
 * AbstractClientHandler::handleChannels() method called on it.
 *
 * Clients that call claim() on channels but do not immediately close them
 * should implement the AbstractClientHandler interface.
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
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *ChannelDispatchOperation::claim()
{
    return new PendingVoid(mPriv->baseInterface->Claim(), this);
}

/**
 * \fn void ChannelDispatchOperation::channelLost(const ChannelPtr &channel,
 *             const QString &errorName, const QString &errorMessage);
 *
 * A channel has closed before it could be claimed or handled. If this is
 * emitted for the last remaining channel in a channel dispatch operation, it
 * will immediately be followed by invalidated() with error
 * #TELEPATHY_QT4_ERROR_OBJECT_REMOVED.
 *
 * \param channel The Channel that closed.
 * \param error The name of a D-Bus error indicating why the channel closed.
 * \param errorMessage The error message.
 */

/**
 * Return the ChannelDispatchOperationInterface for this ChannelDispatchOperation
 * class. This method is protected since the convenience methods provided by
 * this class should always be used instead of the interface by users of the
 * class.
 *
 * \return A pointer to the existing ChannelDispatchOperationInterface for this
 *         ChannelDispatchOperation
 */
Client::ChannelDispatchOperationInterface *ChannelDispatchOperation::baseInterface() const
{
    return mPriv->baseInterface;
}

/**
 * \fn Client::DBus::PropertiesInterface *ChannelDispatchOperation::propertiesInterface() const
 *
 * Convenience function for getting a PropertiesInterface interface proxy object
 * for this account. The ChannelDispatchOperation interface relies on
 * properties, so this interface is always assumed to be present.
 *
 * \return A pointer to the existing Client::DBus::PropertiesInterface object
 *         for this ChannelDispatchOperation object.
 */

void ChannelDispatchOperation::onFinished()
{
    debug() << "ChannelDispatchOperation finished and was removed";
    invalidate(QLatin1String(TELEPATHY_QT4_ERROR_OBJECT_REMOVED),
               QLatin1String("ChannelDispatchOperation finished and was removed"));
}

void ChannelDispatchOperation::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(ChannelDispatchOperation)";
        props = reply.value();

        mPriv->extractMainProps(props, false);

        if (mPriv->connection) {
            connect(mPriv->connection->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onConnectionReady(Tp::PendingOperation *)));
        } else {
            warning() << "Properties.GetAll(ChannelDispatchOperation) is missing "
                "connection property, ignoring";
        }

        if (mPriv->account) {
            connect(mPriv->account->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onAccountReady(Tp::PendingOperation *)));
        } else {
            warning() << "Properties.GetAll(ChannelDispatchOperation) is missing "
                "account property, ignoring";
        }

        foreach (const ChannelPtr &channel, mPriv->channels) {
            connect(channel->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onChannelReady(Tp::PendingOperation *)));
        }

        mPriv->checkReady();
    }
    else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        warning().nospace() << "Properties::GetAll(ChannelDispatchOperation) failed with "
            << reply.error().name() << ": " << reply.error().message();
    }
}

void ChannelDispatchOperation::onConnectionReady(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "ChannelDispatchOperation: Unable to make connection ready";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                op->errorName(), op->errorMessage());
        return;
    }
    mPriv->checkReady();
}

void ChannelDispatchOperation::onAccountReady(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "ChannelDispatchOperation: Unable to make account ready";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                op->errorName(), op->errorMessage());
        return;
    }
    mPriv->checkReady();
}

void ChannelDispatchOperation::onChannelReady(PendingOperation *op)
{
    if (op->isError()) {
        PendingReady *pr = qobject_cast<PendingReady*>(op);
        ChannelPtr channel(qobject_cast<Channel*>(pr->object()));
        // only fail if channel still exists (channelLost was not emitted)
        if (mPriv->channels.contains(channel)) {
            warning() << "ChannelDispatchOperation: Unable to make channel ready";
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                    op->errorName(), op->errorMessage());
        }
        return;
    }
    mPriv->checkReady();
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

} // Tp
