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

#include <TelepathyQt4/ContactManager>
#include "TelepathyQt4/contact-manager-internal.h"

#include "TelepathyQt4/_gen/contact-manager.moc.hpp"
#include "TelepathyQt4/_gen/contact-manager-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/AvatarData>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContactAttributes>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Utils>

#include <QMap>
#include <QWeakPointer>

namespace Tp
{

/**
 * \class ContactManager
 * \ingroup clientconn
 * \headerfile TelepathyQt4/contact-manager.h <TelepathyQt4/ContactManager>
 *
 * \brief The ContactManager class is responsible for managing contacts.
 */

struct TELEPATHY_QT4_NO_EXPORT ContactManager::Private
{
    Private(ContactManager *parent, Connection *connection);

    void ensureTracking(const Feature &feature);

    // roster specific methods
    void processContactListChanges();
    void processContactListUpdates();
    void processContactListGroupsUpdates();
    void processContactListGroupsCreated();
    void processContactListGroupRenamed();
    void processContactListGroupsRemoved();
    void processFinishedModify();

    PendingOperation *queuedFinishVoid(const QDBusPendingCall &call);

    Contacts allKnownContactsFallback() const;
    void computeKnownContactsChangesFallback(const Contacts &added,
            const Contacts &pendingAdded, const Contacts &remotePendingAdded,
            const Contacts &removed, const Channel::GroupMemberChangeDetails &details);
    void updateContactsBlockState();
    void updateContactsPresenceStateFallback();
    PendingOperation *requestPresenceSubscriptionFallback(
            const QList<ContactPtr> &contacts, const QString &message);
    PendingOperation *removePresenceSubscriptionFallback(
            const QList<ContactPtr> &contacts, const QString &message);
    PendingOperation *authorizePresencePublicationFallback(
            const QList<ContactPtr> &contacts, const QString &message);
    PendingOperation *removePresencePublicationFallback(
            const QList<ContactPtr> &contacts, const QString &message);
    PendingOperation *removeContactsFallback(
            const QList<ContactPtr> &contacts, const QString &message);

    // roster group specific methods
    QString addContactListGroupChannelFallback(const ChannelPtr &contactListGroupChannel);
    PendingOperation *addGroupFallback(const QString &group);
    PendingOperation *removeGroupFallback(const QString &group);
    PendingOperation *addContactsToGroupFallback(const QString &group,
            const QList<ContactPtr> &contacts);
    PendingOperation *removeContactsFromGroupFallback(const QString &group,
            const QList<ContactPtr> &contacts);

    // avatar specific methods
    bool buildAvatarFileName(QString token, bool createDir,
        QString &avatarFileName, QString &mimeTypeFileName);

    struct ContactListUpdateInfo;
    struct ContactListGroupsUpdateInfo;
    struct ContactListGroupRenamedInfo;

    ContactManager *parent;
    QWeakPointer<Connection> connection;

    QMap<uint, QWeakPointer<Contact> > contacts;

    QMap<Feature, bool> tracking;
    Features supportedFeatures;

    // roster
    bool fallbackContactList;
    Contacts cachedAllKnownContacts;

    // new roster API
    bool canChangeContactList;
    bool contactListRequestUsesMessage;
    QSet<QString> allKnownGroups;
    bool contactListGroupPropertiesReceived;
    QQueue<void (Private::*)()> contactListChangesQueue;
    QQueue<ContactListUpdateInfo> contactListUpdatesQueue;
    QQueue<ContactListGroupsUpdateInfo> contactListGroupsUpdatesQueue;
    QQueue<QStringList> contactListGroupsCreatedQueue;
    QQueue<ContactListGroupRenamedInfo> contactListGroupRenamedQueue;
    QQueue<QStringList> contactListGroupsRemovedQueue;
    bool processingContactListChanges;

    QMap<Tp::PendingOperation * /* actual */, Tp::RosterModifyFinishOp *> returnedModifyOps;
    QQueue<RosterModifyFinishOp *> modifyFinishQueue;

    // old roster API
    QMap<uint, ContactListChannel> contactListChannels;
    ChannelPtr subscribeChannel;
    ChannelPtr publishChannel;
    ChannelPtr storedChannel;
    ChannelPtr denyChannel;
    QMap<QString, ChannelPtr> contactListGroupChannels;
    QList<ChannelPtr> removedContactListGroupChannels;

    // avatar
    UIntList requestAvatarsQueue;
    bool requestAvatarsIdle;
};

struct ContactManager::Private::ContactListUpdateInfo
{
    ContactListUpdateInfo(const ContactSubscriptionMap &changes, const UIntList &removals)
        : changes(changes),
          removals(removals)
    {
    }

