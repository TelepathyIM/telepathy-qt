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

#ifndef _TelepathyQt4_text_channel_h_HEADER_GUARD_
#define _TelepathyQt4_text_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/PendingOperation>

namespace Tp
{

class PendingReadyChannel;
class Message;
class ReceivedMessage;
class TextChannel;

class PendingSendMessage : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingSendMessage)

public:
    PendingSendMessage(const Message &message, QObject *parent = 0);
    ~PendingSendMessage();

    QString sentMessageToken() const;
    Message message() const;

private Q_SLOTS:
    void onTextSent(QDBusPendingCallWatcher *);
    void onMessageSent(QDBusPendingCallWatcher *);

private:
    friend class TextChannel;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TextChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(TextChannel)

public:
    static const Feature FeatureMessageQueue;
    static const Feature FeatureMessageCapabilities;
    static const Feature FeatureMessageSentSignal;

    static TextChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~TextChannel();

    bool hasMessagesInterface() const;
    bool canInviteContacts() const;

    // requires FeatureMessageCapabilities
    QStringList supportedContentTypes() const;
    MessagePartSupportFlags messagePartSupport() const;
    DeliveryReportingSupportFlags deliveryReportingSupport() const;

    // requires FeatureMessageQueue
    QList<ReceivedMessage> messageQueue() const;

public Q_SLOTS:
    void acknowledge(const QList<ReceivedMessage> &messages);

    void forget(const QList<ReceivedMessage> &messages);

    PendingSendMessage *send(const QString &text,
            ChannelTextMessageType type = ChannelTextMessageTypeNormal);

    PendingSendMessage *send(const MessagePartList &parts);

    inline PendingOperation *inviteContacts(
            const QList<ContactPtr> &contacts,
            const QString &message = QString())
    {
        return groupAddContacts(contacts, message);
    }

Q_SIGNALS:
    // FeatureMessageSentSignal
    void messageSent(const Tp::Message &message,
            Tp::MessageSendingFlags flags,
            const QString &sentMessageToken);

    // FeatureMessageQueue
    void messageReceived(const Tp::ReceivedMessage &message);
    void pendingMessageRemoved(
            const Tp::ReceivedMessage &message);

protected:
    TextChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties);

private Q_SLOTS:
    void onContactsFinished(Tp::PendingOperation *);
    void onAcknowledgePendingMessagesReply(QDBusPendingCallWatcher *);

    void onMessageSent(const Tp::MessagePartList &, uint,
            const QString &);
    void onMessageReceived(const Tp::MessagePartList &);
    void onPendingMessagesRemoved(const Tp::UIntList &);

    void onTextSent(uint, uint, const QString &);
    void onTextReceived(uint, uint, uint, uint, uint, const QString &);
    void onTextSendError(uint, uint, uint, const QString &);

    void gotProperties(QDBusPendingCallWatcher *);
    void gotPendingMessages(QDBusPendingCallWatcher *);

private:
    void processQueue();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
