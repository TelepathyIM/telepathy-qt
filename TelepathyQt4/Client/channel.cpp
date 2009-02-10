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

#include <TelepathyQt4/Client/Channel>

#include "TelepathyQt4/_gen/cli-channel-body.hpp"
#include "TelepathyQt4/_gen/cli-channel.moc.hpp"
#include "TelepathyQt4/Client/_gen/channel.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingFailure>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/PendingSuccess>
#include <TelepathyQt4/Client/ReferencedHandles>
#include <TelepathyQt4/Constants>

#include <QHash>
#include <QQueue>
#include <QTimer>

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
 * \defgroup clientchannel Channel proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Channels and their optional
 * interfaces.
 */

namespace Telepathy
{
namespace Client
{

struct Channel::Private
{
    Private(Channel *parent, Connection *connection);
    ~Private();

    void introspectMain();
    void introspectMainFallbackChannelType();
    void introspectMainFallbackHandle();
    void introspectMainFallbackInterfaces();
    void introspectGroup();
    void introspectGroupFallbackFlags();
    void introspectGroupFallbackMembers();
    void introspectGroupFallbackLocalPendingWithInfo();
    void introspectGroupFallbackSelfHandle();

    void extract0177MainProps(const QVariantMap &props);
    void extract0176GroupProps(const QVariantMap &props);

    void nowHaveInterfaces();

    void buildContacts();
    void processMembersChanged();
    void updateContacts(const QList<QSharedPointer<Contact> > &contacts =
            QList<QSharedPointer<Contact> >());
    bool fakeGroupInterfaceIfNeeded();
    void setReady();

    class PendingReady;
    struct GroupMembersChangedInfo;

    // Public object
    Channel *parent;

    // Instance of generated interface class
    ChannelInterface *baseInterface;

    // Owning connection
    Connection *connection;

    // Optional interface proxies
    ChannelInterfaceGroupInterface *group;
    DBus::PropertiesInterface *properties;

    PendingReady *pendingReady;
    bool ready;

    // Introspection
    QStringList interfaces;
    QQueue<void (Private::*)()> introspectQueue;

    Channel::Features features;

    // Introspected properties

    // Main interface
    QString channelType;
    uint targetHandleType;
    uint targetHandle;
    bool requested;
    uint initiatorHandle;
    QSharedPointer<Contact> initiatorContact;

    // Group flags
    uint groupFlags;

    // Group members
    bool groupHaveMembers;
    bool buildingInitialContacts;
    bool buildingContacts;
    QSet<uint> pendingGroupMembers;
    QSet<uint> pendingGroupLocalPendingMembers;
    QSet<uint> pendingGroupRemotePendingMembers;
    QMap<uint, LocalPendingInfo> pendingGroupMembersChangeInfo;
    UIntList groupMembersToRemove;
    UIntList groupLocalPendingMembersToRemove;
    UIntList groupRemotePendingMembersToRemove;
    QHash<uint, QSharedPointer<Contact> > groupContacts;
    QHash<uint, QSharedPointer<Contact> > groupLocalPendingContacts;
    QHash<uint, QSharedPointer<Contact> > groupRemotePendingContacts;
    QHash<uint, GroupMemberChangeInfo > groupLocalPendingContactsChangeInfo;
    GroupMemberChangeInfo groupSelfContactRemoveInfo;
    QQueue<GroupMembersChangedInfo *> groupMembersChangedQueue;
    GroupMembersChangedInfo *currentGroupMembersChangedInfo;

    // Group handle owners
    bool groupAreHandleOwnersAvailable;
    HandleOwnerMap groupHandleOwners;

    // Group self handle
    bool pendingRetrieveGroupSelfContact;
    bool groupIsSelfHandleTracked;
    uint groupSelfHandle;
    QSharedPointer<Contact> groupSelfContact;
};

struct Channel::Private::GroupMembersChangedInfo
{
    GroupMembersChangedInfo(const QString &message,
            const Telepathy::UIntList &added, const Telepathy::UIntList &removed,
            const Telepathy::UIntList &localPending, const Telepathy::UIntList &remotePending,
            uint actor, uint reason)
        : message(message),
          added(added),
          removed(removed),
          localPending(localPending),
          remotePending(remotePending),
          actor(actor),
          reason(reason)
    {
    }

