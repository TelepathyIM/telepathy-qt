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

#include "cli-channel.h"

#include "_gen/cli-channel-body.hpp"
#include "_gen/cli-channel.moc.hpp"
#include "cli-channel.moc.hpp"

#include <QQueue>

#include "cli-dbus.h"
#include "constants.h"
#include "debug-internal.hpp"

namespace Telepathy
{
namespace Client
{

struct Channel::Private
{
    // Public object
    Channel& parent;

    // Instance of generated interface class
    ChannelInterface* baseInterface;

    // Owning connection
    Connection* connection;

    // Optional interface proxies
    ChannelInterfaceGroupInterface* group;
    DBus::PropertiesInterface* properties;

    // Introspection
    Readiness readiness;
    QStringList interfaces;
    QQueue<void (Private::*)()> introspectQueue;

    // Introspected properties

    // Main interface
    QString channelType;
    uint targetHandleType;
    uint targetHandle;

    // Group flags
    uint groupFlags;

    // Group members
    bool groupHaveMembers;
    QSet<uint> groupMembers;
    GroupMemberChangeInfoMap groupLocalPending;
    QSet<uint> groupRemotePending;

    // Group handle owners
    bool groupAreHandleOwnersAvailable;
    HandleOwnerMap groupHandleOwners;

    // Group self handle
    bool groupIsSelfHandleTracked;
    uint groupSelfHandle;

    // Group remove info
    GroupMemberChangeInfo groupSelfRemoveInfo;

    Private(Channel& parent, Connection* connection)
        : parent(parent)
    {
        debug() << "Creating new Channel";

        baseInterface = 0;
        group = 0;
        properties = 0;
        readiness = ReadinessJustCreated;
        targetHandleType = 0;
        targetHandle = 0;

        groupFlags = 0;
        groupHaveMembers = false;
        groupAreHandleOwnersAvailable = false;
        groupIsSelfHandleTracked = false;
        groupSelfHandle = 0;

        debug() << " Connecting to Channel::Closed()";
        parent.connect(baseInterface,
                       SIGNAL(Closed()),
                       SLOT(onClosed()));

        debug() << " Connection to owning connection's lifetime signals";
        parent.connect(connection,
                       SIGNAL(readinessChanged(uint)),
                       SLOT(onConnectionReadinessChanged(uint)));

        parent.connect(connection,
                       SIGNAL(destroyed()),
                       SLOT(onConnectionDestroyed()));

        if (connection->readiness() == Connection::ReadinessDead) {
            warning() << "Connection given as the owner for a Channel was already dead! Channel will be stillborn.";
            readiness = ReadinessDead;
        }

        introspectQueue.enqueue(&Private::introspectMain);
    }