    ContactSubscriptionMap changes;
    UIntList removals;
};

struct ContactManager::Private::ContactListGroupsUpdateInfo
{
    ContactListGroupsUpdateInfo(const UIntList &contacts,
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

struct ContactManager::Private::ContactListGroupRenamedInfo
{
    ContactListGroupRenamedInfo(const QString &oldName, const QString &newName)
        : oldName(oldName),
          newName(newName)
    {
    }

    QString oldName;
    QString newName;
};

ContactManager::Private::Private(ContactManager *parent, Connection *connection)
    : parent(parent),
      connection(connection),
      fallbackContactList(false),
      canChangeContactList(false),
      contactListRequestUsesMessage(false),
      contactListGroupPropertiesReceived(false),
      processingContactListChanges(false),
      requestAvatarsIdle(false)
{
}

void ContactManager::Private::ensureTracking(const Feature &feature)
{
    if (tracking[feature]) {
        return;
    }

    ConnectionPtr conn(parent->connection());

    if (feature == Contact::FeatureAlias) {
        Client::ConnectionInterfaceAliasingInterface *aliasingInterface =
            conn->interface<Client::ConnectionInterfaceAliasingInterface>();

        parent->connect(
                aliasingInterface,
                SIGNAL(AliasesChanged(Tp::AliasPairList)),
                SLOT(onAliasesChanged(Tp::AliasPairList)));
    } else if (feature == Contact::FeatureAvatarData) {
        Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
            conn->interface<Client::ConnectionInterfaceAvatarsInterface>();

        parent->connect(
                avatarsInterface,
                SIGNAL(AvatarRetrieved(uint,QString,QByteArray,QString)),
                SLOT(onAvatarRetrieved(uint,QString,QByteArray,QString)));
    } else if (feature == Contact::FeatureAvatarToken) {
        Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
            conn->interface<Client::ConnectionInterfaceAvatarsInterface>();

        parent->connect(
                avatarsInterface,
                SIGNAL(AvatarUpdated(uint,QString)),
                SLOT(onAvatarUpdated(uint,QString)));
    } else if (feature == Contact::FeatureCapabilities) {
        Client::ConnectionInterfaceContactCapabilitiesInterface *contactCapabilitiesInterface =
            conn->interface<Client::ConnectionInterfaceContactCapabilitiesInterface>();

        parent->connect(
                contactCapabilitiesInterface,
                SIGNAL(ContactCapabilitiesChanged(Tp::ContactCapabilitiesMap)),
                SLOT(onCapabilitiesChanged(Tp::ContactCapabilitiesMap)));
    } else if (feature == Contact::FeatureInfo) {
        Client::ConnectionInterfaceContactInfoInterface *contactInfoInterface =
            conn->interface<Client::ConnectionInterfaceContactInfoInterface>();

        parent->connect(
                contactInfoInterface,
                SIGNAL(ContactInfoChanged(uint,Tp::ContactInfoFieldList)),
                SLOT(onContactInfoChanged(uint,Tp::ContactInfoFieldList)));
    } else if (feature == Contact::FeatureLocation) {
        Client::ConnectionInterfaceLocationInterface *locationInterface =
            conn->interface<Client::ConnectionInterfaceLocationInterface>();

        parent->connect(
                locationInterface,
                SIGNAL(LocationUpdated(uint,QVariantMap)),
                SLOT(onLocationUpdated(uint,QVariantMap)));
    } else if (feature == Contact::FeatureSimplePresence) {
        Client::ConnectionInterfaceSimplePresenceInterface *simplePresenceInterface =
            conn->interface<Client::ConnectionInterfaceSimplePresenceInterface>();

        parent->connect(
                simplePresenceInterface,
                SIGNAL(PresencesChanged(Tp::SimpleContactPresences)),
                SLOT(onPresencesChanged(Tp::SimpleContactPresences)));
    } else if (feature == Contact::FeatureRosterGroups) {
        // nothing to do here, but we don't want to warn
        ;
    } else {
        warning() << " Unknown feature" << feature
            << "when trying to figure out how to connect change notification!";
    }

    tracking[feature] = true;
}


void ContactManager::Private::processContactListChanges()
{
    if (processingContactListChanges || contactListChangesQueue.isEmpty()) {
        return;
    }

    processingContactListChanges = true;
    (this->*(contactListChangesQueue.dequeue()))();
}

void ContactManager::Private::processContactListUpdates()
{
    ContactListUpdateInfo info = contactListUpdatesQueue.head();

    // construct Contact objects for all contacts in added to the contact list
    UIntList contacts;
    ContactSubscriptionMap::const_iterator begin = info.changes.constBegin();
    ContactSubscriptionMap::const_iterator end = info.changes.constEnd();
    for (ContactSubscriptionMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        contacts << bareHandle;
    }

    Features features;
    if (parent->connection()->isReady(Connection::FeatureRosterGroups)) {
        features << Contact::FeatureRosterGroups;
    }
    PendingContacts *pc = parent->contactsForHandles(contacts, features);
    parent->connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactListNewContactsConstructed(Tp::PendingOperation*)));
}

void ContactManager::Private::processContactListGroupsUpdates()
{
    ContactListGroupsUpdateInfo info = contactListGroupsUpdatesQueue.dequeue();

    foreach (const QString &group, info.groupsAdded) {
        Contacts contacts;
        foreach (uint bareHandle, info.contacts) {
            ContactPtr contact = parent->lookupContactByHandle(bareHandle);
            if (!contact) {
                warning() << "contact with handle" << bareHandle << "was added to a group but "
                    "never added to the contact list, ignoring";
                continue;
            }
            contacts << contact;
            contact->setAddedToGroup(group);
        }

        emit parent->groupMembersChanged(group, contacts,
                Contacts(), Channel::GroupMemberChangeDetails());
    }

    foreach (const QString &group, info.groupsRemoved) {
        Contacts contacts;
        foreach (uint bareHandle, info.contacts) {
            ContactPtr contact = parent->lookupContactByHandle(bareHandle);
            if (!contact) {
                warning() << "contact with handle" << bareHandle << "was removed from a group but "
                    "never added to the contact list, ignoring";
                continue;
            }
            contacts << contact;
            contact->setRemovedFromGroup(group);
        }

        emit parent->groupMembersChanged(group, Contacts(),
                contacts, Channel::GroupMemberChangeDetails());
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Private::processContactListGroupsCreated()
{
    QStringList names = contactListGroupsCreatedQueue.dequeue();
    foreach (const QString &name, names) {
        allKnownGroups.insert(name);
        emit parent->groupAdded(name);
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Private::processContactListGroupRenamed()
{
    Private::ContactListGroupRenamedInfo info = contactListGroupRenamedQueue.dequeue();
    allKnownGroups.remove(info.oldName);
    allKnownGroups.insert(info.newName);
    emit parent->groupRenamed(info.oldName, info.newName);

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Private::processContactListGroupsRemoved()
{
    QStringList names = contactListGroupsRemovedQueue.dequeue();
    foreach (const QString &name, names) {
        allKnownGroups.remove(name);
        emit parent->groupRemoved(name);
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Private::processFinishedModify()
{
    RosterModifyFinishOp *op = modifyFinishQueue.dequeue();
    // Only continue processing changes (and thus, emitting change signals) when the op has signaled
    // finish (it'll only do this after we've returned to the mainloop)
    connect(op,
            SIGNAL(finished(Tp::PendingOperation*)),
            parent,
            SLOT(onModifyFinishSignaled()));
    op->finish();
}

PendingOperation *ContactManager::Private::queuedFinishVoid(const QDBusPendingCall &call)
{
    PendingOperation *actual = new PendingVoid(call, parent->connection());
    connect(actual,
            SIGNAL(finished(Tp::PendingOperation*)),
            parent,
            SLOT(onModifyFinished(Tp::PendingOperation*)));
    RosterModifyFinishOp *toReturn = new RosterModifyFinishOp(parent->connection());
    returnedModifyOps.insert(actual, toReturn);
    return toReturn;
}

void ContactManager::onModifyFinishSignaled()
{
    mPriv->processingContactListChanges = false;
    mPriv->processContactListChanges();
}

Contacts ContactManager::Private::allKnownContactsFallback() const
{
    Contacts contacts;
    foreach (const ContactListChannel &contactListChannel, contactListChannels) {
        ChannelPtr channel = contactListChannel.channel;
        if (!channel) {
            continue;
        }
        contacts.unite(channel->groupContacts());
        contacts.unite(channel->groupLocalPendingContacts());
        contacts.unite(channel->groupRemotePendingContacts());
    }
    return contacts;
}

void ContactManager::Private::computeKnownContactsChangesFallback(const Tp::Contacts& added,
        const Tp::Contacts& pendingAdded, const Tp::Contacts& remotePendingAdded,
        const Tp::Contacts& removed, const Channel::GroupMemberChangeDetails &details)
{
    // First of all, compute the real additions/removals based upon our cache
    Tp::Contacts realAdded;
    realAdded.unite(added);
    realAdded.unite(pendingAdded);
    realAdded.unite(remotePendingAdded);
    realAdded.subtract(cachedAllKnownContacts);
    Tp::Contacts realRemoved = removed;
    realRemoved.intersect(cachedAllKnownContacts);

    // Check if realRemoved have been _really_ removed from all lists
    foreach (const ContactListChannel &contactListChannel, contactListChannels) {
        ChannelPtr channel = contactListChannel.channel;
        if (!channel) {
            continue;
        }
        realRemoved.subtract(channel->groupContacts());
        realRemoved.subtract(channel->groupLocalPendingContacts());
        realRemoved.subtract(channel->groupRemotePendingContacts());
    }

    // Are there any real changes?
    if (!realAdded.isEmpty() || !realRemoved.isEmpty()) {
        // Yes, update our "cache" and emit the signal
        cachedAllKnownContacts.unite(realAdded);
        cachedAllKnownContacts.subtract(realRemoved);
        emit parent->allKnownContactsChanged(realAdded, realRemoved, details);
    }
}

void ContactManager::Private::updateContactsBlockState()
{
    if (!denyChannel) {
        return;
    }

    Contacts denyContacts;
    if (denyChannel) {
        denyContacts = denyChannel->groupContacts();
    }

    foreach (ContactPtr contact, denyContacts) {
        contact->setBlocked(true);
    }
}

void ContactManager::Private::updateContactsPresenceStateFallback()
{
    if (!subscribeChannel && !publishChannel) {
        return;
    }

    Contacts subscribeContacts;
    Contacts subscribeContactsRP;

    if (subscribeChannel) {
        subscribeContacts = subscribeChannel->groupContacts();
        subscribeContactsRP = subscribeChannel->groupRemotePendingContacts();
    }

    Contacts publishContacts;
    Contacts publishContactsLP;
    if (publishChannel) {
        publishContacts = publishChannel->groupContacts();
        publishContactsLP = publishChannel->groupLocalPendingContacts();
    }

    Contacts contacts = allKnownContactsFallback();
    foreach (ContactPtr contact, contacts) {
        if (subscribeChannel) {
            // not in "subscribe" -> No, in "subscribe" lp -> Ask, in "subscribe" current -> Yes
            if (subscribeContacts.contains(contact)) {
                contact->setSubscriptionState(SubscriptionStateYes);
            } else if (subscribeContactsRP.contains(contact)) {
                contact->setSubscriptionState(SubscriptionStateAsk);
            } else {
                contact->setSubscriptionState(SubscriptionStateNo);
            }
        }

        if (publishChannel) {
            // not in "publish" -> No, in "subscribe" rp -> Ask, in "publish" current -> Yes
            if (publishContacts.contains(contact)) {
                contact->setPublishState(SubscriptionStateYes);
            } else if (publishContactsLP.contains(contact)) {
                contact->setPublishState(SubscriptionStateAsk,
                        publishChannel->groupLocalPendingContactChangeInfo(contact).message());
            } else {
                contact->setPublishState(SubscriptionStateNo);
            }
        }
    }
}

PendingOperation *ContactManager::Private::requestPresenceSubscriptionFallback(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!subscribeChannel) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Cannot subscribe to contacts' presence on this protocol"),
                parent->connection());
    }

    return subscribeChannel->groupAddContacts(contacts, message);
}

PendingOperation *ContactManager::Private::removePresenceSubscriptionFallback(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!subscribeChannel) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Cannot subscribe to contacts' presence on this protocol"),
                parent->connection());
    }

    return subscribeChannel->groupRemoveContacts(contacts, message);
}

PendingOperation *ContactManager::Private::authorizePresencePublicationFallback(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!publishChannel) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Cannot control publication of presence on this protocol"),
                parent->connection());
    }

    return publishChannel->groupAddContacts(contacts, message);
}

