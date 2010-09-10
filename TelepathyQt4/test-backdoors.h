/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2009 Nokia Corporation
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

#ifndef _TelepathyQt4_test_backdoors_h_HEADER_GUARD_
#define _TelepathyQt4_test_backdoors_h_HEADER_GUARD_

#ifdef IN_TELEPATHY_QT4_HEADER
#error "This file is an internal header and should never be included by a public one"
#endif

#include <TelepathyQt4/Global>

#include <QString>

namespace Tp
{

class DBusProxy;

// Exported so the tests can use it even if they link dynamically
// The header is not installed though, so this should be considered private API
struct TELEPATHY_QT4_EXPORT TestBackdoors
{
    static void invalidateProxy(DBusProxy *proxy, const QString &reason, const QString &message);
};

} // Tp

#endif