    void introspectMain()
    {
        if (!properties) {
            properties = parent.propertiesInterface();
            Q_ASSERT(properties != 0);
        }

        debug() << "Calling Properties::GetAll(Channel)";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(
                    properties->GetAll(TELEPATHY_INTERFACE_CHANNEL), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackChannelType()
    {
        debug() << "Calling Channel::GetChannelType()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(baseInterface->GetChannelType(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotChannelType(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackHandle()
    {
        debug() << "Calling Channel::GetHandle()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(baseInterface->GetHandle(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotHandle(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackInterfaces()
    {
        debug() << "Calling Channel::GetInterfaces()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(baseInterface->GetInterfaces(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
    }

    void introspectGroup()
    {
        Q_ASSERT(properties != 0);

        if (!group) {
            group = parent.groupInterface();
            Q_ASSERT(group != 0);
        }

        debug() << "Connecting to Channel.Interface.Group::GroupFlagsChanged";
        parent.connect(group,
                       SIGNAL(GroupFlagsChanged(uint, uint)),
                       SLOT(onGroupFlagsChanged(uint, uint)));

        debug() << "Connecting to Channel.Interface.Group::MembersChanged";
        parent.connect(group,
                       SIGNAL(MembersChanged(const QString&, const Telepathy::UIntList&, const Telepathy::UIntList&, const Telepathy::UIntList&, const Telepathy::UIntList&, uint, uint)),
                       SLOT(onMembersChanged(const QString&, const Telepathy::UIntList&, const Telepathy::UIntList&, const Telepathy::UIntList&, const Telepathy::UIntList&, uint, uint)));

        debug() << "Connecting to Channel.Interface.Group::HandleOwnersChanged";
        parent.connect(group,
                       SIGNAL(HandleOwnersChanged(const Telepathy::HandleOwnerMap&, const Telepathy::UIntList&)),
                       SLOT(onHandleOwnersChanged(const Telepathy::HandleOwnerMap&, const Telepathy::UIntList&)));

        debug() << "Connecting to Channel.Interface.Group::SelfHandleChanged";
        parent.connect(group,
                       SIGNAL(SelfHandleChanged(uint)),
                       SLOT(onSelfHandleChanged(uint)));

        debug() << "Calling Properties::GetAll(Channel.Interface.Group)";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(
                    properties->GetAll(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotGroupProperties(QDBusPendingCallWatcher*)));
    }

    void introspectGroupFallbackFlags()
    {
        Q_ASSERT(group != 0);

        debug() << "Calling Channel.Interface.Group::GetGroupFlags()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(group->GetGroupFlags(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotGroupFlags(QDBusPendingCallWatcher*)));
    }

    void introspectGroupFallbackMembers()
    {
        Q_ASSERT(group != 0);

        debug() << "Calling Channel.Interface.Group::GetAllMembers()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(group->GetAllMembers(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotAllMembers(QDBusPendingCallWatcher*)));
    }

    void introspectGroupFallbackLocalPending()
    {
        Q_ASSERT(group != 0);

        debug() << "Calling Channel.Interface.Group::GetLocalPendingMembersWithInfo()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(group->GetLocalPendingMembersWithInfo(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotLocalPending(QDBusPendingCallWatcher*)));
    }

    void introspectGroupFallbackSelfHandle()
    {
        Q_ASSERT(group != 0);

        debug() << "Calling Channel.Interface.Group::GetSelfHandle()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(group->GetSelfHandle(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotSelfHandle(QDBusPendingCallWatcher*)));
    }

    void continueIntrospection()
    {
        if (readiness < ReadinessFull) {
            if (introspectQueue.isEmpty()) {
                changeReadiness(ReadinessFull);
            } else {
                (this->*introspectQueue.dequeue())();
            }
        }
    }

    void extract0177MainProps(const QVariantMap& props)
    {
        bool haveProps = props.size() >= 4
                      && props.contains("ChannelType") && !qdbus_cast<QString>(props["ChannelType"]).isEmpty()
                      && props.contains("Interfaces")
                      && props.contains("TargetHandle")
                      && props.contains("TargetHandleType");

        if (!haveProps) {
            warning() << " Properties specified in 0.17.7 not found";

            introspectQueue.enqueue(&Private::introspectMainFallbackChannelType);
            introspectQueue.enqueue(&Private::introspectMainFallbackHandle);
            introspectQueue.enqueue(&Private::introspectMainFallbackInterfaces);
        } else {
            debug() << " Found properties specified in 0.17.7";

            channelType = qdbus_cast<QString>(props["ChannelType"]);
            interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
            targetHandle = qdbus_cast<uint>(props["TargetHandle"]);
            targetHandleType = qdbus_cast<uint>(props["TargetHandleType"]);

            nowHaveInterfaces();
        }
    }

    void extract0176GroupProps(const QVariantMap& props)
    {
        bool haveProps = props.size() >= 6
                      && (props.contains("GroupFlags") && (qdbus_cast<uint>(props["GroupFlags"]) & ChannelGroupFlagProperties))
                      && props.contains("HandleOwners")
                      && props.contains("LocalPendingMembers")
                      && props.contains("Members")
                      && props.contains("RemotePendingMembers")
                      && props.contains("SelfHandle");

        if (!haveProps) {
            warning() << " Properties specified in 0.17.6 not found";
            warning() << "  Handle owners and self handle tracking disabled";

            introspectQueue.enqueue(&Private::introspectGroupFallbackFlags);
            introspectQueue.enqueue(&Private::introspectGroupFallbackMembers);
            introspectQueue.enqueue(&Private::introspectGroupFallbackLocalPending);
            introspectQueue.enqueue(&Private::introspectGroupFallbackSelfHandle);
        } else {
            debug() << " Found properties specified in 0.17.6";

            groupHaveMembers = true;
            groupAreHandleOwnersAvailable = true;
            groupIsSelfHandleTracked = true;

            groupFlags = qdbus_cast<uint>(props["GroupFlags"]);
            groupHandleOwners = qdbus_cast<HandleOwnerMap>(props["HandleOwners"]);
            groupMembers = QSet<uint>::fromList(qdbus_cast<UIntList>(props["Members"]));
            groupRemotePending = QSet<uint>::fromList(qdbus_cast<UIntList>(props["RemotePendingMembers"]));
            groupSelfHandle = qdbus_cast<uint>(props["SelfHandle"]);

            foreach (LocalPendingInfo info, qdbus_cast<LocalPendingInfoList>(props["LocalPendingMembers"])) {
                groupLocalPending[info.toBeAdded] =
                    GroupMemberChangeInfo(info.actor, info.reason, info.message);
            }
        }
    }

    void nowHaveInterfaces()
    {
        debug() << "Channel has" << interfaces.size() << "optional interfaces:" << interfaces;

        for (QStringList::const_iterator i = interfaces.begin();
                                         i != interfaces.end();
                                         ++i) {
            if (*i == TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP) {
                introspectQueue.enqueue(&Private::introspectGroup);
            }
        }
    }

    void changeReadiness(Readiness newReadiness)
    {
        Q_ASSERT(newReadiness != readiness);
        switch (readiness) {
            case ReadinessJustCreated:
                // We don't allow ReadinessClosed to be reached without ReadinessFull
                // being reached at some point first.
                Q_ASSERT((newReadiness == ReadinessFull) || (newReadiness == ReadinessDead));
                break;
            case ReadinessFull:
                Q_ASSERT((newReadiness == ReadinessDead) || (newReadiness == ReadinessClosed));
                break;
            case ReadinessDead:
            case ReadinessClosed:
            default:
                Q_ASSERT(false);
                break;
        }

        debug() << "Channel readiness changed from" << readiness << "to" << newReadiness;

        if (newReadiness == ReadinessFull) {
            debug() << "Channel fully ready";
            debug() << " Channel type" << channelType;
            debug() << " Target handle" << targetHandle;
            debug() << " Target handle type" << targetHandleType;

            if (interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
                debug() << " Group: flags" << groupFlags;
                if (groupAreHandleOwnersAvailable)
                    debug() << " Group: Number of handle owner mappings" << groupHandleOwners.size();
                else
                    debug() << " Group: No handle owners property present";
                debug() << " Group: Number of current members" << groupMembers.size();
                debug() << " Group: Number of local pending members" << groupLocalPending.size();
                debug() << " Group: Number of remote pending members" << groupRemotePending.size();
                debug() << " Group: Self handle" << groupSelfHandle << "tracked:" << (groupIsSelfHandleTracked ? "yes" : "no");
            }
        } else {
            Q_ASSERT((newReadiness == ReadinessDead) || (newReadiness == ReadinessClosed));

            debug() << "R.I.P. Channel.";

            if (groupSelfRemoveInfo.isValid()) {
                debug() << " Group: removed by  " << groupSelfRemoveInfo.actor();
                debug() << "        because of  " << groupSelfRemoveInfo.reason();
                debug() << "        with message" << groupSelfRemoveInfo.message();
            }
        }

        readiness = newReadiness;
        emit parent.readinessChanged(newReadiness);
    }
};

Channel::Channel(Connection* connection,
                 const QString& objectPath,
                 QObject* parent)
  : StatefulDBusProxy(connection->connection(), connection->service(),
            objectPath, parent),
    mPriv(new Private(*this, connection))
{
    mPriv->baseInterface = new ChannelInterface(this->dbusConnection(),
            this->busName(), this->objectPath(), this);

    // Introspection continued here so mPriv will be initialized (unlike if we
    // continued it from the Private constructor)
    mPriv->continueIntrospection();
}

Channel::~Channel()
{
    delete mPriv;
}

Connection* Channel::connection() const
{
    return mPriv->connection;
}

Channel::Readiness Channel::readiness() const
{
    return mPriv->readiness;
}

QStringList Channel::interfaces() const
{
    // Different check than the others, because the optional interface getters
    // may be used internally with the knowledge about getting the interfaces
    // list, so we don't want this to cause warnings.
    if (mPriv->readiness < ReadinessFull && mPriv->interfaces.empty())
        warning() << "Channel::interfaces() used possibly before the list of interfaces has been received";
    else if (mPriv->readiness == ReadinessDead)
        warning() << "Channel::interfaces() used with readiness ReadinessDead";
    else if (mPriv->readiness == ReadinessClosed)
        warning() << "Channel::interfaces() used with readiness ReadinessClosed";

    return mPriv->interfaces;
}

QString Channel::channelType() const
{
    // Similarly, we don't want warnings triggered when using the type interface
    // proxies internally.
    if (mPriv->readiness < ReadinessFull && mPriv->channelType.isEmpty())
        warning() << "Channel::channelType() before the channel type has been received";
    else if (mPriv->readiness == ReadinessDead)
        warning() << "Channel::channelType() used with readiness ReadinessDead";
    // Channel type will still be valid if the channel has been closed after
    // introspection completed successfully.
    // else if (mPriv->readiness == ReadinessClosed)
    //    warning() << "Channel::channelType() used with readiness ReadinessClosed";

    return mPriv->channelType;
}

uint Channel::targetHandleType() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::targetHandleType() used with readiness" << mPriv->readiness << "!= ReadinessFull";

    return mPriv->targetHandleType;
}

uint Channel::targetHandle() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::targetHandle() used with readiness" << mPriv->readiness << "!= ReadinessFull";

    return mPriv->targetHandle;
}

QDBusPendingReply<> Channel::close()
{
    // Closing a channel does not make sense if it is already dead or closed.
    if ((mPriv->readiness != ReadinessDead) && (mPriv->readiness != ReadinessClosed))
        return mPriv->baseInterface->Close();

    // If the channel is in a readiness where it doesn't make sense to be
    // closed, we emit a warning and return an error QDBusPendingReply.
    warning() << "Channel::close() used with readiness" << mPriv->readiness;

    return QDBusPendingReply<>(QDBusMessage::createError(
            "TELEPATHY_ERROR_NOT_AVAILABLE", "Attempted to close an already dead or closed channel"));
}

uint Channel::groupFlags() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupFlags() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupFlags() used with no group interface";

    return mPriv->groupFlags;
}

QSet<uint> Channel::groupMembers() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupMembers() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupMembers() used with no group interface";