PendingOperation *ContactManager::Private::removePresencePublicationFallback(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!publishChannel) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Cannot control publication of presence on this protocol"),
                parent->connection());
    }

    return publishChannel->groupRemoveContacts(contacts, message);
}

PendingOperation *ContactManager::Private::removeContactsFallback(
        const QList<ContactPtr> &contacts, const QString &message)
{
    /* If the CM implements stored channel correctly, it should have the
     * wanted behaviour. Otherwise we have to fallback to remove from publish
     * and subscribe channels.
     */

    if (storedChannel &&
        storedChannel->groupCanRemoveContacts()) {
        debug() << "Removing contacts from stored list";
        return storedChannel->groupRemoveContacts(contacts, message);
    }

    QList<PendingOperation*> operations;

    if (parent->canRemovePresenceSubscription()) {
        debug() << "Removing contacts from subscribe list";
        operations << parent->removePresenceSubscription(contacts, message);
    }

    if (parent->canRemovePresencePublication()) {
        debug() << "Removing contacts from publish list";
        operations << parent->removePresencePublication(contacts, message);
    }

    if (operations.isEmpty()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Cannot remove contacts on this protocol"),
                parent->connection());
    }

    return new PendingComposite(operations, parent->connection());
}

QString ContactManager::Private::addContactListGroupChannelFallback(
        const ChannelPtr &contactListGroupChannel)
{
    QString id = contactListGroupChannel->immutableProperties().value(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID")).toString();
    contactListGroupChannels.insert(id, contactListGroupChannel);
    parent->connect(contactListGroupChannel.data(),
            SIGNAL(groupMembersChanged(
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Channel::GroupMemberChangeDetails)),
            SLOT(onContactListGroupMembersChangedFallback(
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Channel::GroupMemberChangeDetails)));
    parent->connect(contactListGroupChannel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onContactListGroupRemovedFallback(Tp::DBusProxy*,QString,QString)));

    foreach (const ContactPtr &contact, contactListGroupChannel->groupContacts()) {
        contact->setAddedToGroup(id);
    }
    return id;
}

PendingOperation *ContactManager::Private::addGroupFallback(const QString &group)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                                 QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_LIST));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                                 (uint) Tp::HandleTypeGroup);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                                 group);
    return parent->connection()->lowlevel()->ensureChannel(request);
}

PendingOperation *ContactManager::Private::removeGroupFallback(const QString &group)
{
    if (!contactListGroupChannels.contains(group)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("Invalid group"),
                parent->connection());
    }

    ChannelPtr channel = contactListGroupChannels[group];
    PendingContactManagerRemoveContactListGroup *op =
        new PendingContactManagerRemoveContactListGroup(channel);
    return op;
}

PendingOperation *ContactManager::Private::addContactsToGroupFallback(const QString &group,
        const QList<ContactPtr> &contacts)
{
    if (!contactListGroupChannels.contains(group)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("Invalid group"),
                parent->connection());
    }

    ChannelPtr channel = contactListGroupChannels[group];
    return channel->groupAddContacts(contacts);
}

PendingOperation *ContactManager::Private::removeContactsFromGroupFallback(const QString &group,
        const QList<ContactPtr> &contacts)
{
    if (!contactListGroupChannels.contains(group)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("Invalid group"),
                parent->connection());
    }

    ChannelPtr channel = contactListGroupChannels[group];
    return channel->groupRemoveContacts(contacts);
}

bool ContactManager::Private::buildAvatarFileName(QString token, bool createDir,
        QString &avatarFileName, QString &mimeTypeFileName)
{
    QString cacheDir = QString(QLatin1String(qgetenv("XDG_CACHE_HOME")));
    if (cacheDir.isEmpty()) {
        cacheDir = QString(QLatin1String("%1/.cache")).arg(QLatin1String(qgetenv("HOME")));
    }

    ConnectionPtr conn(parent->connection());
    QString path = QString(QLatin1String("%1/telepathy/avatars/%2/%3")).
        arg(cacheDir).arg(conn->cmName()).arg(conn->protocolName());

    if (createDir && !QDir().mkpath(path)) {
        return false;
    }

    avatarFileName = QString(QLatin1String("%1/%2")).arg(path).arg(escapeAsIdentifier(token));
    mimeTypeFileName = QString(QLatin1String("%1.mime")).arg(avatarFileName);

    return true;
}

ContactManager::ContactManager(Connection *connection)
    : Object(),
      mPriv(new Private(this, connection))
{
}

ContactManager::~ContactManager()
{
    delete mPriv;
}

ConnectionPtr ContactManager::connection() const
{
    return ConnectionPtr(mPriv->connection);
}

Features ContactManager::supportedFeatures() const
{
    if (mPriv->supportedFeatures.isEmpty() &&
        connection()->interfaces().contains(QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS))) {
        Features allFeatures = Features()
            << Contact::FeatureAlias
            << Contact::FeatureAvatarToken
            << Contact::FeatureAvatarData
            << Contact::FeatureSimplePresence
            << Contact::FeatureCapabilities
            << Contact::FeatureLocation
            << Contact::FeatureInfo;
        QStringList interfaces = connection()->lowlevel()->contactAttributeInterfaces();
        foreach (const Feature &feature, allFeatures) {
            if (interfaces.contains(featureToInterface(feature))) {
                mPriv->supportedFeatures.insert(feature);
            }
        }

        debug() << mPriv->supportedFeatures.size() << "contact features supported using" << this;
    }

    return mPriv->supportedFeatures;
}

/**
 * Return a list of relevant contacts (a reasonable guess as to what should
 * be displayed as "the contact list").
 *
 * This may include any or all of: contacts whose presence the user receives,
 * contacts who are allowed to see the user's presence, contacts stored in
 * some persistent contact list on the server, contacts who the user
 * has blocked from communicating with them, or contacts who are relevant
 * in some other way.
 *
 * User interfaces displaying a contact list will probably want to filter this
 * list and display some suitable subset of it.
 *
 * On protocols where there is no concept of presence or a centrally-stored
 * contact list (like IRC), this method may return an empty list.
 *
 * \return Some contacts
 */
Contacts ContactManager::allKnownContacts() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return Contacts();
    }

    if (mPriv->fallbackContactList) {
        return mPriv->allKnownContactsFallback();
    }

    return mPriv->cachedAllKnownContacts;
}

/**
 * Return a list of user-defined contact list groups' names.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \return List of user-defined contact list groups names.
 */
QStringList ContactManager::allKnownGroups() const
{
    if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return QStringList();
    }

    if (mPriv->fallbackContactList) {
        return mPriv->contactListGroupChannels.keys();
    }

    return mPriv->allKnownGroups.toList();
}

/**
 * Attempt to add an user-defined contact list group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * On some protocols (e.g. XMPP) empty groups are not represented on the server,
 * so disconnecting from the server and reconnecting might cause empty groups to
 * vanish.
 *
 * The returned pending operation will finish successfully if the group already
 * exists.
 *
 * FIXME: currently, the returned pending operation will finish as soon as the
 * CM EnsureChannel has returned. At this point however the NewChannels
 * mechanism hasn't yet populated our contactListGroupChannels member, which
 * means one has to wait for groupAdded before being able to actually do
 * something with the group (which is error-prone!). This is fd.o #29728.
 *
 * \param group Group name.
 * \return A pending operation which will return when an attempt has been made
 *         to add an user-defined contact list group.
 * \sa groupAdded(), addContactsToGroup()
 */
