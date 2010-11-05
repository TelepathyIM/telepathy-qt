/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009-2010 Nokia Corporation
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

#ifndef _TelepathyQt4_connection_capabilities_h_HEADER_GUARD_
#define _TelepathyQt4_connection_capabilities_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/CapabilitiesBase>
#include <TelepathyQt4/Types>

namespace Tp
{

class TELEPATHY_QT4_EXPORT ConnectionCapabilities : public CapabilitiesBase
{
public:
    ConnectionCapabilities();
    virtual ~ConnectionCapabilities();

    bool textChatrooms() const;

    bool conferenceStreamedMediaCalls() const;
    bool conferenceStreamedMediaCallsWithInvitees() const;
    bool conferenceTextChats() const;
    bool conferenceTextChatsWithInvitees() const;
    bool conferenceTextChatrooms() const;
    bool conferenceTextChatroomsWithInvitees() const;

    bool contactSearch();
    bool contactSearchWithSpecificServer() const;
    bool contactSearchWithLimit() const;

    TELEPATHY_QT4_DEPRECATED bool supportsTextChatrooms() const;

    TELEPATHY_QT4_DEPRECATED bool supportsConferenceMediaCalls(bool withInitialInvitees = true) const;
    TELEPATHY_QT4_DEPRECATED bool supportsConferenceTextChats(bool withInitialInvitees = true) const;
    TELEPATHY_QT4_DEPRECATED bool supportsConferenceTextChatrooms(bool withInitialInvitees = true) const;

    TELEPATHY_QT4_DEPRECATED bool supportsContactSearch();
    TELEPATHY_QT4_DEPRECATED bool supportsContactSearchWithSpecificServer() const;
    TELEPATHY_QT4_DEPRECATED bool supportsContactSearchWithLimit() const;

private:
    friend class Account;
    friend class Connection;
    friend class ProtocolInfo;

    ConnectionCapabilities(const RequestableChannelClassList &rccs);
    ConnectionCapabilities(const RequestableChannelClassSpecList &rccSpecs);

    struct Private;
    friend struct Private;
    // FIXME: (API/ABI break) Make mPriv a QSharedDataPointer
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::ConnectionCapabilities);

#endif
