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

#include "TelepathyQt/contact-manager-internal.h"

#include "TelepathyQt/_gen/contact-manager-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/PendingReady>

namespace Tp
{

ContactManager::Roster::Roster(ContactManager *contactManager)
    : QObject(),
      contactManager(contactManager),
      hasContactBlockingInterface(false),
      introspectPendingOp(0),
      introspectGroupsPendingOp(0),
      pendingContactListState((uint) -1),
      contactListState((uint) -1),
      canReportAbusive(false),
      gotContactBlockingInitialBlockedContacts(false),
      canChangeContactList(false),
      contactListRequestUsesMessage(false),
      gotContactListInitialContacts(false),
      gotContactListContactsChangedWithId(false),
      groupsReintrospectionRequired(false),
      contactListGroupPropertiesReceived(false),
      processingContactListChanges(false),
      groupsSetSuccess(false)
{
}

ContactManager::Roster::~Roster()
{
}

ContactListState ContactManager::Roster::state() const
{
    return (Tp::ContactListState) contactListState;
}

PendingOperation *ContactManager::Roster::introspect()
{
    ConnectionPtr conn(contactManager->connection());

    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST1)) {
        debug() << "Connection.ContactList found, using it";

        if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_BLOCKING1)) {
            debug() << "Connection.ContactBlocking found. using it";
            hasContactBlockingInterface = true;
            introspectContactBlocking();
        }
    }

    Q_ASSERT(!introspectPendingOp);
    introspectPendingOp = new PendingOperation(conn);
    return introspectPendingOp;
}

PendingOperation *ContactManager::Roster::introspectGroups()
{
    ConnectionPtr conn(contactManager->connection());

    Q_ASSERT(!introspectGroupsPendingOp);

    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST1)) {
        if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS1)) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Roster groups not supported"), conn);
        }

        debug() << "Connection.ContactGroups found, using it";

        if (!gotContactListInitialContacts) {
            debug() << "Initial ContactList contacts not retrieved. Postponing introspection";
            groupsReintrospectionRequired = true;
            return new PendingSuccess(conn);
        }

        Client::ConnectionInterfaceContactGroups1Interface *iface =
            conn->interface<Client::ConnectionInterfaceContactGroups1Interface>();

        connect(iface,
                SIGNAL(GroupsChanged(TpDBus::UIntList,QStringList,QStringList)),
                SLOT(onContactListGroupsChanged(TpDBus::UIntList,QStringList,QStringList)));
        connect(iface,
                SIGNAL(GroupsCreated(QStringList)),
                SLOT(onContactListGroupsCreated(QStringList)));
        connect(iface,
                SIGNAL(GroupRenamed(QString,QString)),
                SLOT(onContactListGroupRenamed(QString,QString)));
        connect(iface,
                SIGNAL(GroupsRemoved(QStringList)),
                SLOT(onContactListGroupsRemoved(QStringList)));

        PendingVariantMap *pvm = iface->requestAllProperties();
        connect(pvm,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotContactListGroupsProperties(Tp::PendingOperation*)));
    }
    if (groupsReintrospectionRequired) {
        return NULL;
    }

    Q_ASSERT(!introspectGroupsPendingOp);
    introspectGroupsPendingOp = new PendingOperation(conn);
    return introspectGroupsPendingOp;
}

void ContactManager::Roster::reset()
{
}

Contacts ContactManager::Roster::allKnownContacts() const
{
    return cachedAllKnownContacts;
}

QStringList ContactManager::Roster::allKnownGroups() const
{
    return cachedAllKnownGroups.toList();
}

PendingOperation *ContactManager::Roster::addGroup(const QString &group)
{
    ConnectionPtr conn(contactManager->connection());

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS1)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    Client::ConnectionInterfaceContactGroups1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroups1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->AddToGroup(group, TpDBus::UIntList()));
}

PendingOperation *ContactManager::Roster::removeGroup(const QString &group)
{
    ConnectionPtr conn(contactManager->connection());

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS1)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    Client::ConnectionInterfaceContactGroups1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroups1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RemoveGroup(group));
}