PendingOperation *ContactManager::addGroup(const QString &group)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->addGroupFallback(group);
    }

    if (!connection()->hasInterface(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Not implemented"),
                connection());
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->AddToGroup(group, UIntList()));
}

/**
 * Attempt to remove an user-defined contact list group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * FIXME: currently, the returned pending operation will finish as soon as the
 * CM close() has returned. At this point however the invalidated()
 * mechanism hasn't yet removed the channel from our contactListGroupChannels
 * member, which means contacts can seemingly still be added to the group etc.
 * until the change is picked up (and groupRemoved is emitted). This is fd.o
 * #29728.
 *
 * \param group Group name.
 * \return A pending operation which will return when an attempt has been made
 *         to remove an user-defined contact list group.
 * \sa groupRemoved(), removeContactsFromGroup()
 */
PendingOperation *ContactManager::removeGroup(const QString &group)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->removeGroupFallback(group);
    }

    if (!connection()->hasInterface(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Not implemented"),
                connection());
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->RemoveGroup(group));
}

/**
 * Return the contacts in the given user-defined contact list group
 * named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \param group Group name.
 * \return List of contacts on a user-defined contact list group, or an empty
 *         list if the group does not exist.
 * \sa allKnownGroups(), contactGroups()
 */
Contacts ContactManager::groupContacts(const QString &group) const
{
    if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return Contacts();
    }

    if (mPriv->fallbackContactList) {
        if (!mPriv->contactListGroupChannels.contains(group)) {
            return Contacts();
        }

        ChannelPtr channel = mPriv->contactListGroupChannels[group];
        return channel->groupContacts();
    }

    Contacts ret;
    foreach (const ContactPtr &contact, allKnownContacts()) {
        if (contact->groups().contains(group))
            ret << contact;
    }
    return ret;
}

/**
 * Attempt to add the given \a contacts to the user-defined contact list
 * group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \param group Group name.
 * \param contacts Contacts to add.
 * \return A pending operation which will return when an attempt has been made
 *         to add the contacts to the user-defined contact list group.
 */
PendingOperation *ContactManager::addContactsToGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->addContactsToGroupFallback(group, contacts);
    }

    if (!connection()->hasInterface(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Not implemented"),
                connection());
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->AddToGroup(group, handles));
}

/**
 * Attempt to remove the given \a contacts from the user-defined contact list
 * group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \param group Group name.
 * \param contacts Contacts to remove.
 * \return A pending operation which will return when an attempt has been made
 *         to remove the contacts from the user-defined contact list group.
 */
PendingOperation *ContactManager::removeContactsFromGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->removeContactsFromGroupFallback(group, contacts);
    }

    if (!connection()->hasInterface(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Not implemented"),
                connection());
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->RemoveFromGroup(group, handles));
}

/**
 * Return whether subscribing to additional contacts' presence is supported
 * on this channel.
 *
 * In some protocols, the list of contacts whose presence can be seen is
 * fixed, so we can't subscribe to the presence of additional contacts.
 *
 * Notably, in link-local XMPP, you can see the presence of everyone on the
 * local network, and trying to add more subscriptions would be meaningless.
 *
 * \return Whether Contact::requestPresenceSubscription and
 *         requestPresenceSubscription are likely to succeed
 */
bool ContactManager::canRequestPresenceSubscription() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            mPriv->subscribeChannel->groupCanAddContacts();
    }

    return mPriv->canChangeContactList;
}

/**
 * Return whether a message can be sent when subscribing to contacts'
 * presence.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * \return Whether the message argument to
 *         Contact::requestPresenceSubscription and
 *         requestPresenceSubscription is actually used
 */
bool ContactManager::subscriptionRequestHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            (mPriv->subscribeChannel->groupFlags() &
             ChannelGroupFlagMessageAdd);
    }

    return mPriv->contactListRequestUsesMessage;
}

/**
 * Attempt to subscribe to the presence of the given contacts.
 *
 * This operation is sometimes called "adding contacts to the buddy
 * list" or "requesting authorization".
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * On most protocols, the contacts will need to give permission
 * before the user will be able to receive their presence: if so, they will
 * be in presence state Contact::PresenceStateAsk until they authorize
 * or deny the request.
 *
 * The returned PendingOperation will return successfully when a request to
 * subscribe to the contacts' presence has been submitted, or fail if this
 * cannot happen. In particular, it does not wait for the contacts to give
 * permission for the presence subscription.
 *
 * \param contacts Contacts whose presence is desired
 * \param message A message from the user which is either transmitted to the
 *                contacts, or ignored, depending on the protocol
 * \return A pending operation which will return when an attempt has been made
 *         to subscribe to the contacts' presence
 */
PendingOperation *ContactManager::requestPresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->requestPresenceSubscriptionFallback(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->RequestSubscription(handles, message));
}

/**
 * Return whether the user can stop receiving the presence of a contact
 * whose presence they have subscribed to.
 *
 * \return Whether removePresenceSubscription and
 *         Contact::removePresenceSubscription are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateYes
 */
bool ContactManager::canRemovePresenceSubscription() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            mPriv->subscribeChannel->groupCanRemoveContacts();
    }

    return mPriv->canChangeContactList;
}

/**
 * Return whether a message can be sent when removing an existing subscription
 * to the presence of a contact.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * \return Whether the message argument to
 *         Contact::removePresenceSubscription and
 *         removePresenceSubscription is actually used,
 *         for contacts with subscription state Contact::PresenceStateYes
 */
bool ContactManager::subscriptionRemovalHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            (mPriv->subscribeChannel->groupFlags() &
             ChannelGroupFlagMessageRemove);
    }

    return false;
}

/**
 * Return whether the user can cancel a request to subscribe to a contact's
 * presence before that contact has responded.
 *
 * \return Whether removePresenceSubscription and
 *         Contact::removePresenceSubscription are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateAsk
 */
bool ContactManager::canRescindPresenceSubscriptionRequest() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            mPriv->subscribeChannel->groupCanRescindContacts();
    }

    return mPriv->canChangeContactList;
}

/**
 * Return whether a message can be sent when cancelling a request to
 * subscribe to the presence of a contact.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * \return Whether the message argument to
 *         Contact::removePresenceSubscription and
 *         removePresenceSubscription is actually used,
 *         for contacts with subscription state Contact::PresenceStateAsk
 */
bool ContactManager::subscriptionRescindingHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            (mPriv->subscribeChannel->groupFlags() &
             ChannelGroupFlagMessageRescind);
    }

    return false;
}

/**
 * Attempt to stop receiving the presence of the given contacts, or cancel
 * a request to subscribe to their presence that was previously sent.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts whose presence is no longer required
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A pending operation which will return when an attempt has been made
 *         to remove any subscription to the contacts' presence
 */
PendingOperation *ContactManager::removePresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->removePresenceSubscriptionFallback(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);

    return mPriv->queuedFinishVoid(iface->Unsubscribe(handles));
}

/**
 * Return true if the publication of the user's presence to contacts can be
 * authorized.
 *
 * This is always true, unless the protocol has no concept of authorizing
 * publication (in which case contacts' publication status can never be
 * Contact::PresenceStateAsk).
 */
bool ContactManager::canAuthorizePresencePublication() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        // do not check for Channel::groupCanAddContacts as all contacts in local
        // pending can be added, even if the Channel::groupFlags() does not contain
        // the flag CanAdd
        return (bool) mPriv->publishChannel;
    }

    return mPriv->canChangeContactList;
}

/**
 * Return whether a message can be sent when authorizing a request from a
 * contact that the user's presence is published to them.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * \return Whether the message argument to
 *         Contact::authorizePresencePublication and
 *         authorizePresencePublication is actually used,
 *         for contacts with subscription state Contact::PresenceStateAsk
 */
bool ContactManager::publicationAuthorizationHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            (mPriv->subscribeChannel->groupFlags() &
             ChannelGroupFlagMessageAccept);
    }

    return false;
}

