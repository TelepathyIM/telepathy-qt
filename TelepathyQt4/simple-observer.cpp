/**
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/SimpleObserver>
#include "TelepathyQt4/simple-observer-internal.h"

#include "TelepathyQt4/_gen/simple-observer.moc.hpp"
#include "TelepathyQt4/_gen/simple-observer-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingComposite>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingSuccess>

namespace Tp
{

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
    debug() << "Registering observer for account" << account->objectPath();
    ClientRegistrarPtr cr = ClientRegistrar::create(
            FakeAccountFactory::create(account),
            account->connectionFactory(),
            account->channelFactory(),
            account->contactFactory());

    observer = SharedPtr<Observer>(new Observer(cr,
                channelFilter, account, extraChannelFeatures));

    QString observerName = QString(QLatin1String("TpQt4SO_%1_%2"))
        .arg(account->dbusConnection().baseService()
            .replace(QLatin1String(":"), QLatin1String("_"))
            .replace(QLatin1String("."), QLatin1String("_")))
        .arg(numObservers++);
    if (!cr->registerClient(observer, observerName, false)) {
        warning() << "Unable to register observer" << observerName;
        observer.reset();
        cr.reset();
        return;
    }

    if (contactIdentifier.isEmpty() || !requiresNormalization) {
        normalizedContactIdentifier = contactIdentifier;
    } else {
        parent->connect(account.data(),
                SIGNAL(connectionChanged(Tp::ConnectionPtr)),
                SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
    }

    parent->connect(observer.data(),
            SIGNAL(newChannels(QList<Tp::ChannelPtr>)),
            SLOT(onNewChannels(QList<Tp::ChannelPtr>)));
    parent->connect(observer.data(),
            SIGNAL(channelInvalidated(Tp::ChannelPtr,QString,QString)),
            SLOT(onChannelInvalidated(Tp::ChannelPtr,QString,QString)));
}

bool SimpleObserver::Private::filterChannel(const ChannelPtr &channel)
{
    if (contactIdentifier.isEmpty()) {
        return true;
    }

    QString targetId = channel->immutableProperties().value(
            TP_QT4_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
    if (targetId != normalizedContactIdentifier) {
        // we didn't filter per contact, let's filter here
        return false;
    }
    return true;
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
    emit parent->newChannels(info.channels);
}

void SimpleObserver::Private::processChannelsInvalidationQueue()
{
    ChannelInvadationInfo info = channelsInvalidationQueue.dequeue();
    emit parent->channelInvalidated(info.channel, info.errorName, info.errorMessage);
}

SimpleObserver::Private::Observer::Observer(const ClientRegistrarPtr &cr,
        const ChannelClassSpecList &channelFilter,
        const AccountPtr &account,
        const QList<ChannelClassFeatures> &extraChannelFeatures)
    : QObject(),
      AbstractClientObserver(channelFilter, false),
      mCr(cr),
      mAccount(account),
      mExtraChannelFeatures(extraChannelFeatures)
{
}

SimpleObserver::Private::Observer::~Observer()
{
    for (QHash<ChannelPtr, ChannelWrapper*>::iterator i = mChannels.begin();
            i != mChannels.end();) {
        delete i.value();
    }
    mChannels.clear();

    // no need to delete context infos here as this observer will never be deleted before all
    // PendingComposites finish (hold reference to this), which will properly delete them

    debug() << "Unregistering observer for account" << mAccount->objectPath();
    mCr->unregisterClient(SharedPtr<Observer>(this));
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
    if (account != mAccount) {
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
            new SimpleObserver::Private::ChannelWrapper(channel,
                featuresFor(ChannelClassSpec(channel->immutableProperties())));
        mIncompleteChannels.insert(channel, wrapper);
        connect(wrapper,
                SIGNAL(channelInvalidated(Tp::ChannelPtr,QString,QString)),
                SLOT(onChannelInvalidated(Tp::ChannelPtr,QString,QString)));

        newChannels.append(channel);
        readyOps.append(wrapper->becomeReady());
    }

    if (readyOps.isEmpty()) {
        context->setFinished();
        return;
    }

    PendingComposite *pc = new PendingComposite(readyOps,
            false /* failOnFirstError */, SharedPtr<Observer>(this));
    mObserveChannelsInfo.insert(pc, new ContextInfo(context, newChannels));
    connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelsReady(Tp::PendingOperation*)));
}

