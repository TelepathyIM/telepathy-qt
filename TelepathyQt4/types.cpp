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

#include "config.h"
#include <TelepathyQt4/Types>

#ifndef HAVE_QDBUSVARIANT_OPERATOR_EQUAL

/* FIXME This is a workaround that should be removed when Qt has support for
 *       this. There is already a merge request
 *       (http://qt.gitorious.org/qt/qt/merge_requests/1657) in place and the
 *       fix should be in next Qt versions.
 */
inline bool operator==(const QDBusVariant &v1, const QDBusVariant &v2)
{ return v1.variant() == v2.variant(); }

#endif

#include "TelepathyQt4/_gen/types-body.hpp"

#include "TelepathyQt4/future-internal.h"
#include "TelepathyQt4/_gen/future-types-body.hpp"

/**
 * \\ingroup types
 * \headerfile TelepathyQt4/types.h <TelepathyQt4/Types>
 *
 * Register the types used by the library with the QtDBus type system.
 *
 * Call this function to register the types used before using anything else in
 * the library.
 */
void Tp::registerTypes()
{
    Tp::_registerTypes();
    TpFuture::_registerTypes();
}
