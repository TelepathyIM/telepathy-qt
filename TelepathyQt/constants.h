/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt_constants_h_HEADER_GUARD_
#define _TelepathyQt_constants_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

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
#define TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE QLatin1String("org.freedesktop.Telepathy.ConnectionManager.")

/**
 * The prefix for a connection manager's object path, to which the CM's name
 * (e.g. "gabble") should be appended.
 */
#define TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE QLatin1String("/org/freedesktop/Telepathy/ConnectionManager/")

/**
 * The prefix for a connection's bus name, to which the CM's name (e.g.
 * "gabble"), the protocol (e.g. "jabber") and an element
 * representing the account should be appended.
 */
#define TP_QT_CONNECTION_BUS_NAME_BASE QLatin1String("org.freedesktop.Telepathy.Connection.")

/**
 * The prefix for a connection's object path, to which the CM's name (e.g.
 * "gabble"), the protocol (e.g. "jabber") and an element
 * representing the account should be appended.
 */
#define TP_QT_CONNECTION_OBJECT_PATH_BASE QLatin1String("/org/freedesktop/Telepathy/Connection/")

/**
 * The well-known bus name of the Account Manager.
 *
 * \see Tp::AccountManager
 */
#define TP_QT_ACCOUNT_MANAGER_BUS_NAME \
    (QLatin1String("org.freedesktop.Telepathy.AccountManager"))

/**
 * The object path of the Account Manager object.
 *
 * \see Tp::AccountManager
 */
#define TP_QT_ACCOUNT_MANAGER_OBJECT_PATH \
    (QLatin1String("/org/freedesktop/Telepathy/AccountManager"))

/**
 * The well-known bus name of the Channel Dispatcher.
 */
#define TP_QT_CHANNEL_DISPATCHER_BUS_NAME \
    (QLatin1String("org.freedesktop.Telepathy.ChannelDispatcher"))

/**
 * The object path of the Channel Dispatcherr object.
 */
#define TP_QT_CHANNEL_DISPATCHER_OBJECT_PATH \
    (QLatin1String("/org/freedesktop/Telepathy/ChannelDispatcher"))

/**
 * The prefix for an Account's object path, to which the CM's name (e.g.
 * "gabble"), the protocol (e.g. "jabber") and an element
 * identifying the particular account should be appended.
 *
 * \see Tp::Account
 */
#define TP_QT_ACCOUNT_OBJECT_PATH_BASE \
    (QLatin1String("/org/freedesktop/Telepathy/Account"))

/**
 * The object path of the Debug object of various services.
 */
#define TP_QT_DEBUG_OBJECT_PATH \
    (QLatin1String("/org/freedesktop/Telepathy/debug"))

/**
 * @}
 */

#include <TelepathyQt/_gen/constants.h>

/**
 * \ingroup errorstrconsts
 *
 * The error name "org.freedesktop.DBus.Error.NameHasNoOwner" as a QLatin1String.
 *
 * Raised by the D-Bus daemon when looking up the owner of a well-known name,
 * if no process owns that name.
 *
 * Also used by DBusProxy to indicate that the owner of a well-known name
 * has disappeared (usually indicating that the process owning that name
 * exited or crashed).
 */
#define TP_QT_DBUS_ERROR_NAME_HAS_NO_OWNER \
    (QLatin1String("org.freedesktop.DBus.Error.NameHasNoOwner"))

/**
 * \ingroup errorstrconsts
 *
 * The error name "org.freedesktop.DBus.Error.UnknownInterface" as a QLatin1String.
 */
#define TP_QT_DBUS_ERROR_UNKNOWN_INTERFACE \
    (QLatin1String("org.freedesktop.DBus.Error.UnknownInterface"))

/**
 * \ingroup errorstrconsts
 *
 * The error name "org.freedesktop.DBus.Error.UnknownMethod" as a QLatin1String.
 *
 * Raised by the D-Bus daemon when the method name invoked isn't
 * known by the object you invoked it on.
 */
#define TP_QT_DBUS_ERROR_UNKNOWN_METHOD \
    (QLatin1String("org.freedesktop.DBus.Error.UnknownMethod"))

/**
 * \ingroup errorstrconsts
 *
 * The error name "org.freedesktop.Telepathy.Qt.Error.ObjectRemoved" as a QLatin1String.
 */
#define TP_QT_ERROR_OBJECT_REMOVED \
    (QLatin1String("org.freedesktop.Telepathy.Qt.Error.ObjectRemoved"))

/**
 * \ingroup errorstrconsts
 *
 * The error name "org.freedesktop.Telepathy.Qt.Error.Inconsistent" as a QLatin1String.
 */
#define TP_QT_ERROR_INCONSISTENT \
    (QLatin1String("org.freedesktop.Telepathy.Qt.Error.Inconsistent"))

/**
 * \ingroup errorstrconsts
 *
 * The error name "org.freedesktop.Telepathy.Qt.Error.Orphaned" as a QLatin1String.
 *
 * This error is used when the "parent" proxy of an object gets invalidated. For example, a Channel
 * whose corresponding Connection is invalidated invalidates itself with this error, as do leftover
 * StreamTube connections when their parent StreamTubeChannel is invalidated. The invalidation
 * reason of the parent proxy might provide more information on the cause of the error.
 */
#define TP_QT_ERROR_ORPHANED \
    (QLatin1String("org.freedesktop.Telepathy.Qt.Error.Orphaned"))

#endif
