/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
 * @license LGPL 2.1
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

#include <TelepathyQt/Channel>
#include "TelepathyQt/channel-internal.h"

#include "TelepathyQt/_gen/cli-channel-body.hpp"
#include "TelepathyQt/_gen/cli-channel.moc.hpp"
#include "TelepathyQt/_gen/channel.moc.hpp"
#include "TelepathyQt/_gen/channel-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include "TelepathyQt/future-internal.h"

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingSuccess>
#include <TelepathyQt/StreamTubeChannel>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/Constants>

#include <QHash>
#include <QQueue>
#include <QSharedData>
#include <QTimer>

namespace Tp
{

using TpFuture::Client::ChannelInterfaceMergeableConferenceInterface;
using TpFuture::Client::ChannelInterfaceSplittableInterface;

struct TP_QT_NO_EXPORT Channel::Private
{
    Private(Channel *parent, const ConnectionPtr &connection,
            const QVariantMap &immutableProperties);
    ~Private();

    static void introspectMain(Private *self);
    void introspectMainProperties();
    void introspectMainFallbackChannelType();
    void introspectMainFallbackHandle();
    void introspectMainFallbackInterfaces();
    void introspectGroup();
    void introspectGroupFallbackFlags();
    void introspectGroupFallbackMembers();
    void introspectGroupFallbackLocalPendingWithInfo();
    void introspectGroupFallbackSelfHandle();
    void introspectConference();

    static void introspectConferenceInitialInviteeContacts(Private *self);

    void continueIntrospection();

    void extractMainProps(const QVariantMap &props);
    void extract0176GroupProps(const QVariantMap &props);

    void nowHaveInterfaces();
    void nowHaveInitialMembers();

    bool setGroupFlags(uint groupFlags);

    void buildContacts();
    void doMembersChangedDetailed(const UIntList &, const UIntList &, const UIntList &,
            const UIntList &, const QVariantMap &);
    void processMembersChanged();
    void updateContacts(const QList<ContactPtr> &contacts =
            QList<ContactPtr>());
    bool fakeGroupInterfaceIfNeeded();
    void setReady();

    QString groupMemberChangeDetailsTelepathyError(
            const GroupMemberChangeDetails &details);

    inline ChannelInterfaceMergeableConferenceInterface *mergeableConferenceInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return parent->optionalInterface<ChannelInterfaceMergeableConferenceInterface>(check);
    }

    inline ChannelInterfaceSplittableInterface *splittableInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return parent->optionalInterface<ChannelInterfaceSplittableInterface>(check);
    }

    void processConferenceChannelRemoved();

    struct GroupMembersChangedInfo;
    struct ConferenceChannelRemovedInfo;

    // Public object
    Channel *parent;

    // Instance of generated interface class
    Client::ChannelInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    // Owning connection - it can be a SharedPtr as Connection does not cache
    // channels
    ConnectionPtr connection;

    QVariantMap immutableProperties;

    // Optional interface proxies
    Client::ChannelInterfaceGroupInterface *group;
    Client::ChannelInterfaceConferenceInterface *conference;

    ReadinessHelper *readinessHelper;

    // Introspection
    QQueue<void (Private::*)()> introspectQueue;

    // Introspected properties

    // Main interface
    QString channelType;
    uint targetHandleType;
    uint targetHandle;
    QString targetId;
    ContactPtr targetContact;
    bool requested;
    uint initiatorHandle;
    ContactPtr initiatorContact;

    // Group flags
    uint groupFlags;
    bool usingMembersChangedDetailed;

    // Group member introspection
    bool groupHaveMembers;
    bool buildingContacts;

    // Queue of received MCD signals to process
    QQueue<GroupMembersChangedInfo *> groupMembersChangedQueue;
    GroupMembersChangedInfo *currentGroupMembersChangedInfo;

    // Pending from the MCD signal currently processed, but contacts not yet built
    QSet<uint> pendingGroupMembers;
    QSet<uint> pendingGroupLocalPendingMembers;
    QSet<uint> pendingGroupRemotePendingMembers;
    UIntList groupMembersToRemove;
    UIntList groupLocalPendingMembersToRemove;
    UIntList groupRemotePendingMembersToRemove;

    // Initial members
    UIntList groupInitialMembers;
    LocalPendingInfoList groupInitialLP;
    UIntList groupInitialRP;

    // Current members
    QHash<uint, ContactPtr> groupContacts;
    QHash<uint, ContactPtr> groupLocalPendingContacts;
    QHash<uint, ContactPtr> groupRemotePendingContacts;

    // Stored change info
    QHash<uint, GroupMemberChangeDetails> groupLocalPendingContactsChangeInfo;
    GroupMemberChangeDetails groupSelfContactRemoveInfo;

    // Group handle owners
    bool groupAreHandleOwnersAvailable;
    HandleOwnerMap groupHandleOwners;

    // Group self identity
    bool pendingRetrieveGroupSelfContact;
    bool groupIsSelfHandleTracked;
    uint groupSelfHandle;
    ContactPtr groupSelfContact;

    // Conference
    bool introspectingConference;
    QHash<QString, ChannelPtr> conferenceChannels;
    QHash<QString, ChannelPtr> conferenceInitialChannels;
    QString conferenceInvitationMessage;
    QHash<uint, ChannelPtr> conferenceOriginalChannels;
    UIntList conferenceInitialInviteeHandles;
    Contacts conferenceInitialInviteeContacts;
    QQueue<ConferenceChannelRemovedInfo *> conferenceChannelRemovedQueue;
    bool buildingConferenceChannelRemovedActorContact;

    static const QString keyActor;
};

struct TP_QT_NO_EXPORT Channel::Private::GroupMembersChangedInfo
{
    GroupMembersChangedInfo(const UIntList &added, const UIntList &removed,
            const UIntList &localPending, const UIntList &remotePending,
            const QVariantMap &details)
        : added(added),
          removed(removed),
          localPending(localPending),
          remotePending(remotePending),
          details(details),
          // TODO most of these probably should be removed once the rest of the code using them is sanitized
          actor(qdbus_cast<uint>(details.value(keyActor))),
          reason(qdbus_cast<uint>(details.value(keyChangeReason))),
          message(qdbus_cast<QString>(details.value(keyMessage)))
    {
    }

    UIntList added;
    UIntList removed;
    UIntList localPending;
    UIntList remotePending;
    QVariantMap details;
    uint actor;
    uint reason;
    QString message;

    static const QString keyChangeReason;
    static const QString keyMessage;
    static const QString keyContactIds;
};

struct TP_QT_NO_EXPORT Channel::Private::ConferenceChannelRemovedInfo
{
    ConferenceChannelRemovedInfo(const QDBusObjectPath &channelPath, const QVariantMap &details)
        : channelPath(channelPath),
          details(details)
    {
    }

    QDBusObjectPath channelPath;
    QVariantMap details;
};

const QString Channel::Private::keyActor(QLatin1String("actor"));
const QString Channel::Private::GroupMembersChangedInfo::keyChangeReason(
        QLatin1String("change-reason"));
const QString Channel::Private::GroupMembersChangedInfo::keyMessage(QLatin1String("message"));
const QString Channel::Private::GroupMembersChangedInfo::keyContactIds(QLatin1String("contact-ids"));

Channel::Private::Private(Channel *parent, const ConnectionPtr &connection,
        const QVariantMap &immutableProperties)
    : parent(parent),
      baseInterface(new Client::ChannelInterface(parent)),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      connection(connection),
      immutableProperties(immutableProperties),
      group(0),
      conference(0),
      readinessHelper(parent->readinessHelper()),
      targetHandleType(0),
      targetHandle(0),
      requested(false),
      initiatorHandle(0),
      groupFlags(0),
      usingMembersChangedDetailed(false),
      groupHaveMembers(false),
      buildingContacts(false),
      currentGroupMembersChangedInfo(0),
      groupAreHandleOwnersAvailable(false),
      pendingRetrieveGroupSelfContact(false),
      groupIsSelfHandleTracked(false),
      groupSelfHandle(0),
      introspectingConference(false),
      buildingConferenceChannelRemovedActorContact(false)
{
    debug() << "Creating new Channel:" << parent->objectPath();

    if (connection->isValid()) {
        debug() << " Connecting to Channel::Closed() signal";
        parent->connect(baseInterface,
                        SIGNAL(Closed()),
                        SLOT(onClosed()));

        debug() << " Connection to owning connection's lifetime signals";
        parent->connect(connection.data(),
                        SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                        SLOT(onConnectionInvalidated()));
    }
    else {
        warning() << "Connection given as the owner for a Channel was "
            "invalid! Channel will be stillborn.";
        parent->invalidate(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Connection given as the owner of this channel was invalid"));
    }

    ReadinessHelper::Introspectables introspectables;

    // As Channel does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    // As Channel does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableConferenceInitialInviteeContacts(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features() << FeatureCore,                                   // dependsOnFeatures
        QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE,  // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectConferenceInitialInviteeContacts,
        this);
    introspectables[FeatureConferenceInitialInviteeContacts] =
        introspectableConferenceInitialInviteeContacts;

    readinessHelper->addIntrospectables(introspectables);
}

Channel::Private::~Private()
{
    delete currentGroupMembersChangedInfo;
    foreach (GroupMembersChangedInfo *info, groupMembersChangedQueue) {
        delete info;
    }
    foreach (ConferenceChannelRemovedInfo *info, conferenceChannelRemovedQueue) {
        delete info;
    }
}

void Channel::Private::introspectMain(Channel::Private *self)
{
    // Make sure connection object is ready, as we need to use some methods that
    // are only available after connection object gets ready.
    debug() << "Calling Connection::becomeReady()";
    self->parent->connect(self->connection->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onConnectionReady(Tp::PendingOperation*)));
}

void Channel::Private::introspectMainProperties()
{
    QVariantMap props;
    bool needIntrospectMainProps = false;
    const unsigned numNames = 8;
    const static QString names[numNames] = {
        QLatin1String("ChannelType"),
        QLatin1String("Interfaces"),
        QLatin1String("TargetHandleType"),
        QLatin1String("TargetHandle"),
        QLatin1String("TargetID"),
        QLatin1String("Requested"),
        QLatin1String("InitiatorHandle"),
        QLatin1String("InitiatorID")
    };
    const static QString qualifiedNames[numNames] = {
        TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".Interfaces"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorID")
    };
    for (unsigned i = 0; i < numNames; ++i) {
        const QString &qualified = qualifiedNames[i];
        if (!immutableProperties.contains(qualified)) {
            needIntrospectMainProps = true;
            break;
        }
        props.insert(names[i], immutableProperties.value(qualified));
    }

    // Save Requested and InitiatorHandle here, so even if the GetAll return doesn't have them but
    // the given immutable props do (eg. due to the PendingChannel fallback guesses) we use them
    requested = qdbus_cast<bool>(props[QLatin1String("Requested")]);
    initiatorHandle = qdbus_cast<uint>(props[QLatin1String("InitiatorHandle")]);

    if (props.contains(QLatin1String("InitiatorID"))) {
        QString initiatorId = qdbus_cast<QString>(props[QLatin1String("InitiatorID")]);
        connection->lowlevel()->injectContactId(initiatorHandle, initiatorId);
    }

    if (needIntrospectMainProps) {
        debug() << "Calling Properties::GetAll(Channel)";
        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    properties->GetAll(TP_QT_IFACE_CHANNEL),
                    parent);
        parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
    } else {
        extractMainProps(props);
        continueIntrospection();
    }
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
        group = parent->interface<Client::ChannelInterfaceGroupInterface>();
        Q_ASSERT(group != 0);
    }

    debug() << "Introspecting Channel.Interface.Group for" << parent->objectPath();

    parent->connect(group,
                    SIGNAL(GroupFlagsChanged(uint,uint)),
                    SLOT(onGroupFlagsChanged(uint,uint)));

    parent->connect(group,
                    SIGNAL(MembersChanged(QString,Tp::UIntList,
                            Tp::UIntList,Tp::UIntList,
                            Tp::UIntList,uint,uint)),
                    SLOT(onMembersChanged(QString,Tp::UIntList,
                            Tp::UIntList,Tp::UIntList,
                            Tp::UIntList,uint,uint)));
    parent->connect(group,
                    SIGNAL(MembersChangedDetailed(Tp::UIntList,
                            Tp::UIntList,Tp::UIntList,
                            Tp::UIntList,QVariantMap)),
                    SLOT(onMembersChangedDetailed(Tp::UIntList,
                            Tp::UIntList,Tp::UIntList,
                            Tp::UIntList,QVariantMap)));

    parent->connect(group,
                    SIGNAL(HandleOwnersChanged(Tp::HandleOwnerMap,
                            Tp::UIntList)),
                    SLOT(onHandleOwnersChanged(Tp::HandleOwnerMap,
                            Tp::UIntList)));

    parent->connect(group,
                    SIGNAL(SelfHandleChanged(uint)),
                    SLOT(onSelfHandleChanged(uint)));

    debug() << "Calling Properties::GetAll(Channel.Interface.Group)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP),
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