    QString message;
    Telepathy::UIntList added;
    Telepathy::UIntList removed;
    Telepathy::UIntList localPending;
    Telepathy::UIntList remotePending;
    uint actor;
    uint reason;
};

class Channel::Private::PendingReady : public PendingOperation
{
    // Channel is a friend so it can call finished() etc.
    friend class Channel;

public:
    PendingReady(Channel *parent);
};

Channel::Private::PendingReady::PendingReady(Channel *parent)
    : PendingOperation(parent)
{
}

Channel::Private::Private(Channel *parent, Connection *connection)
    : parent(parent),
      baseInterface(new ChannelInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      connection(connection),
      group(0),
      properties(0),
      pendingReady(0),
      ready(false),
      features(0),
      targetHandleType(0),
      targetHandle(0),
      requested(false),
      initiatorHandle(0),
      groupFlags(0),
      groupHaveMembers(false),
      buildingInitialContacts(false),
      buildingContacts(false),
      currentGroupMembersChangedInfo(0),
      groupAreHandleOwnersAvailable(false),
      pendingRetrieveGroupSelfContact(false),
      groupIsSelfHandleTracked(false),
      groupSelfHandle(0)
{
    debug() << "Creating new Channel";

    if (connection->isValid()) {
        debug() << " Connecting to Channel::Closed() signal";
        parent->connect(baseInterface,
                        SIGNAL(Closed()),
                        SLOT(onClosed()));

        debug() << " Connection to owning connection's lifetime signals";
        parent->connect(connection,
                        SIGNAL(invalidated(Telepathy::Client::DBusProxy *,
                                           const QString &, const QString &)),
                        SLOT(onConnectionInvalidated()));

        parent->connect(connection,
                        SIGNAL(destroyed()),
                        SLOT(onConnectionDestroyed()));
    }
    else {
        warning() << "Connection given as the owner for a Channel was "
            "invalid! Channel will be stillborn.";
        parent->invalidate(TELEPATHY_ERROR_INVALID_ARGUMENT,
                   "Connection given as the owner of this channel was invalid");
    }
}

Channel::Private::~Private()
{
    delete currentGroupMembersChangedInfo;
    foreach (GroupMembersChangedInfo *info, groupMembersChangedQueue) {
        delete info;
    }
}

void Channel::Private::introspectMain()
{
    if (!properties) {
        properties = parent->propertiesInterface();
        Q_ASSERT(properties != 0);
    }

    debug() << "Calling Properties::GetAll(Channel)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(TELEPATHY_INTERFACE_CHANNEL), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectMainFallbackChannelType()
{
    debug() << "Calling Channel::GetChannelType()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetChannelType(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotChannelType(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectMainFallbackHandle()
{
    debug() << "Calling Channel::GetHandle()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetHandle(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotHandle(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectMainFallbackInterfaces()
{
    debug() << "Calling Channel::GetInterfaces()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetInterfaces(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectGroup()
{
    Q_ASSERT(properties != 0);

    if (!group) {
        group = parent->groupInterface();
        Q_ASSERT(group != 0);
    }

    debug() << "Connecting to Channel.Interface.Group::GroupFlagsChanged";
    parent->connect(group,
                    SIGNAL(GroupFlagsChanged(uint, uint)),
                    SLOT(onGroupFlagsChanged(uint, uint)));

    debug() << "Connecting to Channel.Interface.Group::MembersChanged";
    parent->connect(group,
                    SIGNAL(MembersChanged(const QString&, const Telepathy::UIntList&,
                            const Telepathy::UIntList&, const Telepathy::UIntList&,
                            const Telepathy::UIntList&, uint, uint)),
                    SLOT(onMembersChanged(const QString&, const Telepathy::UIntList&,
                            const Telepathy::UIntList&, const Telepathy::UIntList&,
                            const Telepathy::UIntList&, uint, uint)));

    debug() << "Connecting to Channel.Interface.Group::HandleOwnersChanged";
    parent->connect(group,
                    SIGNAL(HandleOwnersChanged(const Telepathy::HandleOwnerMap&,
                            const Telepathy::UIntList&)),
                    SLOT(onHandleOwnersChanged(const Telepathy::HandleOwnerMap&,
                            const Telepathy::UIntList&)));

    debug() << "Connecting to Channel.Interface.Group::SelfHandleChanged";
    parent->connect(group,
                    SIGNAL(SelfHandleChanged(uint)),
                    SLOT(onSelfHandleChanged(uint)));

    debug() << "Calling Properties::GetAll(Channel.Interface.Group)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP),
                parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotGroupProperties(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectGroupFallbackFlags()
{
    Q_ASSERT(group != 0);

    debug() << "Calling Channel.Interface.Group::GetGroupFlags()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(group->GetGroupFlags(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotGroupFlags(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectGroupFallbackMembers()
{
    Q_ASSERT(group != 0);

    debug() << "Calling Channel.Interface.Group::GetAllMembers()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(group->GetAllMembers(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotAllMembers(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectGroupFallbackLocalPendingWithInfo()
{
    Q_ASSERT(group != 0);

    debug() << "Calling Channel.Interface.Group::GetLocalPendingMembersWithInfo()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(group->GetLocalPendingMembersWithInfo(),
                parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotLocalPendingMembersWithInfo(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectGroupFallbackSelfHandle()
{
    Q_ASSERT(group != 0);

    debug() << "Calling Channel.Interface.Group::GetSelfHandle()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(group->GetSelfHandle(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotSelfHandle(QDBusPendingCallWatcher*)));
}

void Channel::Private::extract0177MainProps(const QVariantMap &props)
{
    bool haveProps = props.size() >= 4
                  && props.contains("ChannelType")
                  && !qdbus_cast<QString>(props["ChannelType"]).isEmpty()
                  && props.contains("Interfaces")
                  && props.contains("TargetHandle")
                  && props.contains("TargetHandleType");

    if (!haveProps) {
        warning() << " Properties specified in 0.17.7 not found";

        introspectQueue.enqueue(&Private::introspectMainFallbackChannelType);
        introspectQueue.enqueue(&Private::introspectMainFallbackHandle);
        introspectQueue.enqueue(&Private::introspectMainFallbackInterfaces);
    }
    else {
        debug() << " Found properties specified in 0.17.7";

        channelType = qdbus_cast<QString>(props["ChannelType"]);
        interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
        targetHandle = qdbus_cast<uint>(props["TargetHandle"]);
        targetHandleType = qdbus_cast<uint>(props["TargetHandleType"]);
        requested = qdbus_cast<uint>(props["Requested"]);
        initiatorHandle = qdbus_cast<uint>(props["InitiatorHandle"]);

        if (!fakeGroupInterfaceIfNeeded() &&
            !interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP) &&
            initiatorHandle) {
            // there is no group interface, so lets try to build the contact
            // object for initiatorHandle now
            buildingInitialContacts = true;
            buildContacts();
        }

        nowHaveInterfaces();
    }

    debug() << "Have initiator handle:" << (initiatorHandle ? "yes" : "no");
}

void Channel::Private::extract0176GroupProps(const QVariantMap &props)
{
    bool haveProps = props.size() >= 6
                  && (props.contains("GroupFlags")
                  && (qdbus_cast<uint>(props["GroupFlags"]) &
                      ChannelGroupFlagProperties))
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
        introspectQueue.enqueue(&Private::introspectGroupFallbackLocalPendingWithInfo);
        introspectQueue.enqueue(&Private::introspectGroupFallbackSelfHandle);
    }
    else {
        debug() << " Found properties specified in 0.17.6";

        groupHaveMembers = true;
        groupAreHandleOwnersAvailable = true;
        groupIsSelfHandleTracked = true;

        groupFlags = qdbus_cast<uint>(props["GroupFlags"]);
        groupHandleOwners = qdbus_cast<HandleOwnerMap>(props["HandleOwners"]);

        pendingGroupMembers = QSet<uint>::fromList(qdbus_cast<UIntList>(props["Members"]));
        foreach (const LocalPendingInfo &info,
                qdbus_cast<LocalPendingInfoList>(props["LocalPendingMembers"])) {
            pendingGroupLocalPendingMembers.insert(info.toBeAdded);
            pendingGroupMembersChangeInfo[info.actor] = info;
        }
        pendingGroupRemotePendingMembers =
            QSet<uint>::fromList(qdbus_cast<UIntList>(props["RemotePendingMembers"]));

        groupSelfHandle = qdbus_cast<uint>(props["SelfHandle"]);

        buildingInitialContacts = true;
        buildContacts();
    }
}

void Channel::Private::nowHaveInterfaces()
{
    debug() << "Channel has" << interfaces.size() <<
        "optional interfaces:" << interfaces;

    if (interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        introspectQueue.enqueue(&Private::introspectGroup);
    }
}

void Channel::Private::buildContacts()
{
    buildingContacts = true;

    ContactManager *manager = connection->contactManager();

    UIntList toBuild = QSet<uint>(pendingGroupMembers +
            pendingGroupLocalPendingMembers +
            pendingGroupRemotePendingMembers).toList();

    foreach (const uint &handle, pendingGroupMembersChangeInfo.keys()) {
        if (handle) {
            toBuild.append(handle);
        }
    }

    if (currentGroupMembersChangedInfo &&
        currentGroupMembersChangedInfo->actor != 0) {
        toBuild.append(currentGroupMembersChangedInfo->actor);
    }

    if (buildingInitialContacts && initiatorHandle) {
        toBuild.append(initiatorHandle);
    }

    // always try to retrieve selfContact and check if it changed on
    // updateContacts or on gotContacts, in case we were not able to retrieve it
    if (groupSelfHandle) {
        toBuild.append(groupSelfHandle);
    }

    PendingContacts *pendingContacts = manager->contactsForHandles(
            toBuild,
            QSet<Contact::Feature>() << Contact::FeatureAlias
                                     << Contact::FeatureAvatarToken
                                     << Contact::FeatureSimplePresence);
    parent->connect(pendingContacts,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(gotContacts(Telepathy::Client::PendingOperation *)));
}

void Channel::Private::processMembersChanged()
{
    if (groupMembersChangedQueue.isEmpty()) {
        if (pendingRetrieveGroupSelfContact) {
            pendingRetrieveGroupSelfContact = false;
            // nothing queued but selfContact changed
            buildContacts();
        }
        return;
    }

    Q_ASSERT(pendingGroupMembers.isEmpty());
    Q_ASSERT(pendingGroupLocalPendingMembers.isEmpty());
    Q_ASSERT(pendingGroupRemotePendingMembers.isEmpty());

    // always set this to false here, as buildContacts will always try to
    // retrieve the selfContact and updateContacts will check if the built
    // contact is the same as the current contact.
    pendingRetrieveGroupSelfContact = false;

    currentGroupMembersChangedInfo = groupMembersChangedQueue.dequeue();

    foreach (uint handle, currentGroupMembersChangedInfo->added) {
        if (!groupContacts.contains(handle)) {
            pendingGroupMembers.insert(handle);
        }

        // the member was added to current members, check if it was in the
        // local/pending lists and if true, schedule for removal from that list
        if (groupLocalPendingContacts.contains(handle)) {
            groupLocalPendingMembersToRemove.append(handle);
        } else if(groupRemotePendingContacts.contains(handle)) {
            groupRemotePendingMembersToRemove.append(handle);
        }
    }

    foreach (uint handle, currentGroupMembersChangedInfo->localPending) {
        if (!groupLocalPendingContacts.contains(handle)) {
            LocalPendingInfo info = {
                handle,
                currentGroupMembersChangedInfo->actor,
                currentGroupMembersChangedInfo->reason,
                currentGroupMembersChangedInfo->message
            };
            pendingGroupLocalPendingMembers.insert(handle);
            pendingGroupMembersChangeInfo[info.actor] = info;
        }
    }

    foreach (uint handle, currentGroupMembersChangedInfo->remotePending) {
        if (!groupRemotePendingContacts.contains(handle)) {
            pendingGroupRemotePendingMembers.insert(handle);
        }
    }

    foreach (uint handle, currentGroupMembersChangedInfo->removed) {
        groupMembersToRemove.append(handle);

        if (handle == groupSelfHandle) {
            LocalPendingInfo info = {
                handle,
                currentGroupMembersChangedInfo->actor,
                currentGroupMembersChangedInfo->reason,
                currentGroupMembersChangedInfo->message
            };
            pendingGroupMembersChangeInfo[info.actor] = info;
        }
    }

    if (pendingGroupMembers.isEmpty() &&
        pendingGroupLocalPendingMembers.isEmpty() &&
        pendingGroupRemotePendingMembers.isEmpty())
    {
        // no member added, just remove the members to be removed and signal
        // membersChanged
        updateContacts();
    } else {
        buildContacts();
    }
}

void Channel::Private::updateContacts(const QList<QSharedPointer<Contact> > &contacts)
{
    QList<QSharedPointer<Contact> > groupContactsAdded;
    QList<QSharedPointer<Contact> > groupLocalPendingContactsAdded;
    QList<QSharedPointer<Contact> > groupRemotePendingContactsAdded;
    QSharedPointer<Contact> actorContact;
    bool selfContactUpdated = false;

    foreach (QSharedPointer<Contact> contact, contacts) {
        uint handle = contact->handle()[0];
        if (pendingGroupMembers.contains(handle)) {
            groupContactsAdded.append(contact);
            groupContacts[handle] = contact;
        } else if (pendingGroupLocalPendingMembers.contains(handle)) {
            groupLocalPendingContactsAdded.append(contact);
            groupLocalPendingContacts[handle] = contact;
            groupLocalPendingContactsChangeInfo[handle] = GroupMemberChangeInfo();
        } else if (pendingGroupRemotePendingMembers.contains(handle)) {
            groupRemotePendingContactsAdded.append(contact);
            groupRemotePendingContacts[handle] = contact;
        }

        if (groupSelfHandle == handle && groupSelfContact != contact) {
            groupSelfContact = contact;
            selfContactUpdated = false;
        }

        if (buildingInitialContacts && initiatorHandle == handle) {
            initiatorContact = contact;
        }

        if (currentGroupMembersChangedInfo &&
            currentGroupMembersChangedInfo->actor == contact->handle()[0]) {
            actorContact = contact;
        }
    }

    // this is not ideal, but we need to make sure groupLocalPendingContactsChangeInfo
    // is there first
    foreach (QSharedPointer<Contact> contact, contacts) {
        uint handle = contact->handle()[0];
        // the key here is the actor
        // if we retrieved the actor as well as the contact, update info for the
        // contact
        if (pendingGroupMembersChangeInfo.contains(handle)) {
            LocalPendingInfo info = pendingGroupMembersChangeInfo[handle];
            if (groupLocalPendingContactsChangeInfo.contains(info.toBeAdded)) {
                groupLocalPendingContactsChangeInfo[info.toBeAdded].update(
                        contact, info.reason, info.message);
            }
            else if (handle == groupSelfHandle) {
                groupSelfContactRemoveInfo = GroupMemberChangeInfo(
                        contact, info.reason, info.message);
            }
        }
    }

    pendingGroupMembers.clear();
    pendingGroupLocalPendingMembers.clear();
    pendingGroupRemotePendingMembers.clear();
    pendingGroupMembersChangeInfo.clear();

    if (buildingInitialContacts) {
        buildingInitialContacts = false;

        if (initiatorHandle && !initiatorContact) {
            warning() << "Unable to create contact object for initiator with handle" <<
                initiatorHandle;
        }

        if (groupSelfHandle && !groupSelfContact) {
            warning() << "Unable to create contact object for self handle" <<
                groupSelfHandle;
        }

        if (introspectQueue.isEmpty()) {
            // if we were building the initial contacts from handles and the
            // introspect queue is empty it means we are ready now, so signal it
            setReady();
        }
        processMembersChanged();
        return;
    }

    QList<QSharedPointer<Contact> > groupContactsRemoved;
    QSharedPointer<Contact> contactToRemove;
    foreach (uint handle, groupMembersToRemove) {
        if (groupContacts.contains(handle)) {
            contactToRemove = groupContacts[handle];
            groupContacts.remove(handle);
        } else if (groupLocalPendingContacts.contains(handle)) {
            contactToRemove = groupLocalPendingContacts[handle];
            groupLocalPendingContacts.remove(handle);
        } else if (groupRemotePendingContacts.contains(handle)) {
            contactToRemove = groupRemotePendingContacts[handle];
            groupRemotePendingContacts.remove(handle);
        }

        if (groupLocalPendingContactsChangeInfo.contains(handle)) {
            groupLocalPendingContactsChangeInfo.remove(handle);
        }

        if (contactToRemove) {
            groupContactsRemoved.append(contactToRemove);
        }
    }
    groupMembersToRemove.clear();

    foreach (uint handle, groupLocalPendingMembersToRemove) {
        groupLocalPendingContacts.remove(handle);
    }
    groupLocalPendingMembersToRemove.clear();

    foreach (uint handle, groupRemotePendingMembersToRemove) {
        groupRemotePendingContacts.remove(handle);
    }
    groupRemotePendingMembersToRemove.clear();

    if (!groupContactsAdded.isEmpty() ||
        !groupLocalPendingContactsAdded.isEmpty() ||
        !groupRemotePendingContactsAdded.isEmpty() ||
        !groupContactsRemoved.isEmpty()) {
        emit parent->groupMembersChanged(
                groupContactsAdded,
                groupLocalPendingContactsAdded,
                groupRemotePendingContactsAdded,
                groupContactsRemoved,
                actorContact,
                currentGroupMembersChangedInfo->reason,
                currentGroupMembersChangedInfo->message);
    }
    delete currentGroupMembersChangedInfo;
    currentGroupMembersChangedInfo = 0;

    if (selfContactUpdated) {
        emit parent->groupSelfContactChanged();
    }

    processMembersChanged();
}

bool Channel::Private::fakeGroupInterfaceIfNeeded()
{
    if (interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        return false;
    }

    bool ret = false;

    // this check isn't really needed as all other target handle types
    // supports the group interface, but let's make sure
    if (targetHandleType == Telepathy::HandleTypeContact) {
        // fake group interface

        if (connection->selfContact() || !targetHandle) {
            // for groupSelfContact()
            groupSelfHandle = connection->selfContact()->handle()[0];

            // for groupContacts()
            pendingGroupMembers.insert(groupSelfHandle);
            pendingGroupMembers.insert(targetHandle);

            debug().nospace() << "Faking a group on channel with self handle=" <<
                groupSelfHandle << " and other handle=" << targetHandle;
            ret = true;

            buildingInitialContacts = true;
            buildContacts();
        }
        else {
            warning() << "Connection::selfContact returned a null contact or targetHandle is 0, "
                "not faking a group on channel";
        }

    } else {
        warning() << "Channel does not support group interface and targetHandleType != Contact, "
            "not faking a group on channel";
    }

    return ret;
}

void Channel::Private::setReady()
{
    Q_ASSERT(!ready);

    ready = true;

    if (pendingReady) {
        pendingReady->setFinished();
        // it will delete itself later
        pendingReady = 0;
    }

    debug() << "Channel fully ready";
    debug() << " Channel type" << channelType;
    debug() << " Target handle" << targetHandle;
    debug() << " Target handle type" << targetHandleType;

    if (interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        debug() << " Group: flags" << groupFlags;
        if (groupAreHandleOwnersAvailable) {
            debug() << " Group: Number of handle owner mappings" <<
                groupHandleOwners.size();
        }
        else {
            debug() << " Group: No handle owners property present";
        }
        debug() << " Group: Number of current members" <<
            groupContacts.size();
        debug() << " Group: Number of local pending members" <<
            groupLocalPendingContacts.size();
        debug() << " Group: Number of remote pending members" <<
            groupRemotePendingContacts.size();
        debug() << " Group: Self handle" << groupSelfHandle <<
            "tracked:" << (groupIsSelfHandleTracked ? "yes" : "no");
    }
}

/**
 * \class Channel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/Client/channel.h> <TelepathyQt4/Client/Channel>
 *
 * High-level proxy object for accessing remote %Telepathy %Channel objects.
 *
 * It adds the following features compared to using ChannelInterface directly:
 * <ul>
 *  <li>Life cycle tracking</li>
 *  <li>Getting the channel type, handle type, handle and interfaces automatically</li>
 *  <li>High-level methods for the group interface</li>
 *  <li>A fake group implementation when handle type != Contact</li>
 *  <li>Shared optional interface proxy instances</li>
 * </ul>
 *
 * The remote object state accessor functions on this object (interfaces(),
 * channelType(), targetHandleType(), targetHandle(), requested(),
 * initiatorContact(), etc) don't make any DBus calls;
 * instead, they return values cached from a previous introspection run. The
 * introspection process populates their values in the most efficient way
 * possible based on what the service implements. However, their value is not
 * defined unless the object is ready, as returned by
 * isReady(). becomeReady should be used to make sure channel is ready.
 *
 * Additionally, the state of the Group interface on the remote object (if
 * present) will be cached in the introspection process, and also tracked for
 * any changes.
 *
 * Each Channel is owned by a Connection. If the Connection becomes dead (as
 * signaled by Connection::statusChanged(Disconnected)) or is deleted, the Channel object
 * will transition to closed too.
 */

/**
 * Construct a new Channel object.
 *
 * \param connection Connection owning this Channel, and specifying the
 *                   service.
 * \param objectPath Channel object path.
 * \param parent Object parent.
 */
Channel::Channel(Connection *connection,
                 const QString &objectPath,
                 QObject *parent)
    : StatefulDBusProxy(connection->dbusConnection(), connection->busName(),
            objectPath, parent),
      OptionalInterfaceFactory<Channel>(this),
      mPriv(new Private(this, connection))
{
    // no need to start introspection if channel is invalid
    if (isValid()) {
        mPriv->introspectQueue.enqueue(&Private::introspectMain);
        QTimer::singleShot(0, this, SLOT(continueIntrospection()));
    }
}

/**
 * Class destructor.
 */
Channel::~Channel()
{
    delete mPriv;
}

/**
 * Return the owning Connection of the Channel.
 *
 * \return A pointer to the Connection object that owns this Channel.
 */
Connection *Channel::connection() const
{
    return mPriv->connection;
}

/**
 * Return a list of optional interfaces implemented by the remote object.
 *
 * \return D-Bus names of the implemented optional interfaces.
 */
QStringList Channel::interfaces() const
{
    // Different check than the others, because the optional interface getters
    // may be used internally with the knowledge about getting the interfaces
    // list, so we don't want this to cause warnings.
    if (!isReady() && mPriv->interfaces.empty()) {
        warning() << "Channel::interfaces() used possibly before the list of "
            "interfaces has been received";
    }
    else if (!isValid()) {
        warning() << "Channel::interfaces() used with channel closed";
    }

    return mPriv->interfaces;
}

/**
 * Return the type of this channel.
 *
 * \return D-Bus interface name for the type of the channel.
 */
QString Channel::channelType() const
{
    // Similarly, we don't want warnings triggered when using the type interface
    // proxies internally.
    if (!isReady() && mPriv->channelType.isEmpty()) {
        warning() << "Channel::channelType() before the channel type has "
            "been received";
    }
    else if (!isValid()) {
        warning() << "Channel::channelType() used with channel closed";
    }

    return mPriv->channelType;
}

/**
 * Return the type of the handle returned by targetHandle().
 *
 * \return The type of the handle, as specified in HandleType.
 */
uint Channel::targetHandleType() const
{
    if (!isReady()) {
        warning() << "Channel::targetHandleType() used channel not ready";
    }

    return mPriv->targetHandleType;
}

/**
 * Return the handle of the remote party with which this channel
 * communicates.
 *
 * \return The handle, which is of the type targetHandleType() indicates.
 */
uint Channel::targetHandle() const
{
    if (!isReady()) {
        warning() << "Channel::targetHandle() used channel not ready";
    }

    return mPriv->targetHandle;
}

/**
 * Return whether this channel was created in response to a
 * local request.
 *
 * Note that the value is undefined until the channel is ready.
 *
 * \return \c true if this channel was created in response to a local request,
 *         \c false otherwise.
 */
bool Channel::isRequested() const
{
    if (!isReady()) {
        warning() << "Channel::isRequested() used channel not ready";
    }

    return mPriv->requested;
}

/**
 * Return the contact who initiated the channel.
 *
 * Note that the value is undefined until the channel is ready.
 *
 * \return A Contact object representing the contact who initiated the channel,
 *         or QSharedPointer that points to null(0) if it can't be retrieved.
 */
QSharedPointer<Contact> Channel::initiatorContact() const
{
    if (!isReady()) {
        warning() << "Channel::initiatorContact() used channel not ready";
    }

    return mPriv->initiatorContact;
}

/**
 * Return whether this object has finished its initial setup.
 *
 * This is mostly useful as a sanity check, in code that shouldn't be run
 * until the object is ready. To wait for the object to be ready, call
 * becomeReady() and connect to the finished signal on the result.
 *
 * \param features Which features should be tested.
 * \return \c true if the object has finished initial setup.
 */
bool Channel::isReady(Features features) const
{
    return mPriv->ready
        && ((mPriv->features & features) == features);
}

/**
 * Return a pending operation which will succeed when this object finishes
 * its initial setup, or will fail if a fatal error occurs during this
 * initial setup.
 *
 * \param features Which features should be tested.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when this object has finished or failed its initial setup.
 */
PendingOperation *Channel::becomeReady(Features features)
{
    if (!isValid()) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel is already closed");
    }

    if (isReady(features)) {
        return new PendingSuccess(this);
    }

    if (features != 0) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Unimplemented");
    }

    if (!mPriv->pendingReady) {
        mPriv->pendingReady = new Private::PendingReady(this);
    }
    return mPriv->pendingReady;
}

/**
 * Start an asynchronous request that the channel be closed.
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * \return A %PendingOperation, which will emit finished when the
 *         request finishes.
 */
PendingOperation *Channel::requestClose()
{
    // Closing a channel does not make sense if it is already closed,
    // just silently Return.
    if (!isValid()) {
        return new PendingSuccess(this);
    }

    return new PendingVoidMethodCall(this, mPriv->baseInterface->Close());
}

/**
 * \name Group interface
 *
 * Cached access to state of the group interface on the associated remote
 * object, if the interface is present. Almost all methods return undefined values
 * if the list returned by interfaces() doesn't include
 * #TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP or if the object is not ready.
 *
 * Some methods can be used when targetHandleType() == HandleTypeContact, such
 * as groupFlags(), groupCanAddContacts(), groupCanRemoveContacts(),
 * groupSelfContact() and groupContacts().
 *
 * As the Group interface state can change freely during the lifetime of the
 * group due to events like new contacts joining the group, the cached state
 * is automatically kept in sync with the remote object's state by hooking
 * to the change notification signals present in the D-Bus interface.
 *
 * As the cached value changes, change notification signals are emitted.
 *
 * There is a change notification signal &lt;attribute&gt;Changed
 * corresponding to each cached attribute. The first parameter for each of
 * these signals is the new value of the attribute, which is suited for
 * displaying the value of the attribute in a widget in a model-view
 * fashion. The remaining arguments depend on the attribute, but in general
 * include at least the delta from the previous state of the attribute to
 * the new state.
 *
 * Check the individual signals' descriptions for details.
 */

//@{

/**
 * Return a set of flags indicating the capabilities and behaviour of the
 * group represented by the remote object.
 *
 * Change notification is via groupFlagsChanged().
 *
 * \return Bitfield combination of flags, as defined in #ChannelGroupFlag.
 */
uint Channel::groupFlags() const
{
    if (!isReady()) {
        warning() << "Channel::groupFlags() used channel not ready";
    }

    return mPriv->groupFlags;
}

/**
 * Return if contacts can be added or invited to this channel.
 *
 * \return \c true if contacts can be added, \c false otherwise.
 * \sa groupAddContacts()
 */
bool Channel::groupCanAddContacts() const
{
    if (!isReady()) {
        warning() << "Channel::groupCanAddContacts() used channel not ready";
    }

    return mPriv->groupFlags & Telepathy::ChannelGroupFlagCanAdd;
}

/**
 * Add contacts to this channel.
 *
 * \param contacts Contacts to be added.
 * \param message A string message, which can be blank if desired.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa groupCanAddContacts()
 */
PendingOperation *Channel::groupAddContacts(const QList<QSharedPointer<Contact> > &contacts,
        const QString &message)
{
    if (!isReady()) {
        warning() << "Channel::groupAddContacts() used channel not ready";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel not ready");
    } else if (!groupCanAddContacts()) {
        warning() << "Channel::groupAddContacts() used but adding contacts is not supported";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Channel does not support adding contacts");
    } else if (contacts.isEmpty()) {
        warning() << "Channel::groupAddContacts() used with empty contacts param";
        return new PendingFailure(this, TELEPATHY_ERROR_INVALID_ARGUMENT,
                "contacts cannot be an empty list");
    }

    foreach (const QSharedPointer<Contact> &contact, contacts) {
        if (!contact) {
            warning() << "Channel::groupAddContacts() used but contacts param contains "
                "invalid contact";
            return new PendingFailure(this, TELEPATHY_ERROR_INVALID_ARGUMENT,
                    "Unable to add invalid contacts");
        }
    }

    if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupAddContacts() used with no group interface";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Channel does not support group interface");
    }

    UIntList handles;
    foreach (const QSharedPointer<Contact> &contact, contacts) {
        handles << contact->handle()[0];
    }
    return new PendingVoidMethodCall(this,
            mPriv->group->AddMembers(handles, message));
}

/**
 * Return if contacts in groupRemotePendingContacts() can be removed from this channel.
 *
 * \return \c true if contacts can be removed, \c false otherwise.
 * \sa groupRemoveContacts()
 */
bool Channel::groupCanRescindContacts() const
{
    if (!isReady()) {
        warning() << "Channel::groupCanRescindContacts() used channel not ready";
    }

    return mPriv->groupFlags & Telepathy::ChannelGroupFlagCanRescind;
}

/**
 * Return if contacts in groupContacts() can be removed from this channel.
 *
 * Note that contacts in local pending lists can always be removed from the
 * channel.
 *
 * \return \c true if contacts can be removed, \c false otherwise.
 * \sa groupRemoveContacts()
 */
bool Channel::groupCanRemoveContacts() const
{
    if (!isReady()) {
        warning() << "Channel::groupCanRemoveContacts() used channel not ready";
    }

    return mPriv->groupFlags & Telepathy::ChannelGroupFlagCanRemove;
}

/**
 * Remove contacts from this channel.
 *
 * \param contacts Contacts to be removed.
 * \param message A string message, which can be blank if desired.
 * \param reason Reason of the change, as specified in
 *               #ChannelGroupChangeReason.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa groupCanRemoveContacts()
 */
PendingOperation *Channel::groupRemoveContacts(const QList<QSharedPointer<Contact> > &contacts,
        const QString &message, uint reason)
{
    if (!isReady()) {
        warning() << "Channel::groupRemoveContacts() used channel not ready";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel not ready");
    }

    if (contacts.isEmpty()) {
        warning() << "Channel::groupRemoveContacts() used with empty contacts param";
        return new PendingFailure(this, TELEPATHY_ERROR_INVALID_ARGUMENT,
                "contacts param cannot be an empty list");
    }

    foreach (const QSharedPointer<Contact> &contact, contacts) {
        if (!contact) {
            warning() << "Channel::groupRemoveContacts() used but contacts param contains "
                "invalid contact:";
            return new PendingFailure(this, TELEPATHY_ERROR_INVALID_ARGUMENT,
                    "Unable to remove invalid contacts");
        }
    }

    if (!groupCanRemoveContacts()) {
        foreach (const QSharedPointer<Contact> &contact, contacts) {
            if (mPriv->groupContacts.contains(contact->handle()[0])) {
                warning() << "Channel::groupRemoveContacts() used but remove a contact "
                    "in groupContacts() but contacts in groupContacts() can't be removed "
                    "on this channel";
                return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                        "Channel does not support removing contacts in groupContacts()");
            }
        }
    }

    if (!groupCanRescindContacts()) {
        foreach (const QSharedPointer<Contact> &contact, contacts) {
            if (mPriv->groupRemotePendingContacts.contains(contact->handle()[0])) {
                warning() << "Channel::groupRemoveContacts() used to rescind a contact "
                    "but contacts can't be rescinded on this channel";
                return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                        "Channel does not support rescinding contacts");
            }
        }
    }

    if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupRemoveContacts() used with no group interface";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Channel does not support group interface");
    }

    UIntList handles;
    foreach (const QSharedPointer<Contact> &contact, contacts) {
        handles << contact->handle()[0];
    }
    return new PendingVoidMethodCall(this,
            mPriv->group->RemoveMembersWithReason(handles, message, reason));
}

/**
 * Return the current contacts of the group.
 *
 * \return List of contact objects.
 */
QList<QSharedPointer<Contact> > Channel::groupContacts() const
{
    if (!isReady()) {
        warning() << "Channel::groupMembers() used channel not ready";
    }

    return mPriv->groupContacts.values();
}

/**
 * Return the contacts currently waiting for local approval to join the
 * group.
 *
 * \Return List of contacts.
 */
QList<QSharedPointer<Contact> > Channel::groupLocalPendingContacts() const
{
    if (!isReady()) {
        warning() << "Channel::groupLocalPending() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupLocalPending() used with no group interface";
    }

    return mPriv->groupLocalPendingContacts.values();
}

/**
 * Return the contacts currently waiting for remote approval to join the
 * group.
 *
 * \Return List of contacts.
 */
QList<QSharedPointer<Contact> > Channel::groupRemotePendingContacts() const
{
    if (!isReady()) {
        warning() << "Channel::groupRemotePending() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupRemotePending() used with no "
            "group interface";
    }

    return mPriv->groupRemotePendingContacts.values();
}

/**
 * Return information of a local pending contact change. If
 * no information is available, an object for which
 * GroupMemberChangeInfo::isValid() Return <code>false</code> is returned.
 *
 * \param A Contact object that is on the local pending contacts list.
 * \return The change info in a GroupMemberChangeInfo object.
 */
Channel::GroupMemberChangeInfo Channel::groupLocalPendingContactChangeInfo(
        const QSharedPointer<Contact> &contact) const
{
    if (!isReady()) {
        warning() << "Channel::groupLocalPending() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupLocalPending() used with no group interface";
    }
    else if (!contact) {
        warning() << "Channel::groupLocalPending() used with null contact param";
        return GroupMemberChangeInfo();
    }

    uint handle = contact->handle()[0];
    return mPriv->groupLocalPendingContactsChangeInfo.value(handle);
}

/**
 * Return information on the removal of the local user from the group. If
 * the user hasn't been removed from the group, an object for which
 * GroupMemberChangeInfo::isValid() Return <code>false</code> is returned.
 *
 * This method should be called only after the channel has been closed.
 * This is useful for getting the remove information after missing the
 * corresponding groupMembersChanged() signal, as the local user being
 * removed usually causes the remote Channel to be closed.
 *
 * The returned information is not guaranteed to be correct if
 * groupIsSelfHandleTracked() Return false and a self handle change has
 * occurred on the remote object.
 *
 * \return The remove info in a GroupMemberChangeInfo object.
 */
Channel::GroupMemberChangeInfo Channel::groupSelfContactRemoveInfo() const
{
    if (!isReady()) {
        warning() << "Channel::groupSelfContactRemoveInfo() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupSelfContactRemoveInfo() used with "
            "no group interface";
    }

    return mPriv->groupSelfContactRemoveInfo;
}

/**
 * Return whether globally valid handles can be looked up using the
 * channel-specific handle on this channel using this object.
 *
 * Handle owner lookup is only available if:
 * <ul>
 *  <li>The object is ready
 *  <li>The list returned by interfaces() contains
 *        #TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP</li>
 *  <li>The set of flags returned by groupFlags() contains
 *        GroupFlagProperties and GroupFlagChannelSpecificHandles</li>
 * </ul>
 *
 * If this function Return <code>false</code>, the return value of
 * groupHandleOwners() is undefined and groupHandleOwnersChanged() will
 * never be emitted.
 *
 * The value returned by this function will stay fixed for the entire time
 * the object is ready, so no change notification is provided.
 *
 * \return If handle owner lookup functionality is available.
 */
bool Channel::groupAreHandleOwnersAvailable() const
{
    if (!isReady()) {
        warning() << "Channel::groupAreHandleOwnersAvailable() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupAreHandleOwnersAvailable() used with "
            "no group interface";
    }

    return mPriv->groupAreHandleOwnersAvailable;
}

/**
 * Return a mapping of handles specific to this channel to globally valid
 * handles.
 *
 * The mapping includes at least all of the channel-specific handles in this
 * channel's members, local-pending and remote-pending sets as keys. Any
 * handle not in the keys of this mapping is not channel-specific in this
 * channel. Handles which are channel-specific, but for which the owner is
 * unknown, appear in this mapping with 0 as owner.
 *
 * \return A mapping from group-specific handles to globally valid handles.
 */
HandleOwnerMap Channel::groupHandleOwners() const
{
    if (!isReady()) {
        warning() << "Channel::groupHandleOwners() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupAreHandleOwnersAvailable() used with no "
            "group interface";
    }
    else if (!groupAreHandleOwnersAvailable()) {
        warning() << "Channel::groupAreHandleOwnersAvailable() used, but handle "
            "owners not available";
    }

    return mPriv->groupHandleOwners;
}

/**
 * Return whether the value returned by groupSelfContact() is guaranteed to
 * stay synchronized with what groupInterface()->GetSelfHandle() would
 * return. Older services not providing group properties don't necessarily
 * emit the SelfHandleChanged signal either, so self contact changes can't be
 * reliably tracked.
 *
 * \return Whether or not changes to the self contact are tracked.
 */
bool Channel::groupIsSelfContactTracked() const
{
    if (!isReady()) {
        warning() << "Channel::groupIsSelfHandleTracked() used channel not ready";
    }
    else if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupIsSelfHandleTracked() used with "
            "no group interface";
    }

    return mPriv->groupIsSelfHandleTracked;
}

/**
 * Return a Contact object representing the user in the group if the user is a
 * member of the group, otherwise either a Contact object representing the user or
 * 0.
 *
 * \return A contact handle representing the user, if possible.
 */
QSharedPointer<Contact> Channel::groupSelfContact() const
{
    if (!isReady()) {
        warning() << "Channel::groupSelfContact() used channel not ready";
    }

    return mPriv->groupSelfContact;
}

/**
 * \fn void groupFlagsChanged(uint flags, uint added, uint removed)
 *
 * Emitted when the value returned by groupFlags() changes.
 *
 * \param flags The value which would now be returned by groupFlags().
 * \param added Flags added compared to the previous value.
 * \param removed Flags removed compared to the previous value.
 */

/**
 * \fn void groupMembersChanged(const QList<QSharedPointer<Contact> > &groupMembersAdded, const QList<QSharedPointer<Contact> > &groupLocalPendingMembersAdded, const QList<QSharedPointer<Contact> > &groupLocalPendingMembersRemoved, const QList<QSharedPointer<Contact> > &groupMembersRemoved, QSharedPointer<Contact> actor, uint reason, const QString &message);
 *
 * Emitted when the value returned by groupContacts(), groupLocalPendingContacts() or
 * groupRemotePendingContacts() changes.
 *
 * \param groupMembersAdded The contacts that were added to this channel.
 * \param groupLocalPendingMembersAdded The local pending contacts that were
 *                                      added to this channel.
 * \param groupRemotePendingMembersAdded The remote pending contacts that were
 *                                       added to this channel.
 * \param groupMembersRemoved The contacts removed from this channel.
 * \param actor The contact requesting or causing the change.
 * \param reason Reason of the change, as specified in
 *               #ChannelGroupChangeReason.
 * \param message Message specified by the actor related to the change, such
 *                as the part message in IRC.
 */

/**
 * \fn void groupHandleOwnersChanged(const HandleOwnerMap &owners, const Telepathy::UIntList &added, const Telepathy::UIntList &removed)
 *
 * Emitted when the value returned by groupHandleOwners() changes.
 *
 * \param owners The value which would now be returned by
 *               groupHandleOwners().
 * \param added Handles which have been added to the mapping as keys, or
 *              existing handle keys for which the mapped-to value has changed.
 * \param removed Handles which have been removed from the mapping.
 */

/**
 * \fn void groupSelfContactChanged()
 *
 * Emitted when the value returned by groupSelfContact() changes.
 */

//@}

/**
 * \name Optional interface proxy factory
 *
 * Factory functions fabricating proxies for optional %Channel interfaces and
 * interfaces for specific channel types.
 */

//@{

/**
 * \fn template <class Interface> Interface *optionalInterface(InterfaceSupportedChecking check) const
 *
 * Return a pointer to a valid instance of a given %Channel optional
 * interface class, associated with the same remote object the Channel is
 * associated with, and destroyed together with the Channel.
 *
 * If the list returned by interfaces() doesn't contain the name of the
 * interface requested <code>0</code> is returned. This check can be
 * bypassed by specifying #BypassInterfaceCheck for <code>check</code>, in
 * which case a valid instance is always returned.
 *
 * If the object is not ready, the list returned by
 * interfaces() isn't guaranteed to yet represent the full set of interfaces
 * supported by the remote object. Hence the check might fail even if the
 * remote object actually supports the requested interface; using
 * #BypassInterfaceCheck is suggested when the channel is not fully ready.
 *
 * \see OptionalInterfaceFactory::interface
 *
 * \tparam Interface Class of the optional interface to get.
 * \param check Should an instance be returned even if it can't be
 *              determined that the remote object supports the
 *              requested interface.
 * \return Pointer to an instance of the interface class, or <code>0</code>.
 */

/**
 * \fn ChannelInterfaceCallStateInterface *callStateInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a CallState interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfaceCallStateInterface>(check)</code>
 */

/**
 * \fn ChannelInterfaceChatStateInterface *chatStateInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a ChatState interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfaceChatStateInterface>(check)</code>
 */

/**
 * \fn ChannelInterfaceDTMFInterface *DTMFInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a DTMF interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfaceDTMFInterface>(check)</code>
 */

/**
 * \fn ChannelInterfaceGroupInterface *groupInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a Group interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfaceGroupInterface>(check)</code>
 */

/**
 * \fn ChannelInterfaceHoldInterface *holdInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a Hold interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfaceHoldInterface>(check)</code>
 */

/**
 * \fn ChannelInterfaceMediaSignallingInterface *mediaSignallingInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a MediaSignalling interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfaceMediaSignallingInterface>(check)</code>
 */

/**
 * \fn ChannelInterfacePasswordInterface *passwordInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a Password interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ChannelInterfacePasswordInterface>(check)</code>
 */

/**
 * \fn DBus::PropertiesInterface *propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * Properties interface is not necessarily reported by the services, so a
 * <code>check</code> parameter is not provided, and the interface is always
 * assumed to be present.
 *
 * \return
 * <code>optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck)</code>
 */

/**
 * \fn template <class Interface> Interface *typeInterface(InterfaceSupportedChecking check) const
 *
 * Return a pointer to a valid instance of a given %Channel type interface
 * class, associated with the same remote object the Channel is
 * associated with, and destroyed together with the Channel.
 *
 * If the interface name returned by channelType() isn't equivalent to the
 * name of the requested interface, or the Channel is not ready,
 * <code>0</code> is returned. This check can be bypassed by
 * specifying #BypassInterfaceCheck for <code>check</code>, in which case a
 * valid instance is always returned.
 *
 * Convenience functions are provided for well-known channel types. However,
 * there is no convenience getter for TypeContactList because the proxy for
 * that interface doesn't actually have any functionality.
 *
 * \see OptionalInterfaceFactory::interface
 *
 * \tparam Interface Class of the optional interface to get.
 * \param check Should an instance be returned even if it can't be
 *              determined that the remote object is of the requested
 *              channel type.
 * \return Pointer to an instance of the interface class, or <code>0</code>.
 */

/**
 * \fn ChannelTypeRoomListInterface *roomListInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a TypeRoomList interface proxy.
 *
 * \param check Passed to typeInterface()
 * \return <code>typeInterface<ChannelTypeRoomListInterface>(check)</code>
 */

/**
 * \fn ChannelTypeStreamedMediaInterface *streamedMediaInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a TypeStreamedMedia interface proxy.
 *
 * \param check Passed to typeInterface()
 * \return <code>typeInterface<ChannelTypeStreamedMediaInterface>(check)</code>
 */

/**
 * \fn ChannelTypeTextInterface *textInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a TypeText interface proxy.
 *
 * \param check Passed to typeInterface()
 * \return <code>typeInterface<ChannelTypeTextInterface>(check)</code>
 */

/**
 * \fn ChannelTypeTubesInterface *tubesInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a TypeTubes interface proxy.
 *
 * \param check Passed to typeInterface()
 * \return <code>typeInterface<ChannelTypeTubesInterface>(check)</code>
 */

/**
 * Get the ChannelInterface for this Channel class. This method is
 * protected since the convenience methods provided by this class should
 * always be used instead of the interface by users of the class.
 *
 * \return A pointer to the existing ChannelInterface for this Channel
 */
ChannelInterface *Channel::baseInterface() const
{
    return mPriv->baseInterface;
}

//@}

void Channel::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(Channel)";
        props = reply.value();
    }
    else {
        warning().nospace() << "Properties::GetAll(Channel) failed with " <<
            reply.error().name() << ": " << reply.error().message();
    }