Contacts ContactManager::Roster::groupContacts(const QString &group) const
{
    Contacts ret;
    foreach (const ContactPtr &contact, allKnownContacts()) {
        if (contact->groups().contains(group))
            ret << contact;
    }
    return ret;
}

PendingOperation *ContactManager::Roster::addContactsToGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    ConnectionPtr conn(contactManager->connection());

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS1)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactGroups1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroups1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->AddToGroup(group, handles));
}

PendingOperation *ContactManager::Roster::removeContactsFromGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    ConnectionPtr conn(contactManager->connection());

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS1)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactGroups1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroups1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RemoveFromGroup(group, handles));
}

bool ContactManager::Roster::canRequestPresenceSubscription() const
{
    return canChangeContactList;
}

bool ContactManager::Roster::subscriptionRequestHasMessage() const
{
    return contactListRequestUsesMessage;
}

PendingOperation *ContactManager::Roster::requestPresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RequestSubscription(handles, message));
}

bool ContactManager::Roster::canRemovePresenceSubscription() const
{
    return canChangeContactList;
}

bool ContactManager::Roster::subscriptionRemovalHasMessage() const
{
    return false;
}

bool ContactManager::Roster::canRescindPresenceSubscriptionRequest() const
{
    return canChangeContactList;
}

bool ContactManager::Roster::subscriptionRescindingHasMessage() const
{
    return false;
}

PendingOperation *ContactManager::Roster::removePresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->Unsubscribe(handles));
}

bool ContactManager::Roster::canAuthorizePresencePublication() const
{
    return canChangeContactList;
}

bool ContactManager::Roster::publicationAuthorizationHasMessage() const
{
    return false;
}

PendingOperation *ContactManager::Roster::authorizePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->AuthorizePublication(handles));
}

bool ContactManager::Roster::publicationRejectionHasMessage() const
{
    return false;
}

bool ContactManager::Roster::canRemovePresencePublication() const
{
    return canChangeContactList;
}

bool ContactManager::Roster::publicationRemovalHasMessage() const
{
    return false;
}

PendingOperation *ContactManager::Roster::removePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->Unpublish(handles));
}

PendingOperation *ContactManager::Roster::removeContacts(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    TpDBus::UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle();
    }

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RemoveContacts(handles));
}

bool ContactManager::Roster::canBlockContacts() const
{
    return hasContactBlockingInterface;
}

bool ContactManager::Roster::canReportAbuse() const
{
    return canReportAbusive;
}

PendingOperation *ContactManager::Roster::blockContacts(
        const QList<ContactPtr> &contacts, bool value, bool reportAbuse)
{
    if (!contactManager->connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                contactManager->connection());
    } else if (!contactManager->connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRoster is not ready"),
                contactManager->connection());
    }

    ConnectionPtr conn(contactManager->connection());
    if (hasContactBlockingInterface) {
        Client::ConnectionInterfaceContactBlocking1Interface *iface =
            conn->interface<Client::ConnectionInterfaceContactBlocking1Interface>();

        TpDBus::UIntList handles;
        foreach (const ContactPtr &contact, contacts) {
            handles << contact->handle();
        }

        Q_ASSERT(iface);
        if(value) {
            return queuedFinishVoid(iface->BlockContacts(handles, reportAbuse));
        } else {
            return queuedFinishVoid(iface->UnblockContacts(handles));
        }

    } else {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Cannot block contacts on this protocol"),
                conn);
    }
}

void ContactManager::Roster::gotContactBlockingCapabilities(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Getting ContactBlockingCapabilities property failed with" <<
            op->errorName() << ":" << op->errorMessage();
        introspectContactList();
        return;
    }

    debug() << "Got ContactBlockingCapabilities property";

    PendingVariant *pv = qobject_cast<PendingVariant*>(op);

    uint contactBlockingCaps = pv->result().toUInt();
    canReportAbusive =
        contactBlockingCaps & ContactBlockingCapabilityCanReportAbusive;

    introspectContactBlockingBlockedContacts();
}

