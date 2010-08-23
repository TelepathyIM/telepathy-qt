/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2009 Nokia Corporation
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

#include <TelepathyQt4/OptionalInterfaceFactory>

#include <TelepathyQt4/AbstractInterface>

#include "TelepathyQt4/debug-internal.h"

#include <QMap>
#include <QString>

namespace Tp
{

/**
 * \enum OptionalInterfaceFactory::InterfaceSupportedChecking
 *
 * Specifies if the interface being supported by the remote object should be
 * checked by optionalInterface() and the convenience functions for it.
 *
 * \sa optionalInterface()
 */

/**
 * \var OptionalInterfaceFactory::InterfaceSupportedChecking OptionalInterfaceFactory::CheckInterfaceSupported
 *
 * Don't return an interface instance unless it can be guaranteed that the
 * remote object actually implements the interface.
 */

/**
 * \var OptionalInterfaceFactory::InterfaceSupportedChecking OptionalInterfaceFactory::BypassInterfaceCheck
 *
 * Return an interface instance even if it can't be verified that the remote
 * object supports the interface.
 */

struct TELEPATHY_QT4_NO_EXPORT OptionalInterfaceCache::Private
{
    QObject *proxy;
    QMap<QString, AbstractInterface*> interfaces;

    Private(QObject *proxy);
};

OptionalInterfaceCache::Private::Private(QObject *proxy)
    : proxy(proxy)
{
}

/**
 * Class constructor.
 */
OptionalInterfaceCache::OptionalInterfaceCache(QObject *proxy)
    : mPriv(new Private(proxy))
{
}

/**
 * Class destructor.
 *
 * Frees all interface instances constructed by this factory.
 */
OptionalInterfaceCache::~OptionalInterfaceCache()
{
    delete mPriv;
}

QObject *OptionalInterfaceCache::proxy() const
{
    return mPriv->proxy;
}

AbstractInterface *OptionalInterfaceCache::getCached(const QString &name) const
{
    if (mPriv->interfaces.contains(name)) {
        return mPriv->interfaces.value(name);
    } else {
        return 0;
    }
}

void OptionalInterfaceCache::cache(AbstractInterface *interface) const
{
    QString name = interface->interface();
    Q_ASSERT(!mPriv->interfaces.contains(name));

    mPriv->interfaces[name] = interface;
}

/**
 * \class OptionalInterfaceFactory
 * \ingroup clientsideproxies
 * \headerfile TelepathyQt4/optional-interface-factory.h <TelepathyQt4/OptionalInterfaceFactory>
 *
 * \brief Implementation helper class for high-level proxy classes willing to
 * offer access to shared instances of interface proxies for optional
 * interfaces.
 *
 * This class is included in the public API for the benefit of high-level
 * proxies in extensions.
 *
 * To use this helper in a subclass of DBusProxy (say, ExampleObject),
 * ExampleObject should inherit privately from
 * OptionalInterfaceFactory<ExampleObject>, and call
 * OptionalInterfaceFactory(this) in its constructor's initialization list.
 *
 * \tparam DBusProxySubclass A subclass of DBusProxy
 */

/**
 * \fn OptionalInterfaceFactory::OptionalInterfaceFactory(DBusProxySubclass *this_)
 *
 * Class constructor.
 *
 * \param this_ The class to which this OptionalInterfaceFactory is
 *              attached
 */

/**
 * \fn OptionalInterfaceFactory::~OptionalInterfaceFactory()
 *
 * Class destructor.
 *
 * Frees all interface instances constructed by this factory.
 */

 /**
  * \fn OptionalInterfaceFactory::interfaces() const;
  *
  * Return a list of interfaces supported by this object.
  *
  * \return List of supported interfaces.
  */

/**
 * \fn template <typename Interface> inline Interface *OptionalInterfaceFactory::interface() const
 *
 * Return a pointer to a valid instance of a interface class, associated
 * with the same remote object as the given main interface instance. The
 * given main interface must be of the class the optional interface is
 * generated for (for eg. ChannelInterfaceGroupInterface this means
 * ChannelInterface) or a subclass.
 *
 * First invocation of this method for a particular optional interface
 * class will construct the instance; subsequent calls will return a
 * pointer to the same instance.
 *
 * The returned instance is freed when the factory is destroyed; using
 * it after destroying the factory will likely produce a crash. As the
 * instance is shared, it should not be freed directly.
 *
 * \tparam Interface Class of the interface instance to get.
 * \return A pointer to an optional interface instance.
 */

} // Tp
