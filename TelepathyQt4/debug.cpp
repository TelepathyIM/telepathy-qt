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

#define IN_TELEPATHY_QT4_HEADER
#include "debug.h"
#include "debug-internal.h"

#include <QIODevice>

namespace Telepathy
{
#ifdef ENABLE_DEBUG

namespace
{
struct DiscardDevice : public QIODevice
{
    qint64 readData(char* data, qint64 maxSize)
    {
        return 0;
    }

    qint64 writeData(const char* data, qint64 maxsize)
    {
        return maxsize;
    }
} discard;

bool debugEnabled = false;
bool warningsEnabled = true;
}

void enableDebug(bool enable)
{
    debugEnabled = enable;
}

void enableWarnings(bool enable)
{
    warningsEnabled = enable;
}

QDebug enabledDebug()
{
    if (debugEnabled)
        return qDebug();
    else
        return QDebug(&discard);
}

QDebug enabledWarning()
{
    if (warningsEnabled)
        return qWarning();
    else
        return QDebug(&discard);
}

#else /* #ifdef ENABLE_DEBUG */

void enableDebug(bool enable)
{
}

void enableWarnings(bool enable)
{
}

#endif /* #ifdef ENABLE_DEBUG */

} /* namespace Telepathy */
