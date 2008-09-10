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
#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

namespace Telepathy
{
namespace Client
{

/**
 * \class Connection
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/cli-connection.h> <TelepathyQt4/Client/Connection>
 *
 * High-level proxy object for accessing remote %Telepathy %Connection objects.
 *
 * It adds the following features compared to using ConnectionInterface
 * directly:
 * <ul>
 *  <li>%Connection status tracking</li>
 *  <li>Calling GetInterfaces() automatically</li>
 *  <li>Calling ConnectionInterfaceAliasingInterface::GetAliasFlags() automatically</li>
 *  <li>Calling ConnectionInterfacePresenceInterface::GetStatuses() automatically</li>
 *  <li>Getting the ConnectionInterfaceSimplePresenceInterface::Statuses property automatically</li>
 * </ul>
 *
 * The remote object state accessor functions on this object (status(),
 * statusReason(), aliasFlags(), presenceStatuses(), simplePresenceStatuses())
 * don't make any %DBus calls; instead, they return values cached from a previous
 * introspection run.
 */
class Connection : public ConnectionInterface, private OptionalInterfaceFactory
{
    Q_OBJECT
    Q_ENUMS(Readiness);

public:
    /**
     * Describes readiness of the Connection for usage. The readiness depends
     * on the state of the remote object. In suitable states, an asynchronous
     * introspection process is started, and the Connection becomes more ready
     * when that process is completed.
     */
    enum Readiness {
        /**
         * The object has just been created and introspection is still in
         * progress. No functionality is available.
         *
         * The readiness can change to any other state depending on the result
         * of the initial state query to the remote object.
         */
        ReadinessJustCreated = 0,

        /**
         * The remote object is in the Disconnected state and introspection
         * relevant to that state has been completed.
         *
         * This state is useful for being able to set your presence status
         * (through the SimplePresence interface) before connecting. Most other
         * functionality is unavailable, though.
         *
         * The readiness can change to ReadinessConnecting and ReadinessDead.
         */
        ReadinessNotYetConnected = 5,

        /**
         * The remote object is in the Connecting state. Most functionality is
         * unavailable.
         *
         * The readiness can change to ReadinessFull and ReadinessDead.
         */
        ReadinessConnecting = 10,

        /**
         * The connection is in the Connected state and all introspection
         * has been completed. Most functionality is available.
         *
         * The readiness can change to ReadinessDead.
         */
        ReadinessFull = 15,

        /**
         * The remote object has gone into a state where it can no longer be
         * used. No functionality is available.
         *
         * No further readiness changes are possible.
         */
        ReadinessDead = 20,

        _ReadinessInvalid = 0xffff
    };

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
     * Returns the current readiness of the Connection.
     *
     * \return The readiness, as defined in #Readiness.
     */
    Readiness readiness() const;

    /**
     * Returns the connection's status. The returned status is undefined if the
     * object has readiness #ReadinessJustCreated.
     *
     * \return The status, as defined in #ConnectionStatus.
     */
    uint status() const;

    /**
     * Returns the reason for the connection's status. The returned reason is
     * undefined if the object has readiness #ReadinessJustCreated.
     *
     * \return The reason, as defined in #ConnectionStatusReason.
     */
    uint statusReason() const;

    /**
     * Returns a list of optional interfaces supported by this object. The
     * contents of the list is undefined unless the Connection has readiness
     * #ReadinessNotYetConnected or #ReadinessFull. The returned value stays
     * constant for the entire time the connection spends in each of these
     * states; however interfaces might be added to the supported set at the
     * time #ReadinessFull is reached.
     *
     * \return Names of the supported interfaces.
     */
    QStringList interfaces() const;

    /**
     * Returns the bitwise OR of flags detailing the behavior of the Aliasing
     * interface on the remote object. The value is undefined if the connection
     * doesn't have readiness #ReadinessFull or if the remote object doesn't
     * implement the Aliasing interface.
     *
     * \return Bitfield of flags, as specified in #ConnectionAliasFlag.
     */
    uint aliasFlags() const;

    /**
     * Returns a dictionary of presence statuses valid for use with the legacy
     * Telepathy Presence interface on the remote object. The value is undefined
     * unless the Connection has readiness #ReadinessFull or if the remote
     * object doesn't implement the legacy Presence interface.
     *
     * \return Dictionary from string identifiers to structs for each valid status.
     */
    StatusSpecMap presenceStatuses() const;