    return mPriv->groupMembers;
}

Channel::GroupMemberChangeInfoMap Channel::groupLocalPending() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupLocalPending() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupLocalPending() used with no group interface";

    return mPriv->groupLocalPending;
}

QSet<uint> Channel::groupRemotePending() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupRemotePending() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupRemotePending() used with no group interface";

    return mPriv->groupRemotePending;
}

bool Channel::groupAreHandleOwnersAvailable() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupAreHandleOwnersAvailable() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupAreHandleOwnersAvailable() used with no group interface";

    return mPriv->groupAreHandleOwnersAvailable;
}

HandleOwnerMap Channel::groupHandleOwners() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupHandleOwners() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupAreHandleOwnersAvailable() used with no group interface";
    else if (!groupAreHandleOwnersAvailable())
        warning() << "Channel::areHandleOwnersAvailable() used, but handle owners not available";

    return mPriv->groupHandleOwners;
}

bool Channel::groupIsSelfHandleTracked() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::isSelfHandleTracked() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupIsSelfHandleTracked() used with no group interface";

    return mPriv->groupIsSelfHandleTracked;
}

uint Channel::groupSelfHandle() const
{
    if (mPriv->readiness != ReadinessFull)
        warning() << "Channel::groupSelfHandle() used with readiness" << mPriv->readiness << "!= ReadinessFull";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupSelfHandle() used with no group interface";

    return mPriv->groupSelfHandle;
}

