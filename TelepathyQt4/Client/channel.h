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

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

#include <QSet>

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
    Q_ENUMS(Readiness)

public:
    enum Feature {
        _Paddding = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(Features, Feature)

    enum Readiness {
        ReadinessJustCreated = 0,
        ReadinessFull = 5,
        ReadinessDead = 10,
        ReadinessClosed = 15,
        _ReadinessInvalid = 0xffff
    };

    Channel(Connection *connection,
            const QString &objectPath,
            QObject *parent = 0);
    ~Channel();

    Connection *connection() const;

    Readiness readiness() const;

    QStringList interfaces() const;

    QString channelType() const;

    uint targetHandleType() const;

    uint targetHandle() const;

    bool isReady(Features features = 0) const;

    PendingOperation *becomeReady(Features features = 0);

public Q_SLOTS:
    QDBusPendingReply<> close();

Q_SIGNALS:
    void readinessChanged(uint newReadiness);

public:
    uint groupFlags() const;

    QSet<uint> groupMembers() const;

    class GroupMemberChangeInfo
    {
    public:
        GroupMemberChangeInfo()
            : mActor(-1), mReason(0), mIsValid(false) {}

        GroupMemberChangeInfo(uint actor, uint reason, const QString &message)
            : mActor(actor), mReason(reason), mMessage(message), mIsValid(true) {}

        bool isValid() const { return mIsValid; }

        uint actor() const { return mActor; }

        uint reason() const { return mReason; }

        const QString &message() const { return mMessage; }

    private:
        uint mActor;
        uint mReason;
        QString mMessage;
        bool mIsValid;
    };

    typedef QMap<uint, GroupMemberChangeInfo> GroupMemberChangeInfoMap;

    GroupMemberChangeInfoMap groupLocalPending() const;

    QSet<uint> groupRemotePending() const;

    bool groupAreHandleOwnersAvailable() const;

    HandleOwnerMap groupHandleOwners() const;

    bool groupIsSelfHandleTracked() const;

    uint groupSelfHandle() const;

    GroupMemberChangeInfo groupSelfRemoveInfo() const;

Q_SIGNALS:
    void groupFlagsChanged(uint flags, uint added, uint removed);

    void groupMembersChanged(const QSet<uint> &members,
            const Telepathy::UIntList &added,
            const Telepathy::UIntList &removed,
            uint actor, uint reason, const QString &message);

    void groupLocalPendingChanged(const GroupMemberChangeInfoMap &localPending,
            const Telepathy::UIntList &added,
            const Telepathy::UIntList &removed,
            uint actor, uint reason, const QString &message);

    void groupRemotePendingChanged(const QSet<uint> &remotePending,
            const Telepathy::UIntList &added,
            const Telepathy::UIntList &removed,
            uint actor, uint reason, const QString &message);

    void groupHandleOwnersChanged(const HandleOwnerMap &owners,
            const Telepathy::UIntList &added, const Telepathy::UIntList &removed);

    void groupSelfHandleChanged(uint selfHandle);

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

    inline ChannelInterfaceGroupInterface *groupInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<ChannelInterfaceGroupInterface>(check);
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
    void gotLocalPending(QDBusPendingCallWatcher *watcher);
    void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    void onGroupFlagsChanged(uint, uint);
    void onMembersChanged(const QString&,
            const Telepathy::UIntList&, const Telepathy::UIntList&,
            const Telepathy::UIntList&, const Telepathy::UIntList&, uint, uint);
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

Q_DECLARE_METATYPE(Telepathy::Client::Channel::GroupMemberChangeInfo);
Q_DECLARE_METATYPE(Telepathy::Client::Channel::GroupMemberChangeInfoMap);

#endif