    mPriv->extract0177MainProps(props);
    // Add extraction (and possible fallbacks) in similar functions,
    // called from here

    continueIntrospection();
}

void Channel::gotChannelType(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetChannelType() failed with " <<
            reply.error().name() << ": " << reply.error().message() <<
            ", Channel officially dead";
        invalidate(reply.error());
        return;
    }

    debug() << "Got reply to fallback Channel::GetChannelType()";
    mPriv->channelType = reply.value();
    continueIntrospection();
}

void Channel::gotHandle(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetHandle() failed with " <<
            reply.error().name() << ": " << reply.error().message() <<
            ", Channel officially dead";
        invalidate(reply.error());
        return;
    }

    debug() << "Got reply to fallback Channel::GetHandle()";
    mPriv->targetHandleType = reply.argumentAt<0>();
    mPriv->targetHandle = reply.argumentAt<1>();
    continueIntrospection();
}

void Channel::gotInterfaces(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetInterfaces() failed with " <<
            reply.error().name() << ": " << reply.error().message() <<
            ", Channel officially dead";
        invalidate(reply.error());
        return;
    }

    debug() << "Got reply to fallback Channel::GetInterfaces()";
    mPriv->interfaces = reply.value();
    mPriv->nowHaveInterfaces();

    mPriv->fakeGroupInterfaceIfNeeded();

    continueIntrospection();
}

