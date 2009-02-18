/* Text channel client-side proxy
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

#ifndef _TelepathyQt4_Client_text_channel_h_HEADER_GUARD_
#define _TelepathyQt4_Client_text_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Client/Channel>

namespace Telepathy
{
namespace Client
{

class TextChannel;
class PendingReadyChannel;

class Message
{
public:

#if 0
    // shared data, like QString
    Message(const Message &other);
    Message &operator=(const Message &other);
    ~Message();

    // Convenient access to headers

    // QDateTime() if unknown (e.g. on Text interface)
    QDateTime sent() const; // QDateTime() if unknown

    // always known (the default is ChannelTextMessageTypeNormal)
    ChannelTextMessageType messageType() const;

    // for Text, Channel_Text_Message_Flag_Truncated; for Messages, at least
    // one part has { 'truncated': True }
    bool isTruncated() const;

    // for Text, Channel_Text_Message_Flag_Non_Text_Content; for Messages,
    // at least one part has no text/plain alternative
    bool hasNonTextContent() const;

    // QString() if there is no token
    QString messageToken() const;

    // false for most messages. If true, and a client doesn't understand
    // the particular message, it should not display it (but it should still
    // acknowledge it, if appropriate)
    bool isSpecificToDBusInterface() const;
    // QString() if isSpecificToDBusInterface() returns false,
    // an interface name if it returns true
    QString dbusInterface() const;

    // Basic access to body
    // For Text: the text
    // For Messages: all text parts except lower-priority alternatives,
    // concatenated (see telepathy-glib's TpMessageMixin for a good algorithm)
    QString text() const;

    // Direct access to the whole message (header and body)

    inline const QVariantMap &header() const
    {
        return messagePart(0);
    }

    // Part 0 is the header, which has a different interpretation
    const QVariantMap &messagePart(uint index) const;
#endif

protected:
    friend class TextChannel;
#if 0
    Message(QList<QVariantMap> parts);
#endif

    struct Private;
    friend struct Private;
    QSharedPointer<Private> mPriv;
};

class ReceivedMessage : public Message
{
public:
#if 0
    // shared data, like QString
    ReceivedMessage(const ReceivedMessage &other);
    ReceivedMessage &operator=(const ReceivedMessage &other);

    // should always be known - use QDateTime::currentDateTime() while
    // initially parsing the incoming message, if necessary
    QDateTime received() const;

    // QSharedPointer<Contact>(null) if unknown
    QSharedPointer<Contact> sender() const;

    // The incoming message was part of a replay of message history
    //
    // This is indicated so that loggers can use better heuristics when
    // eliminating duplicates (a simple implementation would be to not log
    // these messages at all).
    bool isScrollback() const;

    // The incoming message has been seen in a previous channel during the
    // lifetime of the Connection, but had not been acknowledged when that
    // channel closed, causing an identical channel (the channel in which
    // the message now appears) to open
    //
    // practical effect: loggers shouldn't log it again
    bool isRescued() const;
#endif

private:
    friend class TextChannel;
#if 0
    ReceivedMessage(QList<QVariantMap> parts, const TextChannel *channel);
#endif
};

class TextChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(TextChannel)

public:
    TextChannel(Connection *connection, const QString &objectPath,
            const QVariantMap &immutableProperties, QObject *parent = 0);
    ~TextChannel();

    enum Feature {
        // FeatureMessageQueue guarantees that incoming messages go in
        // messageQueue when their corresponding contact objects have been
        // created, and that messageReceived will be emitted. If not enabled,
        // messageReceived will not be emitted.
        FeatureMessageQueue = 1,
        FeatureMessageCapabilities = 2,
        _Padding = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(Features, Feature)

    bool hasMessagesInterface() const;
    bool canInviteContacts() const;

    bool isReady(Channel::Features channelFeatures = 0,
            Features textFeatures = 0) const;
    PendingReadyChannel *becomeReady(Channel::Features channelFeatures = 0,
            Features textFeatures = 0);

    // requires FeatureMessageCapabilities
    QStringList supportedContentTypes() const;
    MessagePartSupportFlags messagePartSupport() const;
    DeliveryReportingSupportFlags deliveryReportingSupport() const;

#if 0
    // requires FeatureMessageQueue
    QList<ReceivedMessage> messageQueue() const;
#endif

#if 0
public Q_SLOTS:

    // Asynchronously acknowledge incoming messages, if not already acked, and
    // remove them from messageQueue.
    //
    // Docs must say that only the primary channel handler can do this.
    //
    // I don't think we need to bother with a PendingOperation here.
    //
    // Implementation: on InvalidArgument, the QList form should fall back to
    // acknowledging each message individually
    //
    // The messages must have come from this channel, therefore this makes no
    // sense if FeatureMessageQueue isn't enabled.
    void acknowledge(ReceivedMessage message);
    void acknowledge(QList<ReceivedMessage> messages);

    // Remove the objects representing incoming messages from messageQueue,
    // without acknowledging (appropriate for Observers like loggers)
    //
    // The messages must have come from this channel, therefore this makes
    // no sense if FeatureMessageQueue isn't enabled.
    void forget(ReceivedMessage message);
    void forget(QList<ReceivedMessage> messages);

    // Returns a sent-message token (as in messageSent), or "".
    QString send(const QString &text,
            ChannelTextMessageType type = ChannelTextMessageTypeNormal);

    // For advanced users.
    //
    // Returns a sent-message token (as in messageSent), or "".
    QString send(QList<QVariantMap> message);

    inline PendingOperation *inviteContacts(
            const QList<QSharedPointer<Contact> > &contacts,
            const QString &message = QString())
    {
        return groupAddContacts(contacts, message);
    }
#endif

#if 0
Q_SIGNALS:

    // If hasMessagesInterface() is false, then flags is always 0 and
    // sentMessageToken is always "".
    //
    // If you miss the signal, it's gone forever - there's no queue of these.
    // You might get a delivery report later, though.
    //
    // Always emitted.
    void messageSent(Message message, MessageSendingFlags flags,
            const QString &sentMessageToken);

    // Change notification for messageQueue()
    //
    // Only emitted when FeatureMessageQueue is enabled.
    void messageReceived(ReceivedMessage message);

    // Change notification for messageQueue()
    //
    // Only emitted when FeatureMessageQueue is enabled and
    // hasMessagesInterface() returns true.
    void pendingMessagesRemoved(QList<ReceivedMessage> messages);
#endif

private Q_SLOTS:
    void onChannelReady(Telepathy::Client::PendingOperation *);

    void onMessageSent(const Telepathy::MessagePartList &, uint,
            const QString &);
    void onMessageReceived(const Telepathy::MessagePartList &);
    void onPendingMessagesRemoved(const Telepathy::UIntList &);
    void onGetAllMessagesReply(QDBusPendingCallWatcher *);

    void onTextSent(uint, uint, const QString &);
    void onTextReceived(uint, uint, uint, uint, uint, const QString &);
    void onTextSendError(uint, uint, uint, const QString &);
    void onListPendingMessagesReply(QDBusPendingCallWatcher *);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
    // Implementation: messageQueue should probably be implemented as a
    // QMap<uint,ReceivedMessage>
};

} // Telepathy::Client
} // Telepathy

#endif
