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

#ifndef _TelepathyQt4_Client_Connection_HEADER_GUARD_
#define _TelepathyQt4_Client_Connection_HEADER_GUARD_

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
 * \defgroup clientconn Connection proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Connections and their optional
 * interfaces.
 */

#include <TelepathyQt4/_gen/cli-connection.h>

namespace Telepathy
{
namespace Client
{

/**
 * \class Connection
 * \ingroup clientconn
 *
 * High-level proxy object for accessing remote %Telepathy %Connection objects.
 *
 * It adds the following features compared to using ConnectionInterface alone:
 * <ul>
 *  <li>%Connection status tracking</li>
 *  <li>Calling GetInterfaces automatically</li>
 * </ul>
 */
class Connection : public QObject
{
    Q_OBJECT
public:
    /**
     * Creates a Connection associated with the given object on the session bus.
     *
     * \param serviceName Name of the service the object is on.
     * \param objectPath  Path to the object on the service.
     * \param parent      Passed to the parent class constructor.
     */
    Connection(const QString& serviceName,
               const QString& objectPath,
               QObject* parent = 0);

    /**
     * Creates a Connection associated with the given object on the given bus.
     *
     * \param connection  The bus via which the object can be reached.
     * \param serviceName Name of the service the object is on.
     * \param objectPath  Path to the object on the service.
     * \param parent      Passed to the parent class constructor.
     */
    Connection(const QDBusConnection& connection,
               const QString &serviceName,
               const QString &objectPath,
               QObject* parent = 0);

    /**
     * Class destructor.
     */
    ~Connection();

    /**
     * Returns a reference to the underlying ConnectionInterface instance for
     * easy direct access to the D-Bus properties, methods and signals on the
     * remote object.
     *
     * Note that this class provides a more convenient interface for some
     * functionality than using the interface class directly.
     *
     * \return A reference to the underlying ConnectionInterface instance.
     */
    ConnectionInterface& interface();
    const ConnectionInterface& interface() const;

private:
    struct Private;
    Private *mPriv;
};

}
}

#endif