void ContactManager::Roster::gotContactBlockingBlockedContacts(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<TpDBus::HandleIdentifierMap> reply = *watcher;

    if (watcher->isError()) {
        warning() << "Getting initial ContactBlocking blocked "
            "contacts failed with" <<
            watcher->error().name() << ":" << watcher->error().message();
        introspectContactList();
        return;
    }

    debug() << "Got initial ContactBlocking blocked contacts";

    gotContactBlockingInitialBlockedContacts = true;

    ConnectionPtr conn(contactManager->connection());
    TpDBus::HandleIdentifierMap contactIds = reply.value();

    if (!contactIds.isEmpty()) {
        conn->lowlevel()->injectContactIds(contactIds);

        //fake change event where all the contacts are added
        contactListBlockedContactsChangedQueue.enqueue(
                BlockedContactsChangedInfo(contactIds, TpDBus::HandleIdentifierMap(), true));
        contactListChangesQueue.enqueue(
                &ContactManager::Roster::processContactListBlockedContactsChanged);
        processContactListChanges();
    } else {
        introspectContactList();
    }
}

void ContactManager::Roster::onContactBlockingBlockedContactsChanged(
        const TpDBus::HandleIdentifierMap &added,
        const TpDBus::HandleIdentifierMap &removed)
{
    if (!gotContactBlockingInitialBlockedContacts) {
        return;
    }

    ConnectionPtr conn(contactManager->connection());
    conn->lowlevel()->injectContactIds(added);
    conn->lowlevel()->injectContactIds(removed);

    contactListBlockedContactsChangedQueue.enqueue(
            BlockedContactsChangedInfo(added, removed));
    contactListChangesQueue.enqueue(
            &ContactManager::Roster::processContactListBlockedContactsChanged);
    processContactListChanges();
}

void ContactManager::Roster::gotContactListProperties(PendingOperation *op)
{
    if (op->isError()) {
        // We may have been in state Failure and then Success, and FeatureRoster is already ready
        if (introspectPendingOp) {
            introspectPendingOp->setFinishedWithError(
                    op->errorName(), op->errorMessage());
            introspectPendingOp = 0;
        }
        return;
    }

    debug() << "Got ContactList properties";

    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);

    QVariantMap props = pvm->result();

    canChangeContactList = qdbus_cast<uint>(props[QLatin1String("CanChangeContactList")]);
    contactListRequestUsesMessage = qdbus_cast<uint>(props[QLatin1String("RequestUsesMessage")]);

    // only update the status if we did not get it from ContactListStateChanged
    if (pendingContactListState == (uint) -1) {
        uint state = qdbus_cast<uint>(props[QLatin1String("ContactListState")]);
        onContactListStateChanged(state);
    }
}

