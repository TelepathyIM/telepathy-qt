/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#include <TelepathyQt/PendingChannel>

#include "TelepathyQt/_gen/pending-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/fake-handler-manager-internal.h"
#include "TelepathyQt/request-temporary-handler-internal.h"

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/Constants>
#include <TelepathyQt/HandledChannelNotifier>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingReady>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingChannel::Private
{
    class FakeAccountFactory;

    ConnectionPtr connection;
    bool create;
    bool yours;
    QString channelType;
    uint handleType;
    uint handle;
    QVariantMap immutableProperties;
    ChannelPtr channel;

    ClientRegistrarPtr cr;
    SharedPtr<RequestTemporaryHandler> handler;
    HandledChannelNotifier *notifier;
    static uint numHandlers;
};

uint PendingChannel::Private::numHandlers = 0;

class TP_QT_NO_EXPORT PendingChannel::Private::FakeAccountFactory : public AccountFactory
{
public:
    static AccountFactoryPtr create(const AccountPtr &account)
    {
        return AccountFactoryPtr(new FakeAccountFactory(account));
    }

    ~FakeAccountFactory() { }

    AccountPtr account() const { return mAccount; }

protected:
    AccountPtr construct(const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory) const
    {
        if (mAccount->objectPath() != objectPath) {
            warning() << "Account received by the fake factory is different from original account";
        }
        return mAccount;
    }

private:
    FakeAccountFactory(const AccountPtr &account)
        : AccountFactory(account->dbusConnection(), Features()),
          mAccount(account)
    {
    }

    AccountPtr mAccount;
};

/**
 * \class PendingChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/pending-channel.h <TelepathyQt/PendingChannel>
 *
 * \brief The PendingChannel class represents the parameters of and the reply to
 * an asynchronous channel request.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is trough Connection or Account.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingChannel object that will fail immediately.
 *
 * \param connection Connection to use.
 * \param errorName The error name.
 * \param errorMessage The error message.
 */
PendingChannel::PendingChannel(const ConnectionPtr &connection, const QString &errorName,
        const QString &errorMessage)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->connection = connection;
    mPriv->yours = false;
    mPriv->handleType = 0;
    mPriv->handle = 0;
    mPriv->notifier = 0;
    mPriv->create = false;

    setFinishedWithError(errorName, errorMessage);
}

/**
 * Construct a new PendingChannel object.
 *
 * \param connection Connection to use.
 * \param request A dictionary containing the desirable properties.
 * \param create Whether createChannel or ensureChannel should be called.
 */
PendingChannel::PendingChannel(const ConnectionPtr &connection,
        const QVariantMap &request, bool create, int timeout)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->connection = connection;
    mPriv->yours = create;
    mPriv->channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
    mPriv->handleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
    mPriv->handle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
    mPriv->notifier = 0;
    mPriv->create = create;

    Client::ConnectionInterfaceRequestsInterface *requestsInterface =
        connection->interface<Client::ConnectionInterfaceRequestsInterface>();
    if (create) {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                requestsInterface->CreateChannel(request, timeout), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onConnectionCreateChannelFinished(QDBusPendingCallWatcher*)));
    }
    else {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                requestsInterface->EnsureChannel(request, timeout), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onConnectionEnsureChannelFinished(QDBusPendingCallWatcher*)));
    }
}

PendingChannel::PendingChannel(const AccountPtr &account,
        const QVariantMap &request, const QDateTime &userActionTime,
        bool create)
    : PendingOperation(account),
      mPriv(new Private)
{
    mPriv->yours = true;
    mPriv->channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
    mPriv->handleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
    mPriv->handle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();

    mPriv->cr = ClientRegistrar::create(
            Private::FakeAccountFactory::create(account),
            account->connectionFactory(),
            account->channelFactory(),
            account->contactFactory());
    mPriv->handler = RequestTemporaryHandler::create(account);
    mPriv->notifier = 0;
    mPriv->create = create;

    QString handlerName = QString(QLatin1String("TpQtRaH_%1_%2"))
        .arg(account->dbusConnection().baseService()
            .replace(QLatin1String(":"), QLatin1String("_"))
            .replace(QLatin1String("."), QLatin1String("_")))
        .arg(Private::numHandlers++);
    if (!mPriv->cr->registerClient(mPriv->handler, handlerName, false)) {
        warning() << "Unable to register handler" << handlerName;
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Unable to register handler"));
        return;
    }

    connect(mPriv->handler.data(),
            SIGNAL(error(QString,QString)),
            SLOT(onHandlerError(QString,QString)));
    connect(mPriv->handler.data(),
            SIGNAL(channelReceived(Tp::ChannelPtr,QDateTime,Tp::ChannelRequestHints)),
            SLOT(onHandlerChannelReceived(Tp::ChannelPtr)));

    handlerName = QString(QLatin1String("org.freedesktop.Telepathy.Client.%1")).arg(handlerName);

    debug() << "Requesting channel through account using handler" << handlerName;
    PendingChannelRequest *pcr;
    if (create) {
        pcr = account->createChannel(request, userActionTime, handlerName, ChannelRequestHints());
    } else {
        pcr = account->ensureChannel(request, userActionTime, handlerName, ChannelRequestHints());
    }
    connect(pcr,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountCreateChannelFinished(Tp::PendingOperation*)));
}

