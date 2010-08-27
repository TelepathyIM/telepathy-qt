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

#ifndef _TelepathyQt4_dbus_proxy_factory_h_HEADER_GUARD_
#define _TelepathyQt4_dbus_proxy_factory_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Global>
#include <TelepathyQt4/SharedPtr>

// For Q_DISABLE_COPY
#include <QtGlobal>

#include <QString>
#include <QVariantMap>

class QDBusConnection;

namespace Tp
{

class Feature;
class Features;
class DBusProxy;
class PendingReady;
class PendingOperation;

class DBusProxyFactory : public RefCounted
{
    Q_DISABLE_COPY(DBusProxyFactory)

public:
    void addFeature(const Feature &feature);
    void addFeatures(const Features &features);

    Features features() const;

    PendingReady *getProxy(const QString &serviceName, const QString &objectPath,
            const QVariantMap &immutableProperties) const;

    virtual ~DBusProxyFactory();

protected:
    DBusProxyFactory(const QDBusConnection &bus);

    // API/ABI break TODO: Make DBusProxy be a RefCounted so this can be SharedPtr<DBusProxy>
    // If we don't want DBusProxy itself be a RefCounted, let's add RefCountedDBusProxy or something
    // as an intermediate subclass?
    virtual SharedPtr<RefCounted> construct(const QString &serviceName, const QString &objectPath,
            const QVariantMap &immutableProperties) const = 0;
    virtual PendingOperation *prepare(const SharedPtr<RefCounted> &object) const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
