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

#include <QDBusPendingCallWatcher>
#include <QStringList>

#include <TelepathyQt4/Constants>

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
     * Note that this class provides a more convenient way to access some
     * functionality than using the interface class directly.
     *
     * \return A reference to the underlying ConnectionInterface instance.
     *
     * \{
     */
    ConnectionInterface& interface();
    const ConnectionInterface& interface() const;

    /**
     * \}
     */

    /**
     * Initially <code>false</code>, changes to <code>true</code> when the
     * connection has gone to the Connected status, introspection is finished
     * and it's ready for use.
     *
     * By the time this property becomes true, the property #interfaces will
     * have been populated.
     */
    Q_PROPERTY(bool ready READ ready)

    /**
     * Getter for the property #ready.
     *
     * \return If the object is ready for use.
     */
    bool ready() const;

    /**
     * The connection's status (one of the values of #ConnectionStatus), or -1
     * if we don't know it yet.
     *
     * Connect to the signal <code>statusChanged</code> on the underlying
     * #interface to watch for changes.
     */
    Q_PROPERTY(long status READ status)

    /**
     * Getter for the property #status.
     *
     * \return The status of the connection.
     */
    long status() const;

    /**
     * The reason why #status changed to its current value, or
     * #ConnectionStatusReasonNoneSpecified if the status is still unknown.
     */
    Q_PROPERTY(uint statusReason READ statusReason)

    /**
     * Getter for the property #statusReason.
     *
     * \return The reason for the current status.
     */
    uint statusReason() const;

    /**
     * List of interfaces implemented by the connection.
     *
     * The value is undefined until the property #ready is <code>true</code>.
     */
    Q_PROPERTY(QStringList interfaces READ interfaces)

    /**
     * Getter for the property #interfaces.
     *
     * \return D-Bus interface names of the implemented interfaces.
     */
    QStringList interfaces() const;

    /**
     * Bitwise OR of flags detailing the behaviour of aliases on the
     * connection.
     *
     * The value is undefined if the value of the property #ready is
     * <code>false</code> or the connection doesn't support the Aliasing
     * interface.
     */
    Q_PROPERTY(Telepathy::ConnectionAliasFlags aliasFlags READ aliasFlags)

    /**
     * Getter for the property #aliasFlags.
     *
     * \return Bitfield of the alias flags, as specified in
     * #ConnectionAliasFlag.
     */
    ConnectionAliasFlags aliasFlags() const;

    /**
     * Dictionary of the valid presence statuses for the connection for use with
     * the legacy Telepathy Presence interface.
     *
     * The value is undefined if the value of the property #ready is
     * <code>false</code> or the connection doesn't support the Presence
     * interface.
     */
    Q_PROPERTY(Telepathy::StatusSpecMap presenceStatuses READ presenceStatuses)

    /**
     * Getter for the property #presenceStatuses.
     *
     * \return Dictionary mapping string identifiers to structs for each status.
     */
    StatusSpecMap presenceStatuses() const;

    /**
     * Dictionary of the valid presence statuses for the connection for use with
     * the new simplified Telepathy SimplePresence interface.
     *
     * Getting the value of this property before the connection is ready (as
     * signified by the property #ready) may cause a synchronous call to the
     * remote service to be made.
     *
     * The value is undefined if the connection doesn't support the
     * SimplePresence interface.
     */
    Q_PROPERTY(Telepathy::SimpleStatusSpecMap simplePresenceStatuses READ simplePresenceStatuses)

    /**
     * Getter for the property #simplePresenceStatuses.
     *
     * \return Dictionary mapping string identifiers to structs for each status.
     */
    SimpleStatusSpecMap simplePresenceStatuses() const;

Q_SIGNALS:
    /**
     * Emitted when the value of the property #ready changes to
     * <code>true</code>.
     */
    void ready();

private Q_SLOTS:
    void statusChanged(uint, uint);
    void gotStatus(QDBusPendingCallWatcher* watcher);
    void gotInterfaces(QDBusPendingCallWatcher* watcher);
    void gotAliasFlags(QDBusPendingCallWatcher* watcher);
    void gotStatuses(QDBusPendingCallWatcher* watcher);
    void gotSimpleStatuses(QDBusPendingCallWatcher* watcher);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif
