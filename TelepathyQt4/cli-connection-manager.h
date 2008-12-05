/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_connection_manager_h_HEADER_GUARD_
#define _TelepathyQt4_cli_connection_manager_h_HEADER_GUARD_

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

/**
 * \defgroup clientcm Connection manager proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy ConnectionManager objects.
 */

#include <TelepathyQt4/_gen/cli-connection-manager.h>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

namespace Telepathy
{
namespace Client
{


/**
 * \class ConnectionManager
 * \ingroup clientcm
 * \headerfile TelepathyQt4/cli-connection-manager.h <TelepathyQt4/Client/ConnectionManager>
 *
 * Object representing a Telepathy connection manager. Connection managers
 * allow connections to be made on one or more protocols.
 *
 * Most client applications should use this functionality via the
 * %AccountManager, to allow connections to be shared between client
 * applications.
 */
class ConnectionManager : public StatelessDBusProxy,
        private OptionalInterfaceFactory
{
    Q_OBJECT

private:
    struct Private;
    friend struct Private;
    Private *mPriv;

public:
    ConnectionManager(const QString& name, QObject* parent = 0);
    ConnectionManager(const QDBusConnection& bus,
            const QString& name, QObject* parent = 0);

    virtual ~ConnectionManager();

    QStringList interfaces() const;

    QStringList supportedProtocols() const;

    // TODO: add some sort of access to protocols' parameters etc.

    /**
     * Convenience function for getting a Properties interface proxy. The
     * Properties interface is not necessarily reported by the services, so a
     * <code>check</code> parameter is not provided, and the interface is
     * always assumed to be present.
     */
    inline DBus::PropertiesInterface* propertiesInterface() const
    {
        return OptionalInterfaceFactory::interface<DBus::PropertiesInterface>(
                *baseInterface());
    }

protected:
    /**
     * Get the ConnectionManagerInterface for this ConnectionManager. This
     * method is protected since the convenience methods provided by this
     * class should generally be used instead of calling D-Bus methods
     * directly.
     *
     * \return A pointer to the existing ConnectionManagerInterface for this
     *         ConnectionManager
     */
    ConnectionManagerInterface* baseInterface() const;

private Q_SLOTS:
    void onGetParametersReturn(QDBusPendingCallWatcher*);
    void onListProtocolsReturn(QDBusPendingCallWatcher*);
    void onGetAllConnectionManagerReturn(QDBusPendingCallWatcher*);
    void onStartIntrospection();
};


}
}

#endif