void Channel::Private::introspectConference()
{
    Q_ASSERT(properties != 0);
    Q_ASSERT(conference == 0);

    debug() << "Introspecting Conference interface";
    conference = parent->interface<Client::ChannelInterfaceConferenceInterface>();
    Q_ASSERT(conference != 0);

    introspectingConference = true;

    debug() << "Connecting to Channel.Interface.Conference.ChannelMerged/Removed";
    parent->connect(conference,
            SIGNAL(ChannelMerged(QDBusObjectPath,uint,QVariantMap)),
            SLOT(onConferenceChannelMerged(QDBusObjectPath,uint,QVariantMap)));
    parent->connect(conference,
            SIGNAL(ChannelRemoved(QDBusObjectPath,QVariantMap)),
            SLOT(onConferenceChannelRemoved(QDBusObjectPath,QVariantMap)));

    debug() << "Calling Properties::GetAll(Channel.Interface.Conference)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE),
            parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotConferenceProperties(QDBusPendingCallWatcher*)));
}

void Channel::Private::introspectConferenceInitialInviteeContacts(Private *self)
{
    if (!self->conferenceInitialInviteeHandles.isEmpty()) {
        ContactManagerPtr manager = self->connection->contactManager();
        PendingContacts *pendingContacts = manager->contactsForHandles(
                self->conferenceInitialInviteeHandles);
        self->parent->connect(pendingContacts,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(gotConferenceInitialInviteeContacts(Tp::PendingOperation *)));
    } else {
        self->readinessHelper->setIntrospectCompleted(
                FeatureConferenceInitialInviteeContacts, true);
    }
}

void Channel::Private::continueIntrospection()
{
    if (introspectQueue.isEmpty()) {
        // this should always be true, but let's make sure
        if (!parent->isReady(Channel::FeatureCore)) {
            if (groupMembersChangedQueue.isEmpty() && !buildingContacts &&
                !introspectingConference) {
                debug() << "Both the IS and the MCD queue empty for the first time. Ready.";
                setReady();
            } else {
                debug() << "Introspection done before contacts done - contacts sets ready";
            }
        }
    } else {
        (this->*(introspectQueue.dequeue()))();
    }
}

void Channel::Private::extractMainProps(const QVariantMap &props)
{
    const static QString keyChannelType(QLatin1String("ChannelType"));
    const static QString keyInterfaces(QLatin1String("Interfaces"));
    const static QString keyTargetHandle(QLatin1String("TargetHandle"));
    const static QString keyTargetHandleType(QLatin1String("TargetHandleType"));

    bool haveProps = props.size() >= 4
                  && props.contains(keyChannelType)
                  && !qdbus_cast<QString>(props[keyChannelType]).isEmpty()
                  && props.contains(keyInterfaces)
                  && props.contains(keyTargetHandle)
                  && props.contains(keyTargetHandleType);

    if (!haveProps) {
        warning() << "Channel properties specified in 0.17.7 not found";

        introspectQueue.enqueue(&Private::introspectMainFallbackChannelType);
        introspectQueue.enqueue(&Private::introspectMainFallbackHandle);
        introspectQueue.enqueue(&Private::introspectMainFallbackInterfaces);
    } else {
        parent->setInterfaces(qdbus_cast<QStringList>(props[keyInterfaces]));
        readinessHelper->setInterfaces(parent->interfaces());
        channelType = qdbus_cast<QString>(props[keyChannelType]);
        targetHandle = qdbus_cast<uint>(props[keyTargetHandle]);
        targetHandleType = qdbus_cast<uint>(props[keyTargetHandleType]);

        const static QString keyTargetId(QLatin1String("TargetID"));
        const static QString keyRequested(QLatin1String("Requested"));
        const static QString keyInitiatorHandle(QLatin1String("InitiatorHandle"));
        const static QString keyInitiatorId(QLatin1String("InitiatorID"));

        if (props.contains(keyTargetId)) {
            targetId = qdbus_cast<QString>(props[keyTargetId]);

            if (targetHandleType == HandleTypeContact) {
                connection->lowlevel()->injectContactId(targetHandle, targetId);
            }
        }

        if (props.contains(keyRequested)) {
            requested = qdbus_cast<uint>(props[keyRequested]);
        }

        if (props.contains(keyInitiatorHandle)) {
            initiatorHandle = qdbus_cast<uint>(props[keyInitiatorHandle]);
        }

        if (props.contains(keyInitiatorId)) {
            QString initiatorId = qdbus_cast<QString>(props[keyInitiatorId]);
            connection->lowlevel()->injectContactId(initiatorHandle, initiatorId);
        }

        if (!fakeGroupInterfaceIfNeeded() &&
            !parent->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP) &&
            initiatorHandle) {
            // No group interface, so nobody will build the poor fellow for us. Will do it ourselves
            // out of pity for him.
            // TODO: needs testing. I would imagine some of the elaborate updateContacts logic
            // tripping over with just this.
            buildContacts();
        }

        nowHaveInterfaces();
    }

    debug() << "Have initiator handle:" << (initiatorHandle ? "yes" : "no");
}

void Channel::Private::extract0176GroupProps(const QVariantMap &props)
{
    const static QString keyGroupFlags(QLatin1String("GroupFlags"));
    const static QString keyHandleOwners(QLatin1String("HandleOwners"));
    const static QString keyLPMembers(QLatin1String("LocalPendingMembers"));
    const static QString keyMembers(QLatin1String("Members"));
    const static QString keyRPMembers(QLatin1String("RemotePendingMembers"));
    const static QString keySelfHandle(QLatin1String("SelfHandle"));

    bool haveProps = props.size() >= 6
                  && (props.contains(keyGroupFlags)
                  && (qdbus_cast<uint>(props[keyGroupFlags]) &
                      ChannelGroupFlagProperties))
                  && props.contains(keyHandleOwners)
                  && props.contains(keyLPMembers)
                  && props.contains(keyMembers)
                  && props.contains(keyRPMembers)
                  && props.contains(keySelfHandle);

    if (!haveProps) {
        warning() << " Properties specified in 0.17.6 not found";
        warning() << "  Handle owners and self handle tracking disabled";

        introspectQueue.enqueue(&Private::introspectGroupFallbackFlags);
        introspectQueue.enqueue(&Private::introspectGroupFallbackMembers);
        introspectQueue.enqueue(&Private::introspectGroupFallbackLocalPendingWithInfo);
        introspectQueue.enqueue(&Private::introspectGroupFallbackSelfHandle);
    } else {
        debug() << " Found properties specified in 0.17.6";

        groupAreHandleOwnersAvailable = true;
        groupIsSelfHandleTracked = true;

        setGroupFlags(qdbus_cast<uint>(props[keyGroupFlags]));
        groupHandleOwners = qdbus_cast<HandleOwnerMap>(props[keyHandleOwners]);

        groupInitialMembers = qdbus_cast<UIntList>(props[keyMembers]);
        groupInitialLP = qdbus_cast<LocalPendingInfoList>(props[keyLPMembers]);
        groupInitialRP = qdbus_cast<UIntList>(props[keyRPMembers]);

        uint propSelfHandle = qdbus_cast<uint>(props[keySelfHandle]);
        // Don't overwrite the self handle we got from the Connection with 0
        if (propSelfHandle) {
            groupSelfHandle = propSelfHandle;
        }

        nowHaveInitialMembers();
    }
}

void Channel::Private::nowHaveInterfaces()
{
    debug() << "Channel has" << parent->interfaces().size() <<
        "optional interfaces:" << parent->interfaces();

    QStringList interfaces = parent->interfaces();

    if (interfaces.contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        introspectQueue.enqueue(&Private::introspectGroup);
    }

    if (interfaces.contains(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE)) {
        introspectQueue.enqueue(&Private::introspectConference);
    }
}

void Channel::Private::nowHaveInitialMembers()
{
    // Must be called with no contacts anywhere in the first place
    Q_ASSERT(!parent->isReady(Channel::FeatureCore));
    Q_ASSERT(!buildingContacts);

    Q_ASSERT(pendingGroupMembers.isEmpty());
    Q_ASSERT(pendingGroupLocalPendingMembers.isEmpty());
    Q_ASSERT(pendingGroupRemotePendingMembers.isEmpty());

    Q_ASSERT(groupContacts.isEmpty());
    Q_ASSERT(groupLocalPendingContacts.isEmpty());
    Q_ASSERT(groupRemotePendingContacts.isEmpty());

    // Set groupHaveMembers so we start queueing fresh MCD signals
    Q_ASSERT(!groupHaveMembers);
    groupHaveMembers = true;

    // Synthesize MCD for current + RP
    groupMembersChangedQueue.enqueue(new GroupMembersChangedInfo(
                groupInitialMembers, // Members
                UIntList(), // Removed - obviously, none
                UIntList(), // LP - will be handled separately, see below
                groupInitialRP, // Remote pending
                QVariantMap())); // No details for members + RP

    // Synthesize one MCD for each initial LP member - they might have different details
    foreach (const LocalPendingInfo &info, groupInitialLP) {
        QVariantMap details;

        if (info.actor != 0) {
            details.insert(QLatin1String("actor"), info.actor);
        }

        if (info.reason != ChannelGroupChangeReasonNone) {
            details.insert(QLatin1String("change-reason"), info.reason);
        }

        if (!info.message.isEmpty()) {
            details.insert(QLatin1String("message"), info.message);
        }

        groupMembersChangedQueue.enqueue(new GroupMembersChangedInfo(UIntList(), UIntList(),
                    UIntList() << info.toBeAdded, UIntList(), details));
    }

    // At least our added MCD event to process
    processMembersChanged();
}

bool Channel::Private::setGroupFlags(uint newGroupFlags)
{
    if (groupFlags == newGroupFlags) {
        return false;
    }

    groupFlags = newGroupFlags;

    // this shouldn't happen but let's make sure
    if (!parent->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        return false;
    }

    if ((groupFlags & ChannelGroupFlagMembersChangedDetailed) &&
        !usingMembersChangedDetailed) {
        usingMembersChangedDetailed = true;
        debug() << "Starting to exclusively listen to MembersChangedDetailed for" <<
            parent->objectPath();
        parent->disconnect(group,
                           SIGNAL(MembersChanged(QString,Tp::UIntList,
                                   Tp::UIntList,Tp::UIntList,
                                   Tp::UIntList,uint,uint)),
                           parent,
                           SLOT(onMembersChanged(QString,Tp::UIntList,
                                   Tp::UIntList,Tp::UIntList,
                                   Tp::UIntList,uint,uint)));
    } else if (!(groupFlags & ChannelGroupFlagMembersChangedDetailed) &&
               usingMembersChangedDetailed) {
        warning() << " Channel service did spec-incompliant removal of MCD from GroupFlags";
        usingMembersChangedDetailed = false;
        parent->connect(group,
                        SIGNAL(MembersChanged(QString,Tp::UIntList,
                                Tp::UIntList,Tp::UIntList,
                                Tp::UIntList,uint,uint)),
                        parent,
                        SLOT(onMembersChanged(QString,Tp::UIntList,
                                Tp::UIntList,Tp::UIntList,
                                Tp::UIntList,uint,uint)));
    }

    return true;
}

