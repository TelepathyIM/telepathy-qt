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
    QMap<uint, GroupLocalPendingInfo> groupLocalPending;
    QSet<uint> groupRemotePending;

    // Group handle owners
    bool groupAreHandleOwnersAvailable;
    HandleOwnerMap groupHandleOwners;

    // Group self handle
    bool groupIsSelfHandleTracked;
    uint groupSelfHandle;

    // Group remove info
    bool groupRemoved;
    uint groupRemoveActor;
    uint groupRemoveReason;
    QString groupRemoveMessage;

    Private(Channel& parent)
        : parent(parent)
    {
        debug() << "Creating new Channel";

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
        groupRemoved = false;
        groupRemoveActor = 0;
        groupRemoveReason = 0;

        debug() << "Connecting to Channel::Closed()";
        parent.connect(&parent,
                       SIGNAL(Closed()),
                       SLOT(onClosed()));

        introspectQueue.enqueue(&Private::introspectMain);
        continueIntrospection();
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
            new QDBusPendingCallWatcher(parent.GetChannelType(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotChannelType(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackHandle()
    {
        debug() << "Calling Channel::GetHandle()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetHandle(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotHandle(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackInterfaces()
    {
        debug() << "Calling Channel::GetInterfaces()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetInterfaces(), &parent);
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
        if (introspectQueue.isEmpty()) {
            if (readiness < ReadinessFull)
                changeReadiness(ReadinessFull);
        } else {
            (this->*introspectQueue.dequeue())();
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

        continueIntrospection();
    }

    void changeReadiness(Readiness newReadiness)
    {
        Q_ASSERT(newReadiness != readiness);

        switch (readiness) {
            case ReadinessJustCreated:
                break;
            case ReadinessFull:
                Q_ASSERT(newReadiness == ReadinessDead);
                break;
            case ReadinessDead:
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
            debug() << "R.I.P. Channel.";
        }

        readiness = newReadiness;
        emit parent.readinessChanged(newReadiness);
    }
};

Channel::Channel(const QString& serviceName,
                 const QString& objectPath,
                 QObject* parent)
    : ChannelInterface(serviceName, objectPath, parent),
      mPriv(new Private(*this))
{
}

Channel::Channel(const QDBusConnection& connection,
                 const QString& serviceName,
                 const QString& objectPath,
                 QObject* parent)
    : ChannelInterface(connection, serviceName, objectPath, parent),
      mPriv(new Private(*this))
{
}

Channel::~Channel()
{
    delete mPriv;
}

Channel::Readiness Channel::readiness() const
{
    return mPriv->readiness;
}

QStringList Channel::interfaces() const
{
    return mPriv->interfaces;
}

QString Channel::channelType() const
{
    return mPriv->channelType;
}

uint Channel::targetHandleType() const
{
    return mPriv->targetHandleType;
}

uint Channel::targetHandle() const
{
    return mPriv->targetHandle;
}

uint Channel::groupFlags() const
{
    return mPriv->groupFlags;
}

void Channel::gotMainProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError())
        props = reply.value();

    QList<bool> conditions;

    conditions << (props.size() >= 4);
    conditions << (props.contains("ChannelType") && !qdbus_cast<QString>(props["ChannelType"]).isEmpty());
    conditions << props.contains("Interfaces");
    conditions << props.contains("TargetHandle");
    conditions << props.contains("TargetHandleType");

    if (conditions.contains(false)) {
        if (reply.isError())
            warning().nospace() << "Properties::GetAll(Channel) failed with " << reply.error().name() << ": " << reply.error().message();
        else
            warning() << "Reply to Properties::GetAll(Channel) didn't contain the expected properties";

        warning() << "Assuming a pre-0.17.7-spec service, falling back to serial inspection";

        mPriv->introspectQueue.enqueue(&Private::introspectMainFallbackChannelType);
        mPriv->introspectQueue.enqueue(&Private::introspectMainFallbackHandle);
        mPriv->introspectQueue.enqueue(&Private::introspectMainFallbackInterfaces);

        mPriv->continueIntrospection();
        return;
    }

    debug() << "Got reply to Properties::GetAll(Channel)";
    mPriv->channelType = qdbus_cast<QString>(props["ChannelType"]);
    mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    mPriv->targetHandle = qdbus_cast<uint>(props["TargetHandle"]);
    mPriv->targetHandleType = qdbus_cast<uint>(props["TargetHandleType"]);
    mPriv->nowHaveInterfaces();
}

void Channel::gotChannelType(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetChannelType() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if (mPriv->readiness != ReadinessDead)
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
        if (mPriv->readiness != ReadinessDead)
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
        if (mPriv->readiness != ReadinessDead)
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetInterfaces()";
    mPriv->interfaces = reply.value();
    mPriv->nowHaveInterfaces();
}

void Channel::onClosed()
{
    debug() << "Got Channel::Closed";

    if (mPriv->readiness != ReadinessDead)
        mPriv->changeReadiness(ReadinessDead);
}

void Channel::gotGroupProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError())
        props = reply.value();

    QList<bool> conditions;

    conditions << (props.size() >= 6);
    conditions << (props.contains("GroupFlags") && (qdbus_cast<uint>(props["GroupFlags"]) & ChannelGroupFlagProperties));
    conditions << props.contains("HandleOwners");
    conditions << props.contains("LocalPendingMembers");
    conditions << props.contains("Members");
    conditions << props.contains("RemotePendingMembers");
    conditions << props.contains("SelfHandle");

    if (conditions.contains(false)) {
        if (reply.isError())
            warning().nospace() << "Properties::GetAll(Channel.Interface.Group) failed with " << reply.error().name() << ": " << reply.error().message();
        else
            warning() << "Reply to Properties::GetAll(Channel.Interface.Group) didn't contain the expected properties";

        warning() << " Assuming a pre-0.17.6-spec service, falling back to serial inspection";
        warning() << " Handle owners and self handle tracking disabled";

        mPriv->introspectQueue.enqueue(&Private::introspectGroupFallbackFlags);
        mPriv->introspectQueue.enqueue(&Private::introspectGroupFallbackMembers);
        mPriv->introspectQueue.enqueue(&Private::introspectGroupFallbackLocalPending);
        mPriv->introspectQueue.enqueue(&Private::introspectGroupFallbackSelfHandle);
        mPriv->continueIntrospection();
        return;
    }

    debug() << "Got reply to Properties::GetAll(Channel.Interface.Group)";

    mPriv->groupHaveMembers = true;
    mPriv->groupAreHandleOwnersAvailable = true;
    mPriv->groupIsSelfHandleTracked = true;

    mPriv->groupFlags = qdbus_cast<uint>(props["GroupFlags"]);
    mPriv->groupHandleOwners = qdbus_cast<HandleOwnerMap>(props["HandleOwners"]);
    mPriv->groupMembers = QSet<uint>::fromList(qdbus_cast<UIntList>(props["Members"]));
    mPriv->groupRemotePending = QSet<uint>::fromList(qdbus_cast<UIntList>(props["RemotePendingMembers"]));
    mPriv->groupSelfHandle = qdbus_cast<uint>(props["SelfHandle"]);

    foreach (LocalPendingInfo info, qdbus_cast<LocalPendingInfoList>(props["LocalPendingMembers"])) {
        mPriv->groupLocalPending[info.toBeAdded] =
            GroupLocalPendingInfo(info.actor, info.reason, info.message);
    }

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
        mPriv->groupRemotePending= QSet<uint>::fromList(reply.argumentAt<2>());

        foreach (uint handle, QSet<uint>::fromList(reply.argumentAt<1>())) {
            mPriv->groupLocalPending[handle] = GroupLocalPendingInfo();
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
                GroupLocalPendingInfo(info.actor, info.reason, info.message);
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

    foreach (uint handle, added) {
        debug() << " +++" << handle;
        mPriv->groupMembers.insert(handle);
    }

    foreach (uint handle, localPending) {
        debug() << " LP" << handle;

        GroupLocalPendingInfo info(actor, reason, message);

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

        mPriv->groupLocalPending[handle] = info;
    }

    foreach (uint handle, remotePending) {
        debug() << " RP" << handle;
        mPriv->groupRemotePending.insert(handle);
    }

    foreach (uint handle, removed) {
        debug() << " ---" << handle;

        mPriv->groupMembers.remove(handle);
        mPriv->groupLocalPending.remove(handle);
        mPriv->groupRemotePending.remove(handle);

        if (handle == mPriv->groupSelfHandle) {
            debug() << " Self handle removed, saving info...";

            mPriv->groupRemoved = true;
            mPriv->groupRemoveActor = actor;
            mPriv->groupRemoveReason = reason;
            mPriv->groupRemoveMessage = message;
        }
    }
}

void Channel::onHandleOwnersChanged(const Telepathy::HandleOwnerMap& added, const Telepathy::UIntList& removed)
{
    debug() << "Got Channel.Interface.Group::HandleOwnersChanged with" << added.size() << "added," << removed.size() << "removed";

    if (!mPriv->groupAreHandleOwnersAvailable) {
        debug() << "Still waiting for initial handle owners, so ignoring delta signal...";
        return;
    }

    for (HandleOwnerMap::const_iterator i = added.begin();
                                        i != added.end();
                                        ++i)
        mPriv->groupHandleOwners[i.key()] = i.value();

    foreach (uint handle, removed)
        mPriv->groupHandleOwners.remove(handle);
}

void Channel::onSelfHandleChanged(uint newSelfHandle)
{
    debug().nospace() << "Got Channel.Interface.Group::SelfHandleChanged(" << newSelfHandle << ")";

    mPriv->groupSelfHandle = newSelfHandle;
}

}
}
