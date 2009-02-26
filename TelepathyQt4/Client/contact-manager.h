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

        QSet<QSharedPointer<Contact> > allKnownContacts() const;

        bool canRequestContactsPresenceSubscription() const;
        PendingOperation *requestContactsPresenceSubscription(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());
        bool canRemoveContactsPresenceSubscription() const;
        PendingOperation *removeContactsPresenceSubscription(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());
        bool canAuthorizeContactsPresencePublication() const;
        PendingOperation *authorizeContactsPresencePublication(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());
        bool canDenyContactsPresencePublication() const;
        PendingOperation *denyContactsPresencePublication(
                const QList<QSharedPointer<Contact> > &contacts, const QString &message = QString());

        PendingContacts *contactsForHandles(const UIntList &handles,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());
        PendingContacts *contactsForHandles(const ReferencedHandles &handles,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *contactsForIdentifiers(const QStringList &identifiers,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *upgradeContacts(const QList<QSharedPointer<Contact> > &contacts,
                const QSet<Contact::Feature> &features);

    Q_SIGNALS:
        void presencePublicationRequested(const QSet<QSharedPointer<Telepathy::Client::Contact> > &contacts);

    private Q_SLOTS:
        void onAliasesChanged(const Telepathy::AliasPairList &);
        void onAvatarUpdated(uint, const QString &);
        void onPresencesChanged(const Telepathy::SimpleContactPresences &);

        void onSubscribeChannelMembersChanged(
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupMembersAdded,
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupLocalPendingMembersAdded,
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupRemotePendingMembersAdded,
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupMembersRemoved,
            const Telepathy::Client::Channel::GroupMemberChangeDetails &details);
        void onPublishChannelMembersChanged(
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupMembersAdded,
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupLocalPendingMembersAdded,
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupRemotePendingMembersAdded,
            const QSet<QSharedPointer<Telepathy::Client::Contact> > &groupMembersRemoved,
            const Telepathy::Client::Channel::GroupMemberChangeDetails &details);

    private:
        struct ContactListChannel
        {
            enum Type {
                TypeSubscribe = 0,
                TypePublish,
                TypeStored,
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
                channel.clear();
            }

            static QString identifierForType(Type type)
            {
                static QString identifiers[LastType] = {
                    "subscribe", "publish", "stored"
                };
                return identifiers[type];
            }

            static uint typeForIdentifier(const QString &identifier)
            {
                static QHash<QString, uint> types;
                if (types.isEmpty()) {
                    types.insert("subscribe", TypeSubscribe);
                    types.insert("publish", TypePublish);
                    types.insert("Stored", TypeStored);
                }
                if (types.contains(identifier)) {
                    return types[identifier];
                }
                return (uint) -1;
            }

            Type type;
            ReferencedHandles handle;
            QSharedPointer<Channel> channel;
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
