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

#include <TelepathyQt4/Client/TextChannel>

#include <QDateTime>

#include "TelepathyQt4/Client/_gen/text-channel.moc.hpp"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingReadyChannel>
#include <TelepathyQt4/Client/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

class Message::Private : public QSharedData
{
public:
    Private(const MessagePartList &parts);
    ~Private();

    MessagePartList parts;

    // if the Text interface says "non-text" we still only have the text,
    // because the interface can't tell us anything else...
    bool forceNonText;

    // for received messages only
    const TextChannel *textChannel;
    QSharedPointer<Contact> sender;

    inline QVariant value(uint index, const char *key) const;
    inline uint getUIntOrZero(uint index, const char *key) const;
    inline QString getStringOrEmpty(uint index, const char *key) const;
    inline bool getBoolean(uint index, const char *key,
            bool assumeIfAbsent) const;
    inline uint senderHandle() const;
    inline uint pendingId() const;
    void clearSenderHandle();
};

Message::Private::Private(const MessagePartList &parts)
    : parts(parts),
      forceNonText(false),
      textChannel(0),
      sender(0)
{
}

Message::Private::~Private()
{
}

inline QVariant Message::Private::value(uint index, const char *key) const
{
    return parts.at(index).value(QLatin1String(key)).variant();
}

inline QString Message::Private::getStringOrEmpty(uint index, const char *key)
    const
{
    QString s = value(index, key).toString();
    if (s.isNull()) {
        s = QString::fromAscii("");
    }
    return s;
}

inline uint Message::Private::getUIntOrZero(uint index, const char *key)
    const
{
    return value(index, key).toUInt();
}

inline bool Message::Private::getBoolean(uint index, const char *key,
        bool assumeIfAbsent) const
{
    QVariant v = value(index, key);
    if (v.isValid() && v.type() == QVariant::Bool) {
        return v.toBool();
    }
    return assumeIfAbsent;
}

inline uint Message::Private::senderHandle() const
{
    return getUIntOrZero(0, "message-sender");
}

inline uint Message::Private::pendingId() const
{
    return getUIntOrZero(0, "pending-message-id");
}

void Message::Private::clearSenderHandle()
{
    parts[0].remove(QLatin1String("message-sender"));
}

/**
 * \class Message
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/text-channel.h <TelepathyQt4/Client/TextChannel>
 *
 * Object representing a message. These objects are implicitly shared, like
 * QString.
 */

/**
 * Default constructor, only used internally.
 */
Message::Message()
{
}

/**
 * Constructor.
 *
 * \param parts The parts of a message as defined by the Telepathy D-Bus
 *              specification. This list must have length at least 1.
 */
Message::Message(const MessagePartList &parts)
    : mPriv(new Private(parts))
{
    Q_ASSERT(parts.size() > 0);
}

/**
 * Constructor, from the parameters of the old Sent signal.
 *
 * \param timestamp The time the message was sent
 * \param type The message type
 * \param text The text of the message
 */
Message::Message(uint timestamp, uint type, const QString &text)
    : mPriv(new Private(MessagePartList() << MessagePart() << MessagePart()))
{
    mPriv->parts[0].insert(QString::fromAscii("message-sent"),
            QDBusVariant(static_cast<qlonglong>(timestamp)));
    mPriv->parts[0].insert(QString::fromAscii("message-type"),
            QDBusVariant(type));

    mPriv->parts[1].insert(QString::fromAscii("content-type"),
            QDBusVariant(QString::fromAscii("text/plain")));
    mPriv->parts[1].insert(QString::fromAscii("content"), QDBusVariant(text));
}

/**
 * Copy constructor.
 */
Message::Message(const Message &other)
    : mPriv(other.mPriv)
{
}

/**
 * Assignment operator.
 */
Message &Message::operator=(const Message &other)
{
    if (this != &other) {
        mPriv = other.mPriv;
    }

    return *this;
}

/**
 * Equality operator.
 */
bool Message::operator==(const Message &other) const
{
    return this->mPriv == other.mPriv;
}

/**
 * Destructor.
 */
Message::~Message()
{
}

/**
 * Return the time the message was sent, or QDateTime() if that time is
 * unknown.
 *
 * \return A timestamp
 */
QDateTime Message::sent() const
{
    // FIXME: Telepathy supports 64-bit time_t, but Qt only does so on
    // ILP64 systems (e.g. sparc64, but not x86_64). If QDateTime
    // gains a fromTimestamp64 method, we should use it instead.
    uint stamp = mPriv->value(0, "message-sent").toUInt();
    if (stamp != 0) {
        return QDateTime::fromTime_t(stamp);
    } else {
        return QDateTime();
    }
}

/**
 * Return the type of message this is, or ChannelTextMessageTypeNormal
 * if the type is not recognised.
 *
 * \return The ChannelTextMessageType for this message
 */
ChannelTextMessageType Message::messageType() const
{
    uint raw = mPriv->value(0, "message-type").toUInt();

    if (raw < static_cast<uint>(NUM_CHANNEL_TEXT_MESSAGE_TYPES)) {
        return ChannelTextMessageType(raw);
    } else {
        return ChannelTextMessageTypeNormal;
    }
}

/**
 * Return whether this message was truncated during delivery.
 */