void Channel::Private::buildContacts()
{
    buildingContacts = true;

    ContactManagerPtr manager = connection->contactManager();
    UIntList toBuild = QSet<uint>(pendingGroupMembers +
            pendingGroupLocalPendingMembers +
            pendingGroupRemotePendingMembers).toList();

    if (currentGroupMembersChangedInfo &&
            currentGroupMembersChangedInfo->actor != 0) {
        toBuild.append(currentGroupMembersChangedInfo->actor);
    }

    if (!initiatorContact && initiatorHandle) {
        // No initiator contact, but Yes initiator handle - might do something about it with just
        // that information
        toBuild.append(initiatorHandle);
    }

    if (!targetContact && targetHandleType == HandleTypeContact && targetHandle != 0) {
        toBuild.append(targetHandle);
    }

    // always try to retrieve selfContact and check if it changed on
    // updateContacts or on gotContacts, in case we were not able to retrieve it
    if (groupSelfHandle) {
        toBuild.append(groupSelfHandle);
    }

    // group self handle changed to 0 <- strange but it may happen, and contacts
    // were being built at the time, so check now
    if (toBuild.isEmpty()) {
        if (!groupSelfHandle && groupSelfContact) {
            groupSelfContact.reset();
            if (parent->isReady(Channel::FeatureCore)) {
                emit parent->groupSelfContactChanged();
            }
        }

        buildingContacts = false;
        return;
    }

    PendingContacts *pendingContacts = manager->contactsForHandles(
            toBuild);
    parent->connect(pendingContacts,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContacts(Tp::PendingOperation*)));
}

void Channel::Private::processMembersChanged()
{
    Q_ASSERT(!buildingContacts);

    if (groupMembersChangedQueue.isEmpty()) {
        if (pendingRetrieveGroupSelfContact) {
            pendingRetrieveGroupSelfContact = false;
            // nothing queued but selfContact changed
            buildContacts();
            return;
        }

        if (!parent->isReady(Channel::FeatureCore)) {
            if (introspectQueue.isEmpty()) {
                debug() << "Both the MCD and the introspect queue empty for the first time. Ready!";

                if (initiatorHandle && !initiatorContact) {
                    warning() << " Unable to create contact object for initiator with handle" <<
                        initiatorHandle;
                }

                if (targetHandleType == HandleTypeContact && targetHandle != 0 && !targetContact) {
                    warning() << " Unable to create contact object for target with handle" <<
                        targetHandle;
                }

                if (groupSelfHandle && !groupSelfContact) {
                    warning() << " Unable to create contact object for self handle" <<
                        groupSelfHandle;
                }

                continueIntrospection();
            } else {
                debug() << "Contact queue empty but introspect queue isn't. IS will set ready.";
            }
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
            pendingGroupLocalPendingMembers.insert(handle);
        }
    }

    foreach (uint handle, currentGroupMembersChangedInfo->remotePending) {
        if (!groupRemotePendingContacts.contains(handle)) {
            pendingGroupRemotePendingMembers.insert(handle);
        }
    }

    foreach (uint handle, currentGroupMembersChangedInfo->removed) {
        groupMembersToRemove.append(handle);
    }

    // Always go through buildContacts - we might have a self/initiator/whatever handle to build
    buildContacts();
}

void Channel::Private::updateContacts(const QList<ContactPtr> &contacts)
{
    Contacts groupContactsAdded;
    Contacts groupLocalPendingContactsAdded;
    Contacts groupRemotePendingContactsAdded;
    ContactPtr actorContact;
    bool selfContactUpdated = false;

    debug() << "Entering Chan::Priv::updateContacts() with" << contacts.size() << "contacts";

    // FIXME: simplify. Some duplication of logic present.
    foreach (ContactPtr contact, contacts) {
        uint handle = contact->handle()[0];
        if (pendingGroupMembers.contains(handle)) {
            groupContactsAdded.insert(contact);
            groupContacts[handle] = contact;
        } else if (pendingGroupLocalPendingMembers.contains(handle)) {
            groupLocalPendingContactsAdded.insert(contact);
            groupLocalPendingContacts[handle] = contact;
            // FIXME: should set the details and actor here too
            groupLocalPendingContactsChangeInfo[handle] = GroupMemberChangeDetails();
        } else if (pendingGroupRemotePendingMembers.contains(handle)) {
            groupRemotePendingContactsAdded.insert(contact);
            groupRemotePendingContacts[handle] = contact;
        }

        if (groupSelfHandle == handle && groupSelfContact != contact) {
            groupSelfContact = contact;
            selfContactUpdated = true;
        }

        if (!initiatorContact && initiatorHandle == handle) {
            // No initiator contact stored, but there's a contact for the initiator handle
            // We can use that!
            initiatorContact = contact;
        }

        if (!targetContact && targetHandleType == HandleTypeContact && targetHandle == handle) {
            targetContact = contact;

            if (targetId.isEmpty()) {
                // For some reason, TargetID was missing from the property map. We can initialize it
                // here in that case.
                targetId = targetContact->id();
            }
        }

        if (currentGroupMembersChangedInfo &&
            currentGroupMembersChangedInfo->actor == contact->handle()[0]) {
            actorContact = contact;
        }
    }

    if (!groupSelfHandle && groupSelfContact) {
        groupSelfContact.reset();
        selfContactUpdated = true;
    }

    pendingGroupMembers.clear();
    pendingGroupLocalPendingMembers.clear();
    pendingGroupRemotePendingMembers.clear();

    // FIXME: This shouldn't be needed. Clearer would be to first scan for the actor being present
    // in the contacts supplied.
    foreach (ContactPtr contact, contacts) {
        uint handle = contact->handle()[0];
        if (groupLocalPendingContactsChangeInfo.contains(handle)) {
            groupLocalPendingContactsChangeInfo[handle] =
                GroupMemberChangeDetails(actorContact,
                        currentGroupMembersChangedInfo ? currentGroupMembersChangedInfo->details : QVariantMap());
        }
    }

    Contacts groupContactsRemoved;
    ContactPtr contactToRemove;
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
            groupContactsRemoved.insert(contactToRemove);
        }
    }
    groupMembersToRemove.clear();

    // FIXME: drop the LPToRemove and RPToRemove sets - they're redundant
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
        GroupMemberChangeDetails details(
                actorContact,
                currentGroupMembersChangedInfo ? currentGroupMembersChangedInfo->details : QVariantMap());

        if (currentGroupMembersChangedInfo
                && currentGroupMembersChangedInfo->removed.contains(groupSelfHandle)) {
            // Update groupSelfContactRemoveInfo with the proper actor in case
            // the actor was not available by the time onMembersChangedDetailed
            // was called.
            groupSelfContactRemoveInfo = details;
        }

        if (parent->isReady(Channel::FeatureCore)) {
            // Channel is ready, we can signal membership changes to the outside world without
            // confusing anyone's fragile logic.
            emit parent->groupMembersChanged(
                    groupContactsAdded,
                    groupLocalPendingContactsAdded,
                    groupRemotePendingContactsAdded,
                    groupContactsRemoved,
                    details);
        }
    }
    delete currentGroupMembersChangedInfo;
    currentGroupMembersChangedInfo = 0;

    if (selfContactUpdated && parent->isReady(Channel::FeatureCore)) {
        emit parent->groupSelfContactChanged();
    }

    processMembersChanged();
}

bool Channel::Private::fakeGroupInterfaceIfNeeded()
{
    if (parent->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        return false;
    } else if (targetHandleType != HandleTypeContact) {
        return false;
    }

    // fake group interface
    if (connection->selfHandle() && targetHandle) {
        // Fake groupSelfHandle and initial members, let the MCD handling take care of the rest
        // TODO connect to Connection::selfHandleChanged
        groupSelfHandle = connection->selfHandle();
        groupInitialMembers = UIntList() << groupSelfHandle << targetHandle;

        debug().nospace() << "Faking a group on channel with self handle=" <<
            groupSelfHandle << " and other handle=" << targetHandle;

        nowHaveInitialMembers();
    } else {
        warning() << "Connection::selfHandle is 0 or targetHandle is 0, "
            "not faking a group on channel";
    }

    return true;
}