/**
 * If the given contacts have asked the user to publish presence to them,
 * grant permission for this publication to take place.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts who should be allowed to receive the user's
 *                 presence
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A pending operation which will return when an attempt has been made
 *         to authorize publication of the user's presence to the contacts
 */
PendingOperation *ContactManager::authorizePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->authorizePresencePublicationFallback(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->AuthorizePublication(handles));
}

/**
 * Return whether a message can be sent when rejecting a request from a
 * contact that the user's presence is published to them.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * \return Whether the message argument to
 *         Contact::removePresencePublication and
 *         removePresencePublication is actually used,
 *         for contacts with subscription state Contact::PresenceStateAsk
 */
bool ContactManager::publicationRejectionHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            (mPriv->subscribeChannel->groupFlags() &
             ChannelGroupFlagMessageReject);
    }

    return false;
}

/**
 * Return true if the publication of the user's presence to contacts can be
 * removed, even after permission has been given.
 *
 * (Rejecting requests for presence to be published is always allowed.)
 *
 * \return Whether removePresencePublication and
 *         Contact::removePresencePublication are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateYes
 */
bool ContactManager::canRemovePresencePublication() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->publishChannel &&
            mPriv->publishChannel->groupCanRemoveContacts();
    }

    return mPriv->canChangeContactList;
}

/**
 * Return whether a message can be sent when revoking earlier permission
 * that the user's presence is published to a contact.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * \return Whether the message argument to
 *         Contact::removePresencePublication and
 *         removePresencePublication is actually used,
 *         for contacts with subscription state Contact::PresenceStateYes
 */
bool ContactManager::publicationRemovalHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    if (mPriv->fallbackContactList) {
        return mPriv->subscribeChannel &&
            (mPriv->subscribeChannel->groupFlags() &
             ChannelGroupFlagMessageRemove);
    }

    return false;
}

/**
 * If the given contacts have asked the user to publish presence to them,
 * deny this request (this should always succeed, unless a network error
 * occurs).
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * If the given contacts already have permission to receive
 * the user's presence, attempt to revoke that permission (this might not
 * be supported by the protocol - canRemovePresencePublication
 * indicates whether it is likely to succeed).
 *
 * \param contacts Contacts who should no longer be allowed to receive the
 *                 user's presence
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A pending operation which will return when an attempt has been made
 *         to remove any publication of the user's presence to the contacts
 */
PendingOperation *ContactManager::removePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->removePresencePublicationFallback(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->Unpublish(handles));
}

/**
 * Remove completely contacts from the server. It has the same effect than
 * calling removePresencePublication() and removePresenceSubscription(),
 * but also remove from 'stored' list if it exists.
 *
 * \param contacts Contacts who should be removed
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A pending operation which will return when an attempt has been made
 *         to remove any publication of the user's presence to the contacts
 */
PendingOperation *ContactManager::removeContacts(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    if (mPriv->fallbackContactList) {
        return mPriv->removeContactsFallback(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        connection()->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return mPriv->queuedFinishVoid(iface->RemoveContacts(handles));
}

/**
 * Return whether this protocol has a list of blocked contacts.
 *
 * \return Whether blockContacts is likely to succeed
 */
bool ContactManager::canBlockContacts() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return (bool) mPriv->denyChannel;
}

/**
 * Set whether the given contacts are blocked. Blocked contacts cannot send
 * messages to the user; depending on the protocol, blocking a contact may
 * have other effects.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts who should be added to, or removed from, the list
 *                 of blocked contacts
 * \param value If true, add the contacts to the list of blocked contacts;
 *              if false, remove them from the list
 * \return A pending operation which will return when an attempt has been made
 *         to take the requested action
 */
PendingOperation *ContactManager::blockContacts(
        const QList<ContactPtr> &contacts, bool value)
{
    if (!connection()->isValid()) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    if (!mPriv->denyChannel) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Cannot block contacts on this protocol"),
                connection());
    }

    if (value) {
        return mPriv->denyChannel->groupAddContacts(contacts);
    } else {
        return mPriv->denyChannel->groupRemoveContacts(contacts);
    }
}

PendingContacts *ContactManager::contactsForHandles(const UIntList &handles,
        const Features &features)
{
    QMap<uint, ContactPtr> satisfyingContacts;
    QSet<uint> otherContacts;
    Features missingFeatures;

    // FeatureAvatarData depends on FeatureAvatarToken
    Features realFeatures(features);
    if (realFeatures.contains(Contact::FeatureAvatarData) &&
        !realFeatures.contains(Contact::FeatureAvatarToken)) {
        realFeatures.insert(Contact::FeatureAvatarToken);
    }

    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), handles, realFeatures, QStringList(),
                satisfyingContacts, otherContacts, QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), handles, realFeatures, QStringList(),
                satisfyingContacts, otherContacts, QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    foreach (uint handle, handles) {
        ContactPtr contact = lookupContactByHandle(handle);
        if (contact) {
            if ((realFeatures - contact->requestedFeatures()).isEmpty()) {
                // Contact exists and has all the requested features
                satisfyingContacts.insert(handle, contact);
            } else {
                // Contact exists but is missing features
                otherContacts.insert(handle);
                missingFeatures.unite(realFeatures - contact->requestedFeatures());
            }
        } else {
            // Contact doesn't exist - we need to get all of the features (same as unite(features))
            missingFeatures = realFeatures;
            otherContacts.insert(handle);
        }
    }

    Features supported = supportedFeatures();
    QSet<QString> interfaces;
    foreach (const Feature &feature, missingFeatures) {
        mPriv->ensureTracking(feature);

        if (supported.contains(feature)) {
            // Only query interfaces which are reported as supported to not get an error
            interfaces.insert(featureToInterface(feature));
        }
    }

    PendingContacts *contacts =
        new PendingContacts(ContactManagerPtr(this), handles, realFeatures, interfaces.toList(),
                satisfyingContacts, otherContacts);
    return contacts;
}

PendingContacts *ContactManager::contactsForHandles(const ReferencedHandles &handles,
        const Features &features)
{
    return contactsForHandles(handles.toList(), features);
}

PendingContacts *ContactManager::contactsForIdentifiers(const QStringList &identifiers,
        const Features &features)
{
    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), identifiers, features,
                QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), identifiers, features,
                QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    PendingContacts *contacts = new PendingContacts(ContactManagerPtr(this), identifiers, features);
    return contacts;
}

PendingContacts *ContactManager::upgradeContacts(const QList<ContactPtr> &contacts,
        const Features &features)
{
    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), contacts, features,
                QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), contacts, features,
                QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    return new PendingContacts(ContactManagerPtr(this), contacts, features);
}

ContactPtr ContactManager::lookupContactByHandle(uint handle)
{
    ContactPtr contact;

    if (mPriv->contacts.contains(handle)) {
        contact = ContactPtr(mPriv->contacts.value(handle));
        if (!contact) {
            // Dangling weak pointer, remove it
            mPriv->contacts.remove(handle);
        }
    }

    return contact;
}

void ContactManager::requestContactAvatar(Contact *contact)
{
    QString avatarFileName;
    QString mimeTypeFileName;

    bool success = (contact->isAvatarTokenKnown() &&
        mPriv->buildAvatarFileName(contact->avatarToken(), false,
            avatarFileName, mimeTypeFileName));

    /* Check if the avatar is already in the cache */
    if (success && QFile::exists(avatarFileName)) {
        QFile mimeTypeFile(mimeTypeFileName);
        mimeTypeFile.open(QIODevice::ReadOnly);
        QString mimeType = QString(QLatin1String(mimeTypeFile.readAll()));
        mimeTypeFile.close();

        debug() << "Avatar found in cache for handle" << contact->handle()[0];
        debug() << "Filename:" << avatarFileName;
        debug() << "MimeType:" << mimeType;

        contact->receiveAvatarData(AvatarData(avatarFileName, mimeType));

        return;
    }

    /* Not found in cache, queue this contact. We do this to group contacts
     * for the AvatarRequest call */
    debug() << "Need to request avatar for handle" << contact->handle()[0];
    if (!mPriv->requestAvatarsIdle) {
        QTimer::singleShot(0, this, SLOT(doRequestAvatars()));
        mPriv->requestAvatarsIdle = true;
    }
    mPriv->requestAvatarsQueue.append(contact->handle()[0]);
}