bool Message::isTruncated() const
{
    for (int i = 1; i < size(); i++) {
        if (mPriv->getBoolean(i, "truncated", false)) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether this message contains parts not representable as plain
 * text.
 *
 * \return true if this message cannot completely be represented as plain text
 */
bool Message::hasNonTextContent() const
{
    if (mPriv->forceNonText || size() <= 1 || isSpecificToDBusInterface()) {
        return true;
    }

    QSet<QString> texts;
    QSet<QString> textNeeded;

    for (int i = 1; i < size(); i++) {
        QString altGroup = mPriv->getStringOrEmpty(i, "alternative");
        QString contentType = mPriv->getStringOrEmpty(i, "content-type");

        if (contentType == QLatin1String("text/plain")) {
            if (!altGroup.isEmpty()) {
                // we can use this as an alternative for a non-text part
                // with the same altGroup
                texts << altGroup;
            }
        } else {
            QString alt = mPriv->getStringOrEmpty(i, "alternative");
            if (altGroup.isEmpty()) {
                // we can't possibly rescue this part by using a text/plain
                // alternative, because it's not in any alternative group
                return true;
            } else {
                // maybe we'll find a text/plain alternative for this
                textNeeded << altGroup;
            }
        }
    }

    textNeeded -= texts;
    return !textNeeded.isEmpty();
}

/**
 * Return the unique token identifying this message (e.g. the id attribute
 * for XMPP messages), or an empty string if there is no suitable token.
 *
 * \return A non-empty message identifier, or an empty string if none
 */
QString Message::messageToken() const
{
    return mPriv->getStringOrEmpty(0, "message-token");
}

/**
 * Return whether this message is specific to a D-Bus interface. This is
 * false in almost all cases.
 *
 * If this function returns true, the message is specific to the interface
 * indicated by dbusInterface. Clients that don't understand that interface
 * should not display the message. However, if the client would acknowledge
 * an ordinary message, it must also acknowledge this interface-specific
 * message.
 *
 * \return true if dbusInterface would return a non-empty string
 */
bool Message::isSpecificToDBusInterface() const
{
    return !dbusInterface().isEmpty();
}

/**
 * Return the D-Bus interface to which this message is specific, or an
 * empty string for normal messages.
 */
QString Message::dbusInterface() const
{
    return mPriv->getStringOrEmpty(0, "interface");
}

QString Message::text() const
{
    // Alternative-groups for which we've already emitted an alternative
    QSet<QString> altGroupsUsed;
    QString text;

    for (int i = 1; i < size(); i++) {
        QString altGroup = mPriv->getStringOrEmpty(i, "alternative");
        QString contentType = mPriv->getStringOrEmpty(i, "content-type");

        if (contentType == QLatin1String("text/plain")) {
            if (!altGroup.isEmpty()) {
                if (altGroupsUsed.contains(altGroup)) {
                    continue;
                } else {
                    altGroupsUsed << altGroup;
                }
            }

            QVariant content = mPriv->value(i, "content");
            if (content.type() == QVariant::String) {
                text += content.toString();
            } else {
                // O RLY?
                debug() << "allegedly text/plain part wasn't";
            }
        }
    }

    return text;
}

/**
 * Return the message's header part, as defined by the Telepathy D-Bus API
 * specification. This is provided for advanced clients that need to access
 * additional information not available through the normal Message API.
 *
 * \return The same thing as messagepart(0)
 */
MessagePart Message::header() const
{
    return part(0);
}

/**
 * Return the number of parts in this message.
 *
 * \return 1 greater than the largest valid argument to part
 */
int Message::size() const
{
    return mPriv->parts.size();
}

/**
 * Return the message's header part, as defined by the Telepathy D-Bus API
 * specification. This is provided for advanced clients that need to access
 * additional information not available through the normal Message API.
 *
 * \param index The part to access, which must be strictly less than size();
 *              part number 0 is the header, parts numbered 1 or greater
 *              are the body of the message.
 * \return Part of the message
 */
MessagePart Message::part(uint index) const
{
    return mPriv->parts.at(index);
}

/**
 * \class ReceivedMessage
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/text-channel.h <TelepathyQt4/Client/TextChannel>
 *
 * Subclass of Message, with additional information that's generally only
 * available on received messages.
 */

/**
 * Default constructor, only used internally.
 */
ReceivedMessage::ReceivedMessage()
{
}

/**
 * Constructor.
 *
 * \param parts The parts of a message as defined by the Telepathy D-Bus
 *              specification. This list must have length at least 1.
 */
ReceivedMessage::ReceivedMessage(const MessagePartList &parts,
        const TextChannel *channel)
    : Message(parts)
{
    if (!mPriv->parts[0].contains(QLatin1String("message-received"))) {
        mPriv->parts[0].insert(QLatin1String("message-received"),
                QDBusVariant(static_cast<qlonglong>(
                        QDateTime::currentDateTime().toTime_t())));
    }
    mPriv->textChannel = channel;
}

/**
 * Copy constructor.
 */
ReceivedMessage::ReceivedMessage(const ReceivedMessage &other)
    : Message(other)
{
}

/**
 * Assignment operator.
 */
ReceivedMessage &ReceivedMessage::operator=(const ReceivedMessage &other)
{
    if (this != &other) {
        mPriv = other.mPriv;
    }

    return *this;
}

/**
 * Destructor.
 */
ReceivedMessage::~ReceivedMessage()
{
}

/**
 * Return the time the message was received.
 *
 * \return A timestamp
 */
QDateTime ReceivedMessage::received() const
{
    // FIXME: Telepathy supports 64-bit time_t, but Qt only does so on
    // ILP64 systems (e.g. sparc64, but not x86_64). If QDateTime
    // gains a fromTimestamp64 method, we should use it instead.
    uint stamp = mPriv->value(0, "message-received").toUInt();
    if (stamp != 0) {
        return QDateTime::fromTime_t(stamp);
    } else {
        return QDateTime();
    }
}

/**
 * Return the Contact who sent the message, or
 * QSharedPointer<Contact>(0) if unknown.
 *
 * \return The sender or QSharedPointer<Contact>(0)
 */
QSharedPointer<Contact> ReceivedMessage::sender() const
{
    return mPriv->sender;
}

/**
 * Return whether the incoming message was part of a replay of message
 * history.
 *
 * If true, loggers can use this to improve their heuristics for elimination
 * of duplicate messages (a simple, correct implementation would be to avoid
 * logging any message that has this flag).
 *
 * \return whether the scrollback flag is set
 */
bool ReceivedMessage::isScrollback() const
{
    return mPriv->getBoolean(0, "scrollback", false);
}

/**
 * Return whether the incoming message was seen in a previous channel during
 * the lifetime of this Connection, but was not acknowledged before that
 * chanenl closed, causing the channel in which it now appears to open.
 *
 * If true, loggers should not log this message again.
 *
 * \return whether the rescued flag is set
 */
bool ReceivedMessage::isRescued() const
{
    return mPriv->getBoolean(0, "rescued", false);
}

struct TextChannel::Private
{
    inline Private();
    inline ~Private();

    TextChannel::Features features;
    TextChannel::Features pendingFeatures;
    QList<PendingReadyChannel *> pendingReady;
    void continueReadying(TextChannel *channel);
    void failReadying(TextChannel *channel, const QString &error,
            const QString &message);

    // requires FeatureMessageCapabilities
    QStringList supportedContentTypes;
    MessagePartSupportFlags messagePartSupport;
    DeliveryReportingSupportFlags deliveryReportingSupport;

    // FeatureMessageQueue
    bool connectedMessageQueueSignals;
    bool getAllMessagesInFlight;
    bool listPendingMessagesCalled;
    bool initialMessagesReceived;
    struct QueuedEvent
    {
        inline QueuedEvent(const ReceivedMessage &message)
            : isMessage(true), message(message),
                removed(0)
        { }
        inline QueuedEvent(uint removed)
            : isMessage(false), message(), removed(removed)
        { }

        bool isMessage;
        ReceivedMessage message;
        uint removed;
    };
    QList<ReceivedMessage> messages;
    QList<QueuedEvent *> incompleteMessages;
    QSet<uint> awaitingContacts;
    QHash<QDBusPendingCallWatcher *, UIntList> acknowledgeBatches;
    void contactLost(uint handle);
    void contactFound(QSharedPointer<Contact> contact);
};

TextChannel::Private::Private()
    : features(0),
      pendingFeatures(0),
      pendingReady(),
      supportedContentTypes(),
      messagePartSupport(0),
      deliveryReportingSupport(0),
      connectedMessageQueueSignals(false),
      getAllMessagesInFlight(false),
      listPendingMessagesCalled(false),
      initialMessagesReceived(false),
      messages(),
      incompleteMessages(),
      awaitingContacts()
{
}

TextChannel::Private::~Private()
{
}

/**
 * \class TextChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/text-channel.h <TelepathyQt4/Client/TextChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the Text
 * channel type.
 *
 * This subclass of Channel will eventually provide a high-level API for the
 * Text and Messages interface. Until then, it's just a Channel.
 */

/**
 * \enum TextChannel::Features
 *
 * Features that can be enabled on a TextChannel using becomeReady().
 *
 * \value FeatureMessageQueue The messageQueue method can be called, and the
 *                            messageReceived and pendingMessageRemoved methods
 *                            can be called
 * \value FeatureMessageCapabilities The supportedContentTypes,
 *                                   messagePartSupport and
 *                                   deliveryReportingSupport methods can
 *                                   be called
 * \value FeatureMessageSentSignal The messageSent signal will be emitted
 *                                 when a message is sent
 */

/**
 * \fn void messageSent(Telepathy::Client::Message message,
 *                      Telepathy::MessageSendingFlags flags,
 *                      const QString &sentMessageToken)
 *
 * Emitted when a message is sent, if the FeatureMessageSentSignal Feature
 * has been enabled.
 *
 * This signal is emitted regardless of whether the message is sent by this
 * client, or another client using the same Channel via D-Bus.
 *
 * \param message A message. This may differ slightly from what the client
 *                requested to send, for instance if it has been altered due
 *                to limitations of the instant messaging protocol used.
 * \param flags MessageSendingFlags that were in effect when the message was
 *              sent. Clients can use these in conjunction with
 *              deliveryReportingSupport to determine whether delivery
 *              reporting can be expected.
 * \param sentMessageToken Either an empty QString, or an opaque token used
 *                         to match the message to any delivery reports.
 */

/**
 * \fn void messageReceived(const Telepathy::Client::ReceivedMessage &message)
 *
 * Emitted when a message is added to messageQueue(), if the
 * FeatureMessageQueue Feature has been enabled.
 *
 * This occurs slightly later than the message being received over D-Bus;
 * see messageQueue() for details.
 */

/**
 * \fn void pendingMessageRemoved(
 *      const Telepathy::Client::ReceivedMessage &message)
 *
 * Emitted when a message is removed from messageQueue(), if the
 * FeatureMessageQueue Feature has been enabled. See messageQueue() for the
 * circumstances in which this happens.
 */

/**
 * Creates a TextChannel associated with the given object on the same service
 * as the given connection.
 *
 * \param connection  Connection owning this TextChannel, and specifying the
 *                    service.
 * \param objectPath  Path to the object on the service.
 * \param parent      Passed to the parent class constructor.
 */
TextChannel::TextChannel(Connection *connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        QObject *parent)
    : Channel(connection, objectPath, immutableProperties, parent),
      mPriv(new Private())
{
}

/**
 * Class destructor.
 */
TextChannel::~TextChannel()
{
    delete mPriv;
}

/**
 * Return whether this channel supports the Telepathy Messages interface.
 * If it does not, some advanced functionality will be unavailable.
 *
 * The result of calling this method is undefined until basic Channel
 * functionality has been enabled by calling becomeReady and waiting for the
 * pending operation to complete.
 *
 * \return true if the Messages interface is supported
 */
bool TextChannel::hasMessagesInterface() const
{
    return interfaces().contains(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES));
}

/**
 * Return whether contacts can be invited into this channel using
 * inviteContacts (which is equivalent to groupAddContacts). Whether this
 * is the case depends on the underlying protocol, the type of channel,
 * and the user's privileges (in some chatrooms, only a privileged user
 * can invite other contacts).
 *
 * This is an alias for groupCanAddContacts, to indicate its meaning more
 * clearly for Text channels.
 *
 * The result of calling this method is undefined until basic Group
 * functionality has been enabled by calling becomeReady and waiting for the
 * pending operation to complete.
 *
 * \return The same thing as groupCanAddContacts
 */
bool TextChannel::canInviteContacts() const
{
    return groupCanAddContacts();
}

/* <!--x--> in the block below is used to escape the star-slash sequence */
/**
 * Return a list of supported MIME content types for messages on this channel.
 * For a simple text channel this will be a list containing one item,
 * "text/plain".
 *
 * This list may contain the special value "*<!--x-->/<!--x-->*", which
 * indicates that any content type is supported.
 *
 * The result of calling this method is undefined until the
 * FeatureMessageCapabilities Feature has been enabled, by calling becomeReady
 * and waiting for the pending operation to complete
 *
 * \return A list of MIME content types
 */
QStringList TextChannel::supportedContentTypes() const
{
    return mPriv->supportedContentTypes;
}

/**
 * Return a set of flags indicating support for multi-part messages on this
 * channel. This is zero on simple text channels, or greater than zero if
 * there is partial or full support for multi-part messages.
 *
 * The result of calling this method is undefined until the
 * FeatureMessageCapabilities Feature has been enabled, by calling becomeReady
 * and waiting for the pending operation to complete.
 *
 * \return A set of MessagePartSupportFlags
 */
MessagePartSupportFlags TextChannel::messagePartSupport() const
{
    return mPriv->messagePartSupport;
}

/**
 * Return a set of flags indicating support for delivery reporting on this
 * channel. This is zero if there are no particular guarantees, or greater
 * than zero if delivery reports can be expected under certain circumstances.
 *
 * The result of calling this method is undefined until the
 * FeatureMessageCapabilities Feature has been enabled, by calling becomeReady
 * and waiting for the pending operation to complete.
 *
 * \return A set of DeliveryReportingSupportFlags
 */
DeliveryReportingSupportFlags TextChannel::deliveryReportingSupport() const
{
    return mPriv->deliveryReportingSupport;
}

/**
 * Return a list of messages received in this channel. This list is empty
 * unless the FeatureMessageQueue Feature has been enabled.
 *
 * Messages are added to this list when they are received from the instant
 * messaging service; the messageReceived signal is emitted.
 *
 * There is a small delay between the message being received over D-Bus and
 * becoming available to users of this C++ API, since a small amount of
 * additional information needs to be fetched. However, the relative ordering
 * of all the messages in a channel is preserved.
 *
 * Messages are removed from this list when they are acknowledged with the
 * acknowledge() or forget() methods. On channels where hasMessagesInterface()
 * returns true, they will also be removed when acknowledged by a different
 * client. In either case, the pendingMessageRemoved signal is emitted.
 *
 * \return The unacknowledged messages in this channel, excluding any that
 *         have been forgotten with forget().
 */
QList<ReceivedMessage> TextChannel::messageQueue() const
{
    return mPriv->messages;
}

void TextChannel::onAcknowledgePendingMessagesReply(
        QDBusPendingCallWatcher *watcher)
{
    UIntList ids = mPriv->acknowledgeBatches.value(watcher);
    QDBusPendingReply<> reply = *watcher;

    if (reply.isError()) {
        // One of the IDs was bad, and we can't know which one. Recover by
        // doing as much as possible, and hope for the best...
        debug() << "Recovering from AcknowledgePendingMessages failure for: "
            << ids;
        foreach (uint id, ids) {
            textInterface()->AcknowledgePendingMessages(UIntList() << id);
        }
    }

    mPriv->acknowledgeBatches.remove(watcher);
    watcher->deleteLater();
}

/**
 * Acknowledge that received messages have been displayed to the user.
 *
 * This method should only be called by the main handler of a Channel, usually
 * meaning the user interface process that displays the Channel to the user
 * (when a ChannelDispatcher is used, the Handler must acknowledge messages,
 * and other Approvers or Observers must not acknowledge messages).
 *
 * Processes other than the main handler of a Channel can free memory used
 * in Telepathy-Qt4 by calling forget() instead.
 *
 * The messages must have come from this channel, therefore this method does
 * not make sense if FeatureMessageQueue has not been enabled.
 *
 * \param messages A list of received messages that have now been displayed.
 */
void TextChannel::acknowledge(const QList<ReceivedMessage> &messages)
{
    UIntList ids;

    foreach (const ReceivedMessage &m, messages) {
        if (m.mPriv->textChannel != this) {
            warning() << "message did not come from this channel, ignoring";
        } else {
            ids << m.mPriv->getUIntOrZero(0, "pending-message-id");
        }
    }

    if (ids.isEmpty()) {
        return;
    }

    // we're going to acknowledge these messages (or as many as possible, if
    // we lose a race with another acknowledging process), so let's remove
    // them from the list immediately
    forget(messages);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            textInterface()->AcknowledgePendingMessages(ids));
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onAcknowledgePendingMessagesReply(QDBusPendingCallWatcher *)));
    mPriv->acknowledgeBatches[watcher] = ids;
}

