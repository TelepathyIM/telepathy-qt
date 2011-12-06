/**
 * This file is part of TelepathyQt
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

#include <TelepathyQt/Message>
#include <TelepathyQt/ReceivedMessage>

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/TextChannel>

#include <QDateTime>
#include <QSet>

namespace Tp
{

namespace
{

QVariant valueFromPart(const MessagePartList &parts, uint index, const char *key)
{
    return parts.at(index).value(QLatin1String(key)).variant();
}

uint uintOrZeroFromPart(const MessagePartList &parts, uint index, const char *key)
{
    return valueFromPart(parts, index, key).toUInt();
}

QString stringOrEmptyFromPart(const MessagePartList &parts, uint index, const char *key)
{
    QString s = valueFromPart(parts, index, key).toString();
    if (s.isNull()) {
        s = QLatin1String("");
    }
    return s;
}

bool booleanFromPart(const MessagePartList &parts, uint index, const char *key,
            bool assumeIfAbsent)
{
    QVariant v = valueFromPart(parts, index, key);
    if (v.isValid() && v.type() == QVariant::Bool) {
        return v.toBool();
    }
    return assumeIfAbsent;
}

MessagePartList partsFromPart(const MessagePartList &parts, uint index, const char *key)
{
    return qdbus_cast<MessagePartList>(valueFromPart(parts, index, key));
}

bool partContains(const MessagePartList &parts, uint index, const char *key)
{
    return parts.at(index).contains(QLatin1String(key));
}

}

struct TP_QT_NO_EXPORT Message::Private : public QSharedData
{
    Private(const MessagePartList &parts);
    ~Private();

    uint senderHandle() const;
    QString senderId() const;
    uint pendingId() const;
    void clearSenderHandle();

    MessagePartList parts;

    // if the Text interface says "non-text" we still only have the text,
    // because the interface can't tell us anything else...
    bool forceNonText;

    // for received messages only
    WeakPtr<TextChannel> textChannel;
    ContactPtr sender;
};

Message::Private::Private(const MessagePartList &parts)
    : parts(parts),
      forceNonText(false),
      sender(0)
{
}

Message::Private::~Private()
{
}

inline uint Message::Private::senderHandle() const
{
    return uintOrZeroFromPart(parts, 0, "message-sender");
}

inline QString Message::Private::senderId() const
{
    return stringOrEmptyFromPart(parts, 0, "message-sender-id");
}

inline uint Message::Private::pendingId() const
{
    return uintOrZeroFromPart(parts, 0, "pending-message-id");
}

void Message::Private::clearSenderHandle()
{
    parts[0].remove(QLatin1String("message-sender"));
}

/**
 * \class Message
 * \ingroup clientchannel
 * \headerfile TelepathyQt/message.h <TelepathyQt/Message>
 *
 * \brief The Message class represents a Telepathy message in a TextChannel.
 *
 * This class is implicitly shared, like QString.
 */

/**
 * \internal Default constructor.
 */
Message::Message()
    : mPriv(new Private(MessagePartList()))
{
}

/**
 * Construct a new Message object.
 *
 * \param parts The parts of a message as defined by the \telepathy_spec.
 *              This list must have length at least 1.
 */
Message::Message(const MessagePartList &parts)
    : mPriv(new Private(parts))
{
    Q_ASSERT(parts.size() > 0);
}

/**
 * Construct a new Message object.
 *
 * \param timestamp The time the message was sent.
 * \param type The message type.
 * \param text The message body.
 */
Message::Message(uint timestamp, uint type, const QString &text)
    : mPriv(new Private(MessagePartList() << MessagePart() << MessagePart()))
{
    mPriv->parts[0].insert(QLatin1String("message-sent"),
            QDBusVariant(static_cast<qlonglong>(timestamp)));
    mPriv->parts[0].insert(QLatin1String("message-type"),
            QDBusVariant(type));

    mPriv->parts[1].insert(QLatin1String("content-type"),
            QDBusVariant(QLatin1String("text/plain")));
    mPriv->parts[1].insert(QLatin1String("content"), QDBusVariant(text));
}

/**
 * Construct a new Message object.
 *
 * \param type The message type.
 * \param text The message body.
 */