void Channel::Private::setReady()
{
    Q_ASSERT(!parent->isReady(Channel::FeatureCore));

    debug() << "Channel fully ready";
    debug() << " Channel type" << channelType;
    debug() << " Target handle" << targetHandle;
    debug() << " Target handle type" << targetHandleType;

    if (parent->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
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

    readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

QString Channel::Private::groupMemberChangeDetailsTelepathyError(
        const GroupMemberChangeDetails &details)
{
    QString error;
    uint reason = details.reason();
    switch (reason) {
        case ChannelGroupChangeReasonOffline:
            error = TP_QT_ERROR_OFFLINE;
            break;
        case ChannelGroupChangeReasonKicked:
            error = TP_QT_ERROR_CHANNEL_KICKED;
            break;
        case ChannelGroupChangeReasonBanned:
            error = TP_QT_ERROR_CHANNEL_BANNED;
            break;
        case ChannelGroupChangeReasonBusy:
            error = TP_QT_ERROR_BUSY;
            break;
        case ChannelGroupChangeReasonNoAnswer:
            error = TP_QT_ERROR_NO_ANSWER;
            break;
        case ChannelGroupChangeReasonPermissionDenied:
            error = TP_QT_ERROR_PERMISSION_DENIED;
            break;
        case ChannelGroupChangeReasonInvalidContact:
            error = TP_QT_ERROR_DOES_NOT_EXIST;
            break;
        // The following change reason are being mapped to default
        // case ChannelGroupChangeReasonNone:
        // case ChannelGroupChangeReasonInvited:   // shouldn't happen
        // case ChannelGroupChangeReasonError:
        // case ChannelGroupChangeReasonRenamed:
        // case ChannelGroupChangeReasonSeparated: // shouldn't happen
        default:
            // let's use the actor handle and selfHandle here instead of the
            // contacts, as the contacts may not be ready.
            error = ((qdbus_cast<uint>(details.allDetails().value(QLatin1String("actor"))) == groupSelfHandle) ?
                     TP_QT_ERROR_CANCELLED :
                     TP_QT_ERROR_TERMINATED);
            break;
    }

    return error;
}

void Channel::Private::processConferenceChannelRemoved()
{
    if (buildingConferenceChannelRemovedActorContact ||
        conferenceChannelRemovedQueue.isEmpty()) {
        return;
    }

    ConferenceChannelRemovedInfo *info = conferenceChannelRemovedQueue.first();
    if (!conferenceChannels.contains(info->channelPath.path())) {
        info = conferenceChannelRemovedQueue.dequeue();
        delete info;
        processConferenceChannelRemoved();
        return;
    }

    buildingConferenceChannelRemovedActorContact = true;

    if (info->details.contains(keyActor)) {
        ContactManagerPtr manager = connection->contactManager();
        PendingContacts *pendingContacts = manager->contactsForHandles(
                UIntList() << qdbus_cast<uint>(info->details.value(keyActor)));
        parent->connect(pendingContacts,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotConferenceChannelRemovedActorContact(Tp::PendingOperation*)));
    } else {
        parent->gotConferenceChannelRemovedActorContact(0);
    }
}

struct TP_QT_NO_EXPORT Channel::GroupMemberChangeDetails::Private : public QSharedData
{
    Private(const ContactPtr &actor, const QVariantMap &details)
        : actor(actor), details(details) {}

    ContactPtr actor;
    QVariantMap details;
};

/**
 * \class Channel::GroupMemberChangeDetails
 * \ingroup clientchannel
 * \headerfile TelepathyQt/channel.h <TelepathyQt/Channel>
 *
 * \brief The Channel::GroupMemberChangeDetails class represents the details of a group membership
 *        change.
 *
 * Extended information is not always available; this will be reflected by
 * the return value of isValid().
 */

/**
 * Constructs a new invalid GroupMemberChangeDetails instance.
 */
Channel::GroupMemberChangeDetails::GroupMemberChangeDetails()
{
}

/**
 * Copy constructor.
 */
Channel::GroupMemberChangeDetails::GroupMemberChangeDetails(const GroupMemberChangeDetails &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
Channel::GroupMemberChangeDetails::~GroupMemberChangeDetails()
{
}

/**
 * Assigns all information (validity, details) from other to this.
 */
Channel::GroupMemberChangeDetails &Channel::GroupMemberChangeDetails::operator=(
        const GroupMemberChangeDetails &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

/**
 * \fn bool Channel::GroupMemberChangeDetails::isValid() const
 *
 * Return whether the details are valid (have actually been received from the service).
 *
 * \return \c true if valid, \c false otherwise.
 */

/**
 * Return whether the details specify an actor.
 *
 * If present, actor() will return the contact object representing the person who made the change.
 *
 * \return \c true if the actor is known, \c false otherwise.
 * \sa actor()
 */
bool Channel::GroupMemberChangeDetails::hasActor() const
{
    return isValid() && !mPriv->actor.isNull();
}

/**
 * Return the contact object representing the person who made the change (actor), if known.
 *
 * \return A pointer to the Contact object, or a null ContactPtr if the actor is unknown.
 * \sa hasActor()
 */
ContactPtr Channel::GroupMemberChangeDetails::actor() const
{
    return isValid() ? mPriv->actor : ContactPtr();
}

/**
 * \fn bool Channel::GroupMemberChangeDetails::hasReason() const
 *
 * Return whether the details specify the reason for the change.
 *
 * \return \c true if the reason is known, \c false otherwise.
 * \sa reason()
 */

/**
 * \fn ChannelGroupChangeReason Channel::GroupMemberChangeDetails::reason() const
 *
 * Return the reason for the change, if known.
 *
 * \return The change reason as #ChannelGroupChangeReason, or #ChannelGroupChangeReasonNone
 *         if the reason is unknown.
 * \sa hasReason()
 */

/**
 * \fn bool Channel::GroupMemberChangeDetails::hasMessage() const
 *
 * Return whether the details specify a human-readable message from the contact represented by
 * actor() pertaining to the change.
 *
 * \return \c true if the message is known, \c false otherwise.
 * \sa message()
 */

/**
 * \fn QString Channel::GroupMemberChangeDetails::message() const
 *
 * Return a human-readable message from the contact represented by actor() pertaining to the change,
 * if known.
 *
 * \return The message, or an empty string if the message is unknown.
 * \sa hasMessage()
 */

/**
 * \fn bool Channel::GroupMemberChangeDetails::hasError() const
 *
 * Return whether the details specify a D-Bus error describing the change.
 *
 * \return \c true if the error is known, \c false otherwise.
 * \sa error()
 */

/**
 * \fn QString Channel::GroupMemberChangeDetails::error() const
 *
 * Return the D-Bus error describing the change, if known.
 *
 * The D-Bus error provides more specific information than the reason() and should be used if
 * applicable.
 *
 * \return A D-Bus error describing the change, or an empty string if the error is unknown.
 * \sa hasError()
 */

/**
 * \fn bool Channel::GroupMemberChangeDetails::hasDebugMessage() const
 *
 * Return whether the details specify a debug message.
 *
 * \return \c true if debug message is present, \c false otherwise.
 * \sa debugMessage()
 */

/**
 * \fn QString Channel::GroupMemberChangeDetails::debugMessage() const
 *
 * Return the debug message specified by the details, if any.
 *
 * The debug message is purely informational, offered for display for bug reporting purposes, and
 * should not be attempted to be parsed.
 *
 * \return The debug message, or an empty string if there is none.
 * \sa hasDebugMessage()
 */

/**
 * Return a map containing all details of the group members change.
 *
 * This is useful for accessing domain-specific additional details.
 *
 * \return The details of the group members change as QVariantMap.
 */
QVariantMap Channel::GroupMemberChangeDetails::allDetails() const
{
    return isValid() ? mPriv->details : QVariantMap();
}

Channel::GroupMemberChangeDetails::GroupMemberChangeDetails(const ContactPtr &actor,
        const QVariantMap &details)
    : mPriv(new Private(actor, details))
{
}

/**
 * \class Channel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/channel.h <TelepathyQt/Channel>
 *
 * \brief The Channel class represents a Telepathy channel.
 *
 * All communication in the Telepathy framework is carried out via channel
 * objects. Specialized classes for some specific channel types such as
 * StreamedMediaChannel, TextChannel, FileTransferChannel are provided.
 *
 * The remote object accessor functions on this object (channelType(), targetHandleType(),
 * and so on) don't make any D-Bus calls; instead, they return/use
 * values cached from a previous introspection run. The introspection process
 * populates their values in the most efficient way possible based on what the
 * service implements.
 *
 * To avoid unnecessary D-Bus traffic, some accessors only return valid
 * information after specific features have been enabled.
 * For instance, to retrieve the initial invitee contacts in a conference channel,
 * it is necessary to enable the feature Channel::FeatureConferenceInitialInviteeContacts.
 * See the individual methods descriptions for more details.
 *
 * Channel features can be enabled by constructing a ChannelFactory and enabling
 * the desired features, and passing it to AccountManager, Account or ClientRegistrar
 * when creating them as appropriate. However, if a particular
 * feature is only ever used in a specific circumstance, such as an user opening
 * some settings dialog separate from the general view of the application,
 * features can be later enabled as needed by calling becomeReady() with the additional
 * features, and waiting for the resulting PendingOperation to finish.
 *
 * Each channel is owned by a connection. If the Connection object becomes invalidated
 * the Channel object will also get invalidated.
 *
 * \section chan_usage_sec Usage
 *
 * \subsection chan_create_sec Creating a channel object
 *
 * Channel objects can be created in various ways, but the preferred way is
 * trough Account channel creation methods such as Account::ensureTextChat(),
 * Account::createFileTransfer(), which uses the channel dispatcher.
 *
 * If you already know the object path, you can just call create().
 * For example:
 *
 * \code
 *
 * ChannelPtr chan = Channel::create(connection, objectPath,
 *         immutableProperties);
 *
 * \endcode
 *
 * \subsection chan_ready_sec Making channel ready to use
 *
 * A Channel object needs to become ready before usage, meaning that the
 * introspection process finished and the object accessors can be used.
 *
 * To make the object ready, use becomeReady() and wait for the
 * PendingOperation::finished() signal to be emitted.
 *
 * \code
 *
 * class MyClass : public QObject
 * {
 *     QOBJECT
 *
 * public:
 *     MyClass(QObject *parent = 0);
 *     ~MyClass() { }
 *
 * private Q_SLOTS:
 *     void onChannelReady(Tp::PendingOperation*);
 *
 * private:
 *     ChannelPtr chan;
 * };
 *
 * MyClass::MyClass(const ConnectionPtr &connection,
 *         const QString &objectPath, const QVariantMap &immutableProperties)
 *     : QObject(parent)
 *       chan(Channel::create(connection, objectPath, immutableProperties))
 * {
 *     connect(chan->becomeReady(),
 *             SIGNAL(finished(Tp::PendingOperation*)),
 *             SLOT(onChannelReady(Tp::PendingOperation*)));
 * }
 *
 * void MyClass::onChannelReady(Tp::PendingOperation *op)
 * {
 *     if (op->isError()) {
 *         qWarning() << "Channel cannot become ready:" <<
 *             op->errorName() << "-" << op->errorMessage();
 *         return;
 *     }
 *
 *     // Channel is now ready
 * }
 *
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the Channel
 * object usable.
 *
 * Note that this feature must be enabled in order to use most Channel methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature Channel::FeatureCore = Feature(QLatin1String(Channel::staticMetaObject.className()), 0, true);

/**
 * Feature used in order to access the conference initial invitee contacts info.
 *
 * \sa conferenceInitialInviteeContacts()
 */
const Feature Channel::FeatureConferenceInitialInviteeContacts = Feature(QLatin1String(Channel::staticMetaObject.className()), 1, true);

/**
 * Create a new Channel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A ChannelPtr object pointing to the newly created Channel object.
 *
 * \todo \a immutableProperties should be used to populate the corresponding accessors (such as
 * channelType()) already on construction, not only when making FeatureCore ready (fd.o #41654)
 */
ChannelPtr Channel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ChannelPtr(new Channel(connection, objectPath, immutableProperties,
                Channel::FeatureCore));
}

/**
 * Construct a new Channel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type. The corresponding introspectable should
 *                    depend on Channel::FeatureCore.
 */
Channel::Channel(const ConnectionPtr &connection,
                 const QString &objectPath,
                 const QVariantMap &immutableProperties,
                 const Feature &coreFeature)
    : StatefulDBusProxy(connection->dbusConnection(), connection->busName(),
            objectPath, coreFeature),
      OptionalInterfaceFactory<Channel>(this),
      mPriv(new Private(this, connection, immutableProperties))
{
}

/**
 * Class destructor.
 */
Channel::~Channel()
{
    delete mPriv;
}

/**
 * Return the connection owning this channel.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr Channel::connection() const
{
    return mPriv->connection;
}

/**
 * Return the immutable properties of the channel.
 *
 * If the channel is ready (isReady(Channel::FeatureCore) returns true), the following keys are
 * guaranteed to be present:
 * org.freedesktop.Telepathy.Channel.ChannelType,
 * org.freedesktop.Telepathy.Channel.TargetHandleType,
 * org.freedesktop.Telepathy.Channel.TargetHandle and
 * org.freedesktop.Telepathy.Channel.Requested.
 *
 * The keys and values in this map are defined by the \telepathy_spec,
 * or by third-party extensions to that specification.
 * These are the properties that cannot change over the lifetime of the
 * channel; they're announced in the result of the request, for efficiency.
 *
 * \return The immutable properties as QVariantMap.
 */
QVariantMap Channel::immutableProperties() const
{
    if (isReady(Channel::FeatureCore)) {
        QString key;

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, mPriv->channelType);
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".Interfaces");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, interfaces());
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, mPriv->targetHandleType);
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, mPriv->targetHandle);
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, mPriv->targetId);
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".Requested");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, mPriv->requested);
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle");
        if (!mPriv->immutableProperties.contains(key)) {
            mPriv->immutableProperties.insert(key, mPriv->initiatorHandle);
        }

        key = TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorID");
        if (!mPriv->immutableProperties.contains(key) && !mPriv->initiatorContact.isNull()) {
            mPriv->immutableProperties.insert(key, mPriv->initiatorContact->id());
        }
    }

    return mPriv->immutableProperties;
}

/**
 * Return the D-Bus interface name for the type of this channel.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return The D-Bus interface name for the type of the channel.
 */
QString Channel::channelType() const
{
    // Similarly, we don't want warnings triggered when using the type interface
    // proxies internally.
    if (!isReady(Channel::FeatureCore) && mPriv->channelType.isEmpty()) {
        warning() << "Channel::channelType() before the channel type has "
            "been received";
    }
    else if (!isValid()) {
        warning() << "Channel::channelType() used with channel closed";
    }

    return mPriv->channelType;
}

/**
 * Return the type of the handle returned by targetHandle() as specified in
 * #HandleType.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return The target handle type as #HandleType.
 * \sa targetHandle(), targetId()
 */
HandleType Channel::targetHandleType() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::targetHandleType() used channel not ready";
    }

    return (HandleType) mPriv->targetHandleType;
}

/**
 * Return the handle of the remote party with which this channel
 * communicates.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return An integer representing the target handle, which is of the type
 *         targetHandleType() indicates.
 * \sa targetHandleType(), targetId()
 */
uint Channel::targetHandle() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::targetHandle() used channel not ready";
    }

    return mPriv->targetHandle;
}

