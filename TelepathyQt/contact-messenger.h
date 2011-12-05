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

#ifndef _TelepathyQt_contact_messenger_h_HEADER_GUARD_
#define _TelepathyQt_contact_messenger_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/Message>
#include <TelepathyQt/Types>

namespace Tp
{

class PendingSendMessage;
class MessageContentPartList;

class TP_QT_EXPORT ContactMessenger  : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ContactMessenger)

public:
    static ContactMessengerPtr create(const AccountPtr &account, const ContactPtr &contact);
    static ContactMessengerPtr create(const AccountPtr &account, const QString &contactIdentifier);

    virtual ~ContactMessenger();

    AccountPtr account() const;
    QString contactIdentifier() const;

    QList<TextChannelPtr> textChats() const;

    PendingSendMessage *sendMessage(const QString &text,
            ChannelTextMessageType type = ChannelTextMessageTypeNormal,
            MessageSendingFlags flags = 0);
    PendingSendMessage *sendMessage(const MessageContentPartList &parts,
            MessageSendingFlags flags = 0);

Q_SIGNALS:
    void messageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void messageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);

private:
    TP_QT_NO_EXPORT ContactMessenger(const AccountPtr &account,
            const QString &contactIdentifier);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
