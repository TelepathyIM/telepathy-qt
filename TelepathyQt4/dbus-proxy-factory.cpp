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
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/PendingReady>

#include <QDBusConnection>

namespace Tp
{

struct DBusProxyFactory::Private
{
    Private(const QDBusConnection &bus)
        : bus(bus), cache(new Cache) {}

    ~Private()
    {
        delete cache;
    }

    QDBusConnection bus;
    Cache *cache;
};

DBusProxyFactory::DBusProxyFactory(const QDBusConnection &bus)
    : mPriv(new Private(bus))
{
}

DBusProxyFactory::~DBusProxyFactory()
{
    delete mPriv;
}

const QDBusConnection &DBusProxyFactory::dbusConnection() const
{
    return mPriv->bus;
}

SharedPtr<RefCounted> DBusProxyFactory::cachedProxy(const QString &busName,
        const QString &objectPath) const
{
    QString finalName = finalBusNameFrom(busName);
    return mPriv->cache->get(Cache::Key(finalName, objectPath));
}

PendingReady *DBusProxyFactory::nowHaveProxy(const SharedPtr<RefCounted> &proxy, bool created) const
{
    Q_ASSERT(!proxy.isNull());

    // I really hate the casts needed in this function - we must really do something about the
    // DBusProxy class hierarchy so that every DBusProxy(Something) is always a ReadyObject and a
    // RefCounted, in the API/ABI break - then most of these proxyMisc-> things become just proxy->

    DBusProxy *proxyProxy = dynamic_cast<DBusProxy *>(proxy.data());
    ReadyObject *proxyReady = dynamic_cast<ReadyObject *>(proxy.data());

    Q_ASSERT(proxyProxy != NULL);
    Q_ASSERT(proxyReady != NULL);

    Features specificFeatures = featuresFor(proxy);

    // TODO: lookup existing prepareOp, if any, from a private mapping
    PendingOperation *prepareOp = NULL;

    if (created) {
        mPriv->cache->put(Cache::Key(proxyProxy->busName(), proxyProxy->objectPath()), proxy);
        prepareOp = prepare(proxy);
        // TODO: insert to private prepare op mapping and make sure it's removed when it finishes/is
        // destroyed
    }

    if (prepareOp || (!specificFeatures.isEmpty() && !proxyReady->isReady(specificFeatures))) {
        return new PendingReady(prepareOp, specificFeatures, proxy, 0);
    }

    // No features requested or they are all ready - optimize a bit by not calling ReadinessHelper
    PendingReady *readyOp = new PendingReady(0, specificFeatures, proxy, 0);
    readyOp->setFinished();
    return readyOp;
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
    SharedPtr<RefCounted> counted(proxies.value(key));

    // We already assert for it being a DBusProxy in put()
    if (!counted || !dynamic_cast<DBusProxy *>(counted.data())->isValid()) {
        // Weak pointer invalidated or proxy invalidated during this mainloop iteration and we still
        // haven't got the invalidated() signal for it
        return SharedPtr<RefCounted>();
    }

    return counted;
}

void DBusProxyFactory::Cache::put(const Key &key, const SharedPtr<RefCounted> &obj)
{
    DBusProxy *proxyProxy = dynamic_cast<DBusProxy *>(obj.data());
    Q_ASSERT(proxyProxy != NULL);

    // This sucks because DBusProxy is not RefCounted...
    connect(proxyProxy,
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
