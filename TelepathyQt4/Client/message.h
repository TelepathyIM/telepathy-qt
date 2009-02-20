/* Message object used by text channel client-side proxy
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

#ifndef _TelepathyQt4_Client_message_h_HEADER_GUARD_
#define _TelepathyQt4_Client_message_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QSharedDataPointer>

namespace Telepathy
{
namespace Client
{

class Message
{
public:

    Message(const Message &other);
    Message &operator=(const Message &other);
    bool operator==(const Message &other) const;
    ~Message();

    // Convenient access to headers

    QDateTime sent() const;

    ChannelTextMessageType messageType() const;

    bool isTruncated() const;

    bool hasNonTextContent() const;

    QString messageToken() const;

    bool isSpecificToDBusInterface() const;
    QString dbusInterface() const;

    QString text() const;

    // Direct access to the whole message (header and body)

    MessagePart header() const;

    int size() const;
    MessagePart part(uint index) const;

private:
    friend class TextChannel;
    friend class ReceivedMessage;

    Message();
    Message(const MessagePartList &parts);
    Message(uint, uint, const QString &);

    class Private;
    QSharedDataPointer<Private> mPriv;
};

class ReceivedMessage : public Message
{
public:
    ReceivedMessage(const ReceivedMessage &other);
    ReceivedMessage &operator=(const ReceivedMessage &other);
    ~ReceivedMessage();

    QDateTime received() const;
    QSharedPointer<Contact> sender() const;
    bool isScrollback() const;
    bool isRescued() const;

private:
    friend class TextChannel;
    ReceivedMessage(const MessagePartList &parts, const TextChannel *channel);
    ReceivedMessage();
};

} // Telepathy::Client
} // Telepathy

#endif