void ContactManager::Roster::gotContactListContacts(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<TpDBus::ContactAttributesMap> reply = *watcher;

    if (watcher->isError()) {
        warning() << "Failed introspecting ContactList contacts";

        contactListState = ContactListStateFailure;
        debug() << "Setting state to failure";
        emit contactManager->stateChanged((Tp::ContactListState) contactListState);

        // We may have been in state Failure and then Success, and FeatureRoster is already ready
        if (introspectPendingOp) {
            introspectPendingOp->setFinishedWithError(
                    reply.error());
            introspectPendingOp = 0;
        }
        return;
    }

    debug() << "Got initial ContactList contacts";

    gotContactListInitialContacts = true;

    ConnectionPtr conn(contactManager->connection());
    TpDBus::ContactAttributesMap attrsMap = reply.value();
    TpDBus::ContactAttributesMap::const_iterator begin = attrsMap.constBegin();
    TpDBus::ContactAttributesMap::const_iterator end = attrsMap.constEnd();
    for (TpDBus::ContactAttributesMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        QVariantMap attrs = i.value();

        ContactPtr contact = contactManager->ensureContact(bareHandle,
                conn->contactFactory()->features(), attrs);
        cachedAllKnownContacts.insert(contact);
        contactListContacts.insert(contact);
    }

    if (contactManager->connection()->requestedFeatures().contains(
                Connection::FeatureRosterGroups)) {
        groupsSetSuccess = true;
    }

    // We may have been in state Failure and then Success, and FeatureRoster is already ready
    // In any case, if we're going to reintrospect Groups, we only advance to state success once
    // that is finished. We connect to the op finishing already here to catch all the failure finish
    // cases as well.
    if (introspectPendingOp) {
        if (!groupsSetSuccess) {
            // Will emit stateChanged() signal when the op is finished in idle
            // callback. This is to ensure FeatureRoster (and Groups) is marked ready.
            connect(introspectPendingOp,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(setStateSuccess()));
        }

        introspectPendingOp->setFinished();
        introspectPendingOp = 0;
    } else if (!groupsSetSuccess) {
        setStateSuccess();
    } else {
        // Verify that Groups is actually going to set the state
        // As far as I can see, this will always be the case.
        Q_ASSERT(groupsReintrospectionRequired);
    }

    if (groupsReintrospectionRequired) {
        introspectGroups();
    }
}

void ContactManager::Roster::setStateSuccess()
{
    if (contactManager->connection()->isValid()) {
        debug() << "State is now success";
        contactListState = ContactListStateSuccess;
        emit contactManager->stateChanged((Tp::ContactListState) contactListState);
    }
}

void ContactManager::Roster::onContactListStateChanged(uint state)
{
    if (pendingContactListState == state) {
        // ignore redundant state changes
        return;
    }

    pendingContactListState = state;

    if (state == ContactListStateSuccess) {
        introspectContactListContacts();
        return;
    }

    contactListState = state;

    if (state == ContactListStateFailure) {
        debug() << "State changed to failure, finishing roster introspection";
    }

    emit contactManager->stateChanged((Tp::ContactListState) state);

    if (state == ContactListStateFailure) {
        // Consider it done here as the state may go from Failure to Success afterwards, in which
        // case the contacts will appear.
        Q_ASSERT(introspectPendingOp);
        introspectPendingOp->setFinished();
        introspectPendingOp = 0;
    }
}

void ContactManager::Roster::onContactListContactsChangedWithId(const TpDBus::ContactSubscriptionMap &changes,
        const TpDBus::HandleIdentifierMap &ids, const TpDBus::HandleIdentifierMap &removals)
{
    debug() << "Got ContactList.ContactsChangedWithID with" << changes.size() <<
        "changes and" << removals.size() << "removals";

    gotContactListContactsChangedWithId = true;

    if (!gotContactListInitialContacts) {
        debug() << "Ignoring ContactList changes until initial contacts are retrieved";
        return;
    }

    ConnectionPtr conn(contactManager->connection());
    conn->lowlevel()->injectContactIds(ids);

    contactListUpdatesQueue.enqueue(UpdateInfo(changes, ids, removals));
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListUpdates);
    processContactListChanges();
}

void ContactManager::Roster::onContactListContactsChanged(const TpDBus::ContactSubscriptionMap &changes,
        const TpDBus::UIntList &removals)
{
    if (gotContactListContactsChangedWithId) {
        return;
    }

    debug() << "Got ContactList.ContactsChanged with" << changes.size() <<
        "changes and" << removals.size() << "removals";

    if (!gotContactListInitialContacts) {
        debug() << "Ignoring ContactList changes until initial contacts are retrieved";
        return;
    }

    TpDBus::HandleIdentifierMap removalsMap;
    foreach (uint handle, removals) {
        removalsMap.insert(handle, QString());
    }

    contactListUpdatesQueue.enqueue(UpdateInfo(changes, TpDBus::HandleIdentifierMap(), removalsMap));
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListUpdates);
    processContactListChanges();
}

