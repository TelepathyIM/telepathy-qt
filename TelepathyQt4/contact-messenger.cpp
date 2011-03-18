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

#include <TelepathyQt4/ContactMessenger>

#include "TelepathyQt4/_gen/contact-messenger.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include "TelepathyQt4/future-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/ChannelDispatcher>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/MessageContentPartList>
#include <TelepathyQt4/PendingSendMessage>
#include <TelepathyQt4/SimpleTextObserver>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ContactMessenger::Private
{
    Private(ContactMessenger *parent, const AccountPtr &account, const QString &contactIdentifier)
        : parent(parent),
          account(account),
          contactIdentifier(contactIdentifier),
          cdMessagesInterface(0)
    {
    }

    PendingSendMessage *sendMessage(const Message &message, MessageSendingFlags flags);

    ContactMessenger *parent;
    AccountPtr account;
    QString contactIdentifier;
    SimpleTextObserverPtr observer;
    TpFuture::Client::ChannelDispatcherInterfaceMessagesInterface *cdMessagesInterface;
};

PendingSendMessage *ContactMessenger::Private::sendMessage(const Message &message,
        MessageSendingFlags flags)
{
    if (!cdMessagesInterface) {
        cdMessagesInterface = new TpFuture::Client::ChannelDispatcherInterfaceMessagesInterface(
                account->dbusConnection(),
                TP_QT4_CHANNEL_DISPATCHER_BUS_NAME, TP_QT4_CHANNEL_DISPATCHER_OBJECT_PATH, parent);
    }

    PendingSendMessage *op = new PendingSendMessage(ContactMessengerPtr(parent), message);

    // TODO: is there a way to avoid this? Ideally TpFuture classes should use Tp types.
    TpFuture::MessagePartList parts;
    foreach (const Tp::MessagePart &part, message.parts()) {
        parts << static_cast<QMap<QString, QDBusVariant> >(part);
    }

    connect(new QDBusPendingCallWatcher(
                cdMessagesInterface->SendMessage(QDBusObjectPath(account->objectPath()),
                    contactIdentifier, parts, (uint) flags)),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            op,
            SLOT(onCDMessageSent(QDBusPendingCallWatcher*)));
    return op;
}

/**
 * \class ContactMessenger
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/contact-messenger.h <TelepathyQt4/ContactMessenger>
 *
 * \brief The ContactMessenger class provides an easy way to send text messages to a contact
 *        and also track sent/receive text messages from the same contact.
 */

/**
 * Create a new ContactMessenger object.
 *
 * \param account The account this messenger is communicating with.
 * \param contact The contact this messenger is communicating with.
 * \return An ContactMessengerPtr object pointing to the newly created ContactMessenger object,
 *         or a null ContactMessengerPtr if \a contact is null.
 */
ContactMessengerPtr ContactMessenger::create(const AccountPtr &account,
        const ContactPtr &contact)
{
    if (!contact) {
        warning() << "Contact used to create a ContactMessenger object must be "
            "valid";
        return ContactMessengerPtr();
    }
    return ContactMessengerPtr(new ContactMessenger(account, contact->id()));
}

/**
 * Create a new ContactMessenger object.
 *
 * \param account The account this messenger is communicating with.
 * \param contactIdentifier The identifier of the contact this messenger is communicating with.
 * \return An ContactMessengerPtr object pointing to the newly created ContactMessenger object,
 *         or a null ContactMessengerPtr if \a contact is null.
 */
ContactMessengerPtr ContactMessenger::create(const AccountPtr &account,
        const QString &contactIdentifier)
{
    if (contactIdentifier.isEmpty()) {
        warning() << "Contact identifier used to create a ContactMessenger object must be "
            "non-empty";
        return ContactMessengerPtr();
    }
    return ContactMessengerPtr(new ContactMessenger(account, contactIdentifier));
}

/**
 * Construct a new ContactMessenger object.
 *
 * \param account The account this messenger is communicating with.
 * \param contactIdentifier The identifier of the contact this messenger is communicating with.
 */
ContactMessenger::ContactMessenger(const AccountPtr &account, const QString &contactIdentifier)
    : mPriv(new Private(this, account, contactIdentifier))
{
    mPriv->observer = SimpleTextObserver::create(account, contactIdentifier);
    connect(mPriv->observer.data(),
            SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)),
            SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)));
    connect(mPriv->observer.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)),
            SIGNAL(messageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)));
}

/**
 * Class destructor.
 */
ContactMessenger::~ContactMessenger()
{
    delete mPriv;
}

/**
 * Return the account this messenger is communicating with.
 *
 * \return The account this messenger is communicating with.
 */
AccountPtr ContactMessenger::account() const
{
    return mPriv->account;
}

/**
 * Return the identifier of the contact this messenger is communicating with.
 *
 * \return The identifier of the contact this messenger is communicating with.
 */
QString ContactMessenger::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

/**
 * Send a message to the contact identified by contactIdentifier() using account().
 *
 * Note that the return from this method isn't ordered in any sane way, meaning that
 * messageSent() can be signalled either before or after the returned PendingSendMessage object
 * finishes.
 *
 * \param text The message text.
 * \param type The message type.
 * \param flags The message flags.
 * \return A PendingSendMessage which will emit PendingSendMessage::finished
 *         once the reply is received and that can be used to check whether sending the
 *         message succeeded or not.
 */
PendingSendMessage *ContactMessenger::sendMessage(const QString &text,
        ChannelTextMessageType type,
        MessageSendingFlags flags)
{
    Message message(type, text);
    return mPriv->sendMessage(message, flags);
}

/**
 * Send a message to the contact identified by contactIdentifier() using account().
 *
 * Note that the return from this method isn't ordered in any sane way, meaning that
 * messageSent() can be signalled either before or after the returned PendingSendMessage object
 * finishes.
 *
 * \param parts The message parts.
 * \param flags The message flags.
 * \return A PendingSendMessage which will emit PendingSendMessage::finished
 *         once the reply is received and that can be used to check whether sending the
 *         message succeeded or not.
 */
PendingSendMessage *ContactMessenger::sendMessage(const MessageContentPartList &parts,
        MessageSendingFlags flags)
{
    Message message(parts.bareParts());
    return mPriv->sendMessage(message, flags);
}

/**
 * \fn void ContactMessenger::messageSent(const Tp::Message &message,
 *                  Tp::MessageSendingFlags flags, const QString &sentMessageToken,
 *                  const Tp::TextChannelPtr &channel);
 *
 * This signal is emitted whenever a text message on account() is sent to the contact
 * identified by contactIdentifier().
 *
 * \param message The message sent.
 * \param flags The flags of the message that was sent.
 * \param sentMessageToken The token of the message that was sent.
 * \param channel The channel from which the message was sent.
 */

/**
 * \fn void ContactMessenger::messageReceived(const Tp::ReceivedMessage &message,
 *                  const Tp::TextChannelPtr &channel);
 *
 * This signal is emitted whenever a text message on account() is received from the contact
 * identified by contactIdentifier().
 *
 * \param message The message received.
 * \param channel The channel from which the message was received.
 */

} // Tp
