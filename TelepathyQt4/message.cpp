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

#include <TelepathyQt4/Message>
#include <TelepathyQt4/ReceivedMessage>

#include <QDateTime>
#include <QPointer>
#include <QSet>

#include <TelepathyQt4/TextChannel>

#include "TelepathyQt4/debug-internal.h"

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

struct TELEPATHY_QT4_NO_EXPORT Message::Private : public QSharedData
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
    QWeakPointer<TextChannel> textChannel;
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
 * \headerfile TelepathyQt4/text-channel.h <TelepathyQt4/TextChannel>
 *
 * \brief The Message class represents a Telepathy message in a text channel.
 * These objects are implicitly shared, like QString.
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
 * \param parts The parts of a message as defined by the \telepathy_spec.
 *              This list must have length at least 1.
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
    mPriv->parts[0].insert(QLatin1String("message-sent"),
            QDBusVariant(static_cast<qlonglong>(timestamp)));
    mPriv->parts[0].insert(QLatin1String("message-type"),
            QDBusVariant(type));

    mPriv->parts[1].insert(QLatin1String("content-type"),
            QDBusVariant(QLatin1String("text/plain")));
    mPriv->parts[1].insert(QLatin1String("content"), QDBusVariant(text));
}

/**
 * Constructor, from the parameters of the old Send method.
 *
 * \param type The message type
 * \param text The text of the message
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
 * \return A timestamp
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
 * Return the type of message this is, or ChannelTextMessageTypeNormal
 * if the type is not recognised.
 *
 * \return The ChannelTextMessageType for this message
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
 * \return A non-empty message identifier, or an empty string if none
 */
QString Message::messageToken() const
{
    return stringOrEmptyFromPart(mPriv->parts, 0, "message-token");
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
    return stringOrEmptyFromPart(mPriv->parts, 0, "interface");
}

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
 * This is provided for advanced clients that need to access
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
 * Return the message's header part, as defined by the \telepathy_spec.
 * This is provided for advanced clients that need to access
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

MessagePartList Message::parts() const
{
    return mPriv->parts;
}

/**
 * \class ReceivedMessage
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/text-channel.h <TelepathyQt4/TextChannel>
 *
 * \brief The ReceivedMessage class is a subclass of Message, representing a
 * received message.
 *
 * It contains additional information that's generally only
 * available on received messages.
 */

/**
 * \class ReceivedMessage::DeliveryDetails
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/text-channel.h <TelepathyQt4/TextChannel>
 *
 * \brief The ReceivedMessage::DeliveryDetails class represents the details of a delivery report.
 */

struct TELEPATHY_QT4_NO_EXPORT ReceivedMessage::DeliveryDetails::Private : public QSharedData
{
    Private(const MessagePartList &parts)
        : parts(parts)
    {
    }

    MessagePartList parts;
};

ReceivedMessage::DeliveryDetails::DeliveryDetails()
{
}

ReceivedMessage::DeliveryDetails::DeliveryDetails(const DeliveryDetails &other)
    : mPriv(other.mPriv)
{
}

ReceivedMessage::DeliveryDetails::DeliveryDetails(const MessagePartList &parts)
    : mPriv(new Private(parts))
{
}

ReceivedMessage::DeliveryDetails::~DeliveryDetails()
{
}

