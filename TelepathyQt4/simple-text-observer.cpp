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

#include <TelepathyQt4/SimpleTextObserver>
#include "TelepathyQt4/simple-text-observer-internal.h"

#include "TelepathyQt4/_gen/simple-text-observer.moc.hpp"
#include "TelepathyQt4/_gen/simple-text-observer-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Message>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>

namespace Tp
{

uint SimpleTextObserver::Private::numObservers = 0;

SimpleTextObserver::Private::Private(SimpleTextObserver *parent,
        const ClientRegistrarPtr &cr, const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization)
    : parent(parent),
      cr(cr),
      account(account),
      contactIdentifier(contactIdentifier),
      requiresNormalization(requiresNormalization)
{
    if (!requiresNormalization) {
        normalizedContactIdentifier = contactIdentifier;
    } else {
        parent->connect(account.data(),
                SIGNAL(connectionChanged(Tp::ConnectionPtr)),
                SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
        parent->onAccountConnectionChanged(account->connection());
    }
}

SimpleTextObserver::Private::~Private()
{
    for (QHash<TextChannelPtr, TextChannelWrapper*>::iterator i = channels.begin();
            i != channels.end();) {
        delete i.value();
    }
}

void SimpleTextObserver::Private::processMessageQueue()
{
    foreach (const TextMessageInfo mi, messageQueue) {
        if (mi.type == TextMessageInfo::Sent) {
            parent->onChannelMessageSent(mi.message, mi.flags, mi.sentMessageToken,
                    mi.channel);
        } else {
            parent->onChannelMessageReceived(*(reinterpret_cast<const ReceivedMessage*>(&mi.message)),
                    mi.channel);
        }
    }
    messageQueue.clear();
}

bool SimpleTextObserver::Private::filterMessage(const Message &message,
        const TextChannelPtr &textChannel)
{
    if (requiresNormalization) {
        QString targetId = textChannel->immutableProperties().value(
                TP_QT4_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
        if (targetId != normalizedContactIdentifier) {
            // we didn't filter per contact, let's filter here
            return false;
        }
    }
    return true;
}

SimpleTextObserver::Private::TextChannelWrapper::TextChannelWrapper(const TextChannelPtr &channel)
    : mChannel(channel)
{
    connect(channel.data(),
            SIGNAL(invalidated(Tp::PendingOperation*)),
            SLOT(onChannelInvalidated()));

    Features features = TextChannel::FeatureMessageQueue | TextChannel::FeatureMessageSentSignal;
    if (!channel->isReady(features)) {
        // The channel factory passed to the Account used by SimpleTextObserver does not contain the
        // neeeded features, request them
        connect(channel->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onChannelReady()));
    } else {
        onChannelReady();
    }
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelInvalidated()
{
    emit channelInvalidated(mChannel);
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelReady()
{
    connect(mChannel.data(),
            SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString)),
            SLOT(onChannelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString)));
    connect(mChannel.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage)),
            SLOT(onChannelMessageReceived(Tp::ReceivedMessage)));

    foreach (const ReceivedMessage &message, mChannel->messageQueue()) {
        onChannelMessageReceived(message);
    }
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelMessageSent(
        const Tp::Message &message, Tp::MessageSendingFlags flags,
        const QString &sentMessageToken)
{
    emit channelMessageSent(message, flags, sentMessageToken, mChannel);
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelMessageReceived(
        const Tp::ReceivedMessage &message)
{
    emit channelMessageReceived(message, mChannel);
}

/**
 * \class SimpleTextObserver
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/simple-text-observer.h <TelepathyQt4/SimpleTextObserver>
 *
 * \brief The SimpleTextObserver class provides an easy way to track sent/receive text messages
 *        in an account and can be optionally filtered by a contact.
 */

/**
 * Create a new SimpleTextObserver object.
 *
 * Events will be signalled for all messages (sent/received) by all contacts in \a account.
 *
 * \param account The account used to listen to events.
 * \return An SimpleTextObserverPtr object pointing to the newly created SimpleTextObserver object.
 */
SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account)
{
    return create(account, QString(), false);
}