Channel::GroupMemberChangeInfo Channel::groupSelfRemoveInfo() const
{
    if (mPriv->readiness != ReadinessClosed)
        warning() << "Channel::groupSelfRemoveInfo() used with readiness" << mPriv->readiness << "!= ReadinessClosed";
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP))
        warning() << "Channel::groupSelfRemoveInfo() used with no group interface";

    return mPriv->groupSelfRemoveInfo;
}

ChannelInterface* Channel::baseInterface() const
{
    return mPriv->baseInterface;
}

void Channel::gotMainProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(Channel)";
        props = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Channel) failed with " << reply.error().name() << ": " << reply.error().message();
    }

    mPriv->extract0177MainProps(props);
    // Add extraction (and possible fallbacks) in similar functions, called from here

    mPriv->continueIntrospection();
}

void Channel::gotChannelType(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetChannelType() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if ((mPriv->readiness != ReadinessDead) && (mPriv->readiness != ReadinessClosed))
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetChannelType()";
    mPriv->channelType = reply.value();
    mPriv->continueIntrospection();
}

void Channel::gotHandle(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetHandle() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if ((mPriv->readiness != ReadinessDead) && (mPriv->readiness != ReadinessClosed))
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetHandle()";
    mPriv->targetHandleType = reply.argumentAt<0>();
    mPriv->targetHandle = reply.argumentAt<1>();
    mPriv->continueIntrospection();
}

