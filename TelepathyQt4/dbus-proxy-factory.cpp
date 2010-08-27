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

#include <TelepathyQt4/Feature>

#include <QDBusConnection>

namespace Tp
{
struct DBusProxyFactory::Private
{
    QDBusConnection bus;
    Features features;

    // TODO implement cache (will be a separate QObject in a private header)

    Private(const QDBusConnection &bus)
        : bus(bus) {}
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

PendingOperation *DBusProxyFactory::prepare(const SharedPtr<DBusProxy> &object) const
{
    // Nothing we could think about needs doing
    return NULL;
}

}