/**
 * Construct a new PendingChannel object that will fail immediately.
 *
 * \param errorName The name of a D-Bus error.
 * \param errorMessage The error message.
 */
PendingChannel::PendingChannel(const QString &errorName, const QString &errorMessage)
    : PendingOperation(ConnectionPtr()), mPriv(new Private)
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingChannel::~PendingChannel()
{
    delete mPriv;
}

/**
 * Return the connection through which the channel request was made.
 *
 * Note that if this channel request was created through Account, a null ConnectionPtr will be
 * returned.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr PendingChannel::connection() const
{
    return mPriv->connection;
}

/**
 * Return whether this channel belongs to this process.
 *
 * If \c false, the caller must assume that some other process is
 * handling this channel; if \c true, the caller should handle it
 * themselves or delegate it to another client.
 *
 * \return \c true if it belongs, \c false otherwise.
 */
bool PendingChannel::yours() const
{
    if (!isFinished()) {
        warning() << "PendingChannel::yours called before finished, returning undefined value";
    }
    else if (!isValid()) {
        warning() << "PendingChannel::yours called when not valid, returning undefined value";
    }

    return mPriv->yours;
}

/**
 * Return the channel type specified in the channel request.
 *
 * \return The D-Bus interface name for the type of the channel.
 */
const QString &PendingChannel::channelType() const
{
    return mPriv->channelType;
}

/**
 * If the channel request has finished, return the handle type of the resulting
 * channel. Otherwise, return the handle type that was requested.
 *
 * (One example of a request producing a different target handle type is that
 * on protocols like MSN, one-to-one conversations don't really exist, and if
 * you request a text channel with handle type HandleTypeContact, what you
 * will actually get is a text channel with handle type HandleTypeNone, with
 * the requested contact as a member.)
 *
 * \return The target handle type as #HandleType.
 * \sa targetHandle()
 */
uint PendingChannel::targetHandleType() const
{
    return mPriv->handleType;
}

/**
 * If the channel request has finished, return the target handle of the
 * resulting channel. Otherwise, return the target handle that was requested
 * (which might be different in some situations - see targetHandleType()).
 *
 * \return An integer representing the target handle, which is of the type
 *         targetHandleType() indicates.
 * \sa targetHandleType()
 */
uint PendingChannel::targetHandle() const
{
    return mPriv->handle;
}

/**
 * If this channel request has finished, return the immutable properties of
 * the resulting channel. Otherwise, return an empty map.
 *
 * The keys and values in this map are defined by the \telepathy_spec,
 * or by third-party extensions to that specification.
 * These are the properties that cannot change over the lifetime of the
 * channel; they're announced in the result of the request, for efficiency.
 * This map should be passed to the constructor of Channel or its subclasses
 * (such as TextChannel).
 *
 * These properties can also be used to process channels in a way that does
 * not require the creation of a Channel object - for instance, a
 * ChannelDispatcher implementation should be able to classify and process
 * channels based on their immutable properties, without needing to create
 * Channel objects.
 *
 * \return The immutable properties as QVariantMap.
 */