void Channel::gotInterfaces(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetInterfaces() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if ((mPriv->readiness != ReadinessDead) && (mPriv->readiness != ReadinessClosed))
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetInterfaces()";
    mPriv->interfaces = reply.value();
    mPriv->nowHaveInterfaces();
    mPriv->continueIntrospection();
}

void Channel::onClosed()
{
    debug() << "Got Channel::Closed";

    if (mPriv->readiness == ReadinessFull)
        mPriv->changeReadiness(ReadinessClosed);
    else if ((mPriv->readiness != ReadinessDead) && (mPriv->readiness != ReadinessClosed))
        mPriv->changeReadiness(ReadinessDead);
}

void Channel::onConnectionReadinessChanged(uint readiness)
{
    if (readiness == Connection::ReadinessDead && mPriv->readiness != ReadinessDead) {
        debug() << "Owning connection died leaving an orphan Channel, changing to ReadinessDead";
        mPriv->changeReadiness(ReadinessDead);
    }
}

void Channel::onConnectionDestroyed()
{
    debug() << "Owning connection destroyed, cutting off dangling pointer";
    mPriv->connection = 0;
    return onConnectionReadinessChanged(Connection::ReadinessDead);
}

void Channel::gotGroupProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(Channel.Interface.Group)";
        props = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Group) failed with " << reply.error().name() << ": " << reply.error().message();
    }

    mPriv->extract0176GroupProps(props);
    // Add extraction (and possible fallbacks) in similar functions, called from here

    mPriv->continueIntrospection();
}

void Channel::gotGroupFlags(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetGroupFlags() failed with " << reply.error().name() << ": " << reply.error().message();
    } else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetGroupFlags()";
        mPriv->groupFlags = reply.value();

        if (mPriv->groupFlags & ChannelGroupFlagProperties) {
            warning() << " Reply included ChannelGroupFlagProperties, even though properties specified in 0.17.7 didn't work! - unsetting";
            mPriv->groupFlags &= ~ChannelGroupFlagProperties;
        }
    }

    mPriv->continueIntrospection();
}

