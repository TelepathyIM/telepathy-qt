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

#ifndef _TelepathyQt4_connection_h_HEADER_GUARD_
#define _TelepathyQt4_connection_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-connection.h>

#include <TelepathyQt4/Contact>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QSet>
#include <QString>
#include <QStringList>

namespace Tp
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
                   public OptionalInterfaceFactory<Connection>,
                   public ReadyObject,
                   public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(Connection)
    Q_ENUMS(Status)

public:
    static const Feature FeatureCore;
    static const Feature FeatureSelfContact;
    static const Feature FeatureSimplePresence;
    static const Feature FeatureRoster;
    static const Feature FeatureRosterGroups;

    enum Status {
        StatusDisconnected = ConnectionStatusDisconnected,
        StatusConnecting = ConnectionStatusConnecting,
        StatusConnected = ConnectionStatusConnected,
        StatusUnknown = 0xFFFFFFFF
    };

    static ConnectionPtr create(const QString &busName,
            const QString &objectPath);
    static ConnectionPtr create(const QDBusConnection &bus,
            const QString &busName, const QString &objectPath);

    virtual ~Connection();

    uint status() const;
    uint statusReason() const;

    uint selfHandle() const;
    ContactPtr selfContact() const;

    SimpleStatusSpecMap allowedPresenceStatuses() const;
    PendingOperation *setSelfPresence(const QString &status, const QString &statusMessage);

    PendingChannel *createChannel(const QVariantMap &request);
    PendingChannel *ensureChannel(const QVariantMap &request);

    PendingReady *requestConnect(const Features &requestedFeatures = Features());
    PendingOperation *requestDisconnect();

    PendingHandles *requestHandles(uint handleType, const QStringList &names);
    PendingHandles *referenceHandles(uint handleType, const UIntList &handles);

    PendingContactAttributes *getContactAttributes(const UIntList &handles,
            const QStringList &interfaces, bool reference = true);
    QStringList contactAttributeInterfaces() const;
    ContactManager *contactManager() const;

    inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<Client::DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

    inline Client::ConnectionInterfaceAliasingInterface *aliasingInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::ConnectionInterfaceAliasingInterface>(check);
    }

    inline Client::ConnectionInterfaceAvatarsInterface *avatarsInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::ConnectionInterfaceAvatarsInterface>(check);
    }

    inline Client::ConnectionInterfaceCapabilitiesInterface *capabilitiesInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::ConnectionInterfaceCapabilitiesInterface>(check);
    }

    inline Client::ConnectionInterfacePresenceInterface *presenceInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::ConnectionInterfacePresenceInterface>(check);
    }

    inline Client::ConnectionInterfaceSimplePresenceInterface *simplePresenceInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::ConnectionInterfaceSimplePresenceInterface>(check);
    }

    inline Client::ConnectionInterfaceRequestsInterface *requestsInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::ConnectionInterfaceRequestsInterface>(check);
    }

Q_SIGNALS:
    void statusChanged(uint newStatus, uint newStatusReason);
    void selfHandleChanged(uint newHandle);
    // FIXME: might not need this when Renaming is fixed and mapped to Contacts
    void selfContactChanged();

protected:
    Connection(const QString &busName, const QString &objectPath);
    Connection(const QDBusConnection &bus, const QString &busName,
            const QString &objectPath);

    Client::ConnectionInterface *baseInterface() const;

private Q_SLOTS:
    void onStatusReady(uint);
    void onStatusChanged(uint, uint);
    void gotStatus(QDBusPendingCallWatcher *watcher);
    void gotInterfaces(QDBusPendingCallWatcher *watcher);
    void gotContactAttributeInterfaces(QDBusPendingCallWatcher *watcher);
    void gotSimpleStatuses(QDBusPendingCallWatcher *watcher);
    void gotSelfContact(Tp::PendingOperation *);
    void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    void gotContactListsHandles(Tp::PendingOperation *);
    void gotContactListChannel(Tp::PendingOperation *);
    void contactListChannelReady();
    void onNewChannels(const Tp::ChannelDetailsList &);
    void onContactListGroupChannelReady(Tp::PendingOperation *);
    void gotChannels(QDBusPendingCallWatcher *);

    void doReleaseSweep(uint type);

    void onSelfHandleChanged(uint);

private:
    class PendingConnect;
    friend class PendingChannel;
    friend class PendingConnect;
    friend class PendingContactAttributes;
    friend class PendingContacts;
    friend class PendingHandles;
    friend class ReferencedHandles;

    void refHandle(uint type, uint handle);
    void unrefHandle(uint type, uint handle);
    void handleRequestLanded(uint type);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