/**
 * Create a new SimpleTextObserver object.
 *
 * If \a contact is not null, events will be signalled for all messages (sent/received) by \a
 * contact, otherwise this method works the same as create(const Tp::AccountPtr &).
 *
 * \param account The account used to listen to events.
 * \param contact The contact used to filter events.
 * \return An SimpleTextObserverPtr object pointing to the newly created SimpleTextObserver object.
 */
SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account,
        const ContactPtr &contact)
{
    if (contact) {
        return create(account, contact->id(), false);
    }
    return create(account, QString(), false);
}

/**
 * Create a new SimpleTextObserver object.
 *
 * If \a contactIdentifier is non-empty, events will be signalled for all messages (sent/received)
 * by a contact identified by \a contactIdentifier, otherwise this method works the same as
 * create(const Tp::AccountPtr &).
 *
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \return An SimpleTextObserverPtr object pointing to the newly created SimpleTextObserver object.
 */
SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account,
        const QString &contactIdentifier)
{
    return create(account, contactIdentifier, true);
}

SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization)
{
    QVariantMap additionalProperties;
    if (!contactIdentifier.isEmpty() && !requiresNormalization) {
        QVariantMap additionalProperties;
        additionalProperties.insert(TP_QT4_IFACE_CHANNEL + QLatin1String(".TargetID"),
                contactIdentifier);
    }
    ChannelClassSpec channelFilter = ChannelClassSpec::textChat(additionalProperties);

    ClientRegistrarPtr cr = ClientRegistrar::create(
            Private::FakeAccountFactory::create(account),
            account->connectionFactory(),
            account->channelFactory(),
            account->contactFactory());

    SimpleTextObserverPtr observer = SimpleTextObserverPtr(
            new SimpleTextObserver(cr, ChannelClassSpecList() << channelFilter,
                account, contactIdentifier, requiresNormalization));

    QString observerName = QString(QLatin1String("TpQt4STO_%1_%2"))
        .arg(account->dbusConnection().baseService()
            .replace(QLatin1String(":"), QLatin1String("_"))
            .replace(QLatin1String("."), QLatin1String("_")))
        .arg(Private::numObservers++);
    if (!cr->registerClient(observer, observerName, false)) {
        warning() << "Unable to register observer" << observerName;
        return SimpleTextObserverPtr();
    }

    return observer;
}

/**
 * Construct a new SimpleTextObserver object.
 *
 * \param cr The ClientRegistrar used to register this observer.
 * \param channelFilter The channel filter used by this observer.
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param requiresNormalization Whether \a contactIdentifier needs to be normalized.
 * \return An SimpleTextObserverPtr object pointing to the newly created SimpleTextObserver object.
 */
SimpleTextObserver::SimpleTextObserver(const ClientRegistrarPtr &cr,
        const ChannelClassSpecList &channelFilter,
        const AccountPtr &account, const QString &contactIdentifier,
        bool requiresNormalization)
    : QObject(),
      AbstractClientObserver(channelFilter, false),
      mPriv(new Private(this, cr, account, contactIdentifier, requiresNormalization))
{
}

/**
 * Class destructor.
 */
SimpleTextObserver::~SimpleTextObserver()
{
    delete mPriv;
}

/**
 * Return the account used to listen to events.
 *
 * \return The account used to listen to events.
 */
AccountPtr SimpleTextObserver::account() const
{
    return mPriv->account;
}

/**
 * Return the identifier of the contact used to filter events, or an empty string if none was
 * provided at construction.
 *
 * \return The identifier of the contact used to filter events.
 */
QString SimpleTextObserver::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