Message::Message(ChannelTextMessageType type, const QString &text)
    : mPriv(new Private(MessagePartList() << MessagePart() << MessagePart()))
{
    mPriv->parts[0].insert(QLatin1String("message-type"),
            QDBusVariant(static_cast<uint>(type)));

    mPriv->parts[1].insert(QLatin1String("content-type"),
            QDBusVariant(QLatin1String("text/plain")));
    mPriv->parts[1].insert(QLatin1String("content"), QDBusVariant(text));
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
 * Class destructor.
 */
Message::~Message()
{
}

/**
 * Return the time the message was sent, or QDateTime() if that time is
 * unknown.
 *
 * \return The timestamp as QDateTime.
 */
QDateTime Message::sent() const
{
    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    uint stamp = valueFromPart(mPriv->parts, 0, "message-sent").toUInt();
    if (stamp != 0) {
        return QDateTime::fromTime_t(stamp);
    } else {
        return QDateTime();
    }
}

/**
 * Return the type of this message, or #ChannelTextMessageTypeNormal
 * if the type is not recognised.
 *
 * \return The type as #ChannelTextMessageType.
 */
ChannelTextMessageType Message::messageType() const
{
    uint raw = valueFromPart(mPriv->parts, 0, "message-type").toUInt();

    if (raw < static_cast<uint>(NUM_CHANNEL_TEXT_MESSAGE_TYPES)) {
        return ChannelTextMessageType(raw);
    } else {
        return ChannelTextMessageTypeNormal;
    }
}

/**
 * Return whether this message was truncated during delivery.
 *
 * \return \c true if truncated, \c false otherwise.
 */
bool Message::isTruncated() const
{
    for (int i = 1; i < size(); i++) {
        if (booleanFromPart(mPriv->parts, i, "truncated", false)) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether this message contains parts not representable as plain
 * text.
 *
 * \return \c true if it cannot completely be represented as plain text, \c false
 *         otherwise.
 */
bool Message::hasNonTextContent() const
{
    if (mPriv->forceNonText || size() <= 1 || isSpecificToDBusInterface()) {
        return true;
    }

    QSet<QString> texts;
    QSet<QString> textNeeded;

    for (int i = 1; i < size(); i++) {
        QString altGroup = stringOrEmptyFromPart(mPriv->parts, i, "alternative");
        QString contentType = stringOrEmptyFromPart(mPriv->parts, i, "content-type");

        if (contentType == QLatin1String("text/plain")) {
            if (!altGroup.isEmpty()) {
                // we can use this as an alternative for a non-text part
                // with the same altGroup
                texts << altGroup;
            }
        } else {
            QString alt = stringOrEmptyFromPart(mPriv->parts, i, "alternative");
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
 * \return The non-empty message identifier, or an empty string if none.
 */
QString Message::messageToken() const
{
    return stringOrEmptyFromPart(mPriv->parts, 0, "message-token");
}

/**
 * Return whether this message is specific to a D-Bus interface. This is
 * \c false in almost all cases.
 *
 * If this function returns \c true, the message is specific to the interface
 * indicated by dbusInterface(). Clients that don't understand that interface
 * should not display the message. However, if the client would acknowledge
 * an ordinary message, it must also acknowledge this interface-specific
 * message.
 *
 * \return \c true if dbusInterface() would return a non-empty string, \c false otherwise.
 * \sa dbusInterface()
 */
bool Message::isSpecificToDBusInterface() const
{
    return !dbusInterface().isEmpty();
}

/**
 * Return the D-Bus interface to which this message is specific, or an
 * empty string for normal messages.
 *
 * \return The D-Bus interface name, or an empty string.
 * \sa isSpecificToDBusInterface()
 */
QString Message::dbusInterface() const
{
    return stringOrEmptyFromPart(mPriv->parts, 0, "interface");
}

/**
 * Return the message body containing all "text/plain" parts.
 *
 * \return The body text.
 */
QString Message::text() const
{
    // Alternative-groups for which we've already emitted an alternative
    QSet<QString> altGroupsUsed;
    QString text;

    for (int i = 1; i < size(); i++) {
        QString altGroup = stringOrEmptyFromPart(mPriv->parts, i, "alternative");
        QString contentType = stringOrEmptyFromPart(mPriv->parts, i, "content-type");

        if (contentType == QLatin1String("text/plain")) {
            if (!altGroup.isEmpty()) {
                if (altGroupsUsed.contains(altGroup)) {
                    continue;
                } else {
                    altGroupsUsed << altGroup;
                }
            }

            QVariant content = valueFromPart(mPriv->parts, i, "content");
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
 * Return the message's header part, as defined by the \telepathy_spec.
 *
 * This is provided for advanced clients that need to access
 * additional information not available through the normal Message API.
 *
 * \return The header as a MessagePart object. The same thing as part(0).
 */
MessagePart Message::header() const
{
    return part(0);
}

/**
 * Return the number of parts in this message.
 *
 * \return 1 greater than the largest valid argument to part().
 * \sa part(), parts()
 */
int Message::size() const
{
    return mPriv->parts.size();
}

/**
 * Return the message's part for \a index, as defined by the \telepathy_spec.
 *
 * This is provided for advanced clients that need to access
 * additional information not available through the normal Message API.
 *
 * \param index The part to access, which must be strictly less than size();
 *              part number 0 is the header, parts numbered 1 or greater
 *              are the body of the message.
 * \return A MessagePart object.
 */
MessagePart Message::part(uint index) const
{
    return mPriv->parts.at(index);
}

/**
 * Return the list of message parts forming this message.
 *
 * \return The list of MessagePart objects.
 */
MessagePartList Message::parts() const
{
    return mPriv->parts;
}

/**
 * \class ReceivedMessage
 * \ingroup clientchannel
 * \headerfile TelepathyQt/message.h <TelepathyQt/ReceivedMessage>
 *
 * \brief The ReceivedMessage class is a subclass of Message, representing a
 * received message only.
 *
 * It contains additional information that's generally only
 * available on received messages.
 */

/**
 * \class ReceivedMessage::DeliveryDetails
 * \ingroup clientchannel
 * \headerfile TelepathyQt/message.h <TelepathyQt/ReceivedMessage>
 *
 * \brief The ReceivedMessage::DeliveryDetails class represents the details of a delivery report.
 */

struct TP_QT_NO_EXPORT ReceivedMessage::DeliveryDetails::Private : public QSharedData
{
    Private(const MessagePartList &parts)
        : parts(parts)
    {
    }

    MessagePartList parts;
};

/**
 * Default constructor.
 */
ReceivedMessage::DeliveryDetails::DeliveryDetails()
{
}

/**
 * Copy constructor.
 */
ReceivedMessage::DeliveryDetails::DeliveryDetails(const DeliveryDetails &other)
    : mPriv(other.mPriv)
{
}

/**
 * Construct a new ReceivedMessage::DeliveryDetails object.
 *
 * \param The message parts.
 */
ReceivedMessage::DeliveryDetails::DeliveryDetails(const MessagePartList &parts)
    : mPriv(new Private(parts))
{
}

/**
 * Class destructor.
 */
ReceivedMessage::DeliveryDetails::~DeliveryDetails()
{
}

/**
 * Assignment operator.
 */
ReceivedMessage::DeliveryDetails &ReceivedMessage::DeliveryDetails::operator=(
        const DeliveryDetails &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

/**
 * Return the delivery status of a message.
 *
 * \return The delivery status as #DeliveryStatus.
 */
DeliveryStatus ReceivedMessage::DeliveryDetails::status() const
{
    if (!isValid()) {
        return DeliveryStatusUnknown;
    }
    return static_cast<DeliveryStatus>(uintOrZeroFromPart(mPriv->parts, 0, "delivery-status"));
}

/**
 * Return whether this delivery report contains an identifier for the message to which it
 * refers.
 *
 * \return \c true if an original message token is known, \c false otherwise.
 * \sa originalToken()
 */
bool ReceivedMessage::DeliveryDetails::hasOriginalToken() const
{
    if (!isValid()) {
        return false;
    }
    return partContains(mPriv->parts, 0, "delivery-token");
}

/**
 * Return an identifier for the message to which this delivery report refers, or an empty string if
 * hasOriginalToken() returns \c false.
 *
 * Clients may match this against the token produced by the TextChannel::send() method and
 * TextChannel::messageSent() signal. A status report with no token could match any sent message,
 * and a sent message with an empty token could match any status report.
 * If multiple sent messages match, clients should use some reasonable heuristic.
 *
 * \return The message token if hasOriginalToken() returns \c true, an empty string otherwise.
 * \sa hasOriginalToken().
 */
QString ReceivedMessage::DeliveryDetails::originalToken() const
{
    if (!isValid()) {
        return QString();
    }
    return stringOrEmptyFromPart(mPriv->parts, 0, "delivery-token");
}

/**
 * Return whether the delivery of the message this delivery report refers to, failed.
 *
 * \return \c true if the message delivery failed, \c false otherwise.
 * \sa error()
 */
bool ReceivedMessage::DeliveryDetails::isError() const
{
    if (!isValid()) {
        return false;
    }
    DeliveryStatus st(status());
    return st == DeliveryStatusTemporarilyFailed || st == DeliveryStatusPermanentlyFailed;
}

/**
 * Return the reason for the delivery failure if known.
 *
 * \return The reason as #ChannelTextSendError.
 * \sa isError()
 */
ChannelTextSendError ReceivedMessage::DeliveryDetails::error() const
{
    if (!isValid()) {
        return ChannelTextSendErrorUnknown;
    }
    return static_cast<ChannelTextSendError>(uintOrZeroFromPart(mPriv->parts, 0, "delivery-error"));
}

/**
 * Return whether this delivery report contains a debugging information on why the message it refers
 * to could not be delivered.
 *
 * \return \c true if a debugging information is provided, \c false otherwise.
 * \sa debugMessage()
 */
bool ReceivedMessage::DeliveryDetails::hasDebugMessage() const
{
    if (!isValid()) {
        return false;
    }
    return partContains(mPriv->parts, 0, "delivery-error-message");
}

/**
 * Return the debugging information on why the message this delivery report refers to could not be
 * delivered.
 *
 * \return The debug string.
 * \sa hasDebugMessage()
 */
QString ReceivedMessage::DeliveryDetails::debugMessage() const
{
    if (!isValid()) {
        return QString();
    }
    return stringOrEmptyFromPart(mPriv->parts, 0, "delivery-error-message");
}

/**
 * Return the reason for the delivery failure if known, specified as a
 * (possibly implementation-specific) D-Bus error.
 *
 * \return The D-Bus error string representing the error.
 */
QString ReceivedMessage::DeliveryDetails::dbusError() const
{
    if (!isValid()) {
        return QString();
    }
    QString ret = stringOrEmptyFromPart(mPriv->parts, 0, "delivery-dbus-error");
    if (ret.isEmpty()) {
        switch (error()) {
            case ChannelTextSendErrorOffline:
                ret = TP_QT_ERROR_OFFLINE;
                break;
            case ChannelTextSendErrorInvalidContact:
                ret = TP_QT_ERROR_DOES_NOT_EXIST;
                break;
            case ChannelTextSendErrorPermissionDenied:
                ret = TP_QT_ERROR_PERMISSION_DENIED;
                break;
            case ChannelTextSendErrorTooLong:
                ret = TP_QT_ERROR_INVALID_ARGUMENT;
                break;
            case ChannelTextSendErrorNotImplemented:
                ret = TP_QT_ERROR_NOT_IMPLEMENTED;
                break;
            default:
                ret = TP_QT_ERROR_NOT_AVAILABLE;
        }
    }
    return ret;
}

/**
 * Return whether the message content for the message this delivery report refers to is known.
 *
 * \return \c true if the original message content is known, \c false otherwise.
 * \sa echoedMessage()
 */
bool ReceivedMessage::DeliveryDetails::hasEchoedMessage() const
{
    if (!isValid()) {
        return false;
    }
    return partContains(mPriv->parts, 0, "delivery-echo");
}

/**
 * Return the Message object for the message this delivery report refers to, omitted if the message
 * is unknown.
 *
 * <div class='rationale'>
 *   <h5>Rationale:</h5>
 *   <div>
 *   Some protocols, like XMPP, echo the failing message back to the sender. This is sometimes the
 *   only way to match it against the sent message, so we include it here.
 *   </div>
 * </div>
 *
 * \return The Message object, or an empty Message object if hasEchoedMessage()
 *         returns \c false.
 * \sa hasEchoedMessage()
 */
Message ReceivedMessage::DeliveryDetails::echoedMessage() const
{
    if (!isValid()) {
        return Message();
    }
    return Message(partsFromPart(mPriv->parts, 0, "delivery-echo"));
}

/**
 * \internal Default constructor.
 */
ReceivedMessage::ReceivedMessage()
{
}

/**
 * Construct a new ReceivedMessage object.
 *
 * \param parts The parts of a message as defined by the \telepathy_spec.
 *              This list must have length at least 1.
 * \param channel The channel owning this message.
 */
ReceivedMessage::ReceivedMessage(const MessagePartList &parts,
        const TextChannelPtr &channel)
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
 * Class destructor.
 */
ReceivedMessage::~ReceivedMessage()
{
}

/**
 * Return the time the message was received.
 *
 * \return The timestamp as QDateTime, or QDateTime() if unknown.
 */
QDateTime ReceivedMessage::received() const
{
    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    uint stamp = valueFromPart(mPriv->parts, 0, "message-received").toUInt();
    if (stamp != 0) {
        return QDateTime::fromTime_t(stamp);
    } else {
        return QDateTime();
    }
}

/**
 * Return the contact who sent the message.
 *
 * \return A pointer to the Contact object.
 * \sa senderNickname()
 */
ContactPtr ReceivedMessage::sender() const
{
    return mPriv->sender;
}

/**
 * Return the nickname chosen by the sender of the message, which can be different for each
 * message in a conversation.
 *
 * \return The nickname.
 * \sa sender()
 */
QString ReceivedMessage::senderNickname() const
{
    QString ret = stringOrEmptyFromPart(mPriv->parts, 0, "sender-nickname");
    if (ret.isEmpty() && mPriv->sender) {
        ret = mPriv->sender->alias();
    }
    return ret;
}

/**
 * If this message replaces a previous message, return the value of
 * messageToken() for that previous message. Otherwise, return an empty string.
 *
 * For instance, a user interface could replace the superseded
 * message with this message, or grey out the superseded message.
 *
 * \return The message token of the superseded message or an empty string if none.
 */
QString ReceivedMessage::supersededToken() const
{
    return stringOrEmptyFromPart(mPriv->parts, 0, "supersedes");
}

/**
 * Return whether the incoming message was part of a replay of message
 * history.
 *
 * If \c true, loggers can use this to improve their heuristics for elimination
 * of duplicate messages (a simple, correct implementation would be to avoid
 * logging any message that has this flag).
 *
 * \return \c true if the scrollback flag is set, \c false otherwise.
 */
bool ReceivedMessage::isScrollback() const
{
    return booleanFromPart(mPriv->parts, 0, "scrollback", false);
}

/**
 * Return whether the incoming message was seen in a previous channel during
 * the lifetime of the connection, but was not acknowledged before that
 * channel closed, causing the channel in which it now appears to open.
 *
 * If \c true, loggers should not log this message again.
 *
 * \return \c true if the rescued flag is set, \c false otherwise.
 */
bool ReceivedMessage::isRescued() const
{
    return booleanFromPart(mPriv->parts, 0, "rescued", false);
}

/**
 * Return whether the incoming message is a delivery report.
 *
 * \return \c true if a delivery report, \c false otherwise.
 * \sa deliveryDetails()
 */
bool ReceivedMessage::isDeliveryReport() const
{
    return messageType() == ChannelTextMessageTypeDeliveryReport;
}

/**
 * Return the details of a delivery report.
 *
 * This method should only be used if isDeliveryReport() returns \c true.
 *
 * \return The delivery report as a ReceivedMessage::DeliveryDetails object.
 * \sa isDeliveryReport()
 */
ReceivedMessage::DeliveryDetails ReceivedMessage::deliveryDetails() const
{
    return DeliveryDetails(parts());
}

/**
 * Return whether this message is from \a channel.
 *
 * \return \c true if the message is from \a channel, \c false otherwise.
 */
bool ReceivedMessage::isFromChannel(const TextChannelPtr &channel) const
{
    return TextChannelPtr(mPriv->textChannel) == channel;
}

uint ReceivedMessage::pendingId() const
{
    return mPriv->pendingId();
}

uint ReceivedMessage::senderHandle() const
{
    return mPriv->senderHandle();
}

QString ReceivedMessage::senderId() const
{
    return mPriv->senderId();
}

void ReceivedMessage::setForceNonText()
{
    mPriv->forceNonText = true;
}

void ReceivedMessage::clearSenderHandle()
{
    mPriv->clearSenderHandle();
}

void ReceivedMessage::setSender(const ContactPtr &sender)
{
    mPriv->sender = sender;
}

} // Tp
