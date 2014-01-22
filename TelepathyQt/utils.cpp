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

#include <TelepathyQt/Utils>

#include "TelepathyQt/key-file.h"

#include <QByteArray>
#include <QStringList>

/**
 * \defgroup utility functions
 */

namespace Tp
{

static inline bool isBad(char c, bool isFirst)
{
    return ((c < 'a' || c > 'z') &&
            (c < 'A' || c > 'Z') &&
            (c < '0' || c > '9' || isFirst));
}

/**
 * Escape an arbitrary string so it follows the rules for a C identifier,
 * and hence an object path component, interface element component,
 * bus name component or member name in D-Bus.
 *
 * This is a reversible encoding, so it preserves distinctness.
 *
 * The escaping consists of replacing all non-alphanumerics, and the first
 * character if it's a digit, with an underscore and two lower-case hex
 * digits:
 *
 *    "0123abc_xyz\x01\xff" -> _30123abc_5fxyz_01_ff
 *
 * i.e. similar to URI encoding, but with _ taking the role of %, and a
 * smaller allowed set. As a special case, "" is escaped to "_" (just for
 * completeness, really).
 *
 * \param string The string to be escaped.
 * \return the escaped string.
 */
QString escapeAsIdentifier(const QString &string)
{
    bool bad = false;
    QByteArray op;
    QByteArray utf8;
    const char *name;
    const char *ptr, *firstOk;

    /* This function is copy/pasted from tp_escape_as_identified from
     * telepathy-glib. */

    /* fast path for empty name */
    if (string.isEmpty()) {
        return QString::fromLatin1("_");
    }

    utf8 = string.toUtf8();
    name = utf8.constData();
    for (ptr = name; *ptr; ptr++) {
        if (isBad(*ptr, ptr == name)) {
            bad = true;
            break;
        }
    }

    /* fast path if it's clean */
    if (!bad) {
        return string;
    }

    /* If strictly less than ptr, firstOk is the first uncopied safe character.
     */
    firstOk = name;
    for (ptr = name; *ptr; ptr++) {
        if (isBad(*ptr, ptr == name)) {
            char buf[4] = { 0, };

            /* copy preceding safe characters if any */
            if (firstOk < ptr) {
                op.append(firstOk, ptr - firstOk);
            }

            /* escape the unsafe character */
            qsnprintf(buf, sizeof (buf), "_%02x", (unsigned char)(*ptr));
            op.append(buf);

            /* restart after it */
            firstOk = ptr + 1;
        }
    }
    /* copy trailing safe characters if any */
    if (firstOk < ptr) {
        op.append(firstOk, ptr - firstOk);
    }

    return QString::fromLatin1(op.constData());
}

bool checkValidProtocolName(const QString &protocolName)
{
    if (protocolName.isEmpty()) {
        return false;
    }

    if (!protocolName[0].isLetter()) {
        return false;
    }

    int length = protocolName.length();
    if (length <= 1) {
        return true;
    }

    QChar ch;
    for (int i = 1; i < length; ++i) {
        ch = protocolName[i];
        if (!ch.isLetterOrNumber() && ch != QLatin1Char('-')) {
            return false;
        }
    }

    return true;
}

QVariant::Type variantTypeFromDBusSignature(const QString &signature)
{
    QVariant::Type type;
    if (signature == QLatin1String("b")) {
        type = QVariant::Bool;
    } else if (signature == QLatin1String("n") || signature == QLatin1String("i")) {
        type = QVariant::Int;
    } else if (signature == QLatin1String("q") || signature == QLatin1String("u")) {
        type = QVariant::UInt;
    } else if (signature == QLatin1String("x")) {
        type = QVariant::LongLong;
    } else if (signature == QLatin1String("t")) {
        type = QVariant::ULongLong;
    } else if (signature == QLatin1String("d")) {
        type = QVariant::Double;
    } else if (signature == QLatin1String("as")) {
        type = QVariant::StringList;
    } else if (signature == QLatin1String("s") || signature == QLatin1String("o")) {
        type = QVariant::String;
    } else {
        type = QVariant::Invalid;
    }

    return type;
}

QVariant parseValueWithDBusSignature(const QString &value,
        const QString &dbusSignature)
{
    QVariant::Type type = variantTypeFromDBusSignature(dbusSignature);

    if (type == QVariant::Invalid) {
        return QVariant(type);
    }

    switch (type) {
        case QVariant::Bool:
            {
                if (value.toLower() == QLatin1String("true") ||
                    value == QLatin1String("1")) {
                    return QVariant(true);
                } else {
                    return QVariant(false);
                }
                break;
            }
        case QVariant::Int:
            return QVariant(value.toInt());
        case QVariant::UInt:
            return QVariant(value.toUInt());
        case QVariant::LongLong:
            return QVariant(value.toLongLong());
        case QVariant::ULongLong:
            return QVariant(value.toULongLong());
        case QVariant::Double:
            return QVariant(value.toDouble());
        case QVariant::StringList:
            {
                QStringList list;
                QByteArray rawValue = value.toLatin1();
                if (KeyFile::unescapeStringList(rawValue, 0, rawValue.size(), list)) {
                    return QVariant(list);
                } else {
                    return QVariant(QVariant::Invalid);
                }
            }
        default:
            break;
    }
    return QVariant(value);
}

} // Tp