void Channel::onClosed()
{
    debug() << "Got Channel::Closed";
    // I think this is the nearest error code we can get at the moment
    invalidate(TELEPATHY_ERROR_CANCELLED, "Closed");
}

void Channel::onConnectionInvalidated()
{
    debug() << "Owning connection died leaving an orphan Channel, "
        "changing to closed";
    invalidate(TELEPATHY_ERROR_CANCELLED,
               "Connection given as the owner of this channel was invalidate");
}

void Channel::onConnectionDestroyed()
{
    debug() << "Owning connection destroyed, cutting off dangling pointer";
    mPriv->connection = 0;
    invalidate(TELEPATHY_ERROR_CANCELLED,
               "Connection given as the owner of this channel was destroyed");
}

void Channel::gotGroupProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(Channel.Interface.Group)";
        props = reply.value();
    }
    else {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Group) "
            "failed with " << reply.error().name() << ": " <<
            reply.error().message();
    }

    mPriv->extract0176GroupProps(props);
    // Add extraction (and possible fallbacks) in similar functions, called from here

    continueIntrospection();
}

void Channel::gotGroupFlags(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetGroupFlags() failed with " <<
            reply.error().name() << ": " << reply.error().message();
    }
    else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetGroupFlags()";
        mPriv->groupFlags = reply.value();

        if (mPriv->groupFlags & ChannelGroupFlagProperties) {
            warning() << " Reply included ChannelGroupFlagProperties, even "
                "though properties specified in 0.17.7 didn't work! - unsetting";
            mPriv->groupFlags &= ~ChannelGroupFlagProperties;
        }
    }

    continueIntrospection();
}

