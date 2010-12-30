/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt4_contact_manager_h_HEADER_GUARD_
#define _TelepathyQt4_contact_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Feature>
#include <TelepathyQt4/Object>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Types>

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class Connection;
class PendingContacts;
class PendingOperation;

class TELEPATHY_QT4_EXPORT ContactManager : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(ContactManager)

public:
    virtual ~ContactManager();

    ConnectionPtr connection() const;

    Features supportedFeatures() const;

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
            const QList<ContactPtr> &contacts,
            const QString &message = QString());
    bool canRemovePresenceSubscription() const;
    bool subscriptionRemovalHasMessage() const;
    bool canRescindPresenceSubscriptionRequest() const;
    bool subscriptionRescindingHasMessage() const;
    PendingOperation *removePresenceSubscription(
            const QList<ContactPtr> &contacts,
            const QString &message = QString());
    bool canAuthorizePresencePublication() const;
    bool publicationAuthorizationHasMessage() const;
    PendingOperation *authorizePresencePublication(
            const QList<ContactPtr> &contacts,
            const QString &message = QString());
    bool publicationRejectionHasMessage() const;
    bool canRemovePresencePublication() const;
    bool publicationRemovalHasMessage() const;
    PendingOperation *removePresencePublication(
            const QList<ContactPtr> &contacts,
            const QString &message = QString());
    PendingOperation *removeContacts(
            const QList<ContactPtr> &contacts,
            const QString &message = QString());

    bool canBlockContacts() const;
    PendingOperation *blockContacts(
            const QList<ContactPtr> &contacts, bool value = true);

    PendingContacts *contactsForHandles(const UIntList &handles,
            const Features &features = Features());
    PendingContacts *contactsForHandles(const ReferencedHandles &handles,
            const Features &features = Features());

    PendingContacts *contactsForIdentifiers(const QStringList &identifiers,
            const Features &features = Features());

    PendingContacts *upgradeContacts(const QList<ContactPtr> &contacts,
            const Features &features);

    ContactPtr lookupContactByHandle(uint handle);

    void requestContactAvatar(Contact *contact);

Q_SIGNALS:
    void presencePublicationRequested(const Tp::Contacts &contacts, const QString &message);
    // deprecated
    void presencePublicationRequested(const Tp::Contacts &contacts,
        const Tp::Channel::GroupMemberChangeDetails &details);

    void groupAdded(const QString &group);
    void groupRenamed(const QString &oldGroup, const QString &newGroup);
    void groupRemoved(const QString &group);

    void groupMembersChanged(const QString &group,
            const Tp::Contacts &groupMembersAdded,
            const Tp::Contacts &groupMembersRemoved,
            const Tp::Channel::GroupMemberChangeDetails &details);

    void allKnownContactsChanged(const Tp::Contacts &contactsAdded,
            const Tp::Contacts &contactsRemoved,
            const Tp::Channel::GroupMemberChangeDetails &details);

protected:
    // FIXME: (API/ABI break) Remove connectNotify
    void connectNotify(const char *);

private Q_SLOTS:
    void onAliasesChanged(const Tp::AliasPairList &);
    void doRequestAvatars();
    void onAvatarUpdated(uint, const QString &);
    void onAvatarRetrieved(uint, const QString &, const QByteArray &, const QString &);
    void onPresencesChanged(const Tp::SimpleContactPresences &);
    void onCapabilitiesChanged(const Tp::ContactCapabilitiesMap &);
    void onLocationUpdated(uint, const QVariantMap &);
    void onContactInfoChanged(uint, const Tp::ContactInfoFieldList &);

    void onContactListNewContactsConstructed(Tp::PendingOperation *op);
    void onContactListGroupsChanged(const Tp::UIntList &contacts,
            const QStringList &added, const QStringList &removed);
    void onContactListGroupsCreated(const QStringList &names);
    void onContactListGroupRenamed(const QString &oldName, const QString &newName);
    void onContactListGroupsRemoved(const QStringList &names);

    void onStoredChannelMembersChangedFallback(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onSubscribeChannelMembersChangedFallback(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onPublishChannelMembersChangedFallback(
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

    void onContactListGroupMembersChangedFallback(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details);
    void onContactListGroupRemovedFallback(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage);

private:
    friend class Connection;
    friend class PendingContacts;

    struct ContactListChannel
    {
        enum Type {
            TypeSubscribe = 0,
            TypePublish,
            TypeStored,
            TypeDeny,
            LastType
        };

        ContactListChannel()
            : type((Type) -1)
        {
        }

        ContactListChannel(Type type)
            : type(type)
        {
        }

        ~ContactListChannel()
        {
        }

        static QString identifierForType(Type type);
        static uint typeForIdentifier(const QString &identifier);

        Type type;
        ReferencedHandles handle;
        ChannelPtr channel;
    };

    ContactManager(Connection *parent);

    ContactPtr ensureContact(const ReferencedHandles &handle,
            const Features &features,
            const QVariantMap &attributes);

    void setUseFallbackContactList(bool value);

    void setContactListProperties(const QVariantMap &props);
    void setContactListContacts(const ContactAttributesMap &attrs);
    void updateContactListContacts(const ContactSubscriptionMap &changes,
            const UIntList &removals);
    void setContactListGroupsProperties(const QVariantMap &props);

    void setContactListChannels(
            const QMap<uint, ContactListChannel> &contactListChannels);

    void setContactListGroupChannelsFallback(
            const QList<ChannelPtr> &contactListGroupChannels);
    void addContactListGroupChannelFallback(
            const ChannelPtr &contactListGroupChannel);

    void resetContactListChannels();

    static QString featureToInterface(const Feature &feature);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