/**
 * Return the persistent unique ID of the remote party with which this channel communicates.
 *
 * If targetHandleType() is #HandleTypeContact, this will be the ID of the remote contact, and
 * similarly the unique ID of the room when targetHandleType() is #HandleTypeRoom.
 *
 * This is not necessarily the best identifier to display to the user, though. In particular, for
 * contacts, their alias should be displayed instead. It can be used for matching channels and UI
 * elements for them across reconnects, though, at which point the old channels and contacts are
 * invalidated.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return The target identifier.
 * \sa targetHandle(), targetContact()
 */
QString Channel::targetId() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::targetId() used, but the channel is not ready";
    }

    return mPriv->targetId;
}

/**
 * Return the contact with which this channel communicates for its lifetime, if applicable.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A pointer to the Contact object, or a null ContactPtr if targetHandleType() is not
 *         #HandleTypeContact.
 * \sa targetHandle(), targetId()
 */
ContactPtr Channel::targetContact() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::targetContact() used, but the channel is not ready";
    } else if (targetHandleType() != HandleTypeContact) {
        warning() << "Channel::targetContact() used with targetHandleType() != Contact";
    }

    return mPriv->targetContact;
}

/**
 * Return whether this channel was created in response to a
 * local request.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if the channel was created in response to a local request,
 *         \c false otherwise.
 */
bool Channel::isRequested() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::isRequested() used channel not ready";
    }

    return mPriv->requested;
}

/**
 * Return the contact who initiated this channel.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A pointer to the Contact object representing the contact who initiated the channel,
 *         or a null ContactPtr if it can't be retrieved.
 */
ContactPtr Channel::initiatorContact() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::initiatorContact() used channel not ready";
    }

    return mPriv->initiatorContact;
}

/**
 * Start an asynchronous request that this channel be closed.
 *
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa requestLeave()
 */
PendingOperation *Channel::requestClose()
{
    // Closing a channel does not make sense if it is already closed,
    // just silently Return.
    if (!isValid()) {
        return new PendingSuccess(ChannelPtr(this));
    }

    return new PendingVoid(mPriv->baseInterface->Close(), ChannelPtr(this));
}

Channel::PendingLeave::PendingLeave(const ChannelPtr &chan, const QString &message,
        ChannelGroupChangeReason reason)
    : PendingOperation(chan)
{
    Q_ASSERT(chan->mPriv->group != NULL);

    QDBusPendingCall call =
        chan->mPriv->group->RemoveMembersWithReason(
                UIntList() << chan->mPriv->groupSelfHandle,
                message,
                reason);

    connect(chan.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            this,
            SLOT(onChanInvalidated(Tp::DBusProxy*)));

    connect(new PendingVoid(call, chan),
            SIGNAL(finished(Tp::PendingOperation*)),
            this,
            SLOT(onRemoveFinished(Tp::PendingOperation*)));
}

void Channel::PendingLeave::onChanInvalidated(Tp::DBusProxy *proxy)
{
    if (isFinished()) {
        return;
    }

    debug() << "Finishing PendingLeave successfully as the channel was invalidated";

    setFinished();
}

void Channel::PendingLeave::onRemoveFinished(Tp::PendingOperation *op)
{
    if (isFinished()) {
        return;
    }

    ChannelPtr chan = ChannelPtr::staticCast(object());

    if (op->isValid()) {
        debug() << "We left the channel" << chan->objectPath();

        ContactPtr c = chan->groupSelfContact();

        if (chan->groupContacts().contains(c)
                || chan->groupLocalPendingContacts().contains(c)
                || chan->groupRemotePendingContacts().contains(c)) {
            debug() << "Waiting for self remove to be picked up";
            connect(chan.data(),
                    SIGNAL(groupMembersChanged(Tp::Contacts,Tp::Contacts,Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails)),
                    this,
                    SLOT(onMembersChanged(Tp::Contacts,Tp::Contacts,Tp::Contacts,Tp::Contacts)));
        } else {
            setFinished();
        }

        return;
    }

    debug() << "Leave RemoveMembersWithReason failed with " << op->errorName() << op->errorMessage()
        << "- falling back to Close";

    // If the channel has been closed or otherwise invalidated already in this mainloop iteration,
    // the requestClose() operation will early-succeed
    connect(chan->requestClose(),
            SIGNAL(finished(Tp::PendingOperation*)),
            this,
            SLOT(onCloseFinished(Tp::PendingOperation*)));
}

void Channel::PendingLeave::onMembersChanged(const Tp::Contacts &, const Tp::Contacts &,
        const Tp::Contacts &, const Tp::Contacts &removed)
{
    if (isFinished()) {
        return;
    }

    ChannelPtr chan = ChannelPtr::staticCast(object());
    ContactPtr c = chan->groupSelfContact();

    if (removed.contains(c)) {
        debug() << "Leave event picked up for" << chan->objectPath();
        setFinished();
    }
}

void Channel::PendingLeave::onCloseFinished(Tp::PendingOperation *op)
{
    if (isFinished()) {
        return;
    }

    ChannelPtr chan = ChannelPtr::staticCast(object());

    if (op->isError()) {
        warning() << "Closing the channel" << chan->objectPath()
            << "as a fallback for leaving it failed with"
            << op->errorName() << op->errorMessage() << "- so didn't leave";
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        debug() << "We left (by closing) the channel" << chan->objectPath();
        setFinished();
    }
}

/**
 * Start an asynchronous request to leave this channel as gracefully as possible.
 *
 * If leaving any more gracefully is not possible, this will revert to the same as requestClose().
 * In particular, this will be the case for channels with no group interface
 * (#TP_QT_IFACE_CHANNEL_INTERFACE_GROUP not in the list returned by interfaces()).
 *
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * A message and a reason may be provided along with the request, which will be sent to the server
 * if supported, which is indicated by #ChannelGroupFlagMessageDepart and/or
 * #ChannelGroupFlagMessageReject.
 *
 * Attempting to leave again when we have already left, either by our request or forcibly, will be a
 * no-op, with the returned PendingOperation immediately finishing successfully.
 *
 * \param message The message, which can be blank if desired.
 * \param reason A reason for leaving.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Channel::requestLeave(const QString &message, ChannelGroupChangeReason reason)
{
    // Leaving a channel does not make sense if it is already closed,
    // just silently Return.
    if (!isValid()) {
        return new PendingSuccess(ChannelPtr(this));
    }

    if (!isReady(Channel::FeatureCore)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel::FeatureCore must be ready to leave a channel"),
                ChannelPtr(this));
    }

    if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        return requestClose();
    }

    ContactPtr self = groupSelfContact();

    if (!groupContacts().contains(self) && !groupLocalPendingContacts().contains(self)
            && !groupRemotePendingContacts().contains(self)) {
        debug() << "Channel::requestLeave() called for " << objectPath() <<
            "which we aren't a member of";
        return new PendingSuccess(ChannelPtr(this));
    }

    return new PendingLeave(ChannelPtr(this), message, reason);
}

/**
 * \name Group interface
 *
 * Cached access to state of the group interface on the associated remote
 * object, if the interface is present.
 *
 * Some methods can be used when targetHandleType() == #HandleTypeContact, such
 * as groupFlags(), groupCanAddContacts(), groupCanRemoveContacts(),
 * groupSelfContact() and groupContacts().
 *
 * As the group interface state can change freely during the lifetime of the
 * channel due to events like new contacts joining the group, the cached state
 * is automatically kept in sync with the remote object's state by hooking
 * to the change notification signals present in the D-Bus interface.
 *
 * As the cached value changes, change notification signals are emitted.
 *
 * Signals such as groupMembersChanged(), groupSelfContactChanged(), etc., are emitted to
 * indicate that properties have changed.
 *
 * Check the individual signals' descriptions for details.
 */

//@{

/**
 * Return a set of flags indicating the capabilities and behaviour of the
 * group on this channel.
 *
 * Change notification is via the groupFlagsChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return The bitfield combination of flags as #ChannelGroupFlags.
 * \sa groupFlagsChanged()
 */
ChannelGroupFlags Channel::groupFlags() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupFlags() used channel not ready";
    }

    return (ChannelGroupFlags) mPriv->groupFlags;
}

/**
 * Return whether contacts can be added or invited to this channel.
 *
 * Change notification is via the groupCanAddContactsChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if contacts can be added or invited to the channel,
 *         \c false otherwise.
 * \sa groupFlags(), groupAddContacts()
 */
bool Channel::groupCanAddContacts() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanAddContacts() used channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagCanAdd;
}

/**
 * Return whether a message is expected when adding/inviting contacts, who
 * are not already members, to this channel.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if a message is expected, \c false otherwise.
 * \sa groupFlags(), groupAddContacts()
 */
bool Channel::groupCanAddContactsWithMessage() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanAddContactsWithMessage() used when channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagMessageAdd;
}

/**
 * Return whether a message is expected when accepting contacts' requests to
 * join this channel.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if a message is expected, \c false otherwise.
 * \sa groupFlags(), groupAddContacts()
 */
bool Channel::groupCanAcceptContactsWithMessage() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanAcceptContactsWithMessage() used when channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagMessageAccept;
}

/**
 * Add contacts to this channel.
 *
 * Contacts on the local pending list (those waiting for permission to join
 * the channel) can always be added. If groupCanAcceptContactsWithMessage()
 * returns \c true, an optional message is expected when doing this; if not,
 * the message parameter is likely to be ignored (so the user should not be
 * asked for a message, and the message parameter should be left empty).
 *
 * Other contacts can only be added if groupCanAddContacts() returns \c true.
 * If groupCanAddContactsWithMessage() returns \c true, an optional message is
 * expected when doing this, and if not, the message parameter is likely to be
 * ignored.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \param contacts Contacts to be added.
 * \param message A string message, which can be blank if desired.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa groupCanAddContacts(), groupCanAddContactsWithMessage(), groupCanAcceptContactsWithMessage()
 */
PendingOperation *Channel::groupAddContacts(const QList<ContactPtr> &contacts,
        const QString &message)
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupAddContacts() used channel not ready";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                ChannelPtr(this));
    } else if (contacts.isEmpty()) {
        warning() << "Channel::groupAddContacts() used with empty contacts param";
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("contacts cannot be an empty list"),
                ChannelPtr(this));
    }

    foreach (const ContactPtr &contact, contacts) {
        if (!contact) {
            warning() << "Channel::groupAddContacts() used but contacts param contains "
                "invalid contact";
            return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("Unable to add invalid contacts"),
                    ChannelPtr(this));
        }
    }

    if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupAddContacts() used with no group interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Channel does not support group interface"),
                ChannelPtr(this));
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }
    return new PendingVoid(mPriv->group->AddMembers(handles, message), ChannelPtr(this));
}

/**
 * Return whether contacts in groupRemotePendingContacts() can be removed from
 * this channel (i.e. whether an invitation can be rescinded).
 *
 * Change notification is via the groupCanRescindContactsChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if contacts can be removed, \c false otherwise.
 * \sa groupFlags(), groupRemoveContacts()
 */
bool Channel::groupCanRescindContacts() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanRescindContacts() used channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagCanRescind;
}

/**
 * Return whether a message is expected when removing contacts who are in
 * groupRemotePendingContacts() from this channel (i.e. rescinding an
 * invitation).
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if a message is expected, \c false otherwise.
 * \sa groupFlags(), groupRemoveContacts()
 */
bool Channel::groupCanRescindContactsWithMessage() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanRescindContactsWithMessage() used when channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagMessageRescind;
}

/**
 * Return if contacts in groupContacts() can be removed from this channel.
 *
 * Note that contacts in local pending lists, and the groupSelfContact(), can
 * always be removed from the channel.
 *
 * Change notification is via the groupCanRemoveContactsChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if contacts can be removed, \c false otherwise.
 * \sa groupFlags(), groupRemoveContacts()
 */
bool Channel::groupCanRemoveContacts() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanRemoveContacts() used channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagCanRemove;
}

/**
 * Return whether a message is expected when removing contacts who are in
 * groupContacts() from this channel.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if a message is expected, \c false otherwise.
 * \sa groupFlags(), groupRemoveContacts()
 */
bool Channel::groupCanRemoveContactsWithMessage() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanRemoveContactsWithMessage() used when channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagMessageRemove;
}

