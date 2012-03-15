/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt_utils_h_HEADER_GUARD_
#define _TelepathyQt_utils_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

#include <QString>
#include <QVariant>

namespace Tp
{

TP_QT_EXPORT QString escapeAsIdentifier(const QString &string);

TP_QT_EXPORT bool checkValidProtocolName(const QString &protocolName);

TP_QT_EXPORT QVariant::Type variantTypeFromDBusSignature(const QString &signature);
TP_QT_EXPORT QVariant parseValueWithDBusSignature(const QString &value,
        const QString &dbusSignature);

} // Tp

#endif