/**
 * Remove messages from messageQueue without acknowledging them.
 *
 * This method frees memory inside the Telepathy-Qt4 TextChannel proxy, but
 * does not free the corresponding memory in the Connection Manager process.
 * It should be used by clients that are not the main handler for a Channel;
 * the main handler for a Channel should use acknowledge instead.
 *
 * The messages must have come from this channel, therefore this method does
 * not make sense if FeatureMessageQueue has not been enabled.
 *
 * \param messages A list of received messages that have now been processed.
 */
void TextChannel::forget(const QList<ReceivedMessage> &messages)
{
    foreach (const ReceivedMessage &m, messages) {
        if (m.mPriv->textChannel != this) {
            warning() << "message did not come from this channel, ignoring";
        } else if (mPriv->messages.removeOne(m)) {
            emit pendingMessageRemoved(m);
        }
    }
}

/**
 * Return whether the desired features are ready for use.
 *
 * \param channelFeatures Features of the Channel class
 * \param textFeatures Features of the TextChannel class
 * \return true if basic Channel functionality, and all the requested features
 *         (if any), are ready for use
 */
bool TextChannel::isReady(Channel::Features channelFeatures,
        Features textFeatures) const
{
    debug() << "Checking whether ready:" << channelFeatures << textFeatures;
    debug() << "Am I ready? mPriv->features =" << mPriv->features;
    return Channel::isReady(channelFeatures) &&
        ((mPriv->features & textFeatures) == textFeatures);
}

