/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_Constants_HEADER_GUARD_
#define _TelepathyQt4_Constants_HEADER_GUARD_

/**
 * \addtogroup typesconstants Types and constants
 *
 * Enumerated, flag, structure, list and mapping types and utility constants.
 */

/**
 * \defgroup utilityconsts Utility string constants
 * \ingroup typesconstants
 *
 * Utility constants which aren't generated from the specification but are
 * useful for working with the Telepathy protocol.
 * @{
 */

/**
 * The prefix for a connection manager's bus name, to which the CM's name (e.g.
 * "gabble") should be appended.
 */
#define TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE "org.freedesktop.Telepathy.ConnectionManager."

/**
 * The prefix for a connection manager's object path, to which the CM's name
 * (e.g. "gabble") should be appended.
 */
#define TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE "/org/freedesktop/Telepathy/ConnectionManager/"

/**
 * The prefix for a connection's bus name, to which the CM's name (e.g.
 * "gabble"), the protocol (e.g. "jabber") and an element or sequence of
 * elements representing the account should be appended.
 */
#define TELEPATHY_CONNECTION_BUS_NAME_BASE "org.freedesktop.Telepathy.Connection."

/**
 * The prefix for a connection's object path, to which the CM's name (e.g.
 * "gabble"), the protocol (e.g. "jabber") and an element or sequence of
 * elements representing the account should be appended.
 */
#define TELEPATHY_CONNECTION_OBJECT_PATH_BASE "/org/freedesktop/Telepathy/Connection/"

/**
 * @}
 */

#include <TelepathyQt4/_gen/constants.h>

#endif
