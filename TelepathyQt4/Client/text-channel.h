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

#include <QSharedDataPointer>

#include <TelepathyQt4/Client/Channel>

namespace Telepathy
{
namespace Client
{

class TextChannel;
class PendingReadyChannel;
class Message;
class ReceivedMessage;

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
        FeatureMessageSentSignal = 4,
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

    // requires FeatureMessageQueue
    QList<ReceivedMessage> messageQueue() const;

public Q_SLOTS:

    void acknowledge(const QList<ReceivedMessage> &messages);

    void forget(const QList<ReceivedMessage> &messages);

#if 0
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

Q_SIGNALS:

    // FeatureMessageSentSignal
    void messageSent(const Telepathy::Client::Message &message,
            Telepathy::MessageSendingFlags flags,
            const QString &sentMessageToken);

    // FeatureMessageQueue
    void messageReceived(const Telepathy::Client::ReceivedMessage &message);
    void pendingMessageRemoved(
            const Telepathy::Client::ReceivedMessage &message);

private Q_SLOTS:
    void onChannelReady(Telepathy::Client::PendingOperation *);
    void onContactsFinished(Telepathy::Client::PendingOperation *);
    void onAcknowledgePendingMessagesReply(QDBusPendingCallWatcher *);

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
    void processQueue();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