/**
 * Gather the necessary information to use the requested features.
 *
 * \param channelFeatures Features of the Channel class
 * \param textFeatures Features of the TextChannel class
 * \return A pending operation which will finish when basic Channel
 *         functionality, and all the requested features (if any), are ready
 *         for use
 */
PendingReadyChannel *TextChannel::becomeReady(
        Channel::Features channelFeatures,
        Features textFeatures)
{
    PendingReadyChannel *textReady;

    if (!isValid()) {
        textReady = new PendingReadyChannel(channelFeatures, this);
        textReady->setFinishedWithError(invalidationReason(),
                invalidationMessage());
        return textReady;
    }

    if (isReady(channelFeatures, textFeatures)) {
        textReady = new PendingReadyChannel(channelFeatures, this);
        textReady->setFinished();
        return textReady;
    }

    if ((textFeatures & (FeatureMessageQueue |
                    FeatureMessageCapabilities |
                    FeatureMessageSentSignal))
            != textFeatures) {
        textReady = new PendingReadyChannel(channelFeatures, this);
        textReady->setFinishedWithError(TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Invalid features argument");
        return textReady;
    }

    PendingReadyChannel *channelReady = Channel::becomeReady(channelFeatures);

    textReady = new PendingReadyChannel(channelFeatures, this);

    connect(channelReady,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(onChannelReady(Telepathy::Client::PendingOperation *)));

    mPriv->pendingReady.append(textReady);
    mPriv->pendingFeatures |= (textFeatures & ~mPriv->features);
    return textReady;
}

void TextChannel::onChannelReady(Telepathy::Client::PendingOperation *op)
{
    // Handle the error and insanity cases
    if (op->isError()) {
        mPriv->failReadying(this, op->errorName(), op->errorMessage());
    }
    if (channelType() != QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)) {
        mPriv->failReadying(this,
                QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("TextChannel object with non-Text channel"));
        return;
    }

    // Now that the basic Channel stuff is ready, we can know whether we have
    // the Messages interface.

    if (mPriv->pendingFeatures & FeatureMessageSentSignal) {
        if (hasMessagesInterface()) {
            connect(messagesInterface(),
                    SIGNAL(MessageSent(const Telepathy::MessagePartList &,
                            uint, const QString &)),
                    this,
                    SLOT(onMessageSent(const Telepathy::MessagePartList &,
                            uint, const QString &)));
        } else {
            connect(textInterface(),
                    SIGNAL(Sent(uint, uint, const QString &)),
                    this,
                    SLOT(onTextSent(uint, uint, const QString &)));
        }

        mPriv->features |= FeatureMessageSentSignal;
        mPriv->pendingFeatures &= ~FeatureMessageSentSignal;
    }

    if (!hasMessagesInterface()) {
        // For plain Text channels, FeatureMessageCapabilities is trivial to
        // implement - we don't do anything special at all - so we might as
        // well set it up even if the library user didn't actually care.
        mPriv->supportedContentTypes =
            (QStringList(QLatin1String("text/plain")));
        mPriv->messagePartSupport = 0;
        mPriv->deliveryReportingSupport = 0;
        mPriv->features |= FeatureMessageCapabilities;
        mPriv->pendingFeatures &= ~FeatureMessageCapabilities;
    }

    mPriv->continueReadying(this);
}

