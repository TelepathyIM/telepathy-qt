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

#include <TelepathyQt/DBusProxyFactory>
#include "TelepathyQt/dbus-proxy-factory-internal.h"

#include "TelepathyQt/_gen/dbus-proxy-factory.moc.hpp"
#include "TelepathyQt/_gen/dbus-proxy-factory-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/ReadyObject>
#include <TelepathyQt/PendingReady>

#include <QDBusConnection>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusProxyFactory::Private
{
    Private(const QDBusConnection &bus)
        : bus(bus),
          cache(new Cache)
    {
    }

    ~Private()
    {
        delete cache;
    }

    QDBusConnection bus;
    Cache *cache;
};

/**
 * \class DBusProxyFactory
 * \ingroup utils
 * \headerfile TelepathyQt/dbus-proxy-factory.h <TelepathyQt/DBusProxyFactory>
 *
 * \brief The DBusProxyFactory class is a base class for all D-Bus proxy factory
 * classes. Handles proxy caching and making them ready as appropriate.
 */

/**
 * Construct a new DBusProxyFactory object.
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
 * Return the D-Bus connection all of the proxies from this factory communicate with.
 *
 * \return A QDBusConnection object.
 */
const QDBusConnection &DBusProxyFactory::dbusConnection() const
{
    return mPriv->bus;
}

/**
 * Return a cached proxy with the given \a busName and \a objectPath.
 *
 * If a proxy has not been previously put into the cache by nowHaveProxy for those identifying
 * attributes, or a previously cached proxy has since been invalidated and/or destroyed, a \c Null
 * shared pointer is returned instead.
 *
 * \param busName Bus name of the proxy to return.
 * \param objectPath Object path of the proxy to return.
 * \return A pointer to the DBusProxy object, if any.
 */
