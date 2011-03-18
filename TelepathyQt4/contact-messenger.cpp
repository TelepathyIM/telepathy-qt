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
            SLOT(onMessageSent(QDBusPendingCallWatcher*)));
    return op;
}

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

ContactMessenger::~ContactMessenger()
{
    delete mPriv;
}

AccountPtr ContactMessenger::account() const
{
    return mPriv->account;
}

QString ContactMessenger::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

PendingSendMessage *ContactMessenger::sendMessage(const QString &text,
        ChannelTextMessageType type,
        MessageSendingFlags flags)
{
    Message message(type, text);
    return mPriv->sendMessage(message, flags);
}

PendingSendMessage *ContactMessenger::sendMessage(const MessageContentPartList &parts,
        MessageSendingFlags flags)
{
    Message message(parts.bareParts());
    return mPriv->sendMessage(message, flags);
}

} // Tp
