/*
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/ConnectionCapabilities>

#include "TelepathyQt4/future-internal.h"

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

namespace Tp
{

/**
 * \class ConnectionCapabilities
 * \ingroup clientconn
 * \headerfile TelepathyQt4/connection-capabilities.h <TelepathyQt4/ConnectionCapabilities>
 *
 * \brief The ConnectionCapabilities class provides an object representing the
 * capabilities of a Connection.
 */

/**
 * Construct a new ConnectionCapabilities object.
 */
ConnectionCapabilities::ConnectionCapabilities()
    : CapabilitiesBase(false)
{
}

/**
 * Construct a new ConnectionCapabilities object using the give \a classes.
 *
 * \param classes RequestableChannelClassList representing the capabilities of a
 *                Connection.
 */
ConnectionCapabilities::ConnectionCapabilities(const RequestableChannelClassList &classes)
    : CapabilitiesBase(classes, false)
{
}

/**
 * Class destructor.
 */
ConnectionCapabilities::~ConnectionCapabilities()
{
}

/**
 * Return true if named text chatrooms can be joined by providing a
 * chatroom identifier.
 *
 * If the protocol is such that chatrooms can be joined, but only via
 * a more elaborate D-Bus API than normal (because more information is needed),
 * then this method will return false.
 *
 * \return \c true if Account::ensureTextChatroom() can be expected to work.
 */
bool ConnectionCapabilities::supportsTextChatrooms() const
{
    QString channelType;
    uint targetHandleType;
    RequestableChannelClassList classes = requestableChannelClasses();
    foreach (const RequestableChannelClass &cls, classes) {
        if (!cls.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) &&
            targetHandleType == HandleTypeRoom) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether merging media calls in a conference is supported.
 *
 * \param withInitialInvitees If \c true (the default), check for the ability to merge media calls
 *                            in a conference, by providing initial invitees; if \c false,
 *                            skip checking if initial invitees is allowed.
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::supportsConferenceMediaCalls(
            bool withInitialInvitees) const
{
    QString channelType;
    RequestableChannelClassList classes = requestableChannelClasses();
    foreach (const RequestableChannelClass &cls, classes) {
        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) &&
            (cls.allowedProperties.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels")) ||
             cls.allowedProperties.contains(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels")))) {
            if (!withInitialInvitees ||
                (cls.allowedProperties.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialInviteeHandles")) ||
                 cls.allowedProperties.contains(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialInviteeHandles")))) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Return whether merging text chats in a conference is supported.
 *
 * \param withInitialInvitees If \c true (the default), check for the ability to merge text chats
 *                            in a conference, by providing initial invitees; if \c false,
 *                            skip checking if initial invitees is allowed.
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::supportsConferenceTextChats(
            bool withInitialInvitees) const
{
    QString channelType;
    RequestableChannelClassList classes = requestableChannelClasses();
    foreach (const RequestableChannelClass &cls, classes) {
        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) &&
            (cls.allowedProperties.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels")) ||
             cls.allowedProperties.contains(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels")))) {
            if (!withInitialInvitees ||
                (cls.allowedProperties.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialInviteeHandles")) ||
                 cls.allowedProperties.contains(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialInviteeHandles")))) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Return whether merging text chat rooms in a conference is supported.
 *
 * \param withInitialInvitees If \c true (the default), check for the ability to merge text chat
 *                            rooms in a conference, by providing initial invitees; if \c false,
 *                            skip checking if initial invitees is allowed.
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::supportsConferenceTextChatrooms(
            bool withInitialInvitees) const
{
    QString channelType;
    uint targetHandleType;
    RequestableChannelClassList classes = requestableChannelClasses();
    foreach (const RequestableChannelClass &cls, classes) {
        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) &&
            targetHandleType == HandleTypeRoom &&
            (cls.allowedProperties.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels")) ||
             cls.allowedProperties.contains(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels")))) {
            if (!withInitialInvitees ||
                (cls.allowedProperties.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialInviteeHandles")) ||
                 cls.allowedProperties.contains(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialInviteeHandles")))) {
                return true;
            }
        }
    }
    return false;
}
} // Tp
