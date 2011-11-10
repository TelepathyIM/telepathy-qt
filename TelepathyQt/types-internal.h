/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_types_internal_h_HEADER_GUARD_
#define _TelepathyQt_types_internal_h_HEADER_GUARD_

#include <QDBusVariant>
#include <QDBusArgument>
#include <TelepathyQt/Types>

namespace Tp
{

/**
 * Private structure used to circumvent QtDBus' strict demarshalling
 */
struct TP_QT_EXPORT SUSocketAddress
{
    /**
     * A dotted-quad IPv4 address literal: four ASCII decimal numbers, each
     * between 0 and 255 inclusive, e.g. &quot;192.168.0.1&quot;.
     */
    QString address;
    /**
     * The TCP or UDP port number.
     */
    uint port;
};

TP_QT_EXPORT bool operator==(const SUSocketAddress& v1, const SUSocketAddress& v2);
TP_QT_EXPORT inline bool operator!=(const SUSocketAddress& v1, const SUSocketAddress& v2)
{
    return !operator==(v1, v2);
}
TP_QT_EXPORT QDBusArgument& operator<<(QDBusArgument& arg, const SUSocketAddress& val);
TP_QT_EXPORT const QDBusArgument& operator>>(const QDBusArgument& arg, SUSocketAddress& val);

} // Tp

// specialise for Tp::SocketAddressIPv4, allowing it to be used in place of QDBusVariant
template<> inline Tp::SocketAddressIPv4 qdbus_cast<Tp::SocketAddressIPv4>(const QDBusArgument &arg,
Tp::SocketAddressIPv4 *)
{
    if (arg.currentSignature() == QLatin1String("(su)")) {
        // Use Tp::SUSocketAddress
        Tp::SUSocketAddress saddr = qdbus_cast<Tp::SUSocketAddress>(arg);
        Tp::SocketAddressIPv4 addr;
        addr.address = saddr.address;
        addr.port = saddr.port;
        return addr;
    } else if (arg.currentSignature() == QLatin1String("(sq)")) {
        // Keep it standard
        Tp::SocketAddressIPv4 item;
        arg >> item;
        return item;
    } else {
        // This should never happen...
        return Tp::SocketAddressIPv4();
    }
}

template<> inline Tp::SocketAddressIPv4 qdbus_cast<Tp::SocketAddressIPv4>(const QVariant &v, Tp::SocketAddressIPv4 *)
{
    int id = v.userType();
    if (id == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(v);

        if (arg.currentSignature() == QLatin1String("(su)")) {
            // Use Tp::SUSocketAddress
            Tp::SUSocketAddress saddr = qdbus_cast<Tp::SUSocketAddress>(arg);
            Tp::SocketAddressIPv4 addr;
            addr.address = saddr.address;
            addr.port = saddr.port;
            return addr;
        } else if (arg.currentSignature() == QLatin1String("(sq)")) {
            // Keep it standard
            Tp::SocketAddressIPv4 item;
            arg >> item;
            return item;
        } else {
            // This should never happen...
            return Tp::SocketAddressIPv4();
        }
    } else
        return qvariant_cast<Tp::SocketAddressIPv4>(v);
}

// specialise for Tp::SocketAddressIPv6, allowing it to be used in place of QDBusVariant
template<> inline Tp::SocketAddressIPv6 qdbus_cast<Tp::SocketAddressIPv6>(const QDBusArgument &arg,
Tp::SocketAddressIPv6 *)
{
    if (arg.currentSignature() == QLatin1String("(su)")) {
        // Use Tp::SUSocketAddress
        Tp::SUSocketAddress saddr = qdbus_cast<Tp::SUSocketAddress>(arg);
        Tp::SocketAddressIPv6 addr;
        addr.address = saddr.address;
        addr.port = saddr.port;
        return addr;
    } else if (arg.currentSignature() == QLatin1String("(sq)")) {
        // Keep it standard
        Tp::SocketAddressIPv6 item;
        arg >> item;
        return item;
    } else {
        // This should never happen...
        return Tp::SocketAddressIPv6();
    }
}

template<> inline Tp::SocketAddressIPv6 qdbus_cast<Tp::SocketAddressIPv6>(const QVariant &v, Tp::SocketAddressIPv6 *)
{
    int id = v.userType();
    if (id == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(v);

        if (arg.currentSignature() == QLatin1String("(su)")) {
            // Use Tp::SUSocketAddress
            Tp::SUSocketAddress saddr = qdbus_cast<Tp::SUSocketAddress>(arg);
            Tp::SocketAddressIPv6 addr;
            addr.address = saddr.address;
            addr.port = saddr.port;
            return addr;
        } else if (arg.currentSignature() == QLatin1String("(sq)")) {
            // Keep it standard
            Tp::SocketAddressIPv6 item;
            arg >> item;
            return item;
        } else {
            // This should never happen...
            return Tp::SocketAddressIPv6();
        }
    } else
        return qvariant_cast<Tp::SocketAddressIPv6>(v);
}

Q_DECLARE_METATYPE(Tp::SUSocketAddress)

#endif
