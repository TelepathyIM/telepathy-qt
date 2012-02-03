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

#ifndef _TelepathyQt_debug_HEADER_GUARD_
#define _TelepathyQt_debug_HEADER_GUARD_

#include <QDebug>

#include <TelepathyQt/Global>

namespace Tp
{

class TP_QT_EXPORT Debug
{
public:
    inline Debug() : debug(0) { }
    inline Debug(QtMsgType type) : type(type), debug(new QDebug(&msg)) { }
    inline Debug(const Debug &a) : type(a.type), debug(a.debug ? new QDebug(&msg) : 0)
    {
        if (debug) {
            (*debug) << qPrintable(a.msg);
        }
    }

    inline Debug &operator=(const Debug &a)
    {
        if (this != &a) {
            type = a.type;
            delete debug;
            debug = 0;

            if (a.debug) {
                debug = new QDebug(&msg);
                (*debug) << qPrintable(a.msg);
            }
        }

        return *this;
    }

    inline ~Debug()
    {
        if (!msg.isEmpty()) {
            invokeDebugCallback();
        }
        delete debug;
    }

    inline Debug &space()
    {
        if (debug) {
            debug->space();
        }

        return *this;
    }

    inline Debug &nospace()
    {
        if (debug) {
            debug->nospace();
        }

        return *this;
    }

    inline Debug &maybeSpace()
    {
        if (debug) {
            debug->maybeSpace();
        }

        return *this;
    }

    template <typename T>
    inline Debug &operator<<(T a)
    {
        if (debug) {
            (*debug) << a;
        }

        return *this;
    }

private:

    QString msg;
    QtMsgType type;
    QDebug *debug;

    void invokeDebugCallback();
};

// The telepathy-farsight Qt 4 binding links to these - they're not API outside
// this source tarball, but they *are* ABI
TP_QT_EXPORT Debug enabledDebug();
TP_QT_EXPORT Debug enabledWarning();

#ifdef ENABLE_DEBUG

inline Debug debug()
{
    return enabledDebug();
}

inline Debug warning()
{
    return enabledWarning();
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