void TextChannel::Private::failReadying(TextChannel *channel,
        const QString &error, const QString &message)
{
    QList<PendingReadyChannel *> ops = pendingReady;
    pendingReady.clear();

    foreach (PendingReadyChannel *op, ops) {
        op->setFinishedWithError(error, message);
    }
    channel->invalidate(error, message);
}

void TextChannel::Private::continueReadying(TextChannel *channel)
{
    Q_ASSERT ((pendingFeatures & features) == 0);

    if (pendingFeatures == 0) {
        // everything we wanted is ready
        QList<PendingReadyChannel *> ops = pendingReady;
        pendingReady.clear();
        foreach (PendingReadyChannel *op, ops) {
            op->setFinished();
        }
        return;
    }

    // else there's more work to do yet

    if (channel->hasMessagesInterface()) {
        // FeatureMessageQueue needs signal connections + Get (but we
        //  might as well do GetAll and reduce the number of code paths)
        // FeatureMessageCapabilities needs GetAll
        // FeatureMessageSentSignal already done

        bool getAll = false;

        if (pendingFeatures & TextChannel::FeatureMessageQueue) {
            if (!connectedMessageQueueSignals) {
                connectedMessageQueueSignals = true;
                channel->connect(channel->messagesInterface(),
                        SIGNAL(MessageReceived(const Telepathy::MessagePartList &)),
                        SLOT(onMessageReceived(const Telepathy::MessagePartList &)));

                channel->connect(channel->messagesInterface(),
                        SIGNAL(PendingMessagesRemoved(const Telepathy::UIntList &)),
                        SLOT(onPendingMessagesRemoved(const Telepathy::UIntList &)));
            }

            if (!initialMessagesReceived) {
                getAll = true;
            }
        }

        if (pendingFeatures & TextChannel::FeatureMessageCapabilities) {
            getAll = true;
        }

        if (getAll && !getAllMessagesInFlight) {
            getAllMessagesInFlight = true;
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    channel->propertiesInterface()->GetAll(
                        QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES)),
                        channel);
            channel->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(onGetAllMessagesReply(QDBusPendingCallWatcher *)));
        }
    } else {
        // FeatureMessageQueue needs signal connections +
        //  ListPendingMessages
        // FeatureMessageCapabilities already done
        // FeatureMessageSentSignal already done

        if (pendingFeatures & TextChannel::FeatureMessageQueue) {
            if (!connectedMessageQueueSignals) {
                connectedMessageQueueSignals = true;

                channel->connect(channel->textInterface(),
                        SIGNAL(Received(uint, uint, uint, uint, uint,
                                const QString &)),
                        SLOT(onTextReceived(uint, uint, uint, uint, uint,
                                const QString &)));

                // we present SendError signals as if they were incoming
                // messages, to be consistent with Messages
                channel->connect(channel->textInterface(),
                        SIGNAL(SendError(uint, uint, uint, const QString &)),
                        SLOT(onTextSendError(uint, uint, uint, const QString &)));
            }

            if (!listPendingMessagesCalled) {
                listPendingMessagesCalled = true;

                QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                        channel->textInterface()->ListPendingMessages(false), channel);
                channel->connect(watcher,
                        SIGNAL(finished(QDBusPendingCallWatcher *)),
                        SLOT(onListPendingMessagesReply(QDBusPendingCallWatcher *)));
            }
        }
    }
}

