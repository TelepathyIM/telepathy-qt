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

        PendingContacts *allKnownContacts(
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *contactsForHandles(const UIntList &handles,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());
        PendingContacts *contactsForHandles(const ReferencedHandles &handles,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *contactsForIdentifiers(const QStringList &identifiers,
                const QSet<Contact::Feature> &features = QSet<Contact::Feature>());

        PendingContacts *upgradeContacts(const QList<QSharedPointer<Contact> > &contacts,
                const QSet<Contact::Feature> &features);

    private Q_SLOTS:
        void onAliasesChanged(const Telepathy::AliasPairList &);
        void onAvatarUpdated(uint, const QString &);
        void onPresencesChanged(const Telepathy::SimpleContactPresences &);

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
                : type((Type) -1), handle(0)
            {
            }

            ContactListChannel(Type type, uint handle = 0)
                : type(type), handle(handle)
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

            Type type;
            uint handle;
            QSharedPointer<Channel> channel;
        };

        ContactManager(Connection *parent);
        ~ContactManager();

        QSharedPointer<Contact> ensureContact(const ReferencedHandles &handle,
                const QSet<Contact::Feature> &features, const QVariantMap &attributes);

        void setContactListChannels(const QMap<uint, ContactListChannel> &contactListsChannels);

        struct Private;
        friend struct Private;
        friend class Connection;
        friend class PendingContacts;
        Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