void Channel::gotAllMembers(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UIntList, UIntList, UIntList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetAllMembers() failed with " <<
            reply.error().name() << ": " << reply.error().message();
    }
    else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetAllMembers()";

        mPriv->groupHaveMembers = true;
        mPriv->pendingGroupMembers = QSet<uint>::fromList(reply.argumentAt<0>());
        mPriv->pendingGroupLocalPendingMembers = QSet<uint>::fromList(reply.argumentAt<1>());
        mPriv->pendingGroupRemotePendingMembers = QSet<uint>::fromList(reply.argumentAt<2>());

        // delay building contacts now until we process gotSelfHandle
    }

    continueIntrospection();
}

void Channel::gotLocalPendingMembersWithInfo(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<LocalPendingInfoList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetLocalPendingMembersWithInfo() "
            "failed with " << reply.error().name() << ": " << reply.error().message();
        warning() << " Falling back to what GetAllMembers returned with no extended info";
    }
    else {
        debug() << "Got reply to fallback "
            "Channel.Interface.Group::GetLocalPendingMembersWithInfo()";

        foreach (LocalPendingInfo info, reply.value()) {
            mPriv->pendingGroupMembersChangeInfo[info.actor] = info;
        }
    }

    continueIntrospection();
}

void Channel::gotSelfHandle(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetSelfHandle() failed with " <<
            reply.error().name() << ": " << reply.error().message();
    }
    else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetSelfHandle()";
        mPriv->groupSelfHandle = reply.value();
    }

    // build contacts even if the call failed
    mPriv->buildingInitialContacts = true;
    mPriv->buildContacts();

    continueIntrospection();
}