void Channel::gotAllMembers(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<UIntList, UIntList, UIntList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetAllMembers() failed with " << reply.error().name() << ": " << reply.error().message();
    } else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetAllMembers()";

        mPriv->groupHaveMembers = true;
        mPriv->groupMembers = QSet<uint>::fromList(reply.argumentAt<0>());
        mPriv->groupRemotePending = QSet<uint>::fromList(reply.argumentAt<2>());

        foreach (uint handle, QSet<uint>::fromList(reply.argumentAt<1>())) {
            mPriv->groupLocalPending[handle] = GroupMemberChangeInfo();
        }
    }

    mPriv->continueIntrospection();
}

void Channel::gotLocalPending(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<LocalPendingInfoList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetLocalPendingMembersWithInfo() failed with " << reply.error().name() << ": " << reply.error().message();
        warning() << " Falling back to what GetAllMembers returned with no extended info";
    } else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetLocalPendingMembersWithInfo()";

        foreach (LocalPendingInfo info, reply.value()) {
            mPriv->groupLocalPending[info.toBeAdded] =
                GroupMemberChangeInfo(info.actor, info.reason, info.message);
        }
    }

    mPriv->continueIntrospection();
}

void Channel::gotSelfHandle(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetSelfHandle() failed with " << reply.error().name() << ": " << reply.error().message();
    } else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetSelfHandle()";
        mPriv->groupSelfHandle = reply.value();
    }

    mPriv->continueIntrospection();
}

void Channel::onGroupFlagsChanged(uint added, uint removed)
{
    debug().nospace() << "Got Channel.Interface.Group::GroupFlagsChanged(" << hex << added << ", " << removed << ")";

    added &= ~(mPriv->groupFlags);
    removed &= mPriv->groupFlags;

    debug().nospace() << "Arguments after filtering (" << hex << added << ", " << removed << ")";

    mPriv->groupFlags |= added;
    mPriv->groupFlags &= ~removed;

    if (added || removed) {
        debug() << "Emitting groupFlagsChanged with" << mPriv->groupFlags << "value" << added << "added" << removed << "removed";
        emit groupFlagsChanged(mPriv->groupFlags, added, removed);
    }
}

