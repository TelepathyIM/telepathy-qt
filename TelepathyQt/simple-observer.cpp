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

#include <TelepathyQt/SimpleObserver>
#include "TelepathyQt/simple-observer-internal.h"

#include "TelepathyQt/_gen/simple-observer.moc.hpp"
#include "TelepathyQt/_gen/simple-observer-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingSuccess>

namespace Tp
{

QHash<QPair<QString, QSet<ChannelClassSpec> >, WeakPtr<SimpleObserver::Private::Observer> > SimpleObserver::Private::observers;
uint SimpleObserver::Private::numObservers = 0;

SimpleObserver::Private::Private(SimpleObserver *parent,
        const AccountPtr &account,
        const ChannelClassSpecList &channelFilter,
        const QString &contactIdentifier,
        bool requiresNormalization,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
    : parent(parent),
      account(account),
      channelFilter(channelFilter),
      contactIdentifier(contactIdentifier),
      extraChannelFeatures(extraChannelFeatures)
{
    QSet<ChannelClassSpec> normalizedChannelFilter = channelFilter.toSet();
    QPair<QString, QSet<ChannelClassSpec> > observerUniqueId(
            account->dbusConnection().baseService(), normalizedChannelFilter);
    observer = SharedPtr<Observer>(observers.value(observerUniqueId));
    if (!observer) {
        SharedPtr<FakeAccountFactory> fakeAccountFactory = FakeAccountFactory::create(
                account->dbusConnection());

        cr = ClientRegistrar::create(
                AccountFactoryPtr::qObjectCast(fakeAccountFactory),
                account->connectionFactory(),
                account->channelFactory(),
                account->contactFactory());

        QString observerName = QString(QLatin1String("TpQtSO_%1_%2"))
            .arg(account->dbusConnection().baseService()
                .replace(QLatin1String(":"), QLatin1String("_"))
                .replace(QLatin1String("."), QLatin1String("_")))
            .arg(numObservers++);
        observer = SharedPtr<Observer>(new Observer(cr, fakeAccountFactory,
                    normalizedChannelFilter.toList(), observerName));
        if (!cr->registerClient(observer, observerName, false)) {
            warning() << "Unable to register observer" << observerName;
            observer.reset();
            cr.reset();
            return;
        }

        debug() << "Observer" << observerName << "registered";
        observers.insert(observerUniqueId, observer);
    } else {
        debug() << "Observer" << observer->observerName() <<
            "already registered and matches filter, using it";
        cr = ClientRegistrarPtr(observer->clientRegistrar());
    }

    observer->registerExtraChannelFeatures(extraChannelFeatures);
    observer->registerAccount(account);

    if (contactIdentifier.isEmpty() || !requiresNormalization) {
        normalizedContactIdentifier = contactIdentifier;
    } else {
        parent->connect(account.data(),
                SIGNAL(connectionChanged(Tp::ConnectionPtr)),
                SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
    }

    parent->connect(observer.data(),
            SIGNAL(newChannels(Tp::AccountPtr,QList<Tp::ChannelPtr>)),
            SLOT(onNewChannels(Tp::AccountPtr,QList<Tp::ChannelPtr>)));
    parent->connect(observer.data(),
            SIGNAL(channelInvalidated(Tp::AccountPtr,Tp::ChannelPtr,QString,QString)),
            SLOT(onChannelInvalidated(Tp::AccountPtr,Tp::ChannelPtr,QString,QString)));
}

bool SimpleObserver::Private::filterChannel(const AccountPtr &channelAccount,
        const ChannelPtr &channel)
{
    if (channelAccount != account) {
        return false;
    }

    if (contactIdentifier.isEmpty()) {
        return true;
    }

    QString targetId = channel->immutableProperties().value(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
    if (targetId != normalizedContactIdentifier) {
        // we didn't filter per contact, let's filter here
        return false;
    }
    return true;
}

void SimpleObserver::Private::insertChannels(const AccountPtr &channelsAccount,
        const QList<ChannelPtr> &newChannels)
{
    QSet<ChannelPtr> match;
    foreach (const ChannelPtr &channel, newChannels) {
        if (!channels.contains(channel) && filterChannel(channelsAccount, channel)) {
            match.insert(channel);
        }
    }

    if (match.isEmpty()) {
        return;
    }

    channels.unite(match);
    emit parent->newChannels(match.toList());
}

void SimpleObserver::Private::removeChannel(const AccountPtr &channelAccount,
        const ChannelPtr &channel,
        const QString &errorName, const QString &errorMessage)
{
    if (!channels.contains(channel) || !filterChannel(channelAccount, channel)) {
        return;
    }

    channels.remove(channel);
    emit parent->channelInvalidated(channel, errorName, errorMessage);
}

void SimpleObserver::Private::processChannelsQueue()
{
    if (channelsQueue.isEmpty()) {
        return;
    }

    while (!channelsQueue.isEmpty()) {
        (this->*(channelsQueue.dequeue()))();
    }
}

void SimpleObserver::Private::processNewChannelsQueue()
{
    NewChannelsInfo info = newChannelsQueue.dequeue();
    insertChannels(info.channelsAccount, info.channels);
}

void SimpleObserver::Private::processChannelsInvalidationQueue()
{
    ChannelInvalidationInfo info = channelsInvalidationQueue.dequeue();
    removeChannel(info.channelAccount, info.channel, info.errorName, info.errorMessage);
}

SimpleObserver::Private::Observer::Observer(const WeakPtr<ClientRegistrar> &cr,
        const SharedPtr<FakeAccountFactory> &fakeAccountFactory,
        const ChannelClassSpecList &channelFilter,
        const QString &observerName)
    : AbstractClient(),
      QObject(),
      AbstractClientObserver(channelFilter, true),
      mCr(cr),
      mFakeAccountFactory(fakeAccountFactory),
      mObserverName(observerName)
{
}

SimpleObserver::Private::Observer::~Observer()
{
    // no need to delete channel wrappers here as they have 'this' as parent

    // no need to delete context infos here as this observer will never be deleted before all
    // PendingComposites finish (hold reference to this), which will properly delete them

    // no need to unregister this observer here as the client registrar destructor will
    // unregister it
}

void SimpleObserver::Private::Observer::observeChannels(
        const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const ChannelDispatchOperationPtr &dispatchOperation,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const ObserverInfo &observerInfo)
{
    if (!mAccounts.contains(account)) {
        context->setFinished();
        return;
    }

    QList<PendingOperation*> readyOps;
    QList<ChannelPtr> newChannels;

    foreach (const ChannelPtr &channel, channels) {
        if (mIncompleteChannels.contains(channel) ||
            mChannels.contains(channel)) {
            // we are already observing this channel
            continue;
        }

        // this shouldn't happen, but in any case
        if (!channel->isValid()) {
            warning() << "Channel received to observe is invalid. "
                "Ignoring channel";
            continue;
        }

        SimpleObserver::Private::ChannelWrapper *wrapper =
            new SimpleObserver::Private::ChannelWrapper(account, channel,
                featuresFor(ChannelClassSpec(channel->immutableProperties())), this);
        mIncompleteChannels.insert(channel, wrapper);
        connect(wrapper,
                SIGNAL(channelInvalidated(Tp::AccountPtr,Tp::ChannelPtr,QString,QString)),
                SLOT(onChannelInvalidated(Tp::AccountPtr,Tp::ChannelPtr,QString,QString)));

        newChannels.append(channel);
        readyOps.append(wrapper->becomeReady());
    }

    if (readyOps.isEmpty()) {
        context->setFinished();
        return;
    }

    PendingComposite *pc = new PendingComposite(readyOps,
            false /* failOnFirstError */, SharedPtr<Observer>(this));
    mObserveChannelsInfo.insert(pc, new ContextInfo(context, account, newChannels));
    connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelsReady(Tp::PendingOperation*)));
}

void SimpleObserver::Private::Observer::onChannelInvalidated(const AccountPtr &channelAccount,
        const ChannelPtr &channel, const QString &errorName, const QString &errorMessage)
{
    if (mIncompleteChannels.contains(channel)) {
        // we are still handling the channel, wait for onChannelsReady that will properly remove
        // it from mChannels
        return;
    }
    emit channelInvalidated(channelAccount, channel, errorName, errorMessage);
    Q_ASSERT(mChannels.contains(channel));
    delete mChannels.take(channel);
}

void SimpleObserver::Private::Observer::onChannelsReady(PendingOperation *op)
{
    ContextInfo *info = mObserveChannelsInfo.value(op);

    foreach (const ChannelPtr &channel, info->channels) {
        Q_ASSERT(mIncompleteChannels.contains(channel));
        ChannelWrapper *wrapper = mIncompleteChannels.take(channel);
        mChannels.insert(channel, wrapper);
    }
    emit newChannels(info->account, info->channels);

    foreach (const ChannelPtr &channel, info->channels) {
        ChannelWrapper *wrapper = mChannels.value(channel);
        if (!channel->isValid()) {
            mChannels.remove(channel);
            emit channelInvalidated(info->account, channel, channel->invalidationReason(),
                    channel->invalidationMessage());
            delete wrapper;
        }
    }

    mObserveChannelsInfo.remove(op);
    info->context->setFinished();
    delete info;
}

Features SimpleObserver::Private::Observer::featuresFor(
        const ChannelClassSpec &channelClass) const
{
    Features features;

    foreach (const ChannelClassFeatures &spec, mExtraChannelFeatures) {
        if (spec.first.isSubsetOf(channelClass)) {
            features.unite(spec.second);
        }
    }

    return features;
}

SimpleObserver::Private::ChannelWrapper::ChannelWrapper(const AccountPtr &channelAccount,
        const ChannelPtr &channel, const Features &extraChannelFeatures, QObject *parent)
    : QObject(parent),
      mChannelAccount(channelAccount),
      mChannel(channel),
      mExtraChannelFeatures(extraChannelFeatures)
{
    connect(channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));
}

PendingOperation *SimpleObserver::Private::ChannelWrapper::becomeReady()
{
    PendingOperation *op;

    if (!mChannel->isReady(mExtraChannelFeatures)) {
        // The channel factory passed to the Account used by SimpleObserver does
        // not contain the extra features, request them
        op = mChannel->becomeReady(mExtraChannelFeatures);
    } else {
        op = new PendingSuccess(mChannel);
    }

    return op;
}

void SimpleObserver::Private::ChannelWrapper::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_ASSERT(proxy == mChannel.data());
    emit channelInvalidated(mChannelAccount, mChannel, errorName, errorMessage);
}

