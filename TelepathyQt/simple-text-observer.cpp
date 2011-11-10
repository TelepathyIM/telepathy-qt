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

#include <TelepathyQt/SimpleTextObserver>
#include "TelepathyQt/simple-text-observer-internal.h"

#include "TelepathyQt/_gen/simple-text-observer.moc.hpp"
#include "TelepathyQt/_gen/simple-text-observer-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Message>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingSuccess>

namespace Tp
{

SimpleTextObserver::Private::Private(SimpleTextObserver *parent,
        const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization)
    : parent(parent),
      account(account),
      contactIdentifier(contactIdentifier)
{
    debug() << "Creating a new SimpleTextObserver";
    ChannelClassSpec channelFilter = ChannelClassSpec::textChat();
    observer = SimpleObserver::create(account, ChannelClassSpecList() << channelFilter,
            contactIdentifier, requiresNormalization,
            QList<ChannelClassFeatures>() << ChannelClassFeatures(channelFilter,
                TextChannel::FeatureMessageQueue | TextChannel::FeatureMessageSentSignal));

    parent->connect(observer.data(),
            SIGNAL(newChannels(QList<Tp::ChannelPtr>)),
            SLOT(onNewChannels(QList<Tp::ChannelPtr>)));
    parent->connect(observer.data(),
            SIGNAL(channelInvalidated(Tp::ChannelPtr,QString,QString)),
            SLOT(onChannelInvalidated(Tp::ChannelPtr)));
}

SimpleTextObserver::Private::~Private()
{
    foreach (TextChannelWrapper *wrapper, channels) {
        delete wrapper;
    }
}

SimpleTextObserver::Private::TextChannelWrapper::TextChannelWrapper(const TextChannelPtr &channel)
    : mChannel(channel)
{
    connect(mChannel.data(),
            SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString)),
            SLOT(onChannelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString)));
    connect(mChannel.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage)),
            SLOT(onChannelMessageReceived(Tp::ReceivedMessage)));
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
 * \ingroup utils
 * \headerfile TelepathyQt/simple-text-observer.h <TelepathyQt/SimpleTextObserver>
 *
 * \brief The SimpleTextObserver class provides an easy way to track sent/received text messages
 *        in an account and can be optionally filtered by a contact.
 */

/**
 * Create a new SimpleTextObserver object.
 *
 * Events will be signalled for all messages sent/received by all contacts in \a account.
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
 * If \a contact is not null, events will be signalled for all messages sent/received by \a
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
 * If \a contactIdentifier is non-empty, events will be signalled for all messages sent/received
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
    return SimpleTextObserverPtr(
            new SimpleTextObserver(account, contactIdentifier, requiresNormalization));
}

/**
 * Construct a new SimpleTextObserver object.
 *
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param requiresNormalization Whether \a contactIdentifier needs to be normalized.
 * \return An SimpleTextObserverPtr object pointing to the newly created SimpleTextObserver object.
 */
SimpleTextObserver::SimpleTextObserver(const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization)
    : mPriv(new Private(this, account, contactIdentifier, requiresNormalization))
{
    if (!mPriv->observer->channels().isEmpty()) {
        onNewChannels(mPriv->observer->channels());
    }
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
 * \return A pointer to the Account object.
 */
AccountPtr SimpleTextObserver::account() const
{
    return mPriv->account;
}

/**
 * Return the identifier of the contact used to filter events, or an empty string if none was
 * provided at construction.
 *
 * \return The identifier of the contact.
 */
QString SimpleTextObserver::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

/**
 * Return the list of text chats currently being observed.
 *
 * \return A list of pointers to TextChannel objects.
 */
QList<TextChannelPtr> SimpleTextObserver::textChats() const
{
    QList<TextChannelPtr> ret;
    foreach (const ChannelPtr &channel, mPriv->observer->channels()) {
        TextChannelPtr textChannel = TextChannelPtr::qObjectCast(channel);
        if (textChannel) {
            ret << textChannel;
        }
    }
    return ret;
}

void SimpleTextObserver::onNewChannels(const QList<ChannelPtr> &channels)
{
    foreach (const ChannelPtr &channel, channels) {
        TextChannelPtr textChannel = TextChannelPtr::qObjectCast(channel);
        if (!textChannel) {
            if (channel->channelType() != TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
                warning() << "Channel received to observe is not of type Text, service confused. "
                    "Ignoring channel";
            } else {
                warning() << "Channel received to observe is not a subclass of TextChannel. "
                    "ChannelFactory set on this observer's account must construct TextChannel "
                    "subclasses for channels of type Text. Ignoring channel";
            }
            continue;
        }

        if (mPriv->channels.contains(channel)) {
            // we are already observing this channel
            continue;
        }

        Private::TextChannelWrapper *wrapper = new Private::TextChannelWrapper(textChannel);
        mPriv->channels.insert(channel, wrapper);
        connect(wrapper,
                SIGNAL(channelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)),
                SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)));
        connect(wrapper,
                SIGNAL(channelMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)),
                SIGNAL(messageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)));

        foreach (const ReceivedMessage &message, textChannel->messageQueue()) {
            emit messageReceived(message, textChannel);
        }
    }
}

void SimpleTextObserver::onChannelInvalidated(const ChannelPtr &channel)
{
    // it may happen that the channel received in onNewChannels is not a text channel somehow, thus
    // the channel won't be added to mPriv->channels
    if (mPriv->channels.contains(channel)) {
        delete mPriv->channels.take(channel);
    }
}

/**
 * \fn void SimpleTextObserver::messageSent(const Tp::Message &message,
 *                  Tp::MessageSendingFlags flags, const QString &sentMessageToken,
 *                  const Tp::TextChannelPtr &channel);
 *
 * Emitted whenever a text message on account() is sent.
 * If contactIdentifier() is non-empty, only messages sent to the contact identified by it will
 * be signalled.
 *
 * \param message The message sent.
 * \param flags The message flags,
 * \param sentMessageToken The message token.
 * \param channel The channel which received the message.
 */

/**
 * \fn void SimpleTextObserver::messageReceived(const Tp::ReceivedMessage &message,
 *                  const Tp::TextChannelPtr &channel);
 *
 * Emitted whenever a text message on account() is received.
 * If contactIdentifier() is non-empty, only messages received by the contact identified by it will
 * be signalled.
 *
 * \param message The message received.
 * \param channel The channel which received the message.
 */

} // Tp
