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

#ifndef _TelepathyQt4_cli_channel_h_HEADER_GUARD_
#define _TelepathyQt4_cli_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-channel.h>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

#include <QSet>
#include <QSharedPointer>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace Telepathy
{
namespace Client
{

class Connection;
class PendingOperation;

class Channel : public StatefulDBusProxy,
                private OptionalInterfaceFactory<Channel>
{
    Q_OBJECT
    Q_DISABLE_COPY(Channel)

public:
    enum Feature {
        _Paddding = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(Features, Feature)

    Channel(Connection *connection,
            const QString &objectPath,
            const QVariantMap &immutableProperties,
            QObject *parent = 0);
    ~Channel();

    Connection *connection() const;

    QStringList interfaces() const;

    QString channelType() const;

    uint targetHandleType() const;

    uint targetHandle() const;

    bool isRequested() const;

    QSharedPointer<Contact> initiatorContact() const;

    bool isReady(Features features = 0) const;

    PendingOperation *becomeReady(Features features = 0);

    PendingOperation *requestClose();

public:
    uint groupFlags() const;

    bool groupCanAddContacts() const;
    PendingOperation *groupAddContacts(const QList<QSharedPointer<Contact> > &contacts,
            const QString &message = QString());
    bool groupCanRescindContacts() const;
    bool groupCanRemoveContacts() const;
    PendingOperation *groupRemoveContacts(const QList<QSharedPointer<Contact> > &contacts,
            const QString &message = QString(),
            uint reason = Telepathy::ChannelGroupChangeReasonNone);

    QList<QSharedPointer<Contact> > groupContacts() const;
    QList<QSharedPointer<Contact> > groupLocalPendingContacts() const;
    QList<QSharedPointer<Contact> > groupRemotePendingContacts() const;

    class GroupMemberChangeDetails
    {
    public:
        GroupMemberChangeDetails()
            : mIsValid(false) {}

        bool isValid() const { return mIsValid; }

        bool hasActor() const { return !mActor.isNull(); }
        QSharedPointer<Contact> actor() const { return mActor; }

        bool hasReason() const { return mDetails.contains("change-reason"); }
        uint reason() const { return qdbus_cast<uint>(mDetails.value("change-reason")); }

        bool hasMessage() const { return mDetails.contains("message"); }
        QString message () const { return qdbus_cast<QString>(mDetails.value("message")); }

        bool hasError() const { return mDetails.contains("error"); }
        QString error() const { return qdbus_cast<QString>(mDetails.value("error")); }

        bool hasDebugMessage() const { return mDetails.contains("debug-message"); }
        QString debugMessage() const { return qdbus_cast<QString>(mDetails.value("debug-message")); }

        QVariantMap allDetails() const { return mDetails; }

    private:
        friend class Channel;

        GroupMemberChangeDetails(const QSharedPointer<Contact> &actor, const QVariantMap &details)
            : mActor(actor), mDetails(details), mIsValid(true) {}

        QSharedPointer<Contact> mActor;
        QVariantMap mDetails;
        bool mIsValid;
    };

    GroupMemberChangeDetails groupLocalPendingContactChangeInfo(const QSharedPointer<Contact> &contact) const;
    GroupMemberChangeDetails groupSelfContactRemoveInfo() const;

    bool groupAreHandleOwnersAvailable() const;
    HandleOwnerMap groupHandleOwners() const;

    bool groupIsSelfContactTracked() const;
    QSharedPointer<Contact> groupSelfContact() const;

Q_SIGNALS:
    void groupFlagsChanged(uint flags, uint added, uint removed);

    void groupCanAddContactsChanged(bool canAddContacts);
    void groupCanRemoveContactsChanged(bool canRemoveContacts);
    void groupCanRescindContactsChanged(bool canRescindContacts);

    void groupMembersChanged(
            const QList<QSharedPointer<Contact> > &groupMembersAdded,
            const QList<QSharedPointer<Contact> > &groupLocalPendingMembersAdded,
            const QList<QSharedPointer<Contact> > &groupLocalPendingMembersRemoved,
            const QList<QSharedPointer<Contact> > &groupMembersRemoved,
            const Channel::GroupMemberChangeDetails &details);

    void groupHandleOwnersChanged(const HandleOwnerMap &owners,
            const Telepathy::UIntList &added, const Telepathy::UIntList &removed);

    void groupSelfContactChanged(const QSharedPointer<Contact> &contact);

public:
    template <class Interface>
    inline Interface *optionalInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && !interfaces().contains(name))
            return 0;

        // If present or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory<Channel>::interface<Interface>();
    }

    inline ChannelInterfaceCallStateInterface *callStateInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceCallStateInterface>(check);
    }

    inline ChannelInterfaceChatStateInterface *chatStateInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceChatStateInterface>(check);
    }

    inline ChannelInterfaceDTMFInterface *DTMFInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceDTMFInterface>(check);
    }

    inline ChannelInterfaceHoldInterface *holdInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceHoldInterface>(check);
    }

    inline ChannelInterfaceMediaSignallingInterface *mediaSignallingInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceMediaSignallingInterface>(check);
    }

    inline ChannelInterfacePasswordInterface *passwordInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfacePasswordInterface>(check);
    }

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

    template <class Interface>
    inline Interface *typeInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object having the correct channel type
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && channelType() != name)
            return 0;

        // If correct type or check bypassed, delegate to OIF
        return OptionalInterfaceFactory<Channel>::interface<Interface>();
    }

    inline ChannelTypeRoomListInterface *roomListInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return typeInterface<ChannelTypeRoomListInterface>(check);
    }

    inline ChannelTypeStreamedMediaInterface *streamedMediaInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return typeInterface<ChannelTypeStreamedMediaInterface>(check);
    }

    inline ChannelTypeTextInterface *textInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return typeInterface<ChannelTypeTextInterface>(check);
    }

    inline ChannelTypeTubesInterface *tubesInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return typeInterface<ChannelTypeTubesInterface>(check);
    }

protected:
    ChannelInterface *baseInterface() const;

    inline ChannelInterfaceGroupInterface *groupInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceGroupInterface>(check);
    }

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void gotChannelType(QDBusPendingCallWatcher *watcher);
    void gotHandle(QDBusPendingCallWatcher *watcher);
    void gotInterfaces(QDBusPendingCallWatcher *watcher);
    void onClosed();

    void onConnectionInvalidated();
    void onConnectionDestroyed();

    void gotGroupProperties(QDBusPendingCallWatcher *watcher);
    void gotGroupFlags(QDBusPendingCallWatcher *watcher);
    void gotAllMembers(QDBusPendingCallWatcher *watcher);
    void gotLocalPendingMembersWithInfo(QDBusPendingCallWatcher *watcher);
    void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    void gotContacts(Telepathy::Client::PendingOperation *op);
    void onGroupFlagsChanged(uint, uint);
    void onMembersChanged(const QString&,
            const Telepathy::UIntList&, const Telepathy::UIntList&,
            const Telepathy::UIntList&, const Telepathy::UIntList&, uint, uint);
    void onMembersChangedDetailed(
        const Telepathy::UIntList &added, const Telepathy::UIntList &removed,
        const Telepathy::UIntList &localPending, const Telepathy::UIntList &remotePending,
        const QVariantMap &details);
    void onHandleOwnersChanged(const Telepathy::HandleOwnerMap&, const Telepathy::UIntList&);
    void onSelfHandleChanged(uint);

    void continueIntrospection();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

Q_DECLARE_METATYPE(Telepathy::Client::Channel::GroupMemberChangeDetails);

#endif