/**
 * Return whether a message is expected when removing contacts who are in
 * groupLocalPendingContacts() from this channel (i.e. rejecting a request to
 * join).
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if a message is expected, \c false otherwise.
 * \sa groupFlags(), groupRemoveContacts()
 */
bool Channel::groupCanRejectContactsWithMessage() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanRejectContactsWithMessage() used when channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagMessageReject;
}

/**
 * Return whether a message is expected when removing the groupSelfContact()
 * from this channel (i.e. departing from the channel).
 *
 * \return \c true if a message is expected, \c false otherwise.
 * \sa groupFlags(), groupRemoveContacts()
 */
bool Channel::groupCanDepartWithMessage() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupCanDepartWithMessage() used when channel not ready";
    }

    return mPriv->groupFlags & ChannelGroupFlagMessageDepart;
}

/**
 * Remove contacts from this channel.
 *
 * Contacts on the local pending list (those waiting for permission to join
 * the channel) can always be removed. If groupCanRejectContactsWithMessage()
 * returns \c true, an optional message is expected when doing this; if not,
 * the message parameter is likely to be ignored (so the user should not be
 * asked for a message, and the message parameter should be left empty).
 *
 * The groupSelfContact() can also always be removed, as a way to leave the
 * group with an optional departure message and/or departure reason indication.
 * If groupCanDepartWithMessage() returns \c true, an optional message is
 * expected when doing this, and if not, the message parameter is likely to
 * be ignored.
 *
 * Contacts in the group can only be removed (e.g. kicked) if
 * groupCanRemoveContacts() returns \c true. If
 * groupCanRemoveContactsWithMessage() returns \c true, an optional message is
 * expected when doing this, and if not, the message parameter is likely to be
 * ignored.
 *
 * Contacts in the remote pending list (those who have been invited to the
 * channel) can only be removed (have their invitations rescinded) if
 * groupCanRescindContacts() returns \c true. If
 * groupCanRescindContactsWithMessage() returns \c true, an optional message is
 * expected when doing this, and if not, the message parameter is likely to be
 * ignored.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \param contacts Contacts to be removed.
 * \param message A string message, which can be blank if desired.
 * \param reason Reason of the change, as specified in
 *               #ChannelGroupChangeReason
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa groupCanRemoveContacts(), groupCanRemoveContactsWithMessage(),
 *     groupCanRejectContactsWithMessage(), groupCanRescindContacts(),
 *     groupCanRescindContacts(), groupCanRescindContactsWithMessage(),
 *     groupCanDepartWithMessage()
 */
PendingOperation *Channel::groupRemoveContacts(const QList<ContactPtr> &contacts,
        const QString &message, ChannelGroupChangeReason reason)
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupRemoveContacts() used channel not ready";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                ChannelPtr(this));
    }

    if (contacts.isEmpty()) {
        warning() << "Channel::groupRemoveContacts() used with empty contacts param";
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("contacts param cannot be an empty list"),
                ChannelPtr(this));
    }

    foreach (const ContactPtr &contact, contacts) {
        if (!contact) {
            warning() << "Channel::groupRemoveContacts() used but contacts param contains "
                "invalid contact:";
            return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("Unable to remove invalid contacts"),
                    ChannelPtr(this));
        }
    }

    if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupRemoveContacts() used with no group interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Channel does not support group interface"),
                ChannelPtr(this));
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }
    return new PendingVoid(
            mPriv->group->RemoveMembersWithReason(handles, message, reason),
            ChannelPtr(this));
}

/**
 * Return the current contacts of the group.
 *
 * It is possible to omit the contact representing the local user, even if
 * the contact is in the set, by passing \c false as the parameter \a
 * includeSelfContact.
 *
 * Change notification is via the groupMembersChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \param includeSelfContact Whether to include the self contact in the returned set.
 * \return A set of pointers to the Contact objects.
 * \sa groupLocalPendingContacts(), groupRemotePendingContacts()
 */
Contacts Channel::groupContacts(bool includeSelfContact) const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupMembers() used channel not ready";
    }

    Contacts ret = mPriv->groupContacts.values().toSet();
    if (!includeSelfContact) {
        ret.remove(groupSelfContact());
    }
    return ret;
}

/**
 * Return the contacts currently waiting for local approval to join the
 * group.
 *
 * It is possible to omit the contact representing the local user, even if
 * the contact is in the set, by passing \c false as the parameter \a
 * includeSelfContact.
 *
 * Change notification is via the groupMembersChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \param includeSelfContact Whether to include the self contact in the returned set.
 * \return A set of pointers to the Contact objects.
 * \sa groupContacts(), groupRemotePendingContacts()
 */
Contacts Channel::groupLocalPendingContacts(bool includeSelfContact) const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupLocalPendingContacts() used channel not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupLocalPendingContacts() used with no group interface";
    }

    Contacts ret = mPriv->groupLocalPendingContacts.values().toSet();
    if (!includeSelfContact) {
        ret.remove(groupSelfContact());
    }
    return ret;
}

/**
 * Return the contacts currently waiting for remote approval to join the
 * group.
 *
 * It is possible to omit the contact representing the local user, even if
 * the contact is in the set, by passing \c false as the parameter \a
 * includeSelfContact.
 *
 * Change notification is via the groupMembersChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \param includeSelfContact Whether to include the self contact in the returned set.
 * \return A set of pointers to the Contact objects.
 * \sa groupContacts(), groupLocalPendingContacts()
 */
Contacts Channel::groupRemotePendingContacts(bool includeSelfContact) const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupRemotePendingContacts() used channel not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupRemotePendingContacts() used with no "
            "group interface";
    }

    Contacts ret = mPriv->groupRemotePendingContacts.values().toSet();
    if (!includeSelfContact) {
        ret.remove(groupSelfContact());
    }
    return ret;
}

/**
 * Return information of a local pending contact change. If
 * no information is available, an object for which
 * GroupMemberChangeDetails::isValid() returns <code>false</code> is returned.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \param contact A Contact object that is on the local pending contacts list.
 * \return The change info as a GroupMemberChangeDetails object.
 */
Channel::GroupMemberChangeDetails Channel::groupLocalPendingContactChangeInfo(
        const ContactPtr &contact) const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupLocalPendingContactChangeInfo() used channel not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupLocalPendingContactChangeInfo() used with no group interface";
    } else if (!contact) {
        warning() << "Channel::groupLocalPendingContactChangeInfo() used with null contact param";
        return GroupMemberChangeDetails();
    }

    uint handle = contact->handle()[0];
    return mPriv->groupLocalPendingContactsChangeInfo.value(handle);
}

/**
 * Return information on the removal of the local user from the group. If
 * the user hasn't been removed from the group, an object for which
 * GroupMemberChangeDetails::isValid() returns <code>false</code> is returned.
 *
 * This method should be called only after you've left the channel.
 * This is useful for getting the remove information after missing the
 * corresponding groupMembersChanged() signal, as the local user being
 * removed usually causes the channel to be closed.
 *
 * The returned information is not guaranteed to be correct if
 * groupIsSelfHandleTracked() returns false and a self handle change has
 * occurred on the remote object.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return The remove info as a GroupMemberChangeDetails object.
 */
Channel::GroupMemberChangeDetails Channel::groupSelfContactRemoveInfo() const
{
    // Oftentimes, the channel will be closed as a result from being left - so checking a channel's
    // self remove info when it has been closed and hence invalidated is valid
    if (isValid() && !isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupSelfContactRemoveInfo() used before Channel::FeatureCore is ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
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
 *  <li>The list returned by interfaces() contains #TP_QT_IFACE_CHANNEL_INTERFACE_GROUP</li>
 *  <li>The set of flags returned by groupFlags() contains
 *      #GroupFlagProperties and #GroupFlagChannelSpecificHandles</li>
 * </ul>
 *
 * If this function returns \c false, the return value of
 * groupHandleOwners() is undefined and groupHandleOwnersChanged() will
 * never be emitted.
 *
 * The value returned by this function will stay fixed for the entire time
 * the object is ready, so no change notification is provided.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if handle owner lookup functionality is available, \c false otherwise.
 */
bool Channel::groupAreHandleOwnersAvailable() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupAreHandleOwnersAvailable() used channel not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
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
 * Change notification is via the groupHandleOwnersChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A mapping from group-specific handles to globally valid handles.
 */
HandleOwnerMap Channel::groupHandleOwners() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupHandleOwners() used channel not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
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
 * accurately represent the local user even after nickname changes, etc.
 *
 * This should always be \c true for new services implementing the group interface.
 *
 * Older services not providing group properties don't necessarily
 * emit the SelfHandleChanged signal either, so self contact changes can't be
 * reliably tracked.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if changes to the self contact are tracked, \c false otherwise.
 */
bool Channel::groupIsSelfContactTracked() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupIsSelfHandleTracked() used channel not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
        warning() << "Channel::groupIsSelfHandleTracked() used with "
            "no group interface";
    }

    return mPriv->groupIsSelfHandleTracked;
}

/**
 * Return a Contact object representing the user in the group if at all possible, otherwise a
 * Contact object representing the user globally.
 *
 * Change notification is via the groupSelfContactChanged() signal.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A pointer to the Contact object.
 */
ContactPtr Channel::groupSelfContact() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupSelfContact() used channel not ready";
    }

    return mPriv->groupSelfContact;
}

/**
 * Return whether the local user is in the "local pending" state. This
 * indicates that the local user needs to take action to accept an invitation,
 * an incoming call, etc.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if local user is in the channel's local-pending set, \c false otherwise.
 */
bool Channel::groupSelfHandleIsLocalPending() const
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupSelfHandleIsLocalPending() used when "
            "channel not ready";
        return false;
    }

    return mPriv->groupLocalPendingContacts.contains(mPriv->groupSelfHandle);
}

/**
 * Attempt to add the local user to this channel. In some channel types,
 * such as Text and StreamedMedia, this is used to accept an invitation or an
 * incoming call.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Channel::groupAddSelfHandle()
{
    if (!isReady(Channel::FeatureCore)) {
        warning() << "Channel::groupAddSelfHandle() used when channel not "
            "ready";
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Channel object not ready"),
                ChannelPtr(this));
    }

    UIntList handles;

    if (mPriv->groupSelfHandle == 0) {
        handles << mPriv->connection->selfHandle();
    } else {
        handles << mPriv->groupSelfHandle;
    }

    return new PendingVoid(
            mPriv->group->AddMembers(handles, QLatin1String("")),
            ChannelPtr(this));
}

//@}

/**
 * Return whether this channel implements the conference interface
 * (#TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE is in the list returned by interfaces()).
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if the conference interface is supported, \c false otherwise.
 */
bool Channel::isConference() const
{
    return hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE);
}

/**
 * Return a list of contacts invited to this conference when it was created.
 *
 * This method requires Channel::FeatureConferenceInitialInviteeContacts to be ready.
 *
 * \return A set of pointers to the Contact objects.
 */
Contacts Channel::conferenceInitialInviteeContacts() const
{
    return mPriv->conferenceInitialInviteeContacts;
}

/**
 * Return the individual channels that are part of this conference.
 *
 * Change notification is via the conferenceChannelMerged() and
 * conferenceChannelRemoved() signals.
 *
 * Note that the returned channels are not guaranteed to be ready. Calling
 * Channel::becomeReady() may be needed.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A list of pointers to Channel objects containing all channels in the conference.
 * \sa conferenceInitialChannels(), conferenceOriginalChannels()
 */
QList<ChannelPtr> Channel::conferenceChannels() const
{
    return mPriv->conferenceChannels.values();
}

/**
 * Return the initial value of conferenceChannels().
 *
 * Note that the returned channels are not guaranteed to be ready. Calling
 * Channel::becomeReady() may be needed.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A list of pointers to Channel objects containing all channels that were initially
 *         part of the conference.
 * \sa conferenceChannels(), conferenceOriginalChannels()
 */
QList<ChannelPtr> Channel::conferenceInitialChannels() const
{
    return mPriv->conferenceInitialChannels.values();
}

