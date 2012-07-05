/**
 * This file is part of TelepathyQt
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

#ifndef _TelepathyQt_contact_manager_h_HEADER_GUARD_
#define _TelepathyQt_contact_manager_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Feature>
#include <TelepathyQt/Object>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/Types>

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

class TP_QT_EXPORT ContactManager : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(ContactManager)

public:
    virtual ~ContactManager();

    ConnectionPtr connection() const;

    Features supportedFeatures() const;

    ContactListState state() const;

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
    bool canReportAbuse() const;
    PendingOperation *blockContacts(const QList<ContactPtr> &contacts);
    PendingOperation *blockContactsAndReportAbuse(const QList<ContactPtr> &contacts);
    PendingOperation *unblockContacts(const QList<ContactPtr> &contacts);

    PendingContacts *contactsForHandles(const UIntList &handles,
            const Features &features = Features());
    PendingContacts *contactsForHandles(const ReferencedHandles &handles,
            const Features &features = Features());
    PendingContacts *contactsForHandles(const HandleIdentifierMap &handles,
            const Features &features = Features());

    PendingContacts *contactsForIdentifiers(const QStringList &identifiers,
            const Features &features = Features());

    PendingContacts *contactsForVCardAddresses(const QString &vcardField,
            const QStringList &vcardAddresses,
            const Features &features = Features());
    PendingContacts *contactsForUris(const QStringList &uris,
            const Features &features = Features());

    PendingContacts *upgradeContacts(const QList<ContactPtr> &contacts,
            const Features &features);

    void requestContactAvatars(const QList<ContactPtr> &contacts);

    PendingOperation *refreshContactInfo(const QList<ContactPtr> &contact);

Q_SIGNALS:
    void stateChanged(Tp::ContactListState state);

    void presencePublicationRequested(const Tp::Contacts &contacts);

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

private Q_SLOTS:
    TP_QT_NO_EXPORT void onAliasesChanged(const Tp::AliasPairList &);
    TP_QT_NO_EXPORT void doRequestAvatars();
    TP_QT_NO_EXPORT void onAvatarUpdated(uint, const QString &);
    TP_QT_NO_EXPORT void onAvatarRetrieved(uint, const QString &, const QByteArray &, const QString &);
    TP_QT_NO_EXPORT void onPresencesChanged(const Tp::SimpleContactPresences &);
    TP_QT_NO_EXPORT void onCapabilitiesChanged(const Tp::ContactCapabilitiesMap &);
    TP_QT_NO_EXPORT void onLocationUpdated(uint, const QVariantMap &);
    TP_QT_NO_EXPORT void onContactInfoChanged(uint, const Tp::ContactInfoFieldList &);
    TP_QT_NO_EXPORT void onClientTypesUpdated(uint, const QStringList &);
    TP_QT_NO_EXPORT void doRefreshInfo();

private:
    class PendingRefreshContactInfo;
    class Roster;
    friend class Channel;
    friend class Connection;
    friend class PendingContacts;
    friend class PendingRefreshContactInfo;
    friend class Roster;

    TP_QT_NO_EXPORT ContactManager(Connection *parent);

    TP_QT_NO_EXPORT ContactPtr lookupContactByHandle(uint handle);

    TP_QT_NO_EXPORT ContactPtr ensureContact(const ReferencedHandles &handle,
            const Features &features,
            const QVariantMap &attributes);
    TP_QT_NO_EXPORT ContactPtr ensureContact(uint bareHandle,
            const QString &id, const Features &features);

    TP_QT_NO_EXPORT static QString featureToInterface(const Feature &feature);
    TP_QT_NO_EXPORT void ensureTracking(const Feature &feature);

    TP_QT_NO_EXPORT PendingOperation *introspectRoster();
    TP_QT_NO_EXPORT PendingOperation *introspectRosterGroups();
    TP_QT_NO_EXPORT void resetRoster();

    TP_QT_NO_EXPORT PendingOperation *refreshContactInfo(Contact *contact);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
