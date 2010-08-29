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

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/Feature>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/PendingReady>

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

PendingReady *DBusProxyFactory::getProxy(const QString &busName, const QString &objectPath,
        const QVariantMap &immutableProperties) const
{
    // I really hate the casts needed in this function - we must really do something about the
    // DBusProxy class hierarchy so that every DBusProxy(Something) is always a ReadyObject and a
    // RefCounted, in the API/ABI break - then most of these proxyMisc-> things become just proxy->

    QString finalName = finalBusNameFrom(busName);
    Features specificFeatures = featuresFor(busName, objectPath, immutableProperties);

    SharedPtr<RefCounted> proxy = mPriv->cache->get(Cache::Key(finalName, objectPath));
    if (!proxy) {
        proxy = construct(bus(), busName, objectPath, immutableProperties);

        QString actualBusName = dynamic_cast<DBusProxy *>(proxy.data())->busName();
        if (actualBusName != finalName) {
            warning() << "Specified final name" << finalName
                      << "doesn't match actual name" << actualBusName;
            warning().nospace() << "For proxy caching to work, reimplement finalBusNameFrom() "
                "correctly in your DBusProxyFactory subclass";
        }

        // We can still make invalidation work by storing it by the actual bus name
        mPriv->cache->put(Cache::Key(actualBusName, objectPath), proxy);

        PendingOperation *prepareOp = prepare(proxy);
        if (prepareOp) {
            QObject *proxyQObject = dynamic_cast<QObject *>(proxy.data());
            Q_ASSERT(proxyQObject != NULL);
            return new PendingReady(prepareOp, specificFeatures, proxyQObject, proxyQObject);
        }
    }

    // This sucks ...
    ReadyObject *proxyReady = dynamic_cast<ReadyObject *>(proxy.data());
    Q_ASSERT(proxyReady != NULL);

    // ... but this sucks even more!
    QObject *proxyQObject = dynamic_cast<QObject *>(proxy.data());
    Q_ASSERT(proxyQObject != NULL);

    if (!specificFeatures.isEmpty() && !proxyReady->isReady(specificFeatures)) {
        return proxyReady->becomeReady(specificFeatures);
    }

    // No features requested or they are all ready - optimize a bit by not calling ReadinessHelper
    PendingReady *readyOp = new PendingReady(specificFeatures, proxyQObject, proxyQObject);
    readyOp->setFinished();
    return readyOp;
}

DBusProxyFactory::~DBusProxyFactory()
{
    delete mPriv;
}

const QDBusConnection &DBusProxyFactory::bus() const
{
    return mPriv->bus;
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

Features DBusProxyFactory::featuresFor(const QString &busName, const QString &objectPath,
        const QVariantMap &immutableProperties) const
{
    Q_UNUSED(busName);
    Q_UNUSED(objectPath);
    Q_UNUSED(immutableProperties);

    return features();
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

    if (!counted || !dynamic_cast<DBusProxy *>(counted.data())->isValid()) {
        // Weak pointer invalidated or proxy invalidated during this mainloop iteration and we still
        // haven't got the invalidated() signal for it
        return SharedPtr<RefCounted>();
    }

    return counted;
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

AccountFactory::~AccountFactory()
{
}

AccountFactoryPtr AccountFactory::create(const QDBusConnection &bus)
{
    return AccountFactoryPtr(new AccountFactory(bus));
}

AccountFactoryPtr AccountFactory::coreFactory(const QDBusConnection &bus)
{
    AccountFactoryPtr factory(new AccountFactory(bus));

    factory->addFeature(Account::FeatureCore);

    return factory;
}

AccountFactory::AccountFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus)
{
}

QString AccountFactory::finalBusNameFrom(const QString &uniqueOrWellKnown) const
{
    return uniqueOrWellKnown;
}

SharedPtr<RefCounted> AccountFactory::construct(const QDBusConnection &busConnection,
        const QString &busName, const QString &objectPath,
        const QVariantMap &immutableProperties) const
{
    Q_UNUSED(immutableProperties);

    return Account::create(busConnection, busName, objectPath);
}

ConnectionFactory::~ConnectionFactory()
{
}

ConnectionFactoryPtr ConnectionFactory::create(const QDBusConnection &bus)
{
    return ConnectionFactoryPtr(new ConnectionFactory(bus));
}

ConnectionFactory::ConnectionFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus)
{
}

QString ConnectionFactory::finalBusNameFrom(const QString &uniqueOrWellKnown) const
{
    return StatefulDBusProxy::uniqueNameFrom(bus(), uniqueOrWellKnown);
}

SharedPtr<RefCounted> ConnectionFactory::construct(const QDBusConnection &busConnection,
        const QString &busName, const QString &objectPath,
        const QVariantMap &immutableProperties) const
{
    Q_UNUSED(immutableProperties);

    return Connection::create(busConnection, busName, objectPath);
}

}