void TextChannel::onMessageSent(const Telepathy::MessagePartList &parts,
        uint flags,
        const QString &sentMessageToken)
{
    emit messageSent(Message(parts), Telepathy::MessageSendingFlag(flags),
            sentMessageToken);
}

void TextChannel::processQueue()
{
    // Proceed as far as we can with the processing of incoming messages
    // and message-removal events; message IDs aren't necessarily globally
    // unique, so we need to process them in the correct order relative
    // to incoming messages
    while (!mPriv->incompleteMessages.isEmpty()) {
        const Private::QueuedEvent *e = mPriv->incompleteMessages.first();
        debug() << "QueuedEvent:" << e;

        if (e->isMessage) {
            if (e->message.mPriv->senderHandle() != 0 &&
                    !e->message.mPriv->sender) {
                // the message doesn't have a sender Contact, but needs one.
                // We'll have to stop processing here, and come back to it
                // when we have more Contact objects
                break;
            }

            // if we reach here, the message is ready
            debug() << "Message is usable, copying to main queue";
            mPriv->messages << e->message;
            emit messageReceived(e->message);
        } else {
            // forget about the message(s) with ID e->removed (there should be
            // at most one under normal circumstances)
            int i = 0;
            while (i < mPriv->messages.size()) {
                if (mPriv->messages.at(i).mPriv->pendingId() == e->removed) {
                    emit pendingMessageRemoved(mPriv->messages.at(i));
                    mPriv->messages.removeAt(i);
                } else {
                    i++;
                }
            }
        }

        debug() << "Dropping first event";
        delete mPriv->incompleteMessages.takeFirst();
    }

    if (mPriv->incompleteMessages.isEmpty()) {
        if (mPriv->pendingFeatures & FeatureMessageQueue) {
            debug() << "incompleteMessages empty for the first time: "
                "FeatureMessageQueue is now ready";
            mPriv->features |= FeatureMessageQueue;
            mPriv->pendingFeatures &= ~FeatureMessageQueue;
            mPriv->continueReadying(this);
        }
        return;
    }

    // What Contact objects do we need in order to proceed, ignoring those
    // for which we've already sent a request?
    QSet<uint> contactsRequired;
    foreach (const Private::QueuedEvent *e, mPriv->incompleteMessages) {
        if (e->isMessage) {
            uint handle = e->message.mPriv->senderHandle();
            if (handle != 0 && !e->message.mPriv->sender
                    && !mPriv->awaitingContacts.contains(handle)) {
                contactsRequired << handle;
            }
        }
    }

    connect(connection()->contactManager()->contactsForHandles(
                contactsRequired.toList()),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onContactsFinished(Telepathy::Client::PendingOperation *)));

    mPriv->awaitingContacts |= contactsRequired;
}

