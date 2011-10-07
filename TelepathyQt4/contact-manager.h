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
    TELEPATHY_QT4_DEPRECATED PendingOperation *blockContacts(
            const QList<ContactPtr> &contacts, bool value);
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

    PendingContacts *upgradeContacts(const QList<ContactPtr> &contacts,
            const Features &features);

    ContactPtr lookupContactByHandle(uint handle);

    TELEPATHY_QT4_DEPRECATED void requestContactAvatar(Contact *contact);
    void requestContactAvatars(const QList<ContactPtr> &contacts);

    PendingOperation *refreshContactsInfo(const QList<ContactPtr> &contact);

Q_SIGNALS:
    void stateChanged(Tp::ContactListState state);

    void presencePublicationRequested(const Tp::Contacts &contacts);
    // deprecated - carry redundant data which can be retrieved (meaningfully) from the Contacts
    // themselves (note: multiple contacts, but just a single message/details!)
    void presencePublicationRequested(const Tp::Contacts &contacts, const QString &message);
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
    TELEPATHY_QT4_NO_EXPORT void onAliasesChanged(const Tp::AliasPairList &);
    TELEPATHY_QT4_NO_EXPORT void doRequestAvatars();
    TELEPATHY_QT4_NO_EXPORT void onAvatarUpdated(uint, const QString &);
    TELEPATHY_QT4_NO_EXPORT void onAvatarRetrieved(uint, const QString &, const QByteArray &, const QString &);
    TELEPATHY_QT4_NO_EXPORT void onPresencesChanged(const Tp::SimpleContactPresences &);
    TELEPATHY_QT4_NO_EXPORT void onCapabilitiesChanged(const Tp::ContactCapabilitiesMap &);
    TELEPATHY_QT4_NO_EXPORT void onLocationUpdated(uint, const QVariantMap &);
    TELEPATHY_QT4_NO_EXPORT void onContactInfoChanged(uint, const Tp::ContactInfoFieldList &);
    TELEPATHY_QT4_NO_EXPORT void doRefreshInfo();

private:
    class PendingRefreshContactInfo;
    class Roster;
    friend class Connection;
    friend class PendingContacts;
    friend class PendingRefreshContactInfo;
    friend class Roster;

    TELEPATHY_QT4_NO_EXPORT ContactManager(Connection *parent);

    TELEPATHY_QT4_NO_EXPORT ContactPtr ensureContact(const ReferencedHandles &handle,
            const Features &features,
            const QVariantMap &attributes);
    TELEPATHY_QT4_NO_EXPORT ContactPtr ensureContact(uint bareHandle,
            const QString &id, const Features &features);

    TELEPATHY_QT4_NO_EXPORT static QString featureToInterface(const Feature &feature);
    TELEPATHY_QT4_NO_EXPORT void ensureTracking(const Feature &feature);

    TELEPATHY_QT4_NO_EXPORT PendingOperation *introspectRoster();
    TELEPATHY_QT4_NO_EXPORT PendingOperation *introspectRosterGroups();
    TELEPATHY_QT4_NO_EXPORT void resetRoster();

    TELEPATHY_QT4_NO_EXPORT PendingOperation *refreshContactInfo(Contact *contact);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
