/**
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/Utils>

#include <QByteArray>
#include <QString>

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

} // Tp
