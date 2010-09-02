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
#include <TelepathyQt4/Types>

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

class TELEPATHY_QT4_EXPORT DBusProxyFactory : public RefCounted
{
    Q_DISABLE_COPY(DBusProxyFactory)

public:
    virtual ~DBusProxyFactory();

    const QDBusConnection &dbusConnection() const;

protected:
    DBusProxyFactory(const QDBusConnection &bus);

    // API/ABI break TODO: Make DBusProxy be a RefCounted so this can be SharedPtr<DBusProxy>
    // If we don't want DBusProxy itself be a RefCounted, let's add RefCountedDBusProxy or something
    // as an intermediate subclass?
    SharedPtr<RefCounted> cachedProxy(const QString &busName, const QString &objectPath) const;

    PendingReady *nowHaveProxy(const SharedPtr<RefCounted> &proxy, bool created) const;

    // I don't want this to be non-pure virtual, because I want ALL subclasses to have to think
    // about whether or not they need to uniquefy the name or not. If a subclass doesn't implement
    // this while it should, matching with the cache for future requests and invalidation breaks.
    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const = 0;

    virtual PendingOperation *prepare(const SharedPtr<RefCounted> &object) const;

    virtual Features featuresFor(const SharedPtr<RefCounted> &proxy) const = 0;

private:
    class Cache;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT FixedFeatureFactory : public DBusProxyFactory
{
public:
    virtual ~FixedFeatureFactory();

    Features features() const;

    void addFeature(const Feature &feature);
    void addFeatures(const Features &features);

protected:
    FixedFeatureFactory(const QDBusConnection &bus);

    virtual Features featuresFor(const SharedPtr<RefCounted> &proxy) const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT AccountFactory : public FixedFeatureFactory
{
public:
    virtual ~AccountFactory();

    static AccountFactoryPtr create(const QDBusConnection &bus);
    static AccountFactoryPtr coreFactory(const QDBusConnection &bus);

    PendingReady *proxy(const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory) const;

protected:
    AccountFactory(const QDBusConnection &bus);

    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const;
    // Nothing we'd like to prepare()
    // Fixed features

private:
    struct Private;
    Private *mPriv; // Currently unused, just for future-proofing
};

class TELEPATHY_QT4_EXPORT ConnectionFactory : public FixedFeatureFactory
{
public:
    virtual ~ConnectionFactory();

    static ConnectionFactoryPtr create(const QDBusConnection &bus);

    PendingReady *proxy(const QString &busName, const QString &objectPath,
            const ChannelFactoryConstPtr &chanFactory) const;

protected:
    ConnectionFactory(const QDBusConnection &bus);

    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const;
    // Nothing we'd like to prepare()
    // Fixed features

private:
    struct Private;
    Private *mPriv; // Currently unused, just for future-proofing
};

} // Tp

#endif
