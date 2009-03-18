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

#ifndef _TelepathyQt4_cli_contact_manager_h_HEADER_GUARD_
#define _TelepathyQt4_cli_contact_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>

#include <QList>
#include <QSet>
#include <QSharedPointer>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/ReferencedHandles>

namespace Telepathy
{
namespace Client
{

class Connection;
class PendingContacts;
class PendingOperation;

class ContactManager : public QObject
{
    Q_OBJECT

    public:

        Connection *connection() const;

        bool isSupported() const;
        QSet<Contact::Feature> supportedFeatures() const;

        Contacts allKnownContacts() const;

        bool canRequestPresenceSubscription() const;
        bool subscriptionRequestHasMessage() const;
        PendingOperation *requestPresenceSubscription(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());
        bool canRemovePresenceSubscription() const;
        bool subscriptionRemovalHasMessage() const;
        bool canRescindPresenceSubscriptionRequest() const;
        bool subscriptionRescindingHasMessage() const;
        PendingOperation *removePresenceSubscription(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());
        bool canAuthorizePresencePublication() const;
        bool publicationAuthorizationHasMessage() const;
        PendingOperation *authorizePresencePublication(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());
        bool publicationRejectionHasMessage() const;
        bool canRemovePresencePublication() const;
        bool publicationRemovalHasMessage() const;
        PendingOperation *removePresencePublication(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());

        bool canBlockContacts() const;
        PendingOperation *blockContacts(
                const QList<QSharedPointer<Contact> > &contacts, bool value = true);

        PendingContacts *contactsForHandles(const UIntList &handles,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());
        PendingContacts *contactsForHandles(const ReferencedHandles &handles,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *contactsForIdentifiers(const QStringList &identifiers,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *upgradeContacts(const QList<QSharedPointer<Contact> > &contacts,
                const QSet<Contact::Feature> &features);

    Q_SIGNALS:
        void presencePublicationRequested(const Telepathy::Client::Contacts &contacts);

    private Q_SLOTS:
        void onAliasesChanged(const Telepathy::AliasPairList &);
        void onAvatarUpdated(uint, const QString &);
        void onPresencesChanged(const Telepathy::SimpleContactPresences &);

        void onSubscribeChannelMembersChanged(
            const Telepathy::Client::Contacts &groupMembersAdded,
            const Telepathy::Client::Contacts &groupLocalPendingMembersAdded,
            const Telepathy::Client::Contacts &groupRemotePendingMembersAdded,
            const Telepathy::Client::Contacts &groupMembersRemoved,
            const Telepathy::Client::Channel::GroupMemberChangeDetails &details);
        void onPublishChannelMembersChanged(
            const Telepathy::Client::Contacts &groupMembersAdded,
            const Telepathy::Client::Contacts &groupLocalPendingMembersAdded,
            const Telepathy::Client::Contacts &groupRemotePendingMembersAdded,
            const Telepathy::Client::Contacts &groupMembersRemoved,
            const Telepathy::Client::Channel::GroupMemberChangeDetails &details);
        void onDenyChannelMembersChanged(
            const Telepathy::Client::Contacts &groupMembersAdded,
            const Telepathy::Client::Contacts &groupLocalPendingMembersAdded,
            const Telepathy::Client::Contacts &groupRemotePendingMembersAdded,
            const Telepathy::Client::Contacts &groupMembersRemoved,
            const Telepathy::Client::Channel::GroupMemberChangeDetails &details);

    private:
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
        ~ContactManager();

        QSharedPointer<Contact> ensureContact(const ReferencedHandles &handle,
                const QSet<Contact::Feature> &features, const QVariantMap &attributes);

        void setContactListChannels(const QMap<uint, ContactListChannel> &contactListsChannels);

        QSharedPointer<Contact> lookupContactByHandle(uint handle);

        struct Private;
        friend struct Private;
        friend class Connection;
        friend class PendingContacts;
        friend class Contact;
        Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