void SimpleTextObserver::observeChannels(
        const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const ChannelDispatchOperationPtr &dispatchOperation,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const ObserverInfo &observerInfo)
{
    if (account != mPriv->account) {
        context->setFinished();
        return;
    }

    foreach (const ChannelPtr &channel, channels) {
        TextChannelPtr textChannel = TextChannelPtr::qObjectCast(channel);
        if (!textChannel) {
            if (channel->channelType() != TP_QT4_IFACE_CHANNEL_TYPE_TEXT) {
                warning() << "Channel received to observe is not of type Text, service confused. "
                    "Ignoring channel";
            } else {
                warning() << "Channel received to observe is not a subclass of TextChannel. "
                    "ChannelFactory set on this observer's account must construct TextChannel "
                    "subclasses for channels of type Text. Ignoring channel";
            }
            continue;
        }

        if (mPriv->channels.contains(textChannel)) {
            // we are already observing this channel
            continue;
        }

        // this shouldn't happen, but in any case
        if (!textChannel->isValid()) {
            warning() << "Channel received to observe is invalid. Ignoring channel";
            continue;
        }

        Private::TextChannelWrapper *wrapper = new Private::TextChannelWrapper(textChannel);
        mPriv->channels.insert(textChannel, wrapper);
        connect(wrapper,
                SIGNAL(channelInvalidated(Tp::TextChannelPtr)),
                SLOT(onChannelInvalidated(Tp::TextChannelPtr)));
        connect(wrapper,
                SIGNAL(channelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)),
                SLOT(onChannelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)));
        connect(wrapper,
                SIGNAL(channelMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)),
                SLOT(onChannelMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)));
    }

    context->setFinished();
}

void SimpleTextObserver::onAccountConnectionChanged(const Tp::ConnectionPtr &connection)
{
    if (connection) {
        if (connection->status() == ConnectionStatusConnected) {
            onAccountConnectionConnected();
        } else {
            connect(connection->becomeReady(Connection::FeatureConnected),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onAccountConnectionConnected()));
        }
    }
}

void SimpleTextObserver::onAccountConnectionConnected()
{
    ConnectionPtr conn = mPriv->account->connection();

    // check here again as the account connection may have changed and the op failed
    if (!conn || conn->status() != ConnectionStatusConnected) {
        return;
    }

    ContactManagerPtr contactManager = conn->contactManager();
    connect(contactManager->contactsForIdentifiers(QStringList() << mPriv->contactIdentifier),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactConstructed(Tp::PendingOperation*)));
}

void SimpleTextObserver::onContactConstructed(Tp::PendingOperation *op)
{
    if (op->isError()) {
        // what should we do here? retry? wait for a new connection?
        return;
    }

    PendingContacts *pc = qobject_cast<PendingContacts*>(op);
    Q_ASSERT(pc->contacts().size() == 1);
    ContactPtr contact = pc->contacts().first();
    mPriv->normalizedContactIdentifier = contact->id();
    mPriv->processMessageQueue();

    // disconnect all account signals we are handling
    disconnect(mPriv->account.data(), 0, this, 0);
}

void SimpleTextObserver::onChannelInvalidated(const TextChannelPtr &textChannel)
{
    delete mPriv->channels.take(textChannel);
}

void SimpleTextObserver::onChannelMessageSent(const Tp::Message &message,
        Tp::MessageSendingFlags flags, const QString &sentMessageToken,
        const Tp::TextChannelPtr &textChannel)
{
    if (mPriv->normalizedContactIdentifier.isEmpty()) {
        mPriv->messageQueue.append(
                Private::TextMessageInfo(message, flags, sentMessageToken, textChannel));
        return;
    }

    if (!mPriv->filterMessage(message, textChannel)) {
        return;
    }

    emit messageSent(message, flags, sentMessageToken, textChannel);
}

void SimpleTextObserver::onChannelMessageReceived(const Tp::ReceivedMessage &message,
        const Tp::TextChannelPtr &textChannel)
{
    if (mPriv->normalizedContactIdentifier.isEmpty()) {
        mPriv->messageQueue.append(
                Private::TextMessageInfo(message, textChannel));
        return;
    }

    if (!mPriv->filterMessage(message, textChannel)) {
        return;
    }

    emit messageReceived(message, textChannel);
}

/**
 * \fn void SimpleTextObserver::messageSent(const Tp::Message &message,
 *                  Tp::MessageSendingFlags flags, const QString &sentMessageToken,
 *                  const Tp::TextChannelPtr &channel);
 *
 * This signal is emitted whenever a text message on account() is sent.
 * If contactIdentifier() is non-empty, only messages sent to the contact identified by it will
 * be signalled.
 */

/**
 * \fn void SimpleTextObserver::messageReceived(const Tp::ReceivedMessage &message,
 *                  const Tp::TextChannelPtr &channel);
 *
 * This signal is emitted whenever a text message on account() is received.
 * If contactIdentifier() is non-empty, only messages received by the contact identified by it will
 * be signalled.
 */

} // Tp