void TextChannel::Private::contactLost(uint handle)
{
    // we're not going to get a Contact object for this handle, so mark the
    // messages from that handle as "unknown sender"
    foreach (QueuedEvent *e, incompleteMessages) {
        if (e->isMessage && e->message.mPriv->senderHandle() == handle
                && !e->message.mPriv->sender) {
            e->message.mPriv->clearSenderHandle();
        }
    }
}

void TextChannel::Private::contactFound(QSharedPointer<Contact> contact)
{
    uint handle = contact->handle().at(0);

    foreach (QueuedEvent *e, incompleteMessages) {
        if (e->isMessage && e->message.mPriv->senderHandle() == handle
                && !e->message.mPriv->sender) {
            e->message.mPriv->sender = contact;
        }
    }
}

void TextChannel::onContactsFinished(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    UIntList failed;

    Q_ASSERT(pc->isForHandles());

    foreach (uint handle, pc->handles()) {
        mPriv->awaitingContacts -= handle;
    }

    if (pc->isError()) {
        warning().nospace() << "Gathering contacts failed: "
            << pc->errorName() << ": " << pc->errorMessage();
        foreach (uint handle, pc->handles()) {
            mPriv->contactLost(handle);
        }
    } else {
        foreach (const QSharedPointer<Contact> &contact, pc->contacts()) {
            mPriv->contactFound(contact);
        }
        foreach (uint handle, pc->invalidHandles()) {
            mPriv->contactLost(handle);
        }
    }
    // all the messages we were asking about should now be ready
    processQueue();
}

void TextChannel::onMessageReceived(const Telepathy::MessagePartList &parts)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }

    mPriv->incompleteMessages << new Private::QueuedEvent(
            ReceivedMessage(parts, this));
    processQueue();
}

void TextChannel::onPendingMessagesRemoved(const Telepathy::UIntList &ids)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }
    foreach (uint id, ids) {
        mPriv->incompleteMessages << new Private::QueuedEvent(id);
    }
    processQueue();
}

void TextChannel::onTextSent(uint timestamp, uint type, const QString &text)
{
    emit messageSent(Message(timestamp, type, text), 0,
            QString::fromAscii(""));
}