/**
 * \class SimpleObserver
 * \ingroup utils
 * \headerfile TelepathyQt/simple-observer.h <TelepathyQt/SimpleObserver>
 *
 * \brief The SimpleObserver class provides an easy way to track channels
 *        in an account and can be optionally filtered by a contact.
 */

/**
 * Create a new SimpleObserver object.
 *
 * Events will be signalled for all channels in \a account that match
 * \a channelFilter for all contacts.
 *
 * \param channelFilter A specification of the channels in which this observer
 *                      is interested.
 * \param account The account used to listen to events.
 * \param extraChannelFeatures Extra channel features to be enabled. All channels emitted in
 *                             newChannels() will have the extra features that match their
 *                             immutable properties enabled.
 * \return An SimpleObserverPtr object pointing to the newly created SimpleObserver object.
 */
SimpleObserverPtr SimpleObserver::create(
        const AccountPtr &account,
        const ChannelClassSpecList &channelFilter,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
{
    return create(account, channelFilter, QString(), false, extraChannelFeatures);
}

/**
 * Create a new SimpleObserver object.
 *
 * Events will be signalled for all channels in \a account established with
 * \a contact, if not null, and that match \a channelFilter.
 *
 * \param channelFilter A specification of the channels in which this observer
 *                      is interested.
 * \param account The account used to listen to events.
 * \param contact The contact used to filter events.
 * \param extraChannelFeatures Extra channel features to be enabled. All channels emitted in
 *                             newChannels() will have the extra features that match their
 *                             immutable properties enabled.
 * \return An SimpleObserverPtr object pointing to the newly created SimpleObserver object.
 */
SimpleObserverPtr SimpleObserver::create(
        const AccountPtr &account,
        const ChannelClassSpecList &channelFilter,
        const ContactPtr &contact,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
{
    if (contact) {
        return create(account, channelFilter, contact->id(), false, extraChannelFeatures);
    }
    return create(account, channelFilter, QString(), false, extraChannelFeatures);
}

/**
 * Create a new SimpleObserver object.
 *
 * Events will be signalled for all channels in \a account established
 * with a contact identified by \a contactIdentifier, if non-empty, and that match
 * \a channelFilter.
 *
 * \param channelFilter A specification of the channels in which this observer
 *                      is interested.
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param extraChannelFeatures Extra channel features to be enabled. All channels emitted in
 *                             newChannels() will have the extra features that match their
 *                             immutable properties enabled.
 * \return An SimpleObserverPtr object pointing to the newly created SimpleObserver object.
 */
SimpleObserverPtr SimpleObserver::create(
        const AccountPtr &account,
        const ChannelClassSpecList &channelFilter,
        const QString &contactIdentifier,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
{
    return create(account, channelFilter, contactIdentifier, true, extraChannelFeatures);
}

SimpleObserverPtr SimpleObserver::create(
        const AccountPtr &account,
        const ChannelClassSpecList &channelFilter,
        const QString &contactIdentifier,
        bool requiresNormalization,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
{
    return SimpleObserverPtr(new SimpleObserver(account, channelFilter, contactIdentifier,
                requiresNormalization, extraChannelFeatures));
}

/**
 * Construct a new SimpleObserver object.
 *
 * \param cr The ClientRegistrar used to register this observer.
 * \param channelFilter The channel filter used by this observer.
 * \param account The account used to listen to events.
 * \param extraChannelFeatures Extra channel features to be enabled. All channels emitted in
 *                             newChannels() will have the extra features that match their
 *                             immutable properties enabled.
 * \return An SimpleObserverPtr object pointing to the newly created SimpleObserver object.
 */
SimpleObserver::SimpleObserver(const AccountPtr &account,
        const ChannelClassSpecList &channelFilter,
        const QString &contactIdentifier, bool requiresNormalization,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
    : mPriv(new Private(this, account, channelFilter, contactIdentifier,
                requiresNormalization, extraChannelFeatures))
{
    if (mPriv->observer) {
        // populate our channels list with current observer channels
        QHash<AccountPtr, QList<ChannelPtr> > channels;
        foreach (const Private::ChannelWrapper *wrapper, mPriv->observer->channels()) {
            channels[wrapper->channelAccount()].append(wrapper->channel());
        }

        QHash<AccountPtr, QList<ChannelPtr> >::const_iterator it = channels.constBegin();
        QHash<AccountPtr, QList<ChannelPtr> >::const_iterator end = channels.constEnd();
        for (; it != end; ++it) {
            onNewChannels(it.key(), it.value());
        }

        if (requiresNormalization) {
            debug() << "Contact id requires normalization. "
                "Queueing events until it is normalized";
            onAccountConnectionChanged(account->connection());
        }
    }
}

/**
 * Class destructor.
 */
SimpleObserver::~SimpleObserver()
{
    delete mPriv;
}

/**
 * Return the account used to listen to events.
 *
 * \return A pointer to the Account object.
 */
AccountPtr SimpleObserver::account() const
{
    return mPriv->account;
}

/**
 * Return a specification of the channels that this observer is interested.
 *
 * \return The specification of the channels as a list of ChannelClassSpec objects.
 */
ChannelClassSpecList SimpleObserver::channelFilter() const
{
    return mPriv->channelFilter;
}

/**
 * Return the extra channel features to be enabled based on the channels immutable properties.
 *
 * \return The features as a list of ChannelClassFeatures objects.
 */
QList<ChannelClassFeatures> SimpleObserver::extraChannelFeatures() const
{
    return mPriv->extraChannelFeatures;
}

/**
 * Return the channels being observed.
 *
 * \return A list of pointers to Channel objects.
 */
QList<ChannelPtr> SimpleObserver::channels() const
{
    return mPriv->channels.toList();
}

/**
 * Return the identifier of the contact used to filter events, or an empty string if none was
 * provided at construction.
 *
 * \return The identifier of the contact.
 */
QString SimpleObserver::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

void SimpleObserver::onAccountConnectionChanged(const Tp::ConnectionPtr &connection)
{
    if (connection) {
        connect(connection->becomeReady(Connection::FeatureConnected),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAccountConnectionConnected()));
    }
}

void SimpleObserver::onAccountConnectionConnected()
{
    ConnectionPtr conn = mPriv->account->connection();

    // check here again as the account connection may have changed and the op failed
    if (!conn || conn->status() != ConnectionStatusConnected) {
        return;
    }

    debug() << "Normalizing contact id" << mPriv->contactIdentifier;
    ContactManagerPtr contactManager = conn->contactManager();
    connect(contactManager->contactsForIdentifiers(QStringList() << mPriv->contactIdentifier),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactConstructed(Tp::PendingOperation*)));
}

void SimpleObserver::onContactConstructed(Tp::PendingOperation *op)
{
    if (op->isError()) {
        // what should we do here? retry? wait for a new connection?
        warning() << "Normalizing contact id failed with" <<
            op->errorName() << " : " << op->errorMessage();
        return;
    }

    PendingContacts *pc = qobject_cast<PendingContacts*>(op);
    Q_ASSERT((pc->contacts().size() + pc->invalidIdentifiers().size()) == 1);
    if (!pc->invalidIdentifiers().isEmpty()) {
        warning() << "Normalizing contact id failed with invalid id" <<
            mPriv->contactIdentifier;
        return;
    }

    ContactPtr contact = pc->contacts().first();
    debug() << "Contact id" << mPriv->contactIdentifier <<
        "normalized to" << contact->id();
    mPriv->normalizedContactIdentifier = contact->id();
    mPriv->processChannelsQueue();

    // disconnect all account signals we are handling
    disconnect(mPriv->account.data(), 0, this, 0);
}

void SimpleObserver::onNewChannels(const AccountPtr &channelsAccount,
        const QList<ChannelPtr> &channels)
{
    if (!mPriv->contactIdentifier.isEmpty() && mPriv->normalizedContactIdentifier.isEmpty()) {
        mPriv->newChannelsQueue.append(Private::NewChannelsInfo(channelsAccount, channels));
        mPriv->channelsQueue.append(&SimpleObserver::Private::processNewChannelsQueue);
        return;
    }

    mPriv->insertChannels(channelsAccount, channels);
}

void SimpleObserver::onChannelInvalidated(const AccountPtr &channelAccount,
        const ChannelPtr &channel, const QString &errorName, const QString &errorMessage)
{
    if (!mPriv->contactIdentifier.isEmpty() && mPriv->normalizedContactIdentifier.isEmpty()) {
        mPriv->channelsInvalidationQueue.append(Private::ChannelInvalidationInfo(channelAccount,
                    channel, errorName, errorMessage));
        mPriv->channelsQueue.append(&SimpleObserver::Private::processChannelsInvalidationQueue);
        return;
    }

    mPriv->removeChannel(channelAccount, channel, errorName, errorMessage);
}

/**
 * \fn void SimpleObserver::newChannels(const QList<Tp::ChannelPtr> &channels)
 *
 * Emitted whenever new channels that match this observer's criteria are created.
 *
 * \param channels The new channels.
 */

/**
 * \fn void SimpleObserver::channelInvalidated(const Tp::ChannelPtr &channel,
 *          const QString &errorName, const QString &errorMessage)
 *
 * Emitted whenever a channel that is being observed is invalidated.
 *
 * \param channel The channel that was invalidated.
 * \param errorName A D-Bus error name (a string in a subset
 *                  of ASCII, prefixed with a reversed domain name).
 * \param errorMessage A debugging message associated with the error.
 */

} // Tp