void ContactManager::onAliasesChanged(const AliasPairList &aliases)
{
    debug() << "Got AliasesChanged for" << aliases.size() << "contacts";

    foreach (AliasPair pair, aliases) {
        ContactPtr contact = lookupContactByHandle(pair.handle);

        if (contact) {
            contact->receiveAlias(pair.alias);
        }
    }
}

void ContactManager::doRequestAvatars()
{
    debug() << "Request" << mPriv->requestAvatarsQueue.size() << "avatar(s)";

    Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
        connection()->interface<Client::ConnectionInterfaceAvatarsInterface>();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        avatarsInterface->RequestAvatars(mPriv->requestAvatarsQueue),
        this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher,
        SLOT(deleteLater()));

    mPriv->requestAvatarsQueue = UIntList();
    mPriv->requestAvatarsIdle = false;
}

void ContactManager::onAvatarUpdated(uint handle, const QString &token)
{
    debug() << "Got AvatarUpdate for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);
    if (contact) {
        contact->receiveAvatarToken(token);
    }
}

void ContactManager::onAvatarRetrieved(uint handle, const QString &token,
    const QByteArray &data, const QString &mimeType)
{
    QString avatarFileName;
    QString mimeTypeFileName;

    debug() << "Got AvatarRetrieved for contact with handle" << handle;

    bool success = mPriv->buildAvatarFileName(token, true, avatarFileName,
        mimeTypeFileName);

    if (success) {
        QFile mimeTypeFile(mimeTypeFileName);
        QFile avatarFile(avatarFileName);

        debug() << "Write avatar in cache for handle" << handle;
        debug() << "Filename:" << avatarFileName;
        debug() << "MimeType:" << mimeType;

        mimeTypeFile.open(QIODevice::WriteOnly);
        mimeTypeFile.write(mimeType.toLatin1());
        mimeTypeFile.close();

        avatarFile.open(QIODevice::WriteOnly);
        avatarFile.write(data);
        avatarFile.close();
    }

    ContactPtr contact = lookupContactByHandle(handle);
    if (contact) {
        contact->setAvatarToken(token);
        contact->receiveAvatarData(AvatarData(avatarFileName, mimeType));
    }
}

void ContactManager::onPresencesChanged(const SimpleContactPresences &presences)
{
    debug() << "Got PresencesChanged for" << presences.size() << "contacts";

    foreach (uint handle, presences.keys()) {
        ContactPtr contact = lookupContactByHandle(handle);

        if (contact) {
            contact->receiveSimplePresence(presences[handle]);
        }
    }
}

void ContactManager::onCapabilitiesChanged(const ContactCapabilitiesMap &caps)
{
    debug() << "Got ContactCapabilitiesChanged for" << caps.size() << "contacts";

    foreach (uint handle, caps.keys()) {
        ContactPtr contact = lookupContactByHandle(handle);

        if (contact) {
            contact->receiveCapabilities(caps[handle]);
        }
    }
}

void ContactManager::onLocationUpdated(uint handle, const QVariantMap &location)
{
    debug() << "Got LocationUpdated for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);

    if (contact) {
        contact->receiveLocation(location);
    }
}

void ContactManager::onContactInfoChanged(uint handle,
        const Tp::ContactInfoFieldList &info)
{
    debug() << "Got ContactInfoChanged for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);

    if (contact) {
        contact->receiveInfo(info);
    }
}

void ContactManager::onContactListNewContactsConstructed(Tp::PendingOperation *op)
{
    if (op->isError()) {
        mPriv->contactListUpdatesQueue.dequeue();
        mPriv->processingContactListChanges = false;
        mPriv->processContactListChanges();
        return;
    }

    Private::ContactListUpdateInfo info = mPriv->contactListUpdatesQueue.dequeue();

    Tp::Contacts added;
    Tp::Contacts removed;

    ContactSubscriptionMap::const_iterator begin = info.changes.constBegin();
    ContactSubscriptionMap::const_iterator end = info.changes.constEnd();
    for (ContactSubscriptionMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        ContactSubscriptions subscriptions = i.value();

        ContactPtr contact = lookupContactByHandle(bareHandle);
        if (!contact) {
            warning() << "Unable to construct contact for handle" << bareHandle;
            continue;
        }

        if (!mPriv->cachedAllKnownContacts.contains(contact)) {
            mPriv->cachedAllKnownContacts.insert(contact);
            added << contact;
        }

        contact->setSubscriptionState((SubscriptionState) subscriptions.subscribe);
        if (!subscriptions.publishRequest.isEmpty() &&
            subscriptions.publish == SubscriptionStateAsk) {
            Channel::GroupMemberChangeDetails publishRequestDetails;
            QVariantMap detailsMap;
            detailsMap.insert(QLatin1String("message"), subscriptions.publishRequest);
            publishRequestDetails = Channel::GroupMemberChangeDetails(ContactPtr(), detailsMap);
            // FIXME (API/ABI break) remove signal with details
            emit presencePublicationRequested(Contacts() << contact, publishRequestDetails);

            emit presencePublicationRequested(Contacts() << contact, subscriptions.publishRequest);
        }
        contact->setPublishState((SubscriptionState) subscriptions.publish,
                subscriptions.publishRequest);
    }

    foreach (uint bareHandle, info.removals) {
        ContactPtr contact = lookupContactByHandle(bareHandle);
        if (!contact) {
            warning() << "Unable to find removed contact with handle" << bareHandle;
            continue;
        }

        Q_ASSERT(mPriv->cachedAllKnownContacts.contains(contact));

        contact->setSubscriptionState(SubscriptionStateNo);
        contact->setPublishState(SubscriptionStateNo);
        mPriv->cachedAllKnownContacts.remove(contact);
        removed << contact;
    }

    if (!added.isEmpty() || !removed.isEmpty()) {
        emit allKnownContactsChanged(added, removed, Channel::GroupMemberChangeDetails());
    }

    mPriv->processingContactListChanges = false;
    mPriv->processContactListChanges();
}

void ContactManager::onContactListGroupsChanged(const Tp::UIntList &contacts,
        const QStringList &added, const QStringList &removed)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    if (!mPriv->contactListGroupPropertiesReceived) {
        return;
    }

    mPriv->contactListGroupsUpdatesQueue.enqueue(Private::ContactListGroupsUpdateInfo(contacts,
                added, removed));
    mPriv->contactListChangesQueue.enqueue(&Private::processContactListGroupsUpdates);
    mPriv->processContactListChanges();
}

void ContactManager::onContactListGroupsCreated(const QStringList &names)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    if (!mPriv->contactListGroupPropertiesReceived) {
        return;
    }

    mPriv->contactListGroupsCreatedQueue.enqueue(names);
    mPriv->contactListChangesQueue.enqueue(&Private::processContactListGroupsCreated);
    mPriv->processContactListChanges();
}

void ContactManager::onContactListGroupRenamed(const QString &oldName, const QString &newName)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    if (!mPriv->contactListGroupPropertiesReceived) {
        return;
    }

    mPriv->contactListGroupRenamedQueue.enqueue(
            Private::ContactListGroupRenamedInfo(oldName, newName));
    mPriv->contactListChangesQueue.enqueue(&Private::processContactListGroupRenamed);
    mPriv->processContactListChanges();
}

void ContactManager::onContactListGroupsRemoved(const QStringList &names)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    if (!mPriv->contactListGroupPropertiesReceived) {
        return;
    }

    mPriv->contactListGroupsRemovedQueue.enqueue(names);
    mPriv->contactListChangesQueue.enqueue(&Private::processContactListGroupsRemoved);
    mPriv->processContactListChanges();
}

void ContactManager::onModifyFinished(Tp::PendingOperation *op)
{
    RosterModifyFinishOp *returned = mPriv->returnedModifyOps.take(op);

    // Finished twice, or we didn't add the returned op at all?
    Q_ASSERT(returned);

    if (op->isError()) {
        returned->setError(op->errorName(), op->errorMessage());
    }

    mPriv->modifyFinishQueue.enqueue(returned);
    mPriv->contactListChangesQueue.enqueue(&Private::processFinishedModify);
    mPriv->processContactListChanges();
}

