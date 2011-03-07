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

#include <TelepathyQt4/ConnectionCapabilities>

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
    : CapabilitiesBase()
{
}

/**
 * Construct a new ConnectionCapabilities object using the give \a rccs.
 *
 * \param rccs RequestableChannelClassList representing the capabilities of a
 *             Connection.
 */
ConnectionCapabilities::ConnectionCapabilities(const RequestableChannelClassList &rccs)
    : CapabilitiesBase(rccs, false)
{
}

/**
 * Construct a new ConnectionCapabilities object using the give \a rccSpecs.
 *
 * \param rccSpecs RequestableChannelClassSpecList representing the capabilities of a
 *                 Connection.
 */
ConnectionCapabilities::ConnectionCapabilities(const RequestableChannelClassSpecList &rccSpecs)
    : CapabilitiesBase(rccSpecs, false)
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
bool ConnectionCapabilities::textChatrooms() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::textChatroom())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating conference media calls is supported.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::conferenceStreamedMediaCalls() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::conferenceStreamedMediaCall())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating conference media calls is supported.
 *
 * This method will also check whether inviting new contacts when creating a conference media call
 * channel by providing additional members to initial invitees (as opposed to merging several
 * channels into one new conference channel) is supported.
 *
 * If providing additional members is supported, it is also possible to request conference media
 * calls with fewer than two (even zero) already established media calls.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::conferenceStreamedMediaCallsWithInvitees() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::conferenceStreamedMediaCallWithInvitees())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating conference text chats is supported.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::conferenceTextChats() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::conferenceTextChat())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating conference text chats is supported.
 *
 * This method will also check whether inviting new contacts when creating a conference text chat
 * channel by providing additional members to initial invitees (as opposed to merging several
 * channels into one new conference channel) is supported.
 *
 * If providing additional members is supported, it is also possible to request conference text
 * chats with fewer than two (even zero) already established text chats.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::conferenceTextChatsWithInvitees() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::conferenceTextChatWithInvitees())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating conference text chat rooms is supported.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::conferenceTextChatrooms() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::conferenceTextChatroom())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating conference text chat rooms is supported.
 *
 * This method will also check whether inviting new contacts when creating a conference text chat
 * room channel by providing additional members to initial invitees (as opposed to merging several
 * channels into one new conference channel) is supported.
 *
 * If providing additional members is supported, it is also possible to request conference text
 * chat rooms with fewer than two (even zero) already established text chat rooms.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::conferenceTextChatroomsWithInvitees() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::conferenceTextChatroomWithInvitees())) {
            return true;
        }
    }
    return false;
}

/**
 * \deprecated Use contactSearches() instead.
 */
bool ConnectionCapabilities::contactSearch()
{
    return contactSearches();
}

/**
 * \deprecated Use contactSearchesWithSpecificServer() instead.
 */
bool ConnectionCapabilities::contactSearchWithSpecificServer() const
{
    return contactSearchesWithSpecificServer();
}

/**
 * \deprecated Use contactSearchesWithLimit() instead.
 */
bool ConnectionCapabilities::contactSearchWithLimit() const
{
    return contactSearchesWithLimit();
}

/**
 * Return whether creating a ContactSearch channel is supported.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::contactSearches() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::contactSearch())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating a ContactSearch channel specifying a server is supported.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::contactSearchesWithSpecificServer() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::contactSearchWithSpecificServer())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether creating a ContactSearch channel specifying a limit is supported.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool ConnectionCapabilities::contactSearchesWithLimit() const
{
    RequestableChannelClassSpecList rccSpecs = allClassSpecs();
    foreach (const RequestableChannelClassSpec &rccSpec, rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::contactSearchWithLimit())) {
            return true;
        }
    }
    return false;
}

} // Tp
