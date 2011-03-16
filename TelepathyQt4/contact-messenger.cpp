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

#include <TelepathyQt4/Account>
#include <TelepathyQt4/PendingSendMessage>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ContactMessenger::Private
{
    Private(const AccountPtr &account, const QString &contactIdentifier)
        : account(account),
          contactIdentifier(contactIdentifier)
    {
    }

    AccountPtr account;
    QString contactIdentifier;
};

ContactMessengerPtr ContactMessenger::create(const AccountPtr &account,
        const QString &contactIdentifier)
{
    if (contactIdentifier.isEmpty()) {
        return ContactMessengerPtr();
    }
    return ContactMessengerPtr(new ContactMessenger(account, contactIdentifier));
}

ContactMessenger::ContactMessenger(const AccountPtr &account, const QString &contactIdentifier)
    : mPriv(new Private(account, contactIdentifier))
{
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
    return NULL;
}

PendingSendMessage *ContactMessenger::sendMessage(const MessageContentPartList &parts,
        MessageSendingFlags flags)
{
    return NULL;
}

} // Tp