void ContactManager::Roster::onContactListBlockedContactsConstructed(Tp::PendingOperation *op)
{
    BlockedContactsChangedInfo info = contactListBlockedContactsChangedQueue.dequeue();

    if (op->isError()) {
        if (info.continueIntrospectionWhenFinished) {
            introspectContactList();
        }
        processingContactListChanges = false;
        processContactListChanges();
        return;
    }

    Contacts newBlockedContacts;
    Contacts unblockedContacts;

    TpDBus::HandleIdentifierMap::const_iterator begin = info.added.constBegin();
    TpDBus::HandleIdentifierMap::const_iterator end = info.added.constEnd();
    for (TpDBus::HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();

        ContactPtr contact = contactManager->lookupContactByHandle(bareHandle);
        if (!contact) {
            warning() << "Unable to construct contact for handle" << bareHandle;
            continue;
        }

        debug() << "Contact" << contact->id() << "is now blocked";
        blockedContacts.insert(contact);
        newBlockedContacts.insert(contact);
        contact->setBlocked(true);
    }

    begin = info.removed.constBegin();
    end = info.removed.constEnd();
    for (TpDBus::HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();

        ContactPtr contact = contactManager->lookupContactByHandle(bareHandle);
        if (!contact) {
            warning() << "Unable to construct contact for handle" << bareHandle;
            continue;
        }

        debug() << "Contact" << contact->id() << "is now unblocked";
        blockedContacts.remove(contact);
        unblockedContacts.insert(contact);
        contact->setBlocked(false);
    }

    if (info.continueIntrospectionWhenFinished) {
        introspectContactList();
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::onContactListNewContactsConstructed(Tp::PendingOperation *op)
{
    if (op->isError()) {
        contactListUpdatesQueue.dequeue();
        processingContactListChanges = false;
        processContactListChanges();
        return;
    }

    UpdateInfo info = contactListUpdatesQueue.dequeue();

    Tp::Contacts added;
    Tp::Contacts removed;

    Tp::Contacts publishRequested;

    TpDBus::ContactSubscriptionMap::const_iterator begin = info.changes.constBegin();
    TpDBus::ContactSubscriptionMap::const_iterator end = info.changes.constEnd();
    for (TpDBus::ContactSubscriptionMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        TpDBus::ContactSubscriptions subscriptions = i.value();

        ContactPtr contact = contactManager->lookupContactByHandle(bareHandle);
        if (!contact) {
            warning() << "Unable to construct contact for handle" << bareHandle;
            continue;
        }

        contactListContacts.insert(contact);
        added << contact;

        Contact::PresenceState oldContactPublishState = contact->publishState();
        QString oldContactPublishStateMessage = contact->publishStateMessage();
        contact->setSubscriptionState((SubscriptionState) subscriptions.subscribe);
        contact->setPublishState((SubscriptionState) subscriptions.publish,
                subscriptions.publishRequest);
        if (subscriptions.publish == SubscriptionStateAsk &&
            (oldContactPublishState != Contact::PresenceStateAsk ||
             oldContactPublishStateMessage != contact->publishStateMessage())) {
            Channel::GroupMemberChangeDetails publishRequestDetails;
            QVariantMap detailsMap;
            detailsMap.insert(QLatin1String("message"), subscriptions.publishRequest);
            publishRequestDetails = Channel::GroupMemberChangeDetails(ContactPtr(), detailsMap);

            publishRequested.insert(contact);
        }
    }

    if (!publishRequested.empty()) {
        emit contactManager->presencePublicationRequested(publishRequested);
    }

    foreach (uint bareHandle, info.removals.keys()) {
        ContactPtr contact = contactManager->lookupContactByHandle(bareHandle);
        if (!contact) {
            warning() << "Unable to find removed contact with handle" << bareHandle;
            continue;
        }

        if (!contactListContacts.contains(contact)) {
            warning() << "Contact" << contact->id() << "removed from ContactList "
                "but it wasn't present, ignoring.";
            continue;
        }

        contactListContacts.remove(contact);
        removed << contact;
    }

    foreach (const Tp::ContactPtr &contact, removed) {
        contact->setSubscriptionState(SubscriptionStateNo);
        contact->setPublishState(SubscriptionStateNo);
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupsChanged(const TpDBus::UIntList &contacts,
        const QStringList &added, const QStringList &removed)
{
    if (!contactListGroupPropertiesReceived) {
        return;
    }

    contactListGroupsUpdatesQueue.enqueue(GroupsUpdateInfo(contacts,
                added, removed));
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListGroupsUpdates);
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupsCreated(const QStringList &names)
{
    if (!contactListGroupPropertiesReceived) {
        return;
    }

    contactListGroupsCreatedQueue.enqueue(names);
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListGroupsCreated);
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupRenamed(const QString &oldName, const QString &newName)
{
    if (!contactListGroupPropertiesReceived) {
        return;
    }

    contactListGroupRenamedQueue.enqueue(GroupRenamedInfo(oldName, newName));
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListGroupRenamed);
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupsRemoved(const QStringList &names)
{
    if (!contactListGroupPropertiesReceived) {
        return;
    }

    contactListGroupsRemovedQueue.enqueue(names);
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListGroupsRemoved);
    processContactListChanges();
}

void ContactManager::Roster::onModifyFinished(Tp::PendingOperation *op)
{
    ModifyFinishOp *returned = returnedModifyOps.take(op);

    // Finished twice, or we didn't add the returned op at all?
    Q_ASSERT(returned);

    if (op->isError()) {
        returned->setError(op->errorName(), op->errorMessage());
    }

    modifyFinishQueue.enqueue(returned);
    contactListChangesQueue.enqueue(&ContactManager::Roster::processFinishedModify);
    processContactListChanges();
}

void ContactManager::Roster::gotContactListGroupsProperties(PendingOperation *op)
{
    Q_ASSERT(introspectGroupsPendingOp != NULL);

    if (groupsSetSuccess) {
        // Connect here, so we catch the following and the other failure cases
        connect(introspectGroupsPendingOp,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(setStateSuccess()));
    }

    if (op->isError()) {
        warning() << "Getting contact list groups properties failed:" << op->errorName() << '-'
            << op->errorMessage();

        introspectGroupsPendingOp->setFinishedWithError(
                op->errorName(), op->errorMessage());
        introspectGroupsPendingOp = 0;

        return;
    }

    debug() << "Got contact list groups properties";
    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);

    QVariantMap props = pvm->result();

    cachedAllKnownGroups = qdbus_cast<QStringList>(props[QLatin1String("Groups")]).toSet();
    contactListGroupPropertiesReceived = true;

    processingContactListChanges = true;
    PendingContacts *pc = contactManager->upgradeContacts(
            contactManager->allKnownContacts().toList(),
            Contact::FeatureRosterGroups);
    connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactListContactsUpgraded(Tp::PendingOperation*)));
}

