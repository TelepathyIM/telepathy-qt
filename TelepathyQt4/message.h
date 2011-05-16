/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_message_h_HEADER_GUARD_
#define _TelepathyQt4_message_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QSharedDataPointer>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Types>

class QDateTime;

namespace Tp
{

class Contact;
class TextChannel;

class TELEPATHY_QT4_EXPORT Message
{
public:
    Message(ChannelTextMessageType, const QString &);
    Message(const Message &other);
    ~Message();

    Message &operator=(const Message &other);
    bool operator==(const Message &other) const;
    inline bool operator!=(const Message &other) const
    {
        return !(*this == other);
    }

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
    MessagePartList parts() const;

private:
    friend class ContactMessenger;
    friend class ReceivedMessage;
    friend class TextChannel;

    TELEPATHY_QT4_NO_EXPORT Message();
    TELEPATHY_QT4_NO_EXPORT Message(const MessagePartList &parts);
    TELEPATHY_QT4_NO_EXPORT Message(uint, uint, const QString &);

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TELEPATHY_QT4_EXPORT ReceivedMessage : public Message
{
public:
    class DeliveryDetails
    {
    public:
        DeliveryDetails();
        DeliveryDetails(const DeliveryDetails &other);
        ~DeliveryDetails();

        DeliveryDetails &operator=(const DeliveryDetails &other);

        bool isValid() const { return mPriv.constData() != 0; }

        DeliveryStatus status() const;

        bool hasOriginalToken() const;
        QString originalToken() const;

        bool isError() const;
        ChannelTextSendError error() const;

        bool hasErrorMessage() const;
        QString errorMessage() const;

        QString dbusError() const;

        bool hasEchoedMessage() const;
        Message echoedMessage() const;

    private:
        friend class ReceivedMessage;

        TELEPATHY_QT4_NO_EXPORT DeliveryDetails(const MessagePartList &parts);

        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    ReceivedMessage(const ReceivedMessage &other);
    ReceivedMessage &operator=(const ReceivedMessage &other);
    ~ReceivedMessage();

    QDateTime received() const;
    ContactPtr sender() const;
    QString senderNickname() const;

    QString supersedes() const;

    bool isScrollback() const;
    bool isRescued() const;

    bool isDeliveryReport() const;
    DeliveryDetails deliveryDetails() const;

    bool isFromChannel(const TextChannelPtr &channel) const;

private:
    friend class TextChannel;

    TELEPATHY_QT4_NO_EXPORT ReceivedMessage(const MessagePartList &parts,
            const TextChannelPtr &channel);
    TELEPATHY_QT4_NO_EXPORT ReceivedMessage();

    TELEPATHY_QT4_NO_EXPORT uint senderHandle() const;
    TELEPATHY_QT4_NO_EXPORT QString senderId() const;
    TELEPATHY_QT4_NO_EXPORT uint pendingId() const;

    TELEPATHY_QT4_NO_EXPORT void setForceNonText();
    TELEPATHY_QT4_NO_EXPORT void clearSenderHandle();
    TELEPATHY_QT4_NO_EXPORT void setSender(const ContactPtr &sender);
};

} // Tp

#endif