ReceivedMessage::DeliveryDetails &ReceivedMessage::DeliveryDetails::operator=(
        const DeliveryDetails &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

DeliveryStatus ReceivedMessage::DeliveryDetails::status() const
{
    if (!isValid()) {
        return DeliveryStatusUnknown;
    }
    return static_cast<DeliveryStatus>(uintOrZeroFromPart(mPriv->parts, 0, "delivery-status"));
}

bool ReceivedMessage::DeliveryDetails::hasOriginalToken() const
{
    if (!isValid()) {
        return false;
    }
    return partContains(mPriv->parts, 0, "delivery-token");
}

QString ReceivedMessage::DeliveryDetails::originalToken() const
{
    if (!isValid()) {
        return QString();
    }
    return stringOrEmptyFromPart(mPriv->parts, 0, "delivery-token");
}

bool ReceivedMessage::DeliveryDetails::isError() const
{
    if (!isValid()) {
        return false;
    }
    DeliveryStatus st(status());
    return st == DeliveryStatusTemporarilyFailed || st == DeliveryStatusPermanentlyFailed;
}

ChannelTextSendError ReceivedMessage::DeliveryDetails::error() const
{
    if (!isValid()) {
        return ChannelTextSendErrorUnknown;
    }
    return static_cast<ChannelTextSendError>(uintOrZeroFromPart(mPriv->parts, 0, "delivery-error"));
}

bool ReceivedMessage::DeliveryDetails::hasDebugMessage() const
{
    if (!isValid()) {
        return false;
    }
    return partContains(mPriv->parts, 0, "delivery-error-message");
}

QString ReceivedMessage::DeliveryDetails::debugMessage() const
{
    if (!isValid()) {
        return QString();
    }
    return stringOrEmptyFromPart(mPriv->parts, 0, "delivery-error-message");
}

QString ReceivedMessage::DeliveryDetails::dbusError() const
{
    if (!isValid()) {
        return QString();
    }
    QString ret = stringOrEmptyFromPart(mPriv->parts, 0, "delivery-dbus-error");
    if (ret.isEmpty()) {
        switch (error()) {
            case ChannelTextSendErrorOffline:
                ret = TP_QT4_ERROR_OFFLINE;
                break;
            case ChannelTextSendErrorInvalidContact:
                ret = TP_QT4_ERROR_DOES_NOT_EXIST;
                break;
            case ChannelTextSendErrorPermissionDenied:
                ret = TP_QT4_ERROR_PERMISSION_DENIED;
                break;
            case ChannelTextSendErrorTooLong:
                ret = TP_QT4_ERROR_INVALID_ARGUMENT;
                break;
            case ChannelTextSendErrorNotImplemented:
                ret = TP_QT4_ERROR_NOT_IMPLEMENTED;
                break;
            default:
                ret = TP_QT4_ERROR_NOT_AVAILABLE;
        }
    }
    return ret;
}

bool ReceivedMessage::DeliveryDetails::hasEchoedMessage() const
{
    if (!isValid()) {
        return false;
    }
    return partContains(mPriv->parts, 0, "delivery-echo");
}

Message ReceivedMessage::DeliveryDetails::echoedMessage() const
{
    if (!isValid()) {
        return Message();
    }
    return Message(partsFromPart(mPriv->parts, 0, "delivery-echo"));
}

/**
 * Default constructor, only used internally.
 */
ReceivedMessage::ReceivedMessage()
{
}

/**
 * Constructor.
 *
 * \param parts The parts of a message as defined by the \telepathy_spec.
 *              This list must have length at least 1.
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
    mPriv->textChannel = QWeakPointer<TextChannel>(channel.data());
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
    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    uint stamp = valueFromPart(mPriv->parts, 0, "message-received").toUInt();
    if (stamp != 0) {
        return QDateTime::fromTime_t(stamp);
    } else {
        return QDateTime();
    }
}

/**
 * Return the Contact who sent the message, or
 * ContactPtr(0) if unknown.
 *
 * \return The sender or ContactPtr(0)
 */
ContactPtr ReceivedMessage::sender() const
{
    return mPriv->sender;
}

/**
 * Return the nickname chosen by the sender of the message, which can be different for each
 * message in a conversation.
 *
 * \return The nickname chosen by the sender of the message.
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
 * \returns The token of the superseded message or an empty string if none.
 */
QString ReceivedMessage::supersededToken() const
{
    return stringOrEmptyFromPart(mPriv->parts, 0, "supersedes");
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
    return booleanFromPart(mPriv->parts, 0, "scrollback", false);
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
    return booleanFromPart(mPriv->parts, 0, "rescued", false);
}

/**
 * Return whether the incoming message is a delivery report.
 *
 * \return Whether the message is a delivery report.
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
 * \return The details of a delivery report.
 */
ReceivedMessage::DeliveryDetails ReceivedMessage::deliveryDetails() const
{
    return DeliveryDetails(parts());
}

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