void Channel::onMembersChanged(const QString& message, const Telepathy::UIntList& added, const Telepathy::UIntList& removed, const Telepathy::UIntList& localPending, const Telepathy::UIntList& remotePending, uint actor, uint reason)
{
    debug() << "Got Channel.Interface.Group::MembersChanged with" << added.size() << "added," << removed.size() << "removed," << localPending.size() << "moved to LP," << remotePending.size() << "moved to RP," << actor << " being the actor," << reason << "the reason and" << message << "the message";

    if (!mPriv->groupHaveMembers) {
        debug() << "Still waiting for initial group members, so ignoring delta signal...";
        return;
    }

    UIntList currentAdded;
    UIntList currentRemoved;
    UIntList localAdded;
    UIntList localRemoved;
    UIntList remoteAdded;
    UIntList remoteRemoved;

    foreach (uint handle, added) {
        if (!mPriv->groupMembers.contains(handle)) {
            debug() << " +++" << handle;
            mPriv->groupMembers.insert(handle);
            currentAdded.append(handle);
        }
    }

    foreach (uint handle, localPending) {
        GroupMemberChangeInfo info(actor, reason, message);

        // Special-case renaming a local-pending contact, if the signal is
        // spec-compliant. Keep the old extended info in this case.
        if (reason == ChannelGroupChangeReasonRenamed
                && added.size() == 0
                && localPending.size() == 1
                && remotePending.size() == 0
                && removed.size() == 1
                && mPriv->groupLocalPending.contains(removed[0])) {
            debug() << " Special-case local pending rename" << removed[0] << " -> " << handle;
            info = mPriv->groupLocalPending[removed[0]];
        }

        if (!mPriv->groupLocalPending.contains(handle)) {
            debug() << " LP" << handle;
            mPriv->groupLocalPending[handle] = info;
            localAdded.append(handle);
        }
    }

    foreach (uint handle, remotePending) {
        if (!mPriv->groupRemotePending.contains(handle)) {
            debug() << " RP" << handle;
            mPriv->groupRemotePending.insert(handle);
            remoteAdded.append(handle);
        }
    }

    foreach (uint handle, removed) {
        debug() << " ---" << handle;

        if (mPriv->groupMembers.remove(handle))
            currentRemoved.append(handle);

        if (mPriv->groupLocalPending.remove(handle))
            localRemoved.append(handle);

        if (mPriv->groupRemotePending.remove(handle))
            remoteRemoved.append(handle);

        if (handle == mPriv->groupSelfHandle) {
            debug() << " Self handle removed, saving info...";
            mPriv->groupSelfRemoveInfo = GroupMemberChangeInfo(actor, reason, message);
        }
    }

    if (currentAdded.size() || currentRemoved.size()) {
        debug() << " Emitting groupMembersChanged with" << currentAdded.size() << "contacts added and" << currentRemoved.size() << "contacts removed";
        emit groupMembersChanged(mPriv->groupMembers, currentAdded, currentRemoved, actor, reason, message);
    }

    if (localAdded.size() || localRemoved.size()) {
        debug() << " Emitting groupLocalPendingChanged with" << localAdded.size() << "contacts added and" << localRemoved.size() << "contacts removed";
        emit groupLocalPendingChanged(mPriv->groupLocalPending, localAdded, localRemoved, actor, reason, message);
    }

    if (remoteAdded.size() || remoteRemoved.size()) {
        debug() << " Emitting groupRemotePendingChanged with" << remoteAdded.size() << "contacts added and" << remoteRemoved.size() << "contacts removed";
        emit groupMembersChanged(mPriv->groupRemotePending, remoteAdded, remoteRemoved, actor, reason, message);
    }
}

void Channel::onHandleOwnersChanged(const Telepathy::HandleOwnerMap& added, const Telepathy::UIntList& removed)
{
    debug() << "Got Channel.Interface.Group::HandleOwnersChanged with" << added.size() << "added," << removed.size() << "removed";

    if (!mPriv->groupAreHandleOwnersAvailable) {
        debug() << "Still waiting for initial handle owners, so ignoring delta signal...";
        return;
    }

    UIntList emitAdded;
    UIntList emitRemoved;

    for (HandleOwnerMap::const_iterator i = added.begin();
                                        i != added.end();
                                        ++i) {
        uint handle = i.key();
        uint global = i.value();

        if (!mPriv->groupHandleOwners.contains(handle)
                || mPriv->groupHandleOwners[handle] != global) {
            debug() << " +++/changed" << handle << "->" << global;
            mPriv->groupHandleOwners[handle] = global;
            emitAdded.append(handle);
        }
    }

    foreach (uint handle, removed) {
        if (mPriv->groupHandleOwners.contains(handle)) {
            debug() << " ---" << handle;
            mPriv->groupHandleOwners.remove(handle);
            emitRemoved.append(handle);
        }
    }

    if (emitAdded.size() || emitRemoved.size()) {
        debug() << "Emitting groupHandleOwnersChanged with" << emitAdded.size() << "added" << emitRemoved.size() << "removed";
        emit groupHandleOwnersChanged(mPriv->groupHandleOwners, emitAdded, emitRemoved);
    }
}

void Channel::onSelfHandleChanged(uint newSelfHandle)
{
    debug().nospace() << "Got Channel.Interface.Group::SelfHandleChanged";

    if (newSelfHandle != mPriv->groupSelfHandle) {
        mPriv->groupSelfHandle = newSelfHandle;
        debug() << " Emitting groupSelfHandleChanged with new self handle" << newSelfHandle;
        emit groupSelfHandleChanged(newSelfHandle);
    }
}

}
}
