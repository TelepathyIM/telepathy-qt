/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008, 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008, 2009 Nokia Corporation
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
#include <TelepathyQt4/Client/ReadinessHelper>
#include <TelepathyQt4/Client/ReadyObject>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

namespace Telepathy
{
namespace Client
{

class Channel;
class Contact;
class ContactManager;
class PendingChannel;
class PendingContactAttributes;
class PendingHandles;
class PendingOperation;
class PendingReady;

class Connection : public StatefulDBusProxy,
                   private OptionalInterfaceFactory<Connection>,
                   public ReadyObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Connection)
    Q_ENUMS(Status)

public:
    static const Feature FeatureCore;
    static const Feature FeatureSelfContact;
    static const Feature FeatureSimplePresence;
    static const Feature FeatureRoster;

    enum Status {
        StatusDisconnected = Telepathy::ConnectionStatusDisconnected,
        StatusConnecting = Telepathy::ConnectionStatusConnecting,
        StatusConnected = Telepathy::ConnectionStatusConnected,
        StatusUnknown = 0xFFFFFFFF
    };

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

    uint selfHandle() const;

    SimpleStatusSpecMap allowedPresenceStatuses() const;
    PendingOperation *setSelfPresence(const QString &status, const QString &statusMessage);

    QSharedPointer<Contact> selfContact() const;

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

    inline ConnectionInterfaceRequestsInterface *requestsInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ConnectionInterfaceRequestsInterface>(check);
    }

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

    PendingChannel *createChannel(const QVariantMap &request);

    PendingChannel *ensureChannel(const QVariantMap &request);

    PendingOperation *requestConnect(const Features &requestedFeatures = Features());

    PendingOperation *requestDisconnect();

    PendingHandles *requestHandles(uint handleType, const QStringList &names);

    PendingHandles *referenceHandles(uint handleType, const UIntList &handles);

    PendingContactAttributes *getContactAttributes(const UIntList &handles,
            const QStringList &interfaces, bool reference = true);
    QStringList contactAttributeInterfaces() const;
    ContactManager *contactManager() const;

Q_SIGNALS:
    void statusChanged(uint newStatus, uint newStatusReason);
    void selfHandleChanged(uint newHandle);
    // FIXME: might not need this when Renaming is fixed and mapped to Contacts
    void selfContactChanged();

protected:
    ConnectionInterface *baseInterface() const;

private Q_SLOTS:
    void onStatusReady(uint);
    void onStatusChanged(uint, uint);
    void gotStatus(QDBusPendingCallWatcher *watcher);
    void gotInterfaces(QDBusPendingCallWatcher *watcher);
    void gotContactAttributeInterfaces(QDBusPendingCallWatcher *watcher);
    void gotSimpleStatuses(QDBusPendingCallWatcher *watcher);
    void gotSelfContact(Telepathy::Client::PendingOperation *);
    void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    void gotContactListsHandles(Telepathy::Client::PendingOperation *);
    void gotContactListChannel(Telepathy::Client::PendingOperation *);
    void contactListChannelReady();

    void doReleaseSweep(uint type);

    void onSelfHandleChanged(uint);

private:
    void refHandle(uint type, uint handle);
    void unrefHandle(uint type, uint handle);
    void handleRequestLanded(uint type);

    struct Private;
    class PendingConnect;
    friend struct Private;
    friend class PendingChannel;
    friend class PendingConnect;
    friend class PendingContactAttributes;
    friend class PendingHandles;
    friend class ReferencedHandles;
    Private *mPriv;
};

typedef QExplicitlySharedDataPointer<Connection> ConnectionPtr;

} // Telepathy::Client
} // Telepathy

#endif