void ContactManager::Roster::onContactListContactsUpgraded(PendingOperation *op)
{
    Q_ASSERT(processingContactListChanges);
    processingContactListChanges = false;

    Q_ASSERT(introspectGroupsPendingOp != NULL);

    if (op->isError()) {
        warning() << "Upgrading contacts with group membership failed:" << op->errorName() << '-'
            << op->errorMessage();

        introspectGroupsPendingOp->setFinishedWithError(
                op->errorName(), op->errorMessage());
        introspectGroupsPendingOp = 0;
        processContactListChanges();
        return;
    }

    introspectGroupsPendingOp->setFinished();
    introspectGroupsPendingOp = 0;
    processContactListChanges();
}

void ContactManager::Roster::introspectContactBlocking()
{
    debug() << "Requesting ContactBlockingCapabilities property";

    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactBlocking1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactBlocking1Interface>();

    PendingVariant *pv = iface->requestPropertyContactBlockingCapabilities();
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContactBlockingCapabilities(Tp::PendingOperation*)));
}

void ContactManager::Roster::introspectContactBlockingBlockedContacts()
{
    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactBlocking1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactBlocking1Interface>();

    Q_ASSERT(iface);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            iface->RequestBlockedContacts(), contactManager);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotContactBlockingBlockedContacts(QDBusPendingCallWatcher*)));

    connect(iface,
            SIGNAL(BlockedContactsChanged(TpDBus::HandleIdentifierMap,Tp::HandleIdentifierMap)),
            SLOT(onContactBlockingBlockedContactsChanged(TpDBus::HandleIdentifierMap,Tp::HandleIdentifierMap)));
}