/**
 * Return a map between channel specific handles and the corresponding channels of this conference.
 *
 * This method is only relevant on GSM conference calls where it is possible to have the same phone
 * number in a conference twice; for instance, it could be the number of a corporate switchboard.
 * This is represented using channel-specific handles; whether or not a channel uses
 * channel-specific handles is reported in groupFlags(). The groupHandleOwners() specifies the
 * mapping from opaque channel-specific handles to actual numbers; this property specifies the
 * original 1-1 channel corresponding to each channel-specific handle in the conference.
 *
 * In protocols where this situation cannot arise, such as XMPP, this method will return an empty
 * hash.
 *
 * Example, consider this situation:
 * 1. Place a call (with path /call/to/simon) to the contact +441234567890 (which is assigned the
 *    handle h, say), and ask to be put through to Simon McVittie;
 * 2. Put that call on hold;
 * 3. Place another call (with path /call/to/jonny) to +441234567890, and ask to be put through to
 *    Jonny Lamb;
 * 4. Request a new conference channel with initial channels: ['/call/to/simon', '/call/to/jonny'].
 *
 * The new channel will have the following properties, for some handles s and j:
 *
 * {
 * groupFlags(): ChannelGroupFlagChannelSpecificHandles | (other flags),
 * groupMembers(): [self handle, s, j],
 * groupHandleOwners(): { s: h, j: h },
 * conferenceInitialChannels(): ['/call/to/simon', '/call/to/jonny'],
 * conferenceChannels(): ['/call/to/simon', '/call/to/jonny'],
 * conferenceOriginalChannels(): { s: '/call/to/simon', j: '/call/to/jonny' },
 * # ...
 * }
 *
 * Note that the returned channels are not guaranteed to be ready. Calling
 * Channel::becomeReady() may be needed.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A map of channel specific handles to pointers to Channel objects.
 * \sa conferenceChannels(), conferenceInitialChannels()
 */
QHash<uint, ChannelPtr> Channel::conferenceOriginalChannels() const
{
    return mPriv->conferenceOriginalChannels;
}

/**
 * Return whether this channel supports conference merging using conferenceMergeChannel().
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if the interface is supported, \c false otherwise.
 * \sa conferenceMergeChannel()
 */
bool Channel::supportsConferenceMerging() const
{
    return interfaces().contains(TP_QT_FUTURE_IFACE_CHANNEL_INTERFACE_MERGEABLE_CONFERENCE);
}

/**
 * Request that the given channel be incorporated into this channel.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa supportsConferenceMerging()
 */
PendingOperation *Channel::conferenceMergeChannel(const ChannelPtr &channel)
{
    if (!supportsConferenceMerging()) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Channel does not support MergeableConference interface"),
                ChannelPtr(this));
    }

    return new PendingVoid(mPriv->mergeableConferenceInterface()->Merge(
                QDBusObjectPath(channel->objectPath())),
                ChannelPtr(this));
}

/**
 * Return whether this channel supports splitting using conferenceSplitChannel().
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if the interface is supported, \c false otherwise.
 * \sa conferenceSplitChannel()
 */
bool Channel::supportsConferenceSplitting() const
{
    return interfaces().contains(TP_QT_FUTURE_IFACE_CHANNEL_INTERFACE_SPLITTABLE);
}

/**
 * Request that this channel is removed from any conference of which it is
 * a part.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa supportsConferenceSplitting()
 */
PendingOperation *Channel::conferenceSplitChannel()
{
    if (!supportsConferenceSplitting()) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Channel does not support Splittable interface"),
                ChannelPtr(this));
    }

    return new PendingVoid(mPriv->splittableInterface()->Split(), ChannelPtr(this));
}

/**
 * Return the Client::ChannelInterface interface proxy object for this channel.
 * This method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing Client::ChannelInterface object for this
 *         Channel object.
 */
Client::ChannelInterface *Channel::baseInterface() const
{
    return mPriv->baseInterface;
}

void Channel::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(Channel)";
        props = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Channel) failed with " <<
            reply.error().name() << ": " << reply.error().message();
    }

    mPriv->extractMainProps(props);

    mPriv->continueIntrospection();
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
    mPriv->continueIntrospection();
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
    mPriv->continueIntrospection();
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
    setInterfaces(reply.value());
    mPriv->readinessHelper->setInterfaces(interfaces());
    mPriv->nowHaveInterfaces();

    mPriv->fakeGroupInterfaceIfNeeded();

    mPriv->continueIntrospection();
}

void Channel::onClosed()
{
    debug() << "Got Channel::Closed";

    QString error;
    QString message;
    if (mPriv->groupSelfContactRemoveInfo.isValid() &&
        mPriv->groupSelfContactRemoveInfo.hasReason()) {
        error = mPriv->groupMemberChangeDetailsTelepathyError(
                mPriv->groupSelfContactRemoveInfo);
        message = mPriv->groupSelfContactRemoveInfo.message();
    } else {
        error = TP_QT_ERROR_CANCELLED;
        message = QLatin1String("channel closed");
    }

    invalidate(error, message);
}

void Channel::onConnectionReady(PendingOperation *op)
{
    if (op->isError()) {
        invalidate(op->errorName(), op->errorMessage());
        return;
    }

    // FIXME: should connect to selfHandleChanged and act accordingly, but that is a PITA for
    // keeping the Contacts built and even if we don't do it, the new code is better than the
    // old one anyway because earlier on we just wouldn't have had a self contact.
    //
    // besides, the only thing which breaks without connecting in the world likely is if you're
    // using idle and decide to change your nick, which I don't think we necessarily even have API
    // to do from tp-qt anyway (or did I make idle change the nick when setting your alias? can't
    // remember)
    //
    // Simply put, I just don't care ATM.

    // Will be overwritten by the group self handle, if we can discover any.
    Q_ASSERT(!mPriv->groupSelfHandle);
    mPriv->groupSelfHandle = mPriv->connection->selfHandle();

    mPriv->introspectMainProperties();
}

void Channel::onConnectionInvalidated()
{
    debug() << "Owning connection died leaving an orphan Channel, "
        "changing to closed";
    invalidate(TP_QT_ERROR_ORPHANED,
               QLatin1String("Connection given as the owner of this channel was invalidated"));
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

    mPriv->continueIntrospection();
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
        mPriv->setGroupFlags(reply.value());

        if (mPriv->groupFlags & ChannelGroupFlagProperties) {
            warning() << " Reply included ChannelGroupFlagProperties, even "
                "though properties specified in 0.17.7 didn't work! - unsetting";
            mPriv->groupFlags &= ~ChannelGroupFlagProperties;
        }
    }

    mPriv->continueIntrospection();
}

void Channel::gotAllMembers(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UIntList, UIntList, UIntList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetAllMembers() failed with " <<
            reply.error().name() << ": " << reply.error().message();
    } else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetAllMembers()";

        mPriv->groupInitialMembers = reply.argumentAt<0>();
        mPriv->groupInitialRP = reply.argumentAt<2>();

        foreach (uint handle, reply.argumentAt<1>()) {
            LocalPendingInfo info = {handle, 0, ChannelGroupChangeReasonNone,
                QLatin1String("")};
            mPriv->groupInitialLP.push_back(info);
        }
    }

    mPriv->continueIntrospection();
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
        // Overrides the previous vague list provided by gotAllMembers
        mPriv->groupInitialLP = reply.value();
    }

    mPriv->continueIntrospection();
}

void Channel::gotSelfHandle(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel.Interface.Group::GetSelfHandle() failed with " <<
            reply.error().name() << ": " << reply.error().message();
    } else {
        debug() << "Got reply to fallback Channel.Interface.Group::GetSelfHandle()";
        // Don't overwrite the self handle we got from the connection with 0
        if (reply.value()) {
            mPriv->groupSelfHandle = reply.value();
        }
    }

    mPriv->nowHaveInitialMembers();

    mPriv->continueIntrospection();
}