DBusProxyPtr DBusProxyFactory::cachedProxy(const QString &busName,
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
 * cached already, doing any initialPrepare()/readyPrepare() work if appropriate, and making the
 * features from featuresFor() ready if they aren't already.
 *
 * The returned PendingReady only finishes when the initialPrepare() and readyPrepare() operations
 * for the proxy has completed, and the requested features have all been made ready (or found unable
 * to be made ready). Note that this might have happened already before calling this function, if
 * the proxy was not a newly created one, but was looked up from the cache. DBusProxyFactory handles
 * the necessary subleties for this to work.
 *
 * Access to the proxy instance is allowed as soon as this method returns through
 * PendingReady::proxy(), if the proxy is needed in a context where it's not required to be ready.
 *
 * \param proxy The proxy which the factory should now make sure is prepared and made ready.
 * \return A PendingReady operation which will emit PendingReady::finished
 *         when the proxy is usable.
 */
PendingReady *DBusProxyFactory::nowHaveProxy(const DBusProxyPtr &proxy) const
{
    Q_ASSERT(!proxy.isNull());

    mPriv->cache->put(proxy);
    return new PendingReady(SharedPtr<DBusProxyFactory>((DBusProxyFactory*) this),
           proxy, featuresFor(proxy));
}

/**
 * \fn QString DBusProxyFactory::finalBusNameFrom(const QString &uniqueOrWellKnown) const
 *
 * "Normalize" a bus name according to the rules for the proxy class to construct.
 *
 * Should be implemented by subclasses to transform the application-specified name \a
 * uniqueOrWellKnown to whatever the proxy constructed for that name would have in its
 * DBusProxy::busName() in the end.
 *
 * For StatelessDBusProxy sub-classes this should mostly be an identity transform, while for
 * StatefulDBusProxy sub-classes StatefulDBusProxy::uniqueNameFrom() or an equivalent thereof should
 * be used in most cases.
 *
 * If this is not implemented correctly, caching won't work properly.
 *
 * \param uniqueOrWellKnown Any valid D-Bus service name, either unique or well-known.
 * \return Whatever that name would turn to, when a proxy is constructed for it.
 */

/**
 * Allows subclasses to do arbitrary manipulation on the proxy before it is attempted to be made
 * ready.
 *
 * If a non-\c NULL operation is returned, the completion of that operation is waited for before
 * starting to make the object ready whenever nowHaveProxy() is called the first time around for a
 * given proxy.
 *
 * \todo FIXME actually implement this... :) Currently just a vtable placeholder.
 * \param proxy The just-constructed proxy to be prepared.
 * \return \c NULL ie. nothing to do.
 */
PendingOperation *DBusProxyFactory::initialPrepare(const DBusProxyPtr &proxy) const
{
    // Nothing we could think about needs doing
    return NULL;
}

/**
 * Allows subclasses to do arbitrary manipulation on the proxy after it has been made ready.
 *
 * If a non-\c NULL operation is returned, the completion of that operation is waited for before
 * signaling that the object is ready for use after ReadyObject::becomeReady() for it has finished
 * whenever nowHaveProxy() is called the first time around for a given proxy.
 *
 * \todo FIXME actually implement this... :) Currently just a vtable placeholder.
 * \param proxy The just-readified proxy to be prepared.
 * \return \c NULL ie. nothing to do.
 */
PendingOperation *DBusProxyFactory::readyPrepare(const DBusProxyPtr &proxy) const
{
    // Nothing we could think about needs doing
    return NULL;
}

/**
 * \fn Features DBusProxyFactory::featuresFor(const SharedPtr<RefCounted> &proxy) const
 *
 * Return the features which should be made ready on a given proxy.
 *
 * This can be used to implement instance-specific features based on arbitrary criteria.
 * FixedFeatureFactory implements this as a fixed set of features independent of the instance,
 * however.
 *
 * It should be noted that if an empty set of features is returned, ReadyObject::becomeReady() is
 * not called at all. In other words, any "core feature" is not automatically added to the requested
 * features. This is to enable setting a factory to not make proxies ready at all, which is useful
 * eg. in the case of account editing UIs which aren't interested in the state of Connection objects
 * for the Account objects they're editing.
 *
 * \param proxy The proxy on which the returned features will be made ready.
 * \return A list of Feature objects.
 */

DBusProxyFactory::Cache::Cache()
{
}

DBusProxyFactory::Cache::~Cache()
{
}

DBusProxyPtr DBusProxyFactory::Cache::get(const Key &key) const
{
    DBusProxyPtr proxy(proxies.value(key));

    if (proxy.isNull() || !proxy->isValid()) {
        // Weak pointer invalidated or proxy invalidated during this mainloop iteration and we still
        // haven't got the invalidated() signal for it
        return DBusProxyPtr();
    }

    return proxy;
}

void DBusProxyFactory::Cache::put(const DBusProxyPtr &proxy)
{
    if (proxy->busName().isEmpty()) {
        debug() << "Not inserting proxy" << proxy.data() << "with no bus name to factory cache";
        return;
    } else if (!proxy->isValid()) {
        debug() << "Not inserting to factory cache invalid proxy - proxy is for" <<
            proxy->busName() << ',' << proxy->objectPath();
        return;
    }

    Key key(proxy->busName(), proxy->objectPath());

    DBusProxyPtr existingProxy(proxies.value(key));
    if (!existingProxy || existingProxy != proxy) {
        // Disconnect the invalidated signal from the proxy we're replacing, so it won't uselessly
        // cause the new (hopefully valid) proxy to be dropped from the cache if it arrives late.
        //
        // The window in which this makes a difference is very slim but existent; namely, somebody
        // must request a proxy from the factory in the same mainloop iteration as an otherwise
        // matching proxy has invalidated itself. The invalidation signal would be delivered and
        // processed only during the next mainloop iteration.
        if (existingProxy) {
            Q_ASSERT(!existingProxy->isValid());
            existingProxy->disconnect(
                    SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                    this,
                    SLOT(onProxyInvalidated(Tp::DBusProxy*)));

            debug() << "Replacing invalidated proxy" << existingProxy.data() << "in cache for name"
                << existingProxy->busName() << ',' << existingProxy->objectPath();
        }

        connect(proxy.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                SLOT(onProxyInvalidated(Tp::DBusProxy*)));

        debug() << "Inserting to factory cache proxy for" << key;
        proxies.insert(key, proxy);
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
