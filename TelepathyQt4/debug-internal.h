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

#ifndef _TelepathyQt4_debug_HEADER_GUARD_
#define _TelepathyQt4_debug_HEADER_GUARD_

#include <QDebug>

#include <config.h>

namespace Tp
{

// The telepathy-farsight Qt 4 binding links to these - they're not API outside
// this source tarball, but they *are* ABI
TELEPATHY_QT4_EXPORT QDebug enabledDebug();
TELEPATHY_QT4_EXPORT QDebug enabledWarning();

#ifdef ENABLE_DEBUG

inline QDebug debug()
{
    QDebug debug = enabledDebug();
    debug.nospace() << PACKAGE_NAME " (version " PACKAGE_VERSION ") DEBUG:";
    return debug.space();
}

inline QDebug warning()
{
    QDebug warning = enabledWarning();
    warning.nospace() << PACKAGE_NAME " (version " PACKAGE_VERSION ") WARNING:";
    return warning.space();
}

#else /* #ifdef ENABLE_DEBUG */

struct NoDebug
{
    template <typename T>
    NoDebug& operator<<(const T&)
    {
        return *this;
    }

    NoDebug& space()
    {
        return *this;
    }

    NoDebug& nospace()
    {
        return *this;
    }

    NoDebug& maybeSpace()
    {
        return *this;
    }
};

inline NoDebug debug()
{
    return NoDebug();
}

inline NoDebug warning()
{
    return NoDebug();
}

#endif /* #ifdef ENABLE_DEBUG */

} // Tp

#endif