QVariantMap PendingChannel::immutableProperties() const
{
    QVariantMap props = mPriv->immutableProperties;

    // This is a reasonable guess - if it's Yours it's guaranteedly Requested by us, and if it's not
    // it could be either Requested by somebody else but also an incoming channel just as well.
    if (!props.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"))) {
        debug() << "CM didn't provide Requested in channel immutable props, guessing"
            << mPriv->yours;
        props[TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")] =
            mPriv->yours;
    }

    // Also, the spec says that if the channel was Requested by the local user, InitiatorHandle must
    // be the Connection's self handle
    if (!props.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle"))) {
        if (qdbus_cast<bool>(props.value(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")))) {
            if (connection() && connection()->isReady(Connection::FeatureCore)) {
                debug() << "CM didn't provide InitiatorHandle in channel immutable props, but we "
                    "know it's the conn's self handle (and have it)";
                props[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] =
                    connection()->selfHandle();
            }
        }
    }

    return props;
}

/**
 * Return the channel resulting from the channel request.
 *
 * \return A pointer to the Channel object.
 */
ChannelPtr PendingChannel::channel() const
{
    if (!isFinished()) {
        warning() << "PendingChannel::channel called before finished, returning 0";
        return ChannelPtr();
    } else if (!isValid()) {
        warning() << "PendingChannel::channel called when not valid, returning 0";
        return ChannelPtr();
    }

    return mPriv->channel;
}

/**
 * If this channel request has finished and was created through Account,
 * return a HandledChannelNotifier object that will keep track of channel() being re-requested.
 *
 * \return A HandledChannelNotifier instance, or 0 if an error occurred.
 */
HandledChannelNotifier *PendingChannel::handledChannelNotifier() const
{
    if (!isFinished()) {
        warning() << "PendingChannel::handledChannelNotifier called before finished, returning 0";
        return 0;
    } else if (!isValid()) {
        warning() << "PendingChannel::handledChannelNotifier called when not valid, returning 0";
        return 0;
    }

    if (mPriv->cr && !mPriv->notifier) {
        mPriv->notifier = new HandledChannelNotifier(mPriv->cr, mPriv->handler);
    }
    return mPriv->notifier;
}

void PendingChannel::onConnectionCreateChannelFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath, QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QString objectPath = reply.argumentAt<0>().path();
        QVariantMap map = reply.argumentAt<1>();

        debug() << "Got reply to Connection.CreateChannel - object path:" << objectPath;

        PendingReady *channelReady =
            connection()->channelFactory()->proxy(connection(), objectPath, map);
        mPriv->channel = ChannelPtr::qObjectCast(channelReady->proxy());

        mPriv->immutableProperties = map;
        mPriv->channelType = map.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
        mPriv->handleType = map.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
        mPriv->handle = map.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();

        connect(channelReady,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onChannelReady(Tp::PendingOperation*)));
    } else {
        debug().nospace() << "CreateChannel failed:" <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingChannel::onConnectionEnsureChannelFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<bool, QDBusObjectPath, QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        mPriv->yours = reply.argumentAt<0>();
        QString objectPath = reply.argumentAt<1>().path();
        QVariantMap map = reply.argumentAt<2>();

        debug() << "Got reply to Connection.EnsureChannel - object path:" << objectPath;

        PendingReady *channelReady =
            connection()->channelFactory()->proxy(connection(), objectPath, map);
        mPriv->channel = ChannelPtr::qObjectCast(channelReady->proxy());

        mPriv->immutableProperties = map;
        mPriv->channelType = map.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
        mPriv->handleType = map.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
        mPriv->handle = map.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();

        connect(channelReady,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onChannelReady(Tp::PendingOperation*)));
    } else {
        debug().nospace() << "EnsureChannel failed:" <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingChannel::onChannelReady(PendingOperation *op)
{
    if (!op->isError()) {
        setFinished();
    } else {
        debug() << "Making the channel ready for" << this << "failed with" << op->errorName()
            << ":" << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
    }
}

void PendingChannel::onHandlerError(const QString &errorName, const QString &errorMessage)
{
    if (isFinished()) {
        return;
    }

    warning() << "Creating/ensuring channel failed with" << errorName
        << ":" << errorMessage;
    setFinishedWithError(errorName, errorMessage);
}

void PendingChannel::onHandlerChannelReceived(const ChannelPtr &channel)
{
    if (isFinished()) {
        warning() << "Handler received the channel but this operation already finished due "
            "to failure in the channel request";
        return;
    }

    mPriv->handleType = channel->targetHandleType();
    mPriv->handle = channel->targetHandle();
    mPriv->immutableProperties = channel->immutableProperties();
    mPriv->channel = channel;

    // register the CR in FakeHandlerManager so that at least one handler per bus stays alive
    // until all channels requested using R&H gets invalidated/destroyed.
    // This is important in case Mission Control happens to restart while
    // the channel is still in use, since it will close each channel it
    // doesn't find a handler for it.
    FakeHandlerManager::instance()->registerClientRegistrar(mPriv->cr);

    setFinished();
}

void PendingChannel::onAccountCreateChannelFinished(PendingOperation *op)
{
    if (isFinished()) {
        if (isError()) {
            warning() << "Creating/ensuring channel finished with a failure after the internal "
                "handler already got a channel, ignoring";
        }
        return;
    }

    if (op->isError()) {
        warning() << "Creating/ensuring channel failed with" << op->errorName()
            << ":" << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    if (!mPriv->handler->isDBusHandlerInvoked()) {
        // Our handler hasn't be called but the channel request is complete.
        // That means another handler handled the channels so we don't own it.
        if (mPriv->create) {
            warning() << "Creating/ensuring channel failed with" << TP_QT_ERROR_SERVICE_CONFUSED
                << ":" << QLatin1String("CD.CreateChannel/WithHints returned successfully and "
                        "the handler didn't receive the channel yet");
            setFinishedWithError(TP_QT_ERROR_SERVICE_CONFUSED,
                    QLatin1String("CD.CreateChannel/WithHints returned successfully and "
                        "the handler didn't receive the channel yet"));
        } else {
            warning() << "Creating/ensuring channel failed with" << TP_QT_ERROR_NOT_YOURS
                << ":" << QLatin1String("Another handler is handling this channel");
            setFinishedWithError(TP_QT_ERROR_NOT_YOURS,
                    QLatin1String("Another handler is handling this channel"));
        }
        return;
    }
}

} // Tp
