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

#ifndef _TelepathyQt4_cli_connection_h_HEADER_GUARD_
#define _TelepathyQt4_cli_connection_h_HEADER_GUARD_

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
#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

namespace Telepathy
{
namespace Client
{

class PendingChannel;

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
 *  <li>Getting the list of supported interfaces automatically</li>
 *  <li>Getting the alias flags automatically</li>
 *  <li>Getting the valid presence statuses automatically</li>
 *  <li>Shared optional interface proxy instances</li>
 * </ul>
 *
 * The remote object state accessor functions on this object (status(),
 * statusReason(), aliasFlags(), and so on) don't make any %DBus calls; instead,
 * they return values cached from a previous introspection run. The
 * introspection process populates their values in the most efficient way
 * possible based on what the service implements.  Their return value is mostly
 * undefined until the introspection process is completed; a readiness change to
 * #ReadinessFull indicates that the introspection process is finished. See the
 * individual accessor descriptions for details on which functions can be used
 * in the different states.
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
     * Returns the connection's status.
     *
     * The returned value may have changed whenever readinessChanged() is
     * emitted. The value is valid in all states except for
     * #ReadinessJustCreated.
     *
     * \return The status, as defined in #ConnectionStatus.
     */
    uint status() const;

    /**
     * Returns the reason for the connection's status (which is returned by
     * status()). The validity and change rules are the same as for status().
     *
     * \return The reason, as defined in #ConnectionStatusReason.
     */
    uint statusReason() const;

    /**
     * Returns a list of optional interfaces supported by this object. The
     * contents of the list is undefined unless the Connection has readiness
     * #ReadinessNotYetConnected or #ReadinessFull. The returned value stays
     * constant for the entire time the connection spends in each of these
     * states; however interfaces might have been added to the supported set by
     * the time #ReadinessFull is reached.
     *
     * \return Names of the supported interfaces.
     */
    QStringList interfaces() const;

    /**
     * Returns the bitwise OR of flags detailing the behavior of the Aliasing
     * interface on the remote object.
     *
     * The returned value is undefined unless the Connection has readiness
     * #ReadinessFull and the list returned by interfaces() contains
     * %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING.
     *
     * \return Bitfield of flags, as specified in #ConnectionAliasFlag.
     */
    uint aliasFlags() const;

    /**
     * Returns a dictionary of presence statuses valid for use with the legacy
     * Telepathy Presence interface on the remote object.
     *
     * The returned value is undefined unless the Connection has readiness
     * #ReadinessFull and the list returned by interfaces() contains
     * %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_PRESENCE.
     *
     * \return Dictionary from string identifiers to structs for each valid status.
     */
    StatusSpecMap presenceStatuses() const;

    /**
     * Returns a dictionary of presence statuses valid for use with the new(er)
     * Telepathy SimplePresence interface on the remote object.
     *
     * The value is undefined if the list returned by interfaces() doesn't
     * contain %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE.
     *
     * The value will stay fixed for the whole time the connection stays with
     * readiness #ReadinessNotYetConnected, but may have changed arbitrarily
     * during the time the Connection spends in readiness #ReadinessConnecting,
     * again staying fixed for the entire time in #ReadinessFull.
     *
     * \return Dictionary from string identifiers to structs for each valid
     * status.
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
     * interface requested <code>0</code> is returned. This check can be
     * bypassed by specifying #BypassInterfaceCheck for <code>check</code>, in
     * which case a valid instance is always returned.
     *
     * If the object doesn't have readiness #ReadinessNotYetConnected or
     * #ReadinessFull, the list returned by interfaces() isn't guaranteed to yet
     * represent the full set of interfaces supported by the remote object.
     * Hence the check might fail even if the remote object actually supports
     * the requested interface; using #BypassInterfaceCheck is suggested when
     * the Connection is not suitably ready.
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
     * \param check Passed to optionalInterface()
     * \return <code>optionalInterface<ConnectionInterfaceAliasingInterface>(check)</code>
     */
    inline ConnectionInterfaceAliasingInterface* aliasingInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceAliasingInterface>(check);
    }