void Channel::gotContacts(PendingOperation *op)
{
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);

    mPriv->buildingContacts = false;

    QList<QSharedPointer<Contact> > contacts;
    if (pending->isValid()) {
        contacts = pending->contacts();

        if (!pending->invalidHandles().isEmpty()) {
            warning() << "Unable to construct Contact objects for handles:" <<
                pending->invalidHandles();

            if (mPriv->groupSelfHandle &&
                pending->invalidHandles().contains(mPriv->groupSelfHandle)) {
                warning() << "Unable to retrieve self contact";
                mPriv->groupSelfContact.clear();
                emit groupSelfContactChanged();
            }
        }
    } else {
        warning().nospace() << "Getting contacts failed with " <<
            pending->errorName() << ":" << pending->errorMessage();
    }

    mPriv->updateContacts(contacts);
}

void Channel::onGroupFlagsChanged(uint added, uint removed)
{
    debug().nospace() << "Got Channel.Interface.Group::GroupFlagsChanged(" <<
        hex << added << ", " << removed << ")";

    added &= ~(mPriv->groupFlags);
    removed &= mPriv->groupFlags;

    debug().nospace() << "Arguments after filtering (" << hex << added <<
        ", " << removed << ")";

    mPriv->groupFlags |= added;
    mPriv->groupFlags &= ~removed;

    if (added || removed) {
        debug() << "Emitting groupFlagsChanged with" << mPriv->groupFlags <<
            "value" << added << "added" << removed << "removed";
        emit groupFlagsChanged(mPriv->groupFlags, added, removed);
    }
}

