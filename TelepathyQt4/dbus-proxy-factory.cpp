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

#include <TelepathyQt4/DBusProxyFactory>
#include "TelepathyQt4/dbus-proxy-factory-internal.h"

#include "TelepathyQt4/_gen/dbus-proxy-factory-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/Feature>

#include <QDBusConnection>

namespace Tp
{
struct DBusProxyFactory::Private
{
    QDBusConnection bus;
    Features features;
    Cache *cache;

    Private(const QDBusConnection &bus)
        : bus(bus), cache(new Cache) {}

    ~Private()
    {
        delete cache;
    }
};

void DBusProxyFactory::addFeature(const Feature &feature)
{
    addFeatures(Features(feature));
}

void DBusProxyFactory::addFeatures(const Features &features)
{
    mPriv->features.unite(features);
}

Features DBusProxyFactory::features() const
{
    return mPriv->features;
}

PendingReady *DBusProxyFactory::getProxy(const QString &serviceName, const QString &objectPath,
        const QVariantMap &immutableProperties) const
{
    /*
     * Should do the following:
     *
     * Find out if proxy identified by (serviceName, objectPath) is already in cache - if so, return
     * it->becomeReady(mPriv->features)
     *
     * Otherwise, do:
     * 1) put construct(serviceName, objectPath, immutableProperties) in cache, keying by
     *   (serviceName, objectPath)
     * 2) do prepare() and if it returns non-NULL, return PendingReady waiting for it and only then
     *    doing becomeReady(mPriv->features) as a nested query (probably implement this only later,
     *    no use-case for now)
     * 3) If prepare() returned NULL, just return becomeReady(mPriv->features)
     */

    return NULL;
}

DBusProxyFactory::~DBusProxyFactory()
{
    delete mPriv;
}

DBusProxyFactory::DBusProxyFactory(const QDBusConnection &bus)
    : mPriv(new Private(bus))
{
}

PendingOperation *DBusProxyFactory::prepare(const SharedPtr<RefCounted> &object) const
{
    // Nothing we could think about needs doing
    return NULL;
}

DBusProxyFactory::Cache::Cache()
{
}

DBusProxyFactory::Cache::~Cache()
{
}

SharedPtr<RefCounted> DBusProxyFactory::Cache::get(const Key &key) const
{
    return SharedPtr<RefCounted>(proxies.value(key));
}

void DBusProxyFactory::Cache::put(const Key &key, const SharedPtr<RefCounted> &obj)
{
    Q_ASSERT(!proxies.contains(key));

    // This sucks because DBusProxy is not RefCounted...
    connect(dynamic_cast<DBusProxy*>(obj.data()),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onProxyInvalidated(Tp::DBusProxy*)));

    debug() << "Inserting to factory cache proxy for" << key;

    proxies.insert(key, obj);
}

void DBusProxyFactory::Cache::onProxyInvalidated(Tp::DBusProxy *proxy)
{
    Key key(proxy->busName(), proxy->objectPath());

    // Not having it would indicate invalidated() signaled twice for the same proxy, or us having
    // connected to two proxies with the same key, neither of which should happen
    Q_ASSERT(proxies.contains(key));

    debug() << "Removing from factory cache invalidated proxy for" << key;

    proxies.remove(key);
}

}
