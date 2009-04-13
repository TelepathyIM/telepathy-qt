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

#include <TelepathyQt4/Message>
#include <TelepathyQt4/ReceivedMessage>

#include <QDateTime>
#include <QPointer>
#include <QSet>

#include <TelepathyQt4/TextChannel>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
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
    WeakPtr<TextChannel> textChannel;
    ContactPtr sender;

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
 * \headerfile <TelepathyQt4/text-channel.h> <TelepathyQt4/TextChannel>
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
 * Constructor, from the parameters of the old Send method.
 *
 * \param type The message type
 * \param text The text of the message
 */
Message::Message(ChannelTextMessageType type, const QString &text)
    : mPriv(new Private(MessagePartList() << MessagePart() << MessagePart()))
{
    mPriv->parts[0].insert(QString::fromAscii("message-type"),
            QDBusVariant(static_cast<uint>(type)));

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

MessagePartList Message::parts() const
{
    return mPriv->parts;
}

/**
 * \class ReceivedMessage
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/text-channel.h> <TelepathyQt4/TextChannel>
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
 * ContactPtr(0) if unknown.
 *
 * \return The sender or ContactPtr(0)
 */
ContactPtr ReceivedMessage::sender() const
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
