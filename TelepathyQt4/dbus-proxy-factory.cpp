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

/**
 * \class DBusProxyFactory
 * \ingroup clientreadiness
 * \headerfile TelepathyQt4/dbus-proxy-factory.h <TelepathyQt4/DBusProxyFactory>
 *
 * Base class for all D-Bus proxy factory classes. Handles proxy caching and making them ready as
 * appropriate.
 */

/**
 * Class constructor.
 *
 * The intention for storing the bus here is that it generally doesn't make sense to construct
 * proxies for multiple buses in the same context. Allowing that would lead to more complex keying
 * needs in the cache, as well.
 *
 * \param bus The D-Bus bus connection for the objects constructed using this factory.
 */
DBusProxyFactory::DBusProxyFactory(const QDBusConnection &bus)
    : mPriv(new Private(bus))
{
}

/**
 * Class destructor.
 */
DBusProxyFactory::~DBusProxyFactory()
{
    delete mPriv;
}

/**
 * Returns the D-Bus connection all of the proxies from this factory communicate with.
 *
 * \return The connection.
 */
const QDBusConnection &DBusProxyFactory::dbusConnection() const
{
    return mPriv->bus;
}

/**
 * Returns a cached proxy with the given \a busName and \a objectPath.
 *
 * If a proxy has not been previously put into the cache by nowHaveProxy for those identifying
 * attributes, or a previously cached proxy has since been invalidated and/or destroyed, a \c Null
 * shared pointer is returned instead.
 *
 * \param busName Bus name of the proxy to return.
 * \param objectPath Object path of the proxy to return.
 * \return The proxy, if any.
 */
SharedPtr<RefCounted> DBusProxyFactory::cachedProxy(const QString &busName,
        const QString &objectPath) const
{
    QString finalName = finalBusNameFrom(busName);
    return mPriv->cache->get(Cache::Key(finalName, objectPath));
}

/**
 * Should be called by subclasses when they have a proxy, be it a newly-constructed one or one from
 * the cache.
 *
 * This function will then do the rest of the factory work, including caching the proxy if it's not
 * cached already, doing any prepare() work if appropriate, and making the features from
 * featuresFor() ready if they aren't already.
 *
 * The returned PendingReady only finishes when the prepare() operation for the proxy has completed,
 * and the requested features have all been made ready (or found unable to be made ready). Note that
 * this might have happened already before calling this function, if the proxy was not a newly
 * created one, but was looked up from the cache. DBusProxyFactory handles the necessary subleties
 * for this to work.
 *
 * Access to the proxy instance is allowed as soon as this method returns through
 * PendingReady::proxy(), if the proxy is needed in a context where it's not required to be ready.
 *
 * \param proxy The proxy which the factory should now make sure is prepared and made ready.
 * \return Readifying operation, which finishes when the proxy is usable.
 */
PendingReady *DBusProxyFactory::nowHaveProxy(const SharedPtr<RefCounted> &proxy) const
{
    Q_ASSERT(!proxy.isNull());

    // I really hate the casts needed in this function - we must really do something about the
    // DBusProxy class hierarchy so that every DBusProxy(Something) is always a ReadyObject and a
    // RefCounted, in the API/ABI break - then most of these proxyMisc-> things become just proxy->

    ReadyObject *proxyReady = dynamic_cast<ReadyObject *>(proxy.data());
    Q_ASSERT(proxyReady != NULL);

    Features specificFeatures = featuresFor(proxy);

    // FIXME: prepare currently doesn't work
    PendingOperation *prepareOp = NULL;

    mPriv->cache->put(proxy);

    if (prepareOp || (!specificFeatures.isEmpty() && !proxyReady->isReady(specificFeatures))) {
        return new PendingReady(prepareOp, specificFeatures, proxy, 0);
    }

    // No features requested or they are all ready - optimize a bit by not calling ReadinessHelper
    PendingReady *readyOp = new PendingReady(0, specificFeatures, proxy, 0);
    readyOp->setFinished();
    return readyOp;
}

/**
 * Allows subclasses to do arbitrary manipulation on the object before it is attempted to be made
 * ready.
 *
 * If a non-\c NULL operation is returned, the completion of that operation is waited for before
 * starting to make the object ready whenever nowHaveProxy() is called the first time around for a
 * given proxy.
 *
 * \todo FIXME actually implement this... :)
 * \return \c NULL ie. nothing to do.
 */
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

void DBusProxyFactory::Cache::put(const SharedPtr<RefCounted> &obj)
{
    DBusProxy *proxyProxy = dynamic_cast<DBusProxy *>(obj.data());
    Q_ASSERT(proxyProxy != NULL);

    Key key(proxyProxy->busName(), proxyProxy->objectPath());

    SharedPtr<RefCounted> existingProxy = SharedPtr<RefCounted>(proxies.value(key));
    if (!existingProxy || existingProxy != obj) {
        connect(proxyProxy,
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                SLOT(onProxyInvalidated(Tp::DBusProxy*)));

        debug() << "Inserting to factory cache proxy for" << key;
        proxies.insert(key, obj);
    }
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
