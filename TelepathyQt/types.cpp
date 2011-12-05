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

#include <TelepathyQt/Types>

#include <TelepathyQt/types-internal.h>

#include "TelepathyQt/_gen/types-body.hpp"

#include "TelepathyQt/future-internal.h"
#include "TelepathyQt/_gen/future-types-body.hpp"

namespace Tp {
/**
 * \\ingroup types
 * \headerfile TelepathyQt/types.h <TelepathyQt/Types>
 *
 * Register the types used by the library with the QtDBus type system.
 *
 * Call this function to register the types used before using anything else in
 * the library.
 */
void registerTypes()
{
    qDBusRegisterMetaType<Tp::SUSocketAddress>();

    Tp::_registerTypes();
    TpFuture::_registerTypes();
}

bool operator==(const SUSocketAddress& v1, const SUSocketAddress& v2)
{
    return ((v1.address == v2.address)
            && (v1.port == v2.port)
            );
}

QDBusArgument& operator<<(QDBusArgument& arg, const SUSocketAddress& val)
{
    arg.beginStructure();
    arg << val.address << val.port;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, SUSocketAddress& val)
{
    arg.beginStructure();
    arg >> val.address >> val.port;
    arg.endStructure();
    return arg;
}

}