void ContactManager::Roster::introspectContactList()
{
    debug() << "Requesting ContactList properties";

    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();

    connect(iface,
            SIGNAL(ContactListStateChanged(uint)),
            SLOT(onContactListStateChanged(uint)));
    connect(iface,
            SIGNAL(ContactsChangedWithID(TpDBus::ContactSubscriptionMap,Tp::HandleIdentifierMap,Tp::HandleIdentifierMap)),
            SLOT(onContactListContactsChangedWithId(TpDBus::ContactSubscriptionMap,Tp::HandleIdentifierMap,Tp::HandleIdentifierMap)));
    connect(iface,
            SIGNAL(ContactsChanged(TpDBus::ContactSubscriptionMap,Tp::UIntList)),
            SLOT(onContactListContactsChanged(TpDBus::ContactSubscriptionMap,Tp::UIntList)));

    PendingVariantMap *pvm = iface->requestAllProperties();
    connect(pvm,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContactListProperties(Tp::PendingOperation*)));
}

void ContactManager::Roster::introspectContactListContacts()
{
    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactList1Interface *iface =
        conn->interface<Client::ConnectionInterfaceContactList1Interface>();

    Features features(conn->contactFactory()->features());
    Features supportedFeatures(contactManager->supportedFeatures());
    QSet<QString> interfaces;
    foreach (const Feature &feature, features) {
        contactManager->ensureTracking(feature);

        if (supportedFeatures.contains(feature)) {
            // Only query interfaces which are reported as supported to not get an error
            interfaces.insert(contactManager->featureToInterface(feature));
        }
    }
    interfaces.insert(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST1);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            iface->GetContactListAttributes(interfaces.toList(), true), contactManager);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotContactListContacts(QDBusPendingCallWatcher*)));
}

void ContactManager::Roster::processContactListChanges()
{
    if (processingContactListChanges || contactListChangesQueue.isEmpty()) {
        return;
    }

    processingContactListChanges = true;
    (this->*(contactListChangesQueue.dequeue()))();
}

void ContactManager::Roster::processContactListBlockedContactsChanged()
{
    BlockedContactsChangedInfo info = contactListBlockedContactsChangedQueue.head();

    TpDBus::UIntList contacts;
    TpDBus::HandleIdentifierMap::const_iterator begin = info.added.constBegin();
    TpDBus::HandleIdentifierMap::const_iterator end = info.added.constEnd();
    for (TpDBus::HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        contacts << bareHandle;
    }

    begin = info.removed.constBegin();
    end = info.removed.constEnd();
    for (TpDBus::HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        contacts << bareHandle;
    }

    Features features;
    if (contactManager->connection()->isReady(Connection::FeatureRosterGroups)) {
        features << Contact::FeatureRosterGroups;
    }
    PendingContacts *pc = contactManager->contactsForHandles(contacts, features);
    connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactListBlockedContactsConstructed(Tp::PendingOperation*)));
}

void ContactManager::Roster::processContactListUpdates()
{
    UpdateInfo info = contactListUpdatesQueue.head();

    // construct Contact objects for all contacts in added to the contact list
    TpDBus::UIntList contacts;
    TpDBus::ContactSubscriptionMap::const_iterator begin = info.changes.constBegin();
    TpDBus::ContactSubscriptionMap::const_iterator end = info.changes.constEnd();
    for (TpDBus::ContactSubscriptionMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        contacts << bareHandle;
    }

    Features features;
    if (contactManager->connection()->isReady(Connection::FeatureRosterGroups)) {
        features << Contact::FeatureRosterGroups;
    }
    PendingContacts *pc = contactManager->contactsForHandles(contacts, features);
    connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactListNewContactsConstructed(Tp::PendingOperation*)));
}