void SimpleObserver::Private::Observer::onChannelInvalidated(
        const ChannelPtr &channel, const QString &errorName, const QString &errorMessage)
{
    foreach (ContextInfo *info, mObserveChannelsInfo) {
        if (info->channels.contains(channel)) {
            // we are still handling the channel, wait for onChannelsReady that will properly remove
            // it from mChannels
            return;
        }
    }
    emit channelInvalidated(channel, errorName, errorMessage);
    delete mChannels.take(channel);
    mIncompleteChannels.remove(channel);
}

void SimpleObserver::Private::Observer::onChannelsReady(PendingOperation *op)
{
    ContextInfo *info = mObserveChannelsInfo.value(op);

    emit newChannels(info->channels);

    foreach (const ChannelPtr &channel, info->channels) {
        ChannelWrapper *wrapper = mIncompleteChannels.take(channel);
        if (!channel->isValid()) {
            emit channelInvalidated(channel, channel->invalidationReason(),
                    channel->invalidationMessage());
            delete mChannels.take(channel);
        } else {
            mChannels.insert(channel, wrapper);
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

SimpleObserver::Private::ChannelWrapper::ChannelWrapper(
        const ChannelPtr &channel, const Features &extraChannelFeatures)
    : mChannel(channel),
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
    Q_UNUSED(proxy);
    emit channelInvalidated(mChannel, errorName, errorMessage);
}

/**
 * \class SimpleObserver
 * \ingroup utils
 * \headerfile TelepathyQt4/simple-observer.h <TelepathyQt4/SimpleObserver>
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
    return create(account, channelFilter, extraChannelFeatures);
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
    if (mPriv->observer && requiresNormalization) {
        debug() << "Contact id requires normalization. "
            "Queueing events until it is normalized";
        onAccountConnectionChanged(account->connection());
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
 * \return The account used to listen to events.
 */
AccountPtr SimpleObserver::account() const
{
    return mPriv->account;
}

/**
 * Return a specification of the channels that this observer is interested.
 *
 * \return The specification of the channels that this channel observer is interested.
 */
ChannelClassSpecList SimpleObserver::channelFilter() const
{
    return mPriv->channelFilter;
}

/**
 * Return the extra channel features to be enabled based on the channels immutable properties.
 *
 * \return The extra channel features to be enabled based on the channels immutable properties.
 */
QList<ChannelClassFeatures> SimpleObserver::extraChannelFeatures() const
{
    return mPriv->extraChannelFeatures;
}

QList<ChannelPtr> SimpleObserver::channels() const
{
    return mPriv->observer ? mPriv->observer->channels() : QList<ChannelPtr>();
}

/**
 * Return the identifier of the contact used to filter events, or an empty string if none was
 * provided at construction.
 *
 * \return The identifier of the contact used to filter events.
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

void SimpleObserver::onNewChannels(const QList<ChannelPtr> &channels)
{
    if (!mPriv->contactIdentifier.isEmpty() && mPriv->normalizedContactIdentifier.isEmpty()) {
        mPriv->newChannelsQueue.append(Private::NewChannelsInfo(channels));
        mPriv->channelsQueue.append(&SimpleObserver::Private::processNewChannelsQueue);
        return;
    }

    QSet<ChannelPtr> match;
    foreach (const ChannelPtr &channel, channels) {
        if (mPriv->filterChannel(channel)) {
            match.insert(channel);
        }
    }

    emit newChannels(match.toList());
}

void SimpleObserver::onChannelInvalidated(const ChannelPtr &channel, const QString &errorName,
        const QString &errorMessage)
{
    if (!mPriv->contactIdentifier.isEmpty() && mPriv->normalizedContactIdentifier.isEmpty()) {
        mPriv->channelsInvalidationQueue.append(Private::ChannelInvadationInfo(channel, errorName,
                    errorMessage));
        mPriv->channelsQueue.append(&SimpleObserver::Private::processChannelsInvalidationQueue);
        return;
    }

    if (!mPriv->filterChannel(channel)) {
        return;
    }

    emit channelInvalidated(channel, errorName, errorMessage);
}

/**
 * \fn void SimpleObserver::newChannels(const QList<Tp::ChannelPtr> &channels)
 *
 * This signal is emitted whenever new channels that match this observer's criteria are created.
 *
 * \param channels The new channels.
 */

/**
 * \fn void SimpleObserver::channelInvalidated(const Tp::ChannelPtr &channel,
 *          const QString &errorName, const QString &errorMessage)
 *
 * This signal is emitted whenever a channel that is being observed is invalidated.
 *
 * \param channel The channel that was invalidated.
 * \param errorName A D-Bus error name (a string in a subset
 *                  of ASCII, prefixed with a reversed domain name).
 * \param errorMessage A debugging message associated with the error.
 */

} // Tp