void ContactManager::onStoredChannelMembersChangedFallback(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    if (!groupLocalPendingMembersAdded.isEmpty()) {
        warning() << "Found local pending contacts on stored list";
    }

    if (!groupRemotePendingMembersAdded.isEmpty()) {
        warning() << "Found remote pending contacts on stored list";
    }

    foreach (ContactPtr contact, groupMembersAdded) {
        debug() << "Contact" << contact->id() << "on stored list";
    }

    foreach (ContactPtr contact, groupMembersRemoved) {
        debug() << "Contact" << contact->id() << "removed from stored list";
    }

    // Perform the needed computation for allKnownContactsChanged
    mPriv->computeKnownContactsChangesFallback(groupMembersAdded,
            groupLocalPendingMembersAdded, groupRemotePendingMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::onSubscribeChannelMembersChangedFallback(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    if (!groupLocalPendingMembersAdded.isEmpty()) {
        warning() << "Found local pending contacts on subscribe list";
    }

    foreach (ContactPtr contact, groupMembersAdded) {
        debug() << "Contact" << contact->id() << "on subscribe list";
        contact->setSubscriptionState(SubscriptionStateYes);
    }

    foreach (ContactPtr contact, groupRemotePendingMembersAdded) {
        debug() << "Contact" << contact->id() << "added to subscribe list";
        contact->setSubscriptionState(SubscriptionStateAsk);
    }

    foreach (ContactPtr contact, groupMembersRemoved) {
        debug() << "Contact" << contact->id() << "removed from subscribe list";
        contact->setSubscriptionState(SubscriptionStateNo);
    }

    // Perform the needed computation for allKnownContactsChanged
    mPriv->computeKnownContactsChangesFallback(groupMembersAdded,
            groupLocalPendingMembersAdded, groupRemotePendingMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::onPublishChannelMembersChangedFallback(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    if (!groupRemotePendingMembersAdded.isEmpty()) {
        warning() << "Found remote pending contacts on publish list";
    }

    foreach (ContactPtr contact, groupMembersAdded) {
        debug() << "Contact" << contact->id() << "on publish list";
        contact->setPublishState(SubscriptionStateYes);
    }

    foreach (ContactPtr contact, groupLocalPendingMembersAdded) {
        debug() << "Contact" << contact->id() << "added to publish list";
        contact->setPublishState(SubscriptionStateAsk, details.message());
    }

    foreach (ContactPtr contact, groupMembersRemoved) {
        debug() << "Contact" << contact->id() << "removed from publish list";
        contact->setPublishState(SubscriptionStateNo);
    }

    if (!groupLocalPendingMembersAdded.isEmpty()) {
        // FIXME (API/ABI break) remove signal with details
        emit presencePublicationRequested(groupLocalPendingMembersAdded,
            details);

        emit presencePublicationRequested(groupLocalPendingMembersAdded,
            details.message());
    }

    // Perform the needed computation for allKnownContactsChanged
    mPriv->computeKnownContactsChangesFallback(groupMembersAdded,
            groupLocalPendingMembersAdded, groupRemotePendingMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::onDenyChannelMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    if (!groupLocalPendingMembersAdded.isEmpty()) {
        warning() << "Found local pending contacts on deny list";
    }

    if (!groupRemotePendingMembersAdded.isEmpty()) {
        warning() << "Found remote pending contacts on deny list";
    }

    foreach (ContactPtr contact, groupMembersAdded) {
        debug() << "Contact" << contact->id() << "added to deny list";
        contact->setBlocked(true);
    }

    foreach (ContactPtr contact, groupMembersRemoved) {
        debug() << "Contact" << contact->id() << "removed from deny list";
        contact->setBlocked(false);
    }
}

void ContactManager::onContactListGroupMembersChangedFallback(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details)
{
    ChannelPtr contactListGroupChannel = ChannelPtr(
            qobject_cast<Channel*>(sender()));
    QString id = contactListGroupChannel->immutableProperties().value(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID")).toString();

    foreach (const ContactPtr &contact, groupMembersAdded) {
        contact->setAddedToGroup(id);
    }
    foreach (const ContactPtr &contact, groupMembersRemoved) {
        contact->setRemovedFromGroup(id);
    }

    emit groupMembersChanged(id, groupMembersAdded, groupMembersRemoved, details);
}

void ContactManager::onContactListGroupRemovedFallback(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(errorName);
    Q_UNUSED(errorMessage);

    // Is it correct to assume that if an user-defined contact list
    // gets invalidated it means it was removed? Spec states that if a
    // user-defined contact list gets closed it was removed, and Channel
    // invalidates itself when it gets closed.
    ChannelPtr contactListGroupChannel = ChannelPtr(qobject_cast<Channel*>(proxy));
    QString id = contactListGroupChannel->immutableProperties().value(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID")).toString();
    mPriv->contactListGroupChannels.remove(id);
    mPriv->removedContactListGroupChannels.append(contactListGroupChannel);
    disconnect(contactListGroupChannel.data(), 0, 0, 0);
    emit groupRemoved(id);
}

ContactPtr ContactManager::ensureContact(const ReferencedHandles &handle,
        const Features &features, const QVariantMap &attributes)
{
    uint bareHandle = handle[0];
    ContactPtr contact = lookupContactByHandle(bareHandle);

    if (!contact) {
        contact = connection()->contactFactory()->construct(this, handle, features, attributes);
        mPriv->contacts.insert(bareHandle, contact.data());
    } else {
        contact->augment(features, attributes);
    }

    return contact;
}

void ContactManager::setUseFallbackContactList(bool value)
{
    mPriv->fallbackContactList = value;
}

void ContactManager::setContactListProperties(const QVariantMap &props)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    mPriv->canChangeContactList = qdbus_cast<uint>(props[QLatin1String("CanChangeContactList")]);
    mPriv->contactListRequestUsesMessage = qdbus_cast<uint>(props[QLatin1String("RequestUsesMessage")]);
}

void ContactManager::setContactListContacts(const ContactAttributesMap &attrsMap)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    ContactAttributesMap::const_iterator begin = attrsMap.constBegin();
    ContactAttributesMap::const_iterator end = attrsMap.constEnd();
    for (ContactAttributesMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        QVariantMap attrs = i.value();

        ContactPtr contact = ensureContact(ReferencedHandles(connection(),
                    HandleTypeContact, UIntList() << bareHandle),
                Features(), attrs);
        mPriv->cachedAllKnownContacts.insert(contact);
    }
}

void ContactManager::updateContactListContacts(const ContactSubscriptionMap &changes,
        const UIntList &removals)
{
    Q_ASSERT(mPriv->fallbackContactList == false);

    mPriv->contactListUpdatesQueue.enqueue(Private::ContactListUpdateInfo(changes, removals));
    mPriv->contactListChangesQueue.enqueue(&Private::processContactListUpdates);
    mPriv->processContactListChanges();
}

void ContactManager::setContactListGroupsProperties(const QVariantMap &props)
{
    Q_ASSERT(mPriv->fallbackContactList == false);
    Q_ASSERT(mPriv->contactListGroupPropertiesReceived == false);

    mPriv->allKnownGroups = qdbus_cast<QStringList>(props[QLatin1String("Groups")]).toSet();
    mPriv->contactListGroupPropertiesReceived = true;
}

void ContactManager::setContactListChannels(
        const QMap<uint, ContactListChannel> &contactListChannels)
{
    if (!mPriv->fallbackContactList) {
        Q_ASSERT(!contactListChannels.contains(ContactListChannel::TypeSubscribe));
        Q_ASSERT(!contactListChannels.contains(ContactListChannel::TypePublish));
        Q_ASSERT(!contactListChannels.contains(ContactListChannel::TypeStored));
    }

    mPriv->contactListChannels = contactListChannels;

    if (mPriv->contactListChannels.contains(ContactListChannel::TypeSubscribe)) {
        mPriv->subscribeChannel = mPriv->contactListChannels[ContactListChannel::TypeSubscribe].channel;
    }

    if (mPriv->contactListChannels.contains(ContactListChannel::TypePublish)) {
        mPriv->publishChannel = mPriv->contactListChannels[ContactListChannel::TypePublish].channel;
    }

    if (mPriv->contactListChannels.contains(ContactListChannel::TypeStored)) {
        mPriv->storedChannel = mPriv->contactListChannels[ContactListChannel::TypeStored].channel;
    }

    if (mPriv->contactListChannels.contains(ContactListChannel::TypeDeny)) {
        mPriv->denyChannel = mPriv->contactListChannels[ContactListChannel::TypeDeny].channel;
    }

    mPriv->updateContactsBlockState();

    if (mPriv->fallbackContactList) {
        mPriv->updateContactsPresenceStateFallback();
        // Refresh the cache for the current known contacts
        mPriv->cachedAllKnownContacts = allKnownContacts();
    }

    uint type;
    ChannelPtr channel;
    const char *method;
    for (QMap<uint, ContactListChannel>::const_iterator i = contactListChannels.constBegin();
            i != contactListChannels.constEnd(); ++i) {
        type = i.key();
        channel = i.value().channel;
        if (!channel) {
            continue;
        }

        if (type == ContactListChannel::TypeStored) {
            method = SLOT(onStoredChannelMembersChangedFallback(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        }else if (type == ContactListChannel::TypeSubscribe) {
            method = SLOT(onSubscribeChannelMembersChangedFallback(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        } else if (type == ContactListChannel::TypePublish) {
            method = SLOT(onPublishChannelMembersChangedFallback(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        } else if (type == ContactListChannel::TypeDeny) {
            method = SLOT(onDenyChannelMembersChanged(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        } else {
            continue;
        }

        connect(channel.data(),
                SIGNAL(groupMembersChanged(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails)),
                method);
    }
}

void ContactManager::setContactListGroupChannelsFallback(
        const QList<ChannelPtr> &contactListGroupChannels)
{
    Q_ASSERT(mPriv->fallbackContactList == true);

    Q_ASSERT(mPriv->contactListGroupChannels.isEmpty());

    foreach (const ChannelPtr &contactListGroupChannel, contactListGroupChannels) {
        mPriv->addContactListGroupChannelFallback(contactListGroupChannel);
    }
}

void ContactManager::addContactListGroupChannelFallback(
        const ChannelPtr &contactListGroupChannel)
{
    Q_ASSERT(mPriv->fallbackContactList == true);

    QString id = mPriv->addContactListGroupChannelFallback(contactListGroupChannel);
    emit groupAdded(id);
}

void ContactManager::resetContactListChannels()
{
    mPriv->contactListChannels.clear();
    mPriv->subscribeChannel.reset();
    mPriv->publishChannel.reset();
    mPriv->storedChannel.reset();
    mPriv->denyChannel.reset();
    mPriv->contactListGroupChannels.clear();
    mPriv->removedContactListGroupChannels.clear();
}

QString ContactManager::featureToInterface(const Feature &feature)
{
    if (feature == Contact::FeatureAlias) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_ALIASING;
    } else if (feature == Contact::FeatureAvatarToken) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_AVATARS;
    } else if (feature == Contact::FeatureAvatarData) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_AVATARS;
    } else if (feature == Contact::FeatureSimplePresence) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE;
    } else if (feature == Contact::FeatureCapabilities) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES;
    } else if (feature == Contact::FeatureLocation) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_LOCATION;
    } else if (feature == Contact::FeatureInfo) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_INFO;
    } else if (feature == Contact::FeatureRosterGroups) {
        return TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS;
    } else {
        warning() << "ContactManager doesn't know which interface corresponds to feature"
            << feature;
        return QString();
    }
}

QString ContactManager::ContactListChannel::identifierForType(Type type)
{
    static QString identifiers[LastType] = {
        QLatin1String("subscribe"),
        QLatin1String("publish"),
        QLatin1String("stored"),
        QLatin1String("deny"),
    };
    return identifiers[type];
}

uint ContactManager::ContactListChannel::typeForIdentifier(const QString &identifier)
{
    static QHash<QString, uint> types;
    if (types.isEmpty()) {
        types.insert(QLatin1String("subscribe"), TypeSubscribe);
        types.insert(QLatin1String("publish"), TypePublish);
        types.insert(QLatin1String("stored"), TypeStored);
        types.insert(QLatin1String("deny"), TypeDeny);
    }
    if (types.contains(identifier)) {
        return types[identifier];
    }
    return (uint) -1;
}

/**
 * \fn void ContactManager::presencePublicationRequested(const Tp::Contacts &contacts,
 *          const QString &message);
 *
 * This signal is emitted whenever some contacts request for presence publication.
 *
 * \param contacts A set of contacts which requested presence publication.
 * \param message An optional message that was sent by the contacts asking to receive the local
 *                user's presence.
 */

/**
 * \fn void ContactManager::presencePublicationRequested(const Tp::Contacts &contacts,
 *          const Tp::Channel::GroupMemberChangeDetails &details);
 *
 * \deprecated Use presencePublicationRequested(const Tp::Contacts &contact, const QString &message)
 *             instead.
 */

/**
 * \fn void ContactManager::groupMembersChanged(const QString &group,
 *          const Tp::Contacts &groupMembersAdded,
 *          const Tp::Contacts &groupMembersRemoved,
 *          const Tp::Channel::GroupMemberChangeDetails &details);
 *
 * This signal is emitted whenever some contacts got removed or added from
 * a group.
 *
 * \param group The name of the group that changed.
 * \param groupMembersAdded A set of contacts which were added to the group \a group.
 * \param groupMembersRemoved A set of contacts which were removed from the group \a group.
 * \param details The change details.
 */

/**
 * \fn void ContactManager::allKnownContactsChanged(const Tp::Contacts &contactsAdded,
 *          const Tp::Contacts &contactsRemoved,
 *          const Tp::Channel::GroupMemberChangeDetails &details);
 *
 * This signal is emitted whenever some contacts got removed or added from
 * ContactManager's known contact list. It is useful for monitoring which contacts
 * are currently known by ContactManager.
 *
 * \param contactsAdded A set of contacts which were added to the known contact list.
 * \param contactsRemoved A set of contacts which were removed from the known contact list.
 * \param details The change details.
 *
 * \note Please note that, in some protocols, this signal could stream newly added contacts
 *       with both presence subscription and publication state set to No. Be sure to watch
 *       over publication and/or subscription state changes if that is the case.
 */

PendingContactManagerRemoveContactListGroup::PendingContactManagerRemoveContactListGroup(
        const ChannelPtr &channel)
    : PendingOperation(channel)
{
    Contacts contacts = channel->groupContacts();
    if (!contacts.isEmpty()) {
        connect(channel->groupRemoveContacts(contacts.toList()),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactsRemoved(Tp::PendingOperation*)));
    } else {
        connect(channel->requestClose(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onChannelClosed(Tp::PendingOperation*)));
    }
}

void PendingContactManagerRemoveContactListGroup::onContactsRemoved(PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    // Let's ignore possible errors and try to remove the group
    ChannelPtr channel = ChannelPtr(qobject_cast<Channel*>((Channel *) object().data()));
    connect(channel->requestClose(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelClosed(Tp::PendingOperation*)));
}

void PendingContactManagerRemoveContactListGroup::onChannelClosed(PendingOperation *op)
{
    if (!op->isError()) {
        setFinished();
    } else {
        setFinishedWithError(op->errorName(), op->errorMessage());
    }
}

RosterModifyFinishOp::RosterModifyFinishOp(const ConnectionPtr &conn)
    : PendingOperation(conn)
{
}

void RosterModifyFinishOp::setError(const QString &errorName, const QString &errorMessage)
{
    Q_ASSERT(this->errorName.isEmpty());
    Q_ASSERT(this->errorMessage.isEmpty());

    Q_ASSERT(!errorName.isEmpty());

    this->errorName = errorName;
    this->errorMessage = errorMessage;
}

void RosterModifyFinishOp::finish()
{
    if (errorName.isEmpty()) {
        setFinished();
    } else {
        setFinishedWithError(errorName, errorMessage);
    }
}

void ContactManager::connectNotify(const char *signalName)
{
    if (qstrcmp(signalName, SIGNAL(presencePublicationRequested(Tp::Contacts,Tp::Channel::GroupMemberChangeDetails))) == 0) {
        warning() << "Connecting to deprecated signal presencePublicationRequested(Tp::Contacts,Tp::Channel::GroupMemberChangeDetails)";
    }
}

} // Tp
