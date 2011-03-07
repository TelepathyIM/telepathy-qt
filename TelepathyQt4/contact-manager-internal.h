/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt4_contact_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt4_contact_manager_internal_h_HEADER_GUARD_

#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Global>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Types>

#include <QList>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT ContactManager::Roster : public QObject
{
    Q_OBJECT

public:
    Roster(ContactManager *manager);
    virtual ~Roster();

    ContactListState state() const;

    PendingOperation *introspect();
    PendingOperation *introspectGroups();
    void reset();

    Contacts allKnownContacts() const;
    QStringList allKnownGroups() const;

    PendingOperation *addGroup(const QString &group);
    PendingOperation *removeGroup(const QString &group);

    Contacts groupContacts(const QString &group) const;
    PendingOperation *addContactsToGroup(const QString &group,
            const QList<ContactPtr> &contacts);
    PendingOperation *removeContactsFromGroup(const QString &group,
            const QList<ContactPtr> &contacts);

    bool canRequestPresenceSubscription() const;
    bool subscriptionRequestHasMessage() const;
    PendingOperation *requestPresenceSubscription(
            const QList<ContactPtr> &contacts, const QString &message);
    bool canRemovePresenceSubscription() const;
    bool subscriptionRemovalHasMessage() const;
    bool canRescindPresenceSubscriptionRequest() const;
    bool subscriptionRescindingHasMessage() const;
    PendingOperation *removePresenceSubscription(
            const QList<ContactPtr> &contacts, const QString &message);
    bool canAuthorizePresencePublication() const;
    bool publicationAuthorizationHasMessage() const;
    PendingOperation *authorizePresencePublication(
            const QList<ContactPtr> &contacts, const QString &message);
    bool publicationRejectionHasMessage() const;
    bool canRemovePresencePublication() const;
    bool publicationRemovalHasMessage() const;
    PendingOperation *removePresencePublication(
            const QList<ContactPtr> &contacts, const QString &message);
    PendingOperation *removeContacts(
            const QList<ContactPtr> &contacts, const QString &message);

    bool canBlockContacts() const;
    PendingOperation *blockContacts(const QList<ContactPtr> &contacts, bool value);

private Q_SLOTS:
    void gotContactListProperties(Tp::PendingOperation *op);
    void gotContactListContacts(QDBusPendingCallWatcher *watcher);
    void onContactListStateChanged(uint state);
    void onContactListContactsChanged(const Tp::ContactSubscriptionMap &changes,
            const Tp::UIntList &removals);

    void onContactListNewContactsConstructed(Tp::PendingOperation *op);
    void onContactListGroupsChanged(const Tp::UIntList &contacts,
            const QStringList &added, const QStringList &removed);
    void onContactListGroupsCreated(const QStringList &names);
    void onContactListGroupRenamed(const QString &oldName, const QString &newName);
    void onContactListGroupsRemoved(const QStringList &names);

    void onModifyFinished(Tp::PendingOperation *op);
    void onModifyFinishSignaled();

    void gotContactListChannelHandle(Tp::PendingOperation *op);
    void gotContactListChannel(Tp::PendingOperation *op);
    void onContactListChannelReady();

    void gotContactListGroupsProperties(Tp::PendingOperation *op);
    void onContactListContactsUpgraded(Tp::PendingOperation *op);

    void onNewChannels(const Tp::ChannelDetailsList &channelDetailsList);
    void onContactListGroupChannelReady(Tp::PendingOperation *op);
    void gotChannels(QDBusPendingCallWatcher *watcher);

    void onStoredChannelMembersChanged(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onSubscribeChannelMembersChanged(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onPublishChannelMembersChanged(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onDenyChannelMembersChanged(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);

    void onContactListGroupMembersChanged(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onContactListGroupRemoved(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage);

private:
    struct ChannelInfo;
    struct UpdateInfo;
    struct GroupsUpdateInfo;
    struct GroupRenamedInfo;
    class ModifyFinishOp;
    class RemoveGroupOp;

    void introspectContactList();
    void introspectContactListContacts();
    void processContactListChanges();
    void processContactListUpdates();
    void processContactListGroupsUpdates();
    void processContactListGroupsCreated();
    void processContactListGroupRenamed();
    void processContactListGroupsRemoved();
    void processFinishedModify();
    PendingOperation *queuedFinishVoid(const QDBusPendingCall &call);
    void setContactListChannelsReady();
    void updateContactsBlockState();
    void updateContactsPresenceState();
    void computeKnownContactsChanges(const Contacts &added,
            const Contacts &pendingAdded, const Contacts &remotePendingAdded,
            const Contacts &removed, const Channel::GroupMemberChangeDetails &details);
    void checkContactListGroupsReady();
    void setContactListGroupChannelsReady();
    QString addContactListGroupChannel(const ChannelPtr &contactListGroupChannel);

    ContactManager *contactManager;

    Contacts cachedAllKnownContacts;

    bool usingFallbackContactList;

    PendingOperation *introspectPendingOp;
    PendingOperation *introspectGroupsPendingOp;
    uint pendingContactListState;
    uint contactListState;
    bool canChangeContactList;
    bool contactListRequestUsesMessage;
    bool gotContactListInitialContacts;
    bool groupsReintrospectionRequired;
    QSet<QString> cachedAllKnownGroups;
    bool contactListGroupPropertiesReceived;
    QQueue<void (ContactManager::Roster::*)()> contactListChangesQueue;
    QQueue<UpdateInfo> contactListUpdatesQueue;
    QQueue<GroupsUpdateInfo> contactListGroupsUpdatesQueue;
    QQueue<QStringList> contactListGroupsCreatedQueue;
    QQueue<GroupRenamedInfo> contactListGroupRenamedQueue;
    QQueue<QStringList> contactListGroupsRemovedQueue;
    bool processingContactListChanges;

    QMap<PendingOperation * /* actual */, ModifyFinishOp *> returnedModifyOps;
    QQueue<ModifyFinishOp *> modifyFinishQueue;

    // old roster API
    uint contactListChannelsReady;
    QMap<uint, ChannelInfo> contactListChannels;
    ChannelPtr subscribeChannel;
    ChannelPtr publishChannel;
    ChannelPtr storedChannel;
    ChannelPtr denyChannel;

    // Number of things left to do before the Groups feature is ready
    // 1 for Get("Channels") + 1 per channel not ready
    uint featureContactListGroupsTodo;
    QList<ChannelPtr> pendingContactListGroupChannels;
    QMap<QString, ChannelPtr> contactListGroupChannels;
    QList<ChannelPtr> removedContactListGroupChannels;
};

struct TELEPATHY_QT4_NO_EXPORT ContactManager::Roster::ChannelInfo
{
    enum Type {
        TypeSubscribe = 0,
        TypePublish,
        TypeStored,
        TypeDeny,
        LastType
    };

    ChannelInfo()
        : type((Type) -1)
    {
    }

    ChannelInfo(Type type)
        : type(type)
    {
    }

    static QString identifierForType(Type type);
    static uint typeForIdentifier(const QString &identifier);

    Type type;
    ReferencedHandles handle;
    ChannelPtr channel;
};

struct TELEPATHY_QT4_NO_EXPORT ContactManager::Roster::UpdateInfo
{
    UpdateInfo(const ContactSubscriptionMap &changes, const UIntList &removals)
        : changes(changes),
          removals(removals)
    {
    }

    ContactSubscriptionMap changes;
    UIntList removals;
};

struct TELEPATHY_QT4_NO_EXPORT ContactManager::Roster::GroupsUpdateInfo
{
    GroupsUpdateInfo(const UIntList &contacts,
            const QStringList &groupsAdded, const QStringList &groupsRemoved)
        : contacts(contacts),
          groupsAdded(groupsAdded),
          groupsRemoved(groupsRemoved)
    {
    }

    UIntList contacts;
    QStringList groupsAdded;
    QStringList groupsRemoved;
};

struct TELEPATHY_QT4_NO_EXPORT ContactManager::Roster::GroupRenamedInfo
{
    GroupRenamedInfo(const QString &oldName, const QString &newName)
        : oldName(oldName),
          newName(newName)
    {
    }

    QString oldName;
    QString newName;
};

class TELEPATHY_QT4_NO_EXPORT ContactManager::Roster::ModifyFinishOp : public PendingOperation
{
    Q_OBJECT

public:
    ModifyFinishOp(const ConnectionPtr &conn);
    ~ModifyFinishOp() {};

    void setError(const QString &errorName, const QString &errorMessage);
    void finish();

private:
    QString errorName, errorMessage;
};

class TELEPATHY_QT4_NO_EXPORT ContactManager::Roster::RemoveGroupOp : public PendingOperation
{
    Q_OBJECT

public:
    RemoveGroupOp(const ChannelPtr &channel);
    ~RemoveGroupOp() {};

private Q_SLOTS:
    void onContactsRemoved(Tp::PendingOperation *);
    void onChannelClosed(Tp::PendingOperation *);
};

} // Tp

#endif
