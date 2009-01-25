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

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-connection.h>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QString>
#include <QStringList>

namespace Telepathy
{
namespace Client
{

class Channel;
class Connection;
class PendingChannel;
class PendingHandles;
class PendingOperation;

class Connection : public StatefulDBusProxy,
                   private OptionalInterfaceFactory<Connection>
{
    Q_OBJECT
    Q_DISABLE_COPY(Connection)

public:
    enum Feature {
        FeatureAliasing = 1,
        FeaturePresence = 2,
        FeatureSimplePresence = 4,
        _Padding = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(Features, Feature)

    Connection(const QString &serviceName,
               const QString &objectPath,
               QObject *parent = 0);

    Connection(const QDBusConnection &bus,
               const QString &serviceName,
               const QString &objectPath,
               QObject *parent = 0);

    ~Connection();

    uint status() const;
    uint statusReason() const;

    QStringList interfaces() const;

    uint aliasFlags() const;

    StatusSpecMap presenceStatuses() const;

    SimpleStatusSpecMap simplePresenceStatuses() const;

    template <class Interface>
    inline Interface *optionalInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && !interfaces().contains(name)) {
            return 0;
        }

        // If present or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory<Connection>::interface<Interface>();
    }

    inline ConnectionInterfaceAliasingInterface *aliasingInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceAliasingInterface>(check);
    }

    inline ConnectionInterfaceAvatarsInterface *avatarsInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceAvatarsInterface>(check);
    }

    inline ConnectionInterfaceCapabilitiesInterface *capabilitiesInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceCapabilitiesInterface>(check);
    }

    inline ConnectionInterfacePresenceInterface *presenceInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfacePresenceInterface>(check);
    }

    inline ConnectionInterfaceSimplePresenceInterface *simplePresenceInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceSimplePresenceInterface>(check);
    }

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

    PendingChannel *requestChannel(const QString &channelType,
                                   uint handleType, uint handle);

    PendingOperation *requestConnect();

    PendingOperation *requestDisconnect();

    PendingHandles *requestHandles(uint handleType, const QStringList &names);

    PendingHandles *referenceHandles(uint handleType, const UIntList &handles);

    bool isReady(Features features = 0) const;

    PendingOperation *becomeReady(Features features = 0);

Q_SIGNALS:
    void statusChanged(uint newStatus, uint newStatusReason);

public:
    ConnectionInterface *baseInterface() const;

private Q_SLOTS:
    void onStatusChanged(uint, uint);
    void gotStatus(QDBusPendingCallWatcher *watcher);
    void gotInterfaces(QDBusPendingCallWatcher *watcher);
    void gotAliasFlags(QDBusPendingCallWatcher *watcher);
    void gotStatuses(QDBusPendingCallWatcher *watcher);
    void gotSimpleStatuses(QDBusPendingCallWatcher *watcher);

    void doReleaseSweep(uint type);

    void continueIntrospection();

private:
    void refHandle(uint type, uint handle);
    void unrefHandle(uint type, uint handle);
    void handleRequestLanded(uint type);

    struct Private;
    friend struct Private;
    friend class PendingHandles;
    friend class ReferencedHandles;
    Private *mPriv;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Connection::Features)

} // Telepathy::Client
} // Telepathy

#endif