    /**
     * Convenience function for getting an Avatars interface proxy.
     *
     * \param check Passed to optionalInterface()
     * \return <code>optionalInterface<ConnectionInterfaceAvatarsInterface>(check)</code>
     */
    inline ConnectionInterfaceAvatarsInterface* avatarsInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceAvatarsInterface>(check);
    }

    /**
     * Convenience function for getting a Capabilities interface proxy.
     *
     * \param check Passed to optionalInterface()
     * \return <code>optionalInterface<ConnectionInterfaceCapabilitiesInterface>(check)</code>
     */
    inline ConnectionInterfaceCapabilitiesInterface* capabilitiesInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceCapabilitiesInterface>(check);
    }

    /**
     * Convenience function for getting a Presence interface proxy.
     *
     * \param check Passed to optionalInterface()
     * \return <code>optionalInterface<ConnectionInterfacePresenceInterface>(check)</code>
     */
    inline ConnectionInterfacePresenceInterface* presenceInterface(InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfacePresenceInterface>(check);
    }

    /**
     * Convenience function for getting a SimplePresence interface proxy.
     *
     * \param check Passed to optionalInterface()
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
     * \return
     * <code>optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck)</code>
     */
    inline DBus::PropertiesInterface* propertiesInterface() const
    {
        return optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

    /**
     * Asynchronously requests a channel satisfying the given channel type and
     * communicating with the contact, room, list etc. given by the handle type
     * and handle.
     *
     * Upon completion, the reply to the request can be retrieved through the
     * returned PendingChannel object. The object also provides access to the
     * parameters with which the call was made and a signal to connect to to get
     * notification of the request finishing processing. See the documentation
     * for that class for more info.
     *
     * The returned PendingChannel object should be freed using
     * its QObject::deleteLater() method after it is no longer used. However,
     * all PendingChannel objects resulting from requests to a particular
     * Connection will be freed when the Connection itself is freed. Conversely,
     * this means that the PendingChannel object should not be used after the
     * Connection is destroyed.
     *
     * \sa PendingChannel
     *
     * \param channelType D-Bus interface name of the channel type to request,
     *                    such as TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT.
     * \param handleType Type of the handle given, as specified in #HandleType.
     * \param handle Handle specifying the remote entity to communicate with.
     * \return Pointer to a newly constructed PendingChannel object, tracking
     *         the progress of the request.
     */
    PendingChannel* requestChannel(const QString& channelType, uint handleType, uint handle);

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

/**
 * \class PendingChannel
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/cli-connection.h> <TelepathyQt4/Client/Connection>
 *
 * Class containing the parameters of and the reply to an asynchronous channel
 * request. Instances of this class cannot be constructed directly; the only way
 * to get one is to use Connection::requestChannel().
 */
class PendingChannel : public QObject
{
    Q_OBJECT

public:
    /**
     * Class destructor.
     */
    ~PendingChannel();

    /**
     * Returns the Connection object through which the channel request was made.
     *
     * \return Pointer to the Connection.
     */
    Connection* connection() const;

    /**
     * Returns the channel type specified in the channel request.
     *
     * \return The D-Bus interface name of the interface specific to the
     *         requested channel type.
     */
    const QString& channelType() const;

    /**
     * Returns the handle type specified in the channel request.
     *
     * \return The handle type, as specified in #HandleType.
     */
    uint handleType() const;

    /**
     * Returns the handle specified in the channel request.
     *
     * \return The handle.
     */
    uint handle() const;

    /**
     * Returns whether or not the request has finished processing.
     *
     * \sa finished()
     *
     * \return If the request is finished.
     */
    bool isFinished() const;

    /**
     * Returns whether or not the request resulted in an error. If the request
     * has not yet finished processing (isFinished() returns <code>false</code>),
     * this cannot yet be known, and <code>false</code> will be returned.
     *
     * \return <code>true</code> iff the request has finished processing AND has
     *         resulted in an error.
     */
    bool isError() const;

    /**
     * Returns the error which the request resulted in, if any. If isError()
     * returns <code>false</code>, the request has not (at least yet) resulted
     * in an error, and an undefined value will be returned.
     *
     * \return The error as a QDBusError.
     */
    const QDBusError& error() const;

    /**
     * Returns whether or not the request completed successfully. If the request
     * has not yet finished processing (isFinished() returns <code>false</code>),
     * this cannot yet be known, and <code>false</code> will be returned.
     *
     * \return <code>true</code> iff the request has finished processing AND has
     *         completed successfully.
     */
    bool isValid() const;

    /**
     * Returns a newly constructed Channel high-level proxy object associated
     * with the remote channel resulting from the channel request. If isValid()
     * returns <code>false</code>, the request has not (at least yet) completed
     * successfully, and 0 will be returned.
     *
     * \param parent Passed to the Channel constructor.
     * \return Pointer to the new Channel object.
     */
    Channel* channel(QObject* parent = 0) const;

Q_SIGNALS:
    /**
     * Emitted when the request finishes processing. isFinished() will then
     * start returning <code>true</code> and isError(), error(), isValid() and
     * channel() will become meaningful to use.
     *
     * \param pendingChannel The PendingChannel object for which the request has
     *                       corresponding to the finished request.
     */
    void finished(Telepathy::Client::PendingChannel* pendingChannel);

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);

private:
    friend class Connection;

    PendingChannel(Connection* connection, const QString& type, uint handleType, uint handle);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif
