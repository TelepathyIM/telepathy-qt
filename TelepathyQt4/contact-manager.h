/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2009 Nokia Corporation
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

#include <QObject>

#include <QList>
#include <QSet>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ReferencedHandles>

namespace Tp
{

class Connection;
class PendingContacts;
class PendingOperation;

class ContactManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ContactManager)

public:
    ConnectionPtr connection() const;

    QSet<Contact::Feature> supportedFeatures() const;

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

    bool canBlockContacts() const;
    PendingOperation *blockContacts(
            const QList<ContactPtr> &contacts, bool value = true);

    PendingContacts *contactsForHandles(const UIntList &handles,
            const QSet<Contact::Feature> &features = QSet<Contact::Feature>());
    PendingContacts *contactsForHandles(const ReferencedHandles &handles,
            const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

    PendingContacts *contactsForIdentifiers(const QStringList &identifiers,
            const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

    PendingContacts *upgradeContacts(const QList<ContactPtr> &contacts,
            const QSet<Contact::Feature> &features);

    ContactPtr lookupContactByHandle(uint handle);

Q_SIGNALS:
    void presencePublicationRequested(const Tp::Contacts &contacts);
    void groupAdded(const QString &group);
    void groupRemoved(const QString &group);
    void groupMembersChanged(const QString &group,
            const Tp::Contacts &groupMembersAdded,
            const Tp::Contacts &groupMembersRemoved);

private Q_SLOTS:
    void onAliasesChanged(const Tp::AliasPairList &);
    void onAvatarUpdated(uint, const QString &);
    void onPresencesChanged(const Tp::SimpleContactPresences &);

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
    void onContactListGroupRemoved(Tp::DBusProxy *,
        const QString &, const QString &);

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

    ContactManager(const ConnectionPtr &parent);
    ~ContactManager();

    ContactPtr ensureContact(const ReferencedHandles &handle,
            const QSet<Contact::Feature> &features,
            const QVariantMap &attributes);

    void setContactListChannels(
            const QMap<uint, ContactListChannel> &contactListChannels);
    void setContactListGroupChannels(
            const QList<ChannelPtr> &contactListGroupChannels);
    void addContactListGroupChannel(const ChannelPtr &contactListGroupChannel);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