void Channel::gotContacts(PendingOperation *op)
{
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);

    mPriv->buildingContacts = false;

    QList<ContactPtr> contacts;
    if (pending->isValid()) {
        contacts = pending->contacts();

        if (!pending->invalidHandles().isEmpty()) {
            warning() << "Unable to construct Contact objects for handles:" <<
                pending->invalidHandles();

            if (mPriv->groupSelfHandle &&
                pending->invalidHandles().contains(mPriv->groupSelfHandle)) {
                warning() << "Unable to retrieve self contact";
                mPriv->groupSelfContact.reset();
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

    uint groupFlags = mPriv->groupFlags;
    groupFlags |= added;
    groupFlags &= ~removed;
    // just emit groupFlagsChanged and related signals if the flags really
    // changed and we are ready
    if (mPriv->setGroupFlags(groupFlags) && isReady(Channel::FeatureCore)) {
        debug() << "Emitting groupFlagsChanged with" << mPriv->groupFlags <<
            "value" << added << "added" << removed << "removed";
        emit groupFlagsChanged((ChannelGroupFlags) mPriv->groupFlags,
                (ChannelGroupFlags) added, (ChannelGroupFlags) removed);

        if (added & ChannelGroupFlagCanAdd ||
            removed & ChannelGroupFlagCanAdd) {
            debug() << "Emitting groupCanAddContactsChanged";
            emit groupCanAddContactsChanged(groupCanAddContacts());
        }

        if (added & ChannelGroupFlagCanRemove ||
            removed & ChannelGroupFlagCanRemove) {
            debug() << "Emitting groupCanRemoveContactsChanged";
            emit groupCanRemoveContactsChanged(groupCanRemoveContacts());
        }

        if (added & ChannelGroupFlagCanRescind ||
            removed & ChannelGroupFlagCanRescind) {
            debug() << "Emitting groupCanRescindContactsChanged";
            emit groupCanRescindContactsChanged(groupCanRescindContacts());
        }
    }
}

void Channel::onMembersChanged(const QString &message,
        const UIntList &added, const UIntList &removed,
        const UIntList &localPending, const UIntList &remotePending,
        uint actor, uint reason)
{
    // Ignore the signal if we're using the MCD signal to not duplicate events
    if (mPriv->usingMembersChangedDetailed) {
        return;
    }

    debug() << "Got Channel.Interface.Group::MembersChanged with" << added.size() <<
        "added," << removed.size() << "removed," << localPending.size() <<
        "moved to LP," << remotePending.size() << "moved to RP," << actor <<
        "being the actor," << reason << "the reason and" << message << "the message";
    debug() << " synthesizing a corresponding MembersChangedDetailed signal";

    QVariantMap details;

    if (!message.isEmpty()) {
        details.insert(QLatin1String("message"), message);
    }

    if (actor != 0) {
        details.insert(QLatin1String("actor"), actor);
    }

    details.insert(QLatin1String("change-reason"), reason);

    mPriv->doMembersChangedDetailed(added, removed, localPending, remotePending, details);
}

void Channel::onMembersChangedDetailed(
        const UIntList &added, const UIntList &removed,
        const UIntList &localPending, const UIntList &remotePending,
        const QVariantMap &details)
{
    // Ignore the signal if we aren't (yet) using MCD to not duplicate events
    if (!mPriv->usingMembersChangedDetailed) {
        return;
    }

    debug() << "Got Channel.Interface.Group::MembersChangedDetailed with" << added.size() <<
        "added," << removed.size() << "removed," << localPending.size() <<
        "moved to LP," << remotePending.size() << "moved to RP and with" << details.size() <<
        "details";

    mPriv->doMembersChangedDetailed(added, removed, localPending, remotePending, details);
}

void Channel::Private::doMembersChangedDetailed(
        const UIntList &added, const UIntList &removed,
        const UIntList &localPending, const UIntList &remotePending,
        const QVariantMap &details)
{
    if (!groupHaveMembers) {
        debug() << "Still waiting for initial group members, "
            "so ignoring delta signal...";
        return;
    }

    if (added.isEmpty() && removed.isEmpty() &&
        localPending.isEmpty() && remotePending.isEmpty()) {
        debug() << "Nothing really changed, so skipping membersChanged";
        return;
    }

    // let's store groupSelfContactRemoveInfo here as we may not have time
    // to build the contacts in case self contact is removed,
    // as Closed will be emitted right after
    if (removed.contains(groupSelfHandle)) {
        if (qdbus_cast<uint>(details.value(QLatin1String("change-reason"))) ==
                ChannelGroupChangeReasonRenamed) {
            if (removed.size() != 1 ||
                (added.size() + localPending.size() + remotePending.size()) != 1) {
                // spec-incompliant CM, ignoring members changed
                warning() << "Received MembersChangedDetailed with reason "
                    "Renamed and removed.size != 1 or added.size + "
                    "localPending.size + remotePending.size != 1. Ignoring";
                return;
            }
            uint newHandle = 0;
            if (!added.isEmpty()) {
                newHandle = added.first();
            } else if (!localPending.isEmpty()) {
                newHandle = localPending.first();
            } else if (!remotePending.isEmpty()) {
                newHandle = remotePending.first();
            }
            parent->onSelfHandleChanged(newHandle);
            return;
        }

        // let's try to get the actor contact from contact manager if available
        groupSelfContactRemoveInfo = GroupMemberChangeDetails(
                connection->contactManager()->lookupContactByHandle(
                    qdbus_cast<uint>(details.value(QLatin1String("actor")))),
                details);
    }

    HandleIdentifierMap contactIds = qdbus_cast<HandleIdentifierMap>(
            details.value(GroupMembersChangedInfo::keyContactIds));
    connection->lowlevel()->injectContactIds(contactIds);

    groupMembersChangedQueue.enqueue(
            new Private::GroupMembersChangedInfo(
                added, removed,
                localPending, remotePending,
                details));

    if (!buildingContacts) {
        // if we are building contacts, we should wait it to finish so we don't
        // present the user with wrong information
        processMembersChanged();
    }
}

void Channel::onHandleOwnersChanged(const HandleOwnerMap &added,
        const UIntList &removed)
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

    // just emit groupHandleOwnersChanged if it really changed and
    // we are ready
    if ((emitAdded.size() || emitRemoved.size()) && isReady(Channel::FeatureCore)) {
        debug() << "Emitting groupHandleOwnersChanged with" << emitAdded.size() <<
            "added" << emitRemoved.size() << "removed";
        emit groupHandleOwnersChanged(mPriv->groupHandleOwners,
                emitAdded, emitRemoved);
    }
}

void Channel::onSelfHandleChanged(uint selfHandle)
{
    debug().nospace() << "Got Channel.Interface.Group::SelfHandleChanged";

    if (selfHandle != mPriv->groupSelfHandle) {
        mPriv->groupSelfHandle = selfHandle;
        debug() << " Emitting groupSelfHandleChanged with new self handle" <<
            selfHandle;

        // FIXME: fix self contact building with no group
        mPriv->pendingRetrieveGroupSelfContact = true;
    }
}

void Channel::gotConferenceProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    mPriv->introspectingConference = false;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(Channel.Interface.Conference)";
        props = reply.value();

        ConnectionPtr conn = connection();
        ChannelFactoryConstPtr chanFactory = conn->channelFactory();

        ObjectPathList channels =
            qdbus_cast<ObjectPathList>(props[QLatin1String("Channels")]);
        foreach (const QDBusObjectPath &channelPath, channels) {
            if (mPriv->conferenceChannels.contains(channelPath.path())) {
                continue;
            }

            PendingReady *readyOp = chanFactory->proxy(conn,
                    channelPath.path(), QVariantMap());
            ChannelPtr channel(ChannelPtr::qObjectCast(readyOp->proxy()));
            Q_ASSERT(!channel.isNull());

            mPriv->conferenceChannels.insert(channelPath.path(), channel);
        }

        ObjectPathList initialChannels =
            qdbus_cast<ObjectPathList>(props[QLatin1String("InitialChannels")]);
        foreach (const QDBusObjectPath &channelPath, initialChannels) {
            if (mPriv->conferenceInitialChannels.contains(channelPath.path())) {
                continue;
            }

            PendingReady *readyOp = chanFactory->proxy(conn,
                    channelPath.path(), QVariantMap());
            ChannelPtr channel(ChannelPtr::qObjectCast(readyOp->proxy()));
            Q_ASSERT(!channel.isNull());

            mPriv->conferenceInitialChannels.insert(channelPath.path(), channel);
        }

        mPriv->conferenceInitialInviteeHandles =
            qdbus_cast<UIntList>(props[QLatin1String("InitialInviteeHandles")]);
        QStringList conferenceInitialInviteeIds =
            qdbus_cast<QStringList>(props[QLatin1String("InitialInviteeIDs")]);
        if (mPriv->conferenceInitialInviteeHandles.size() == conferenceInitialInviteeIds.size()) {
            HandleIdentifierMap contactIds;
            int i = 0;
            foreach (uint handle, mPriv->conferenceInitialInviteeHandles) {
                contactIds.insert(handle, conferenceInitialInviteeIds.at(i++));
            }
            mPriv->connection->lowlevel()->injectContactIds(contactIds);
        }

        mPriv->conferenceInvitationMessage =
            qdbus_cast<QString>(props[QLatin1String("InvitationMessage")]);

        ChannelOriginatorMap originalChannels = qdbus_cast<ChannelOriginatorMap>(
                props[QLatin1String("OriginalChannels")]);
        for (ChannelOriginatorMap::const_iterator i = originalChannels.constBegin();
                i != originalChannels.constEnd(); ++i) {
            PendingReady *readyOp = chanFactory->proxy(conn,
                    i.value().path(), QVariantMap());
            ChannelPtr channel(ChannelPtr::qObjectCast(readyOp->proxy()));
            Q_ASSERT(!channel.isNull());

            mPriv->conferenceOriginalChannels.insert(i.key(), channel);
        }
    } else {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Conference) "
            "failed with " << reply.error().name() << ": " <<
            reply.error().message();
    }

    mPriv->continueIntrospection();
}

void Channel::gotConferenceInitialInviteeContacts(PendingOperation *op)
{
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);

    if (pending->isValid()) {
        mPriv->conferenceInitialInviteeContacts = pending->contacts().toSet();
    } else {
        warning().nospace() << "Getting conference initial invitee contacts "
            "failed with " << pending->errorName() << ":" <<
            pending->errorMessage();
    }

    mPriv->readinessHelper->setIntrospectCompleted(
            FeatureConferenceInitialInviteeContacts, true);
}

void Channel::onConferenceChannelMerged(const QDBusObjectPath &channelPath,
        uint channelSpecificHandle, const QVariantMap &properties)
{
    if (mPriv->conferenceChannels.contains(channelPath.path())) {
        return;
    }

    ConnectionPtr conn = connection();
    ChannelFactoryConstPtr chanFactory = conn->channelFactory();
    PendingReady *readyOp = chanFactory->proxy(conn,
            channelPath.path(), properties);
    ChannelPtr channel(ChannelPtr::qObjectCast(readyOp->proxy()));
    Q_ASSERT(!channel.isNull());

    mPriv->conferenceChannels.insert(channelPath.path(), channel);
    emit conferenceChannelMerged(channel);

    if (channelSpecificHandle != 0) {
        mPriv->conferenceOriginalChannels.insert(channelSpecificHandle, channel);
    }
}

void Channel::onConferenceChannelMerged(const QDBusObjectPath &channelPath)
{
    onConferenceChannelMerged(channelPath, 0, QVariantMap());
}

void Channel::onConferenceChannelRemoved(const QDBusObjectPath &channelPath,
        const QVariantMap &details)
{
    if (!mPriv->conferenceChannels.contains(channelPath.path())) {
        return;
    }

    HandleIdentifierMap contactIds = qdbus_cast<HandleIdentifierMap>(
            details.value(Private::GroupMembersChangedInfo::keyContactIds));
    mPriv->connection->lowlevel()->injectContactIds(contactIds);

    mPriv->conferenceChannelRemovedQueue.enqueue(
            new Private::ConferenceChannelRemovedInfo(channelPath, details));
    mPriv->processConferenceChannelRemoved();
}

void Channel::onConferenceChannelRemoved(const QDBusObjectPath &channelPath)
{
    onConferenceChannelRemoved(channelPath, QVariantMap());
}

void Channel::gotConferenceChannelRemovedActorContact(PendingOperation *op)
{
    ContactPtr actorContact;

    if (op) {
        PendingContacts *pc = qobject_cast<PendingContacts *>(op);

        if (pc->isValid()) {
            Q_ASSERT(pc->contacts().size() == 1);
            actorContact = pc->contacts().first();
        } else {
            warning().nospace() << "Getting conference channel removed actor "
                "failed with " << pc->errorName() << ":" <<
                pc->errorMessage();
        }
    }

    Private::ConferenceChannelRemovedInfo *info = mPriv->conferenceChannelRemovedQueue.dequeue();

    ChannelPtr channel = mPriv->conferenceChannels[info->channelPath.path()];
    mPriv->conferenceChannels.remove(info->channelPath.path());
    emit conferenceChannelRemoved(channel, GroupMemberChangeDetails(actorContact,
                info->details));

    for (QHash<uint, ChannelPtr>::iterator i = mPriv->conferenceOriginalChannels.begin();
            i != mPriv->conferenceOriginalChannels.end();) {
        if (i.value() == channel) {
            i = mPriv->conferenceOriginalChannels.erase(i);
        } else {
            ++i;
        }
    }

    delete info;

    mPriv->buildingConferenceChannelRemovedActorContact = false;
    mPriv->processConferenceChannelRemoved();
}

/**
 * \fn void Channel::groupFlagsChanged(uint flags, uint added, uint removed)
 *
 * Emitted when the value of groupFlags() changes.
 *
 * \param flags The value which would now be returned by groupFlags().
 * \param added Flags added compared to the previous value.
 * \param removed Flags removed compared to the previous value.
 */

/**
 * \fn void Channel::groupCanAddContactsChanged(bool canAddContacts)
 *
 * Emitted when the value of groupCanAddContacts() changes.
 *
 * \param canAddContacts Whether a contact can be added to this channel.
 * \sa groupCanAddContacts()
 */

/**
 * \fn void Channel::groupCanRemoveContactsChanged(bool canRemoveContacts)
 *
 * Emitted when the value of groupCanRemoveContacts() changes.
 *
 * \param canRemoveContacts Whether a contact can be removed from this channel.
 * \sa groupCanRemoveContacts()
 */

/**
 * \fn void Channel::groupCanRescindContactsChanged(bool canRescindContacts)
 *
 * Emitted when the value of groupCanRescindContacts() changes.
 *
 * \param canRescindContacts Whether contact invitations can be rescinded.
 * \sa groupCanRescindContacts()
 */

/**
 * \fn void Channel::groupMembersChanged(
 *     const Tp::Contacts &groupMembersAdded,
 *     const Tp::Contacts &groupLocalPendingMembersAdded,
 *     const Tp::Contacts &groupRemotePendingMembersAdded,
 *     const Tp::Contacts &groupMembersRemoved,
 *     const Channel::GroupMemberChangeDetails &details)
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
 * \param details Additional details such as the contact requesting or causing
 *                the change.
 */

/**
 * \fn void Channel::groupHandleOwnersChanged(const HandleOwnerMap &owners,
 *            const Tp::UIntList &added, const Tp::UIntList &removed)
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
 * \fn void Channel::groupSelfContactChanged()
 *
 * Emitted when the value returned by groupSelfContact() changes.
 */

/**
 * \fn void Channel::conferenceChannelMerged(const Tp::ChannelPtr &channel)
 *
 * Emitted when a new channel is added to the value of conferenceChannels().
 *
 * \param channel The channel that was added to conferenceChannels().
 */

/**
 * \fn void Channel::conferenceChannelRemoved(const Tp::ChannelPtr &channel,
 *          const Tp::Channel::GroupMemberChangeDetails &details)
 *
 * Emitted when a new channel is removed from the value of conferenceChannels().
 *
 * \param channel The channel that was removed from conferenceChannels().
 * \param details The change details.
 */

} // Tp
