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

#include <TelepathyQt4/Client/PendingReadyChannel>

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
    TextChannel::Features desiredFeatures;
    QList<PendingReadyChannel *> pendingReady;
    void continueReadying(TextChannel *channel);
    void failReadying(TextChannel *channel, const QString &error,
            const QString &message);

    // requires FeatureMessageCapabilities
    QStringList supportedContentTypes;
    MessagePartSupportFlags messagePartSupport;
    DeliveryReportingSupportFlags deliveryReportingSupport;
};

TextChannel::Private::Private()
    : features(0),
      desiredFeatures(0),
      pendingReady(),
      supportedContentTypes(),
      messagePartSupport(0),
      deliveryReportingSupport(0)
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
 * \value FeatureMessageQueue Doesn't do anything yet
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
    mPriv->desiredFeatures |= textFeatures;
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

    if ((mPriv->desiredFeatures & FeatureMessageSentSignal) &&
            !(mPriv->features & FeatureMessageSentSignal)) {
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
    if ((desiredFeatures & features) == desiredFeatures) {
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

        if (desiredFeatures & TextChannel::FeatureMessageQueue) {
            channel->connect(channel->messagesInterface(),
                    SIGNAL(MessageReceived(const Telepathy::MessagePartList &)),
                    SLOT(onMessageReceived(const Telepathy::MessagePartList &)));

            channel->connect(channel->messagesInterface(),
                    SIGNAL(PendingMessagesRemoved(const Telepathy::UIntList &)),
                    SLOT(onPendingMessagesRemoved(const Telepathy::UIntList &)));
        }

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                channel->propertiesInterface()->GetAll(
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES)),
                    channel);
        channel->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onGetAllMessagesReply(QDBusPendingCallWatcher *)));
    } else {
        // FeatureMessageQueue needs signal connections +
        //  ListPendingMessages
        // FeatureMessageCapabilities already done
        // FeatureMessageSentSignal already done

        channel->connect(channel->textInterface(),
                SIGNAL(Received(uint, uint, uint, uint, uint,
                        const QString &)),
                SLOT(onTextReceived(uint, uint, uint, uint, uint,
                        const QString &)));

        // we present SendError signals as if they were incoming messages,
        // to be consistent with Messages
        channel->connect(channel->textInterface(),
                SIGNAL(SendError(uint, uint, uint, const QString &)),
                SLOT(onTextSendError(uint, uint, uint, const QString &)));

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                channel->textInterface()->ListPendingMessages(false), channel);
        channel->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onListPendingMessagesReply(QDBusPendingCallWatcher *)));
    }
}

void TextChannel::onMessageSent(const Telepathy::MessagePartList &parts,
        uint flags,
        const QString &sentMessageToken)
{
    emit messageSent(Message(parts), Telepathy::MessageSendingFlag(flags),
            sentMessageToken);
}

void TextChannel::onMessageReceived(const Telepathy::MessagePartList &parts)
{
}

void TextChannel::onPendingMessagesRemoved(const Telepathy::UIntList &ids)
{
}

void TextChannel::onTextSent(uint timestamp, uint type, const QString &text)
{
    emit messageSent(Message(timestamp, type, text), 0,
            QString::fromAscii(""));
}

void TextChannel::onTextReceived(uint id, uint timestamp, uint sender,
        uint type, uint flags, const QString &text)
{
}

void TextChannel::onTextSendError(uint error, uint timestamp, uint type,
        const QString &text)
{
}

void TextChannel::onGetAllMessagesReply(QDBusPendingCallWatcher *watcher)
{
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

    if (mPriv->desiredFeatures & FeatureMessageQueue) {
        // FIXME: actually put the messages in the queue

        mPriv->features |= FeatureMessageQueue;
    }

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
    mPriv->continueReadying(this);
}

void TextChannel::onListPendingMessagesReply(QDBusPendingCallWatcher *watcher)
{
    // FIXME: actually put the messages in the queue

    mPriv->features |= FeatureMessageQueue;
    mPriv->continueReadying(this);
}

} // Telepathy::Client
} // Telepathy