void ContactManager::Roster::processContactListGroupsUpdates()
{
    GroupsUpdateInfo info = contactListGroupsUpdatesQueue.dequeue();

    foreach (const QString &group, info.groupsAdded) {
        Contacts contacts;
        foreach (uint bareHandle, info.contacts) {
            ContactPtr contact = contactManager->lookupContactByHandle(bareHandle);
            if (!contact) {
                warning() << "contact with handle" << bareHandle << "was added to a group but "
                    "never added to the contact list, ignoring";
                continue;
            }
            contacts << contact;
            contact->setAddedToGroup(group);
        }

        emit contactManager->groupMembersChanged(group, contacts,
                Contacts(), Channel::GroupMemberChangeDetails());
    }

    foreach (const QString &group, info.groupsRemoved) {
        Contacts contacts;
        foreach (uint bareHandle, info.contacts) {
            ContactPtr contact = contactManager->lookupContactByHandle(bareHandle);
            if (!contact) {
                warning() << "contact with handle" << bareHandle << "was removed from a group but "
                    "never added to the contact list, ignoring";
                continue;
            }
            contacts << contact;
            contact->setRemovedFromGroup(group);
        }

        emit contactManager->groupMembersChanged(group, Contacts(),
                contacts, Channel::GroupMemberChangeDetails());
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::processContactListGroupsCreated()
{
    QStringList names = contactListGroupsCreatedQueue.dequeue();
    foreach (const QString &name, names) {
        cachedAllKnownGroups.insert(name);
        emit contactManager->groupAdded(name);
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::processContactListGroupRenamed()
{
    GroupRenamedInfo info = contactListGroupRenamedQueue.dequeue();
    cachedAllKnownGroups.remove(info.oldName);
    cachedAllKnownGroups.insert(info.newName);
    emit contactManager->groupRenamed(info.oldName, info.newName);

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::processContactListGroupsRemoved()
{
    QStringList names = contactListGroupsRemovedQueue.dequeue();
    foreach (const QString &name, names) {
        cachedAllKnownGroups.remove(name);
        emit contactManager->groupRemoved(name);
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::processFinishedModify()
{
    ModifyFinishOp *op = modifyFinishQueue.dequeue();
    // Only continue processing changes (and thus, emitting change signals) when the op has signaled
    // finish (it'll only do this after we've returned to the mainloop)
    connect(op,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onModifyFinishSignaled()));
    op->finish();
}

PendingOperation *ContactManager::Roster::queuedFinishVoid(const QDBusPendingCall &call)
{
    PendingOperation *actual = new PendingVoid(call, contactManager->connection());
    connect(actual,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onModifyFinished(Tp::PendingOperation*)));
    ModifyFinishOp *toReturn = new ModifyFinishOp(contactManager->connection());
    returnedModifyOps.insert(actual, toReturn);
    return toReturn;
}

void ContactManager::Roster::onModifyFinishSignaled()
{
    processingContactListChanges = false;
    processContactListChanges();
}

/**** ContactManager::Roster::ModifyFinishOp ****/
ContactManager::Roster::ModifyFinishOp::ModifyFinishOp(const ConnectionPtr &conn)
    : PendingOperation(conn)
{
}

void ContactManager::Roster::ModifyFinishOp::setError(const QString &errorName, const QString &errorMessage)
{
    Q_ASSERT(this->errorName.isEmpty());
    Q_ASSERT(this->errorMessage.isEmpty());

    Q_ASSERT(!errorName.isEmpty());

    this->errorName = errorName;
    this->errorMessage = errorMessage;
}

void ContactManager::Roster::ModifyFinishOp::finish()
{
    if (errorName.isEmpty()) {
        setFinished();
    } else {
        setFinishedWithError(errorName, errorMessage);
    }
}

} // Tp