void Channel::onMembersChanged(const QString &message,
        const Telepathy::UIntList &added, const Telepathy::UIntList &removed,
        const Telepathy::UIntList &localPending, const Telepathy::UIntList &remotePending,
        uint actor, uint reason)
{
    debug() << "Got Channel.Interface.Group::MembersChanged with" << added.size() <<
        "added," << removed.size() << "removed," << localPending.size() <<
        "moved to LP," << remotePending.size() << "moved to RP," << actor <<
        "being the actor," << reason << "the reason and" << message << "the message";

    if (!mPriv->groupHaveMembers) {
        debug() << "Still waiting for initial group members, "
            "so ignoring delta signal...";
        return;
    }

    if (added.isEmpty() && removed.isEmpty() &&
        localPending.isEmpty() && remotePending.isEmpty()) {
        debug() << "Nothing really changed, so skipping membersChanged";
        return;
    }

    mPriv->groupMembersChangedQueue.enqueue(
            new Private::GroupMembersChangedInfo(message, added, removed,
                localPending, remotePending, actor, reason));

    if (!mPriv->buildingContacts) {
        // if we are building contacts, we should wait it to finish so we don't
        // present the user with wrong information
        mPriv->processMembersChanged();
    }
}

void Channel::onHandleOwnersChanged(const Telepathy::HandleOwnerMap &added,
        const Telepathy::UIntList &removed)
{
    debug() << "Got Channel.Interface.Group::HandleOwnersChanged with" <<
        added.size() << "added," << removed.size() << "removed";

    if (!mPriv->groupAreHandleOwnersAvailable) {
        debug() << "Still waiting for initial handle owners, so ignoring "
            "delta signal...";
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
        debug() << "Emitting groupHandleOwnersChanged with" << emitAdded.size() <<
            "added" << emitRemoved.size() << "removed";
        emit groupHandleOwnersChanged(mPriv->groupHandleOwners,
                emitAdded, emitRemoved);
    }
}

