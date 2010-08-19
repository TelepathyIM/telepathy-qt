/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_utils_h_HEADER_GUARD_
#define _TelepathyQt4_utils_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Global>

#include <QByteArray>
#include <QDBusSignature>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Tp
{

TELEPATHY_QT4_EXPORT QString escapeAsIdentifier(const QString &string);

TELEPATHY_QT4_EXPORT bool unescapeString(const QByteArray &data, int from, int to,
        QString &result);
TELEPATHY_QT4_EXPORT bool unescapeStringList(const QByteArray &data, int from, int to,
        QStringList &result);

TELEPATHY_QT4_EXPORT QVariant::Type variantTypeFromDBusSignature(const QDBusSignature &signature);
TELEPATHY_QT4_EXPORT QVariant variantFromValueWithDBusSignature(const QString &value,
        const QDBusSignature &signature);

} // Tp

#endif