    /**
     * Returns a dictionary of presence statuses valid for use with the new(er)
     * Telepathy SimplePresence interface on the remote object. The value is
     * undefined unless the Connection has readiness #ReadinessNotYetConnected
     * or #ReadinessFull, or if the remote object doesn't implement the
     * SimplePresence interface.
     *
     * The value will stay fixed for the whole time the connection stays with
     * readiness #ReadinessNotYetConnected, but may change arbitrarily at the
     * time #ReadinessFull is reached, staying fixed for the remaining lifetime
     * of the Connection.
     *
     * \return Dictionary from string identifiers to structs for each valid status.
     */
    SimpleStatusSpecMap simplePresenceStatuses() const;

    /**
     * Specifies if the interface being supported by the remote object should be
     * checked by optionalInterface() and the convenience functions for it.
     */
    enum InterfaceSupportedChecking
    {
        /**
         * Don't return an interface instance unless it can be guaranteed that
         * the remote object actually implements the interface.
         */
        CheckInterfaceSupported,

        /**
         * Return an interface instance even if it can't be verified that the
         * remote object supports the interface.
         */
        BypassInterfaceCheck
    };

    /**
     * Returns a pointer to a valid instance of a given %Connection optional
     * interface class, associated with the same remote object the Connection is
     * associated with, and destroyed at the same time the Connection is
     * destroyed.
     *
     * If the list returned by interfaces() doesn't contain the name of the
     * interface requested, or the connection doesn't have readiness
     * #ReadinessNotYetConnected or #ReadinessFull, <code>0</code> is returned.
     * This check can be bypassed by specifying #BypassInterfaceCheck for
     * <code>check</code>, in in which case a valid instance is always returned.
     *
     * \see OptionalInterfaceFactory::interface
     *
     * \tparam Interface Class of the optional interface to get.
     * \param check Should an instance be returned even if it can't be
     *              determined that the remote object supports the
     *              requested interface.
     * \return Pointer to an instance of the interface class, or <code>0</code>.
     */
    template <class Interface>
    inline Interface* optionalInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && !interfaces().contains(name))
            return 0;

        // If present or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory::interface<Interface>(*this);
    }

    /**
     * Convenience function for getting an Aliasing interface proxy.
     *
     * \see optionalInterface()
     *
     * \param check If the interface being present check should be
     *              bypassed.
     * \return <code>optionalInterface<ConnectionInterfaceAliasingInterface>(check)</code>
     */
    inline ConnectionInterfaceAliasingInterface* aliasingInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceAliasingInterface>(check);
    }

    /**
     * Convenience function for getting an Avatars interface proxy.
     *
     * \see optionalInterface()
     *
     * \param check If the interface being present check should be
     *              bypassed.
     * \return <code>optionalInterface<ConnectionInterfaceAvatarsInterface>(check)</code>
     */
    inline ConnectionInterfaceAvatarsInterface* avatarsInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceAvatarsInterface>(check);
    }

    /**
     * Convenience function for getting a Capabilities interface proxy.
     *
     * \see optionalInterface()
     *
     * \param check If the interface being present check should be
     *              bypassed.
     * \return <code>optionalInterface<ConnectionInterfaceCapabilitiesInterface>(check)</code>
     */
    inline ConnectionInterfaceCapabilitiesInterface* capabilitiesInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceCapabilitiesInterface>(check);
    }

    /**
     * Convenience function for getting a Presence interface proxy.
     *
     * \see optionalInterface()
     *
     * \param check If the interface being present check should be
     *              bypassed.
     * \return <code>optionalInterface<ConnectionInterfacePresenceInterface>(check)</code>
     */
    inline ConnectionInterfacePresenceInterface* presenceInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfacePresenceInterface>(check);
    }

    /**
     * Convenience function for getting a SimplePresence interface proxy.
     *
     * \see optionalInterface()
     *
     * \param check If the interface being present check should be
     *              bypassed.
     * \return
     * <code>optionalInterface<ConnectionInterfaceSimplePresenceInterface>(check)</code>
     */
    inline ConnectionInterfaceSimplePresenceInterface* simplePresenceInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceSimplePresenceInterface>(check);
    }

    /**
     * Convenience function for getting a Properties interface proxy. The
     * Properties interface is not necessarily reported by the services, so a
     * <code>check</code> parameter is not provided, and the interface is
     * always assumed to be present.
     *
     * \see optionalInterface()
     *
     * \return <code>optionalInterface<DBus::PropertiesInterface>(check)</code>
     */
    inline DBus::PropertiesInterface* propertiesInterface() const
    {
        return optionalInterface<DBus::PropertiesInterface>(CheckInterfaceSupported);
    }

Q_SIGNALS:
    /**
     * Emitted whenever the readiness of the Connection changes.
     *
     * \param newReadiness The new readiness, as defined in #Readiness.
     */
    void readinessChanged(Telepathy::Client::Connection::Readiness newReadiness);

private Q_SLOTS:
    void onStatusChanged(uint, uint);
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