void Channel::onSelfHandleChanged(uint newSelfHandle)
{
    debug().nospace() << "Got Channel.Interface.Group::SelfHandleChanged";

    if (newSelfHandle != mPriv->groupSelfHandle) {
        mPriv->groupSelfHandle = newSelfHandle;
        debug() << " Emitting groupSelfHandleChanged with new self handle" <<
            newSelfHandle;

        if (mPriv->groupSelfHandle) {
            if (!mPriv->buildingContacts) {
                mPriv->buildContacts();
            } else {
                // next call to processMembersChanged will build selfContact again
                mPriv->pendingRetrieveGroupSelfContact = true;
            }
        } else {
            // newSelfHandle == 0 <- strange
            // no need to call buildContacts ...
            mPriv->groupSelfContact.clear();
            emit groupSelfContactChanged();
        }
    }
}

void Channel::continueIntrospection()
{
    if (mPriv->introspectQueue.isEmpty()) {
        // we are still building initial contact objects so delay call to ready
        if (!mPriv->buildingInitialContacts) {
            mPriv->setReady();
        }
    }
    else {
        (mPriv->*(mPriv->introspectQueue.dequeue()))();
    }
}

/**
 * \class Channel::GroupMemberChangeInfo
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/channel.h <TelepathyQt4/Client/Channel>
 *
 * Class opaquely storing information on a group membership change for a
 * single member.
 *
 * Extended information is not always available; this will be reflected by
 * the return value of isValid().
 */

/**
 * \fn GroupMemberChangeInfo()
 *
 * \internal
 */

/**
 * \fn GroupMemberChangeInfo(uint actor, uint reason, const QString &message)
 *
 * \internal
 */

/**
 * \fn bool isValid() const;
 *
 * Return whether or not this object actually contains valid
 * information received from the service. If the returned value is
 * false, the values returned by the other methods for this object are
 * undefined.
 *
 * \return Whether the information stored in this object is valid.
 */

/**
 * \fn uint actor() const
 *
 * Return the contact requesting or causing the change.
 *
 * \return The handle of the contact.
 */

/**
 * \fn uint reason() const
 *
 * Return the reason for the change.
 *
 * \return The reason, as specified in #ChannelGroupChangeReason.
 */

/**
 * \fn const QString &message() const
 * Return a human-readable message from the contact represented by
 * actor() pertaining to the change, or an empty string if there is no
 * message.
 *
 * \return The message as a string.
 */

}
}
