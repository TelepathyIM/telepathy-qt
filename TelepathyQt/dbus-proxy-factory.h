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

#ifndef _TelepathyQt_dbus_proxy_factory_h_HEADER_GUARD_
#define _TelepathyQt_dbus_proxy_factory_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>
#include <TelepathyQt/SharedPtr>
#include <TelepathyQt/Types>

// For Q_DISABLE_COPY
#include <QtGlobal>

#include <QString>

class QDBusConnection;

namespace Tp
{

class Features;
class PendingReady;
class PendingOperation;

class TP_QT_EXPORT DBusProxyFactory : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(DBusProxyFactory)

public:
    virtual ~DBusProxyFactory();

    const QDBusConnection &dbusConnection() const;

protected:
    DBusProxyFactory(const QDBusConnection &bus);

    DBusProxyPtr cachedProxy(const QString &busName, const QString &objectPath) const;

    PendingReady *nowHaveProxy(const DBusProxyPtr &proxy) const;

    // I don't want this to be non-pure virtual, because I want ALL subclasses to have to think
    // about whether or not they need to uniquefy the name or not. If a subclass doesn't implement
    // this while it should, matching with the cache for future requests and invalidation breaks.
    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const = 0;

    virtual PendingOperation *initialPrepare(const DBusProxyPtr &proxy) const;
    virtual PendingOperation *readyPrepare(const DBusProxyPtr &proxy) const;

    virtual Features featuresFor(const DBusProxyPtr &proxy) const = 0;

private:
    class Cache;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