void TextChannel::onTextReceived(uint id, uint timestamp, uint sender,
        uint type, uint flags, const QString &text)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }

    MessagePart header;

    if (timestamp == 0) {
        timestamp = QDateTime::currentDateTime().toTime_t();
    }
    header.insert(QString::fromAscii("message-received"),
            QDBusVariant(static_cast<qlonglong>(timestamp)));

    header.insert(QString::fromAscii("pending-message-id"), QDBusVariant(id));
    header.insert(QString::fromAscii("message-sender"), QDBusVariant(sender));
    header.insert(QString::fromAscii("message-type"), QDBusVariant(type));

    if (flags & ChannelTextMessageFlagScrollback) {
        header.insert(QString::fromAscii("scrollback"), QDBusVariant(true));
    }
    if (flags & ChannelTextMessageFlagRescued) {
        header.insert(QString::fromAscii("rescued"), QDBusVariant(true));
    }

    MessagePart body;

    body.insert(QString::fromAscii("content-type"),
            QDBusVariant(QString::fromAscii("text/plain")));
    body.insert(QString::fromAscii("content"), QDBusVariant(text));

    if (flags & ChannelTextMessageFlagTruncated) {
        header.insert(QString::fromAscii("truncated"), QDBusVariant(true));
    }

    MessagePartList parts;
    parts << header;
    parts << body;

    ReceivedMessage m(parts, this);

    if (flags & ChannelTextMessageFlagNonTextContent) {
        // set the "you are not expected to understand this" flag
        m.mPriv->forceNonText = true;
    }

    mPriv->incompleteMessages << new Private::QueuedEvent(m);
    processQueue();
}

void TextChannel::onTextSendError(uint error, uint timestamp, uint type,
        const QString &text)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }

    MessagePart header;

    header.insert(QString::fromAscii("message-received"),
            QDBusVariant(static_cast<qlonglong>(
                    QDateTime::currentDateTime().toTime_t())));
    header.insert(QString::fromAscii("message-type"),
            QDBusVariant(static_cast<uint>(
                    ChannelTextMessageTypeDeliveryReport)));

    // we can't tell whether it's a temporary or permanent failure here,
    // so guess based on the delivery-error
    uint deliveryStatus;
    switch (error) {
        case ChannelTextSendErrorOffline:
        case ChannelTextSendErrorPermissionDenied:
            deliveryStatus = DeliveryStatusTemporarilyFailed;
            break;

        case ChannelTextSendErrorInvalidContact:
        case ChannelTextSendErrorTooLong:
        case ChannelTextSendErrorNotImplemented:
            deliveryStatus = DeliveryStatusPermanentlyFailed;
            break;

        case ChannelTextSendErrorUnknown:
        default:
            deliveryStatus = DeliveryStatusTemporarilyFailed;
            break;
    }

    header.insert(QString::fromAscii("delivery-status"),
            QDBusVariant(deliveryStatus));
    header.insert(QString::fromAscii("delivery-error"), QDBusVariant(error));

    MessagePart echoHeader;
    echoHeader.insert(QString::fromAscii("message-sent"),
            QDBusVariant(timestamp));
    echoHeader.insert(QString::fromAscii("message-type"),
            QDBusVariant(type));

    MessagePart echoBody;
    echoBody.insert(QString::fromAscii("content-type"),
            QDBusVariant(QString::fromAscii("text/plain")));
    echoBody.insert(QString::fromAscii("content"), QDBusVariant(text));

    MessagePartList echo;
    echo << echoHeader;
    echo << echoBody;
    header.insert(QString::fromAscii("delivery-echo"),
            QDBusVariant(QVariant::fromValue(echo)));

    MessagePartList parts;
    parts << header;
}

void TextChannel::onGetAllMessagesReply(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT(mPriv->getAllMessagesInFlight);
    mPriv->getAllMessagesInFlight = false;

    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Properties::GetAll(Channel.Interface.Messages) returned";
        props = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Messages)"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();
        // ... and act as though props had been empty
    }

    if (!mPriv->initialMessagesReceived &&
        (mPriv->pendingFeatures & FeatureMessageQueue)) {
        mPriv->initialMessagesReceived = true;

        MessagePartListList messages = qdbus_cast<MessagePartListList>(
                props["PendingMessages"]);
        foreach (const MessagePartList &message, messages) {
            onMessageReceived(message);
        }
    }

    // Since we have the capabilities, we might as well set them - doing this
    // multiple times is a no-op

    mPriv->supportedContentTypes = qdbus_cast<QStringList>(
            props["SupportedContentTypes"]);
    if (mPriv->supportedContentTypes.isEmpty()) {
        mPriv->supportedContentTypes << QLatin1String("text/plain");
    }
    mPriv->messagePartSupport = MessagePartSupportFlags(qdbus_cast<uint>(
            props["MessagePartSupportFlags"]));
    mPriv->deliveryReportingSupport = DeliveryReportingSupportFlags(
            qdbus_cast<uint>(props["DeliveryReportingSupport"]));

    mPriv->features |= FeatureMessageCapabilities;
    mPriv->pendingFeatures &= ~FeatureMessageCapabilities;
    mPriv->continueReadying(this);
}

void TextChannel::onListPendingMessagesReply(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT(!mPriv->initialMessagesReceived);
    Q_ASSERT(mPriv->listPendingMessagesCalled);

    mPriv->initialMessagesReceived = true;

    QDBusPendingReply<PendingTextMessageList> reply = *watcher;
    PendingTextMessageList list;

    if (!reply.isError()) {
        debug() << "Text::ListPendingMessages returned";
        list = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Messages)"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();
        // ... and act as though list was empty
    }

    foreach (const PendingTextMessage &message, list) {
        onTextReceived(message.identifier, message.unixTimestamp,
                message.sender, message.messageType, message.flags,
                message.text);
    }

    mPriv->continueReadying(this);
}

} // Telepathy::Client
} // Telepathy
