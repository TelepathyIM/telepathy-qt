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

#include <TelepathyQt/ConnectionFactory>

#include <TelepathyQt/Connection>
#include <TelepathyQt/DBusProxy>

namespace Tp
{

/**
 * \class ConnectionFactory
 * \ingroup utils
 * \headerfile TelepathyQt/connection-factory.h <TelepathyQt/ConnectionFactory>
 *
 * \brief The ConnectionFactory class is responsible for constructing Connection
 * objects according to application-defined settings.
 *
 * The class is used by Account and other classes which construct Connection proxy
 * instances to enable sharing instances of application-defined Connection
 * subclasses with certain features always ready.
 */

/**
 * Create a new ConnectionFactory object.
 *
 * Optionally, the \a features to make ready on all constructed proxies can be specified. The
 * default is to make no features ready. It should be noted that unlike Connection::becomeReady(),
 * FeatureCore isn't assumed. If no features are specified, which is the default behavior, no
 * Connection::becomeReady() call is made at all and the proxy won't be Connection::isReady().
 *
 * \param bus The QDBusConnection for proxies constructed using this factory to use.
 * \param features The features to make ready on constructed Connections.
 * \return A ConnectionFactoryPtr object pointing to the newly created
 *         ConnectionFactory object.
 */
ConnectionFactoryPtr ConnectionFactory::create(const QDBusConnection &bus,
        const Features &features)
{
    return ConnectionFactoryPtr(new ConnectionFactory(bus, features));
}

/**
 * Construct a new ConnectionFactory object.
 *
 * As in create(), it should be noted that unlike Connection::becomeReady(), FeatureCore isn't
 * assumed.  If no \a features are specified, no Connection::becomeReady() call is made at all and
 * the proxy won't be Connection::isReady().
 *
 * \param bus The QDBusConnection for proxies constructed using this factory to use.
 * \param features The features to make ready on constructed Connections.
 */
ConnectionFactory::ConnectionFactory(const QDBusConnection &bus, const Features &features)
    : FixedFeatureFactory(bus)
{
    addFeatures(features);
}

/**
 * Class destructor.
 */
ConnectionFactory::~ConnectionFactory()
{
}

/**
 * Constructs a Connection proxy and begins making it ready.
 *
 * If a valid proxy already exists in the factory cache for the given combination of \a busName and
 * \a objectPath, it is returned instead. All newly created proxies are automatically cached until
 * they're either DBusProxy::invalidated() or the last reference to them outside the factory has
 * been dropped.
 *
 * The proxy can be accessed immediately after this function returns using PendingReady::proxy().
 * The ready operation only finishes, however, when the features specified by features(), if any,
 * are made ready as much as possible. If the service doesn't support a given feature, they won't
 * obviously be ready even if the operation finished successfully, as is the case for
 * Connection::becomeReady().
 *
 * \param busName The bus/service name of the D-Bus connection object the proxy is constructed for.
 * \param objectPath The object path of the connection.
 * \param chanFactory The channel factory to use for the Connection.
 * \param contactFactory The channel factory to use for the Connection.
 * \return A PendingReady operation with the proxy in PendingReady::proxy().
 */
PendingReady *ConnectionFactory::proxy(const QString &busName, const QString &objectPath,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory) const
{
    DBusProxyPtr proxy = cachedProxy(busName, objectPath);
    if (proxy.isNull()) {
        proxy = construct(busName, objectPath, chanFactory, contactFactory);
    }

    return nowHaveProxy(proxy);
}

/**
 * Can be used by subclasses to override the Connection subclass constructed by the factory.
 *
 * This is automatically called by proxy() to construct proxy instances if no valid cached proxy is
 * found.
 *
 * The default implementation constructs Tp::Connection objects.
 *
 * \param busName The bus/service name of the D-Bus Connection object the proxy is constructed for.
 * \param objectPath The object path of the connection.
 * \param chanFactory The channel factory to use for the Connection.
 * \param contactFactory The channel factory to use for the Connection.
 * \return A pointer to the constructed Connection object.
 */
ConnectionPtr ConnectionFactory::construct(const QString &busName, const QString &objectPath,
        const ChannelFactoryConstPtr &chanFactory,
        const ContactFactoryConstPtr &contactFactory) const
{
    return Connection::create(dbusConnection(), busName, objectPath, chanFactory, contactFactory);
}

/**
 * Transforms well-known names to the corresponding unique names, as is appropriate for Connection
 *
 * \param uniqueOrWellKnown The name to transform.
 * \return The unique name corresponding to \a uniqueOrWellKnown (which may be it itself).
 */
QString ConnectionFactory::finalBusNameFrom(const QString &uniqueOrWellKnown) const
{
    return StatefulDBusProxy::uniqueNameFrom(dbusConnection(), uniqueOrWellKnown);
}

}
