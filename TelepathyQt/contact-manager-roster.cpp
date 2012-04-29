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
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReferencedHandles>

namespace Tp
{

ContactManager::Roster::Roster(ContactManager *contactManager)
    : QObject(),
      contactManager(contactManager),
      usingFallbackContactList(false),
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
      contactListChannelsReady(0),
      featureContactListGroupsTodo(0),
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

    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST)) {
        debug() << "Connection.ContactList found, using it";

        usingFallbackContactList = false;

        if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_BLOCKING)) {
            debug() << "Connection.ContactBlocking found. using it";
            hasContactBlockingInterface = true;
            introspectContactBlocking();
        } else {
            debug() << "Connection.ContactBlocking not found, falling back "
                "to contact list deny channel";

            debug() << "Requesting handle for deny channel";

            contactListChannels.insert(ChannelInfo::TypeDeny,
                    ChannelInfo(ChannelInfo::TypeDeny));

            PendingHandles *ph = conn->lowlevel()->requestHandles(HandleTypeList,
                    QStringList() << ChannelInfo::identifierForType(
                        ChannelInfo::TypeDeny));
            connect(ph,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(gotContactListChannelHandle(Tp::PendingOperation*)));
        }
    } else {
        debug() << "Connection.ContactList not found, falling back to contact list channels";

        usingFallbackContactList = true;

        for (uint i = 0; i < ChannelInfo::LastType; ++i) {
            QString channelId = ChannelInfo::identifierForType(
                    (ChannelInfo::Type) i);

            debug() << "Requesting handle for" << channelId << "channel";

            contactListChannels.insert(i,
                    ChannelInfo((ChannelInfo::Type) i));

            PendingHandles *ph = conn->lowlevel()->requestHandles(HandleTypeList,
                    QStringList() << channelId);
            connect(ph,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(gotContactListChannelHandle(Tp::PendingOperation*)));
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

    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST)) {
        if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Roster groups not supported"), conn);
        }

        debug() << "Connection.ContactGroups found, using it";

        if (!gotContactListInitialContacts) {
            debug() << "Initial ContactList contacts not retrieved. Postponing introspection";
            groupsReintrospectionRequired = true;
            return new PendingSuccess(conn);
        }

        Client::ConnectionInterfaceContactGroupsInterface *iface =
            conn->interface<Client::ConnectionInterfaceContactGroupsInterface>();

        connect(iface,
                SIGNAL(GroupsChanged(Tp::UIntList,QStringList,QStringList)),
                SLOT(onContactListGroupsChanged(Tp::UIntList,QStringList,QStringList)));
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
    } else {
        debug() << "Connection.ContactGroups not found, falling back to contact list group channels";

        ++featureContactListGroupsTodo; // decremented in gotChannels

        // we already checked if requests interface exists, so bypass requests
        // interface checking
        Client::ConnectionInterfaceRequestsInterface *iface =
            conn->interface<Client::ConnectionInterfaceRequestsInterface>();

        debug() << "Connecting to Requests.NewChannels";
        connect(iface,
                SIGNAL(NewChannels(Tp::ChannelDetailsList)),
                SLOT(onNewChannels(Tp::ChannelDetailsList)));

        debug() << "Retrieving channels";
        Client::DBus::PropertiesInterface *properties =
            contactManager->connection()->interface<Client::DBus::PropertiesInterface>();
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                properties->Get(
                    TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS,
                    QLatin1String("Channels")), this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotChannels(QDBusPendingCallWatcher*)));
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
    contactListChannels.clear();
    subscribeChannel.reset();
    publishChannel.reset();
    storedChannel.reset();
    denyChannel.reset();
    contactListGroupChannels.clear();
    removedContactListGroupChannels.clear();
}

Contacts ContactManager::Roster::allKnownContacts() const
{
    return cachedAllKnownContacts;
}

QStringList ContactManager::Roster::allKnownGroups() const
{
    if (usingFallbackContactList) {
        return contactListGroupChannels.keys();
    }

    return cachedAllKnownGroups.toList();
}

PendingOperation *ContactManager::Roster::addGroup(const QString &group)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        QVariantMap request;
        request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                                     TP_QT_IFACE_CHANNEL_TYPE_CONTACT_LIST);
        request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                                     (uint) Tp::HandleTypeGroup);
        request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                                     group);
        return conn->lowlevel()->ensureChannel(request);
    }

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->AddToGroup(group, UIntList()));
}

PendingOperation *ContactManager::Roster::removeGroup(const QString &group)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        if (!contactListGroupChannels.contains(group)) {
            return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("Invalid group"),
                    conn);
        }

        ChannelPtr channel = contactListGroupChannels[group];
        return new RemoveGroupOp(channel);
    }

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RemoveGroup(group));
}

Contacts ContactManager::Roster::groupContacts(const QString &group) const
{
    if (usingFallbackContactList) {
        if (!contactListGroupChannels.contains(group)) {
            return Contacts();
        }

        ChannelPtr channel = contactListGroupChannels[group];
        return channel->groupContacts();
    }

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

    if (usingFallbackContactList) {
        if (!contactListGroupChannels.contains(group)) {
            return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("Invalid group"),
                    conn);
        }

        ChannelPtr channel = contactListGroupChannels[group];
        return channel->groupAddContacts(contacts);
    }

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->AddToGroup(group, handles));
}

PendingOperation *ContactManager::Roster::removeContactsFromGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        if (!contactListGroupChannels.contains(group)) {
            return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                    QLatin1String("Invalid group"),
                    conn);
        }

        ChannelPtr channel = contactListGroupChannels[group];
        return channel->groupRemoveContacts(contacts);
    }

    if (!conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Not implemented"),
                conn);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactGroupsInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactGroupsInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RemoveFromGroup(group, handles));
}

bool ContactManager::Roster::canRequestPresenceSubscription() const
{
    if (usingFallbackContactList) {
        return subscribeChannel && subscribeChannel->groupCanAddContacts();
    }

    return canChangeContactList;
}

bool ContactManager::Roster::subscriptionRequestHasMessage() const
{
    if (usingFallbackContactList) {
        return subscribeChannel &&
            (subscribeChannel->groupFlags() & ChannelGroupFlagMessageAdd);
    }

    return contactListRequestUsesMessage;
}

PendingOperation *ContactManager::Roster::requestPresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        if (!subscribeChannel) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Cannot subscribe to contacts' presence on this protocol"),
                    conn);
        }

        return subscribeChannel->groupAddContacts(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RequestSubscription(handles, message));
}

bool ContactManager::Roster::canRemovePresenceSubscription() const
{
    if (usingFallbackContactList) {
        return subscribeChannel && subscribeChannel->groupCanRemoveContacts();
    }

    return canChangeContactList;
}

bool ContactManager::Roster::subscriptionRemovalHasMessage() const
{
    if (usingFallbackContactList) {
        return subscribeChannel &&
            (subscribeChannel->groupFlags() & ChannelGroupFlagMessageRemove);
    }

    return false;
}

bool ContactManager::Roster::canRescindPresenceSubscriptionRequest() const
{
    if (usingFallbackContactList) {
        return subscribeChannel && subscribeChannel->groupCanRescindContacts();
    }

    return canChangeContactList;
}

bool ContactManager::Roster::subscriptionRescindingHasMessage() const
{
    if (usingFallbackContactList) {
        return subscribeChannel &&
            (subscribeChannel->groupFlags() & ChannelGroupFlagMessageRescind);
    }

    return false;
}

PendingOperation *ContactManager::Roster::removePresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        if (!subscribeChannel) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Cannot subscribe to contacts' presence on this protocol"),
                    conn);
        }

        return subscribeChannel->groupRemoveContacts(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->Unsubscribe(handles));
}

bool ContactManager::Roster::canAuthorizePresencePublication() const
{
    if (usingFallbackContactList) {
        // do not check for Channel::groupCanAddContacts as all contacts in local
        // pending can be added, even if the Channel::groupFlags() does not contain
        // the flag CanAdd
        return (bool) publishChannel;
    }

    return canChangeContactList;
}

bool ContactManager::Roster::publicationAuthorizationHasMessage() const
{
    if (usingFallbackContactList) {
        return subscribeChannel &&
            (subscribeChannel->groupFlags() & ChannelGroupFlagMessageAccept);
    }

    return false;
}

PendingOperation *ContactManager::Roster::authorizePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        if (!publishChannel) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Cannot control publication of presence on this protocol"),
                    conn);
        }

        return publishChannel->groupAddContacts(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->AuthorizePublication(handles));
}

bool ContactManager::Roster::publicationRejectionHasMessage() const
{
    if (usingFallbackContactList) {
        return subscribeChannel &&
            (subscribeChannel->groupFlags() & ChannelGroupFlagMessageReject);
    }

    return false;
}

bool ContactManager::Roster::canRemovePresencePublication() const
{
    if (usingFallbackContactList) {
        return publishChannel && publishChannel->groupCanRemoveContacts();
    }

    return canChangeContactList;
}

bool ContactManager::Roster::publicationRemovalHasMessage() const
{
    if (usingFallbackContactList) {
        return subscribeChannel &&
            (subscribeChannel->groupFlags() & ChannelGroupFlagMessageRemove);
    }

    return false;
}

PendingOperation *ContactManager::Roster::removePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        if (!publishChannel) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Cannot control publication of presence on this protocol"),
                    conn);
        }

        return publishChannel->groupRemoveContacts(contacts, message);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->Unpublish(handles));
}

PendingOperation *ContactManager::Roster::removeContacts(
        const QList<ContactPtr> &contacts, const QString &message)
{
    ConnectionPtr conn(contactManager->connection());

    if (usingFallbackContactList) {
        /* If the CM implements stored channel correctly, it should have the
         * wanted behaviour. Otherwise we have to  to remove from publish
         * and subscribe channels.
         */

        if (storedChannel && storedChannel->groupCanRemoveContacts()) {
            debug() << "Removing contacts from stored list";
            return storedChannel->groupRemoveContacts(contacts, message);
        }

        QList<PendingOperation*> operations;

        if (canRemovePresenceSubscription()) {
            debug() << "Removing contacts from subscribe list";
            operations << removePresenceSubscription(contacts, message);
        }

        if (canRemovePresencePublication()) {
            debug() << "Removing contacts from publish list";
            operations << removePresencePublication(contacts, message);
        }

        if (operations.isEmpty()) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Cannot remove contacts on this protocol"),
                    conn);
        }

        return new PendingComposite(operations, conn);
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles << contact->handle()[0];
    }

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();
    Q_ASSERT(iface);
    return queuedFinishVoid(iface->RemoveContacts(handles));
}

bool ContactManager::Roster::canBlockContacts() const
{
    if (!usingFallbackContactList && hasContactBlockingInterface) {
        return true;
    } else {
        return (bool) denyChannel;
    }
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

    if (!usingFallbackContactList && hasContactBlockingInterface) {
        ConnectionPtr conn(contactManager->connection());
        Client::ConnectionInterfaceContactBlockingInterface *iface =
            conn->interface<Client::ConnectionInterfaceContactBlockingInterface>();

        UIntList handles;
        foreach (const ContactPtr &contact, contacts) {
            handles << contact->handle()[0];
        }

        Q_ASSERT(iface);
        if(value) {
            return queuedFinishVoid(iface->BlockContacts(handles, reportAbuse));
        } else {
            return queuedFinishVoid(iface->UnblockContacts(handles));
        }

    } else {
        ConnectionPtr conn(contactManager->connection());

        if (!denyChannel) {
            return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Cannot block contacts on this protocol"),
                    conn);
        }

        if (value) {
            return denyChannel->groupAddContacts(contacts);
        } else {
            return denyChannel->groupRemoveContacts(contacts);
        }
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
    QDBusPendingReply<HandleIdentifierMap> reply = *watcher;

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
    HandleIdentifierMap contactIds = reply.value();

    if (!contactIds.isEmpty()) {
        conn->lowlevel()->injectContactIds(contactIds);

        //fake change event where all the contacts are added
        contactListBlockedContactsChangedQueue.enqueue(
                BlockedContactsChangedInfo(contactIds, HandleIdentifierMap(), true));
        contactListChangesQueue.enqueue(
                &ContactManager::Roster::processContactListBlockedContactsChanged);
        processContactListChanges();
    } else {
        introspectContactList();
    }
}

void ContactManager::Roster::onContactBlockingBlockedContactsChanged(
        const HandleIdentifierMap &added,
        const HandleIdentifierMap &removed)
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
    QDBusPendingReply<ContactAttributesMap> reply = *watcher;

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
    ContactAttributesMap attrsMap = reply.value();
    ContactAttributesMap::const_iterator begin = attrsMap.constBegin();
    ContactAttributesMap::const_iterator end = attrsMap.constEnd();
    for (ContactAttributesMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        QVariantMap attrs = i.value();

        ContactPtr contact = contactManager->ensureContact(ReferencedHandles(conn,
                    HandleTypeContact, UIntList() << bareHandle),
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

void ContactManager::Roster::onContactListContactsChangedWithId(const Tp::ContactSubscriptionMap &changes,
        const Tp::HandleIdentifierMap &ids, const Tp::HandleIdentifierMap &removals)
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

void ContactManager::Roster::onContactListContactsChanged(const Tp::ContactSubscriptionMap &changes,
        const Tp::UIntList &removals)
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

    HandleIdentifierMap removalsMap;
    foreach (uint handle, removals) {
        removalsMap.insert(handle, QString());
    }

    contactListUpdatesQueue.enqueue(UpdateInfo(changes, HandleIdentifierMap(), removalsMap));
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

    HandleIdentifierMap::const_iterator begin = info.added.constBegin();
    HandleIdentifierMap::const_iterator end = info.added.constEnd();
    for (HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
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
    for (HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
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

    // Perform the needed computation for allKnownContactsChanged
    computeKnownContactsChanges(newBlockedContacts, Contacts(),
            Contacts(), unblockedContacts, Channel::GroupMemberChangeDetails());

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

    ContactSubscriptionMap::const_iterator begin = info.changes.constBegin();
    ContactSubscriptionMap::const_iterator end = info.changes.constEnd();
    for (ContactSubscriptionMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        ContactSubscriptions subscriptions = i.value();

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

    computeKnownContactsChanges(added, Contacts(), Contacts(),
            removed, Channel::GroupMemberChangeDetails());

    foreach (const Tp::ContactPtr &contact, removed) {
        contact->setSubscriptionState(SubscriptionStateNo);
        contact->setPublishState(SubscriptionStateNo);
    }

    processingContactListChanges = false;
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupsChanged(const Tp::UIntList &contacts,
        const QStringList &added, const QStringList &removed)
{
    Q_ASSERT(usingFallbackContactList == false);

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
    Q_ASSERT(usingFallbackContactList == false);

    if (!contactListGroupPropertiesReceived) {
        return;
    }

    contactListGroupsCreatedQueue.enqueue(names);
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListGroupsCreated);
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupRenamed(const QString &oldName, const QString &newName)
{
    Q_ASSERT(usingFallbackContactList == false);

    if (!contactListGroupPropertiesReceived) {
        return;
    }

    contactListGroupRenamedQueue.enqueue(GroupRenamedInfo(oldName, newName));
    contactListChangesQueue.enqueue(&ContactManager::Roster::processContactListGroupRenamed);
    processContactListChanges();
}

void ContactManager::Roster::onContactListGroupsRemoved(const QStringList &names)
{
    Q_ASSERT(usingFallbackContactList == false);

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

void ContactManager::Roster::gotContactListChannelHandle(PendingOperation *op)
{
    PendingHandles *ph = qobject_cast<PendingHandles*>(op);
    Q_ASSERT(ph->namesRequested().size() == 1);
    QString channelId = ph->namesRequested().first();
    uint type = ChannelInfo::typeForIdentifier(channelId);

    if (op->isError()) {
        // let's not fail, because the contact lists are not supported
        debug() << "Unable to retrieve handle for" << channelId << "channel, ignoring";
        contactListChannels.remove(type);
        onContactListChannelReady();
        return;
    }

    if (ph->invalidNames().size() == 1) {
        // let's not fail, because the contact lists are not supported
        debug() << "Unable to retrieve handle for" << channelId << "channel, ignoring";
        contactListChannels.remove(type);
        onContactListChannelReady();
        return;
    }

    Q_ASSERT(ph->handles().size() == 1);

    debug() << "Got handle for" << channelId << "channel";

    if (!usingFallbackContactList) {
        Q_ASSERT(type == ChannelInfo::TypeDeny);
    } else {
        Q_ASSERT(type != (uint) -1 && type < ChannelInfo::LastType);
    }

    ReferencedHandles handle = ph->handles();
    contactListChannels[type].handle = handle;

    debug() << "Requesting channel for" << channelId << "channel";
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
            TP_QT_IFACE_CHANNEL_TYPE_CONTACT_LIST);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
            (uint) HandleTypeList);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
            handle[0]);
    ConnectionPtr conn(contactManager->connection());
    /* Request the channel passing INT_MAX as timeout (meaning no timeout), as
     * some CMs may take too long to return from ensureChannel when still
     * loading the contact list */
    connect(conn->lowlevel()->ensureChannel(request, INT_MAX),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContactListChannel(Tp::PendingOperation*)));
}

void ContactManager::Roster::gotContactListChannel(PendingOperation *op)
{
    if (op->isError()) {
        debug() << "Unable to create channel, ignoring";
        onContactListChannelReady();
        return;
    }

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    ChannelPtr channel = pc->channel();
    Q_ASSERT(channel);
    uint handle = pc->targetHandle();
    Q_ASSERT(handle);

    for (uint i = 0; i < ChannelInfo::LastType; ++i) {
        if (contactListChannels.contains(i) &&
            contactListChannels[i].handle.size() > 0 &&
            contactListChannels[i].handle[0] == handle) {
            Q_ASSERT(!contactListChannels[i].channel);
            contactListChannels[i].channel = channel;

            // deref connection refcount here as connection will keep a ref to channel and we don't
            // want a contact list channel keeping a ref of connection, otherwise connection will
            // leak, thus the channels.
            channel->connection()->deref();

            connect(channel->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onContactListChannelReady()));
        }
    }
}

void ContactManager::Roster::onContactListChannelReady()
{
    if (!usingFallbackContactList) {
        setContactListChannelsReady();

        updateContactsBlockState();

        if (denyChannel) {
            cachedAllKnownContacts.unite(denyChannel->groupContacts());
        }

        introspectContactList();
    } else if (++contactListChannelsReady == ChannelInfo::LastType) {
        if (contactListChannels.isEmpty()) {
            contactListState = ContactListStateFailure;
            debug() << "State is failure, roster not supported";
            emit contactManager->stateChanged((Tp::ContactListState) contactListState);

            Q_ASSERT(introspectPendingOp);
            introspectPendingOp->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Roster not supported"));
            introspectPendingOp = 0;
            return;
        }

        setContactListChannelsReady();

        updateContactsBlockState();

        // Refresh the cache for the current known contacts
        foreach (const ChannelInfo &contactListChannel, contactListChannels) {
            ChannelPtr channel = contactListChannel.channel;
            if (!channel) {
                continue;
            }
            cachedAllKnownContacts.unite(channel->groupContacts());
            cachedAllKnownContacts.unite(channel->groupLocalPendingContacts());
            cachedAllKnownContacts.unite(channel->groupRemotePendingContacts());
        }

        updateContactsPresenceState();

        Q_ASSERT(introspectPendingOp);

        if (!contactManager->connection()->requestedFeatures().contains(
                    Connection::FeatureRosterGroups)) {
            // Will emit stateChanged() signal when the op is finished in idle
            // callback. This is to ensure FeatureRoster is marked ready.
            connect(introspectPendingOp,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(setStateSuccess()));
        } else {
            Q_ASSERT(!groupsSetSuccess);
            groupsSetSuccess = true;
        }

        introspectPendingOp->setFinished();
        introspectPendingOp = 0;
    }
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

void ContactManager::Roster::onNewChannels(const Tp::ChannelDetailsList &channelDetailsList)
{
    ConnectionPtr conn(contactManager->connection());

    QString channelType;
    uint handleType;
    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        channelType = channelDetails.properties.value(
                TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
        if (channelType != TP_QT_IFACE_CHANNEL_TYPE_CONTACT_LIST) {
            continue;
        }

        handleType = channelDetails.properties.value(
                TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
        if (handleType != Tp::HandleTypeGroup) {
            continue;
        }

        ++featureContactListGroupsTodo; // decremented in onContactListGroupChannelReady
        ChannelPtr channel = Channel::create(conn,
                channelDetails.channel.path(), channelDetails.properties);
        pendingContactListGroupChannels.append(channel);

        // deref connection refcount here as connection will keep a ref to channel and we don't
        // want a contact list group channel keeping a ref of connection, otherwise connection will
        // leak, thus the channels.
        channel->connection()->deref();

        connect(channel->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactListGroupChannelReady(Tp::PendingOperation*)));
    }
}

void ContactManager::Roster::onContactListGroupChannelReady(PendingOperation *op)
{
    --featureContactListGroupsTodo; // incremented in onNewChannels

    ConnectionPtr conn(contactManager->connection());

    if (introspectGroupsPendingOp) {
        checkContactListGroupsReady();
    } else {
        PendingReady *pr = qobject_cast<PendingReady*>(op);
        ChannelPtr channel = ChannelPtr::qObjectCast(pr->proxy());
        QString id = addContactListGroupChannel(channel);
        emit contactManager->groupAdded(id);
        pendingContactListGroupChannels.removeOne(channel);
    }
}

void ContactManager::Roster::gotChannels(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got channels";
        onNewChannels(qdbus_cast<ChannelDetailsList>(reply.value()));
    } else {
        warning().nospace() << "Getting channels failed with " <<
            reply.error().name() << ":" << reply.error().message();
    }

    --featureContactListGroupsTodo; // incremented in introspectRosterGroups

    checkContactListGroupsReady();

    watcher->deleteLater();
}

void ContactManager::Roster::onStoredChannelMembersChanged(
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
    computeKnownContactsChanges(groupMembersAdded,
            groupLocalPendingMembersAdded, groupRemotePendingMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::Roster::onSubscribeChannelMembersChanged(
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
    computeKnownContactsChanges(groupMembersAdded,
            groupLocalPendingMembersAdded, groupRemotePendingMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::Roster::onPublishChannelMembersChanged(
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
        emit contactManager->presencePublicationRequested(groupLocalPendingMembersAdded);
    }

    // Perform the needed computation for allKnownContactsChanged
    computeKnownContactsChanges(groupMembersAdded,
            groupLocalPendingMembersAdded, groupRemotePendingMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::Roster::onDenyChannelMembersChanged(
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

    // Perform the needed computation for allKnownContactsChanged
    computeKnownContactsChanges(groupMembersAdded, Contacts(),
            Contacts(), groupMembersRemoved, details);
}

void ContactManager::Roster::onContactListGroupMembersChanged(
        const Tp::Contacts &groupMembersAdded,
        const Tp::Contacts &groupLocalPendingMembersAdded,
        const Tp::Contacts &groupRemotePendingMembersAdded,
        const Tp::Contacts &groupMembersRemoved,
        const Tp::Channel::GroupMemberChangeDetails &details)
{
    ChannelPtr contactListGroupChannel = ChannelPtr(
            qobject_cast<Channel*>(sender()));
    QString id = contactListGroupChannel->immutableProperties().value(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();

    foreach (const ContactPtr &contact, groupMembersAdded) {
        contact->setAddedToGroup(id);
    }
    foreach (const ContactPtr &contact, groupMembersRemoved) {
        contact->setRemovedFromGroup(id);
    }

    emit contactManager->groupMembersChanged(id, groupMembersAdded,
            groupMembersRemoved, details);
}

void ContactManager::Roster::onContactListGroupRemoved(Tp::DBusProxy *proxy,
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
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
    contactListGroupChannels.remove(id);
    removedContactListGroupChannels.append(contactListGroupChannel);
    disconnect(contactListGroupChannel.data(), 0, 0, 0);
    emit contactManager->groupRemoved(id);
}

void ContactManager::Roster::introspectContactBlocking()
{
    debug() << "Requesting ContactBlockingCapabilities property";

    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactBlockingInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactBlockingInterface>();

    PendingVariant *pv = iface->requestPropertyContactBlockingCapabilities();
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContactBlockingCapabilities(Tp::PendingOperation*)));
}

void ContactManager::Roster::introspectContactBlockingBlockedContacts()
{
    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactBlockingInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactBlockingInterface>();

    Q_ASSERT(iface);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            iface->RequestBlockedContacts(), contactManager);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotContactBlockingBlockedContacts(QDBusPendingCallWatcher*)));

    connect(iface,
            SIGNAL(BlockedContactsChanged(Tp::HandleIdentifierMap,Tp::HandleIdentifierMap)),
            SLOT(onContactBlockingBlockedContactsChanged(Tp::HandleIdentifierMap,Tp::HandleIdentifierMap)));
}

void ContactManager::Roster::introspectContactList()
{
    debug() << "Requesting ContactList properties";

    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();

    connect(iface,
            SIGNAL(ContactListStateChanged(uint)),
            SLOT(onContactListStateChanged(uint)));
    connect(iface,
            SIGNAL(ContactsChangedWithID(Tp::ContactSubscriptionMap,Tp::HandleIdentifierMap,Tp::HandleIdentifierMap)),
            SLOT(onContactListContactsChangedWithId(Tp::ContactSubscriptionMap,Tp::HandleIdentifierMap,Tp::HandleIdentifierMap)));
    connect(iface,
            SIGNAL(ContactsChanged(Tp::ContactSubscriptionMap,Tp::UIntList)),
            SLOT(onContactListContactsChanged(Tp::ContactSubscriptionMap,Tp::UIntList)));

    PendingVariantMap *pvm = iface->requestAllProperties();
    connect(pvm,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContactListProperties(Tp::PendingOperation*)));
}

void ContactManager::Roster::introspectContactListContacts()
{
    ConnectionPtr conn(contactManager->connection());

    Client::ConnectionInterfaceContactListInterface *iface =
        conn->interface<Client::ConnectionInterfaceContactListInterface>();

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
    interfaces.insert(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST);

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

    UIntList contacts;
    HandleIdentifierMap::const_iterator begin = info.added.constBegin();
    HandleIdentifierMap::const_iterator end = info.added.constEnd();
    for (HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
        uint bareHandle = i.key();
        contacts << bareHandle;
    }

    begin = info.removed.constBegin();
    end = info.removed.constEnd();
    for (HandleIdentifierMap::const_iterator i = begin; i != end; ++i) {
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
    UIntList contacts;
    ContactSubscriptionMap::const_iterator begin = info.changes.constBegin();
    ContactSubscriptionMap::const_iterator end = info.changes.constEnd();
    for (ContactSubscriptionMap::const_iterator i = begin; i != end; ++i) {
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

void ContactManager::Roster::setContactListChannelsReady()
{
    if (!usingFallbackContactList) {
        Q_ASSERT(!contactListChannels.contains(ChannelInfo::TypeSubscribe));
        Q_ASSERT(!contactListChannels.contains(ChannelInfo::TypePublish));
        Q_ASSERT(!contactListChannels.contains(ChannelInfo::TypeStored));
    }

    if (contactListChannels.contains(ChannelInfo::TypeSubscribe)) {
        subscribeChannel = contactListChannels[ChannelInfo::TypeSubscribe].channel;
    }

    if (contactListChannels.contains(ChannelInfo::TypePublish)) {
        publishChannel = contactListChannels[ChannelInfo::TypePublish].channel;
    }

    if (contactListChannels.contains(ChannelInfo::TypeStored)) {
        storedChannel = contactListChannels[ChannelInfo::TypeStored].channel;
    }

    if (contactListChannels.contains(ChannelInfo::TypeDeny)) {
        denyChannel = contactListChannels[ChannelInfo::TypeDeny].channel;
    }

    uint type;
    ChannelPtr channel;
    const char *method;
    for (QHash<uint, ChannelInfo>::const_iterator i = contactListChannels.constBegin();
            i != contactListChannels.constEnd(); ++i) {
        type = i.key();
        channel = i.value().channel;
        if (!channel) {
            continue;
        }

        if (type == ChannelInfo::TypeStored) {
            method = SLOT(onStoredChannelMembersChanged(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        } else if (type == ChannelInfo::TypeSubscribe) {
            method = SLOT(onSubscribeChannelMembersChanged(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        } else if (type == ChannelInfo::TypePublish) {
            method = SLOT(onPublishChannelMembersChanged(
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Contacts,
                        Tp::Channel::GroupMemberChangeDetails));
        } else if (type == ChannelInfo::TypeDeny) {
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

void ContactManager::Roster::updateContactsBlockState()
{
    Q_ASSERT(!hasContactBlockingInterface);

    if (!denyChannel) {
        return;
    }

    Contacts denyContacts = denyChannel->groupContacts();
    foreach (const ContactPtr &contact, denyContacts) {
        contact->setBlocked(true);
    }
}

void ContactManager::Roster::updateContactsPresenceState()
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

    Contacts contacts = cachedAllKnownContacts;
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

void ContactManager::Roster::computeKnownContactsChanges(const Tp::Contacts& added,
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
    foreach (const ChannelInfo &contactListChannel, contactListChannels) {
        ChannelPtr channel = contactListChannel.channel;
        if (!channel) {
            continue;
        }
        realRemoved.subtract(channel->groupContacts());
        realRemoved.subtract(channel->groupLocalPendingContacts());
        realRemoved.subtract(channel->groupRemotePendingContacts());
    }

    // ...and from the Conn.I.ContactList / Conn.I.ContactBlocking contacts
    realRemoved.subtract(contactListContacts);
    realRemoved.subtract(blockedContacts);

    // Are there any real changes?
    if (!realAdded.isEmpty() || !realRemoved.isEmpty()) {
        // Yes, update our "cache" and emit the signal
        cachedAllKnownContacts.unite(realAdded);
        cachedAllKnownContacts.subtract(realRemoved);
        emit contactManager->allKnownContactsChanged(realAdded, realRemoved, details);
    }
}

void ContactManager::Roster::checkContactListGroupsReady()
{
    if (featureContactListGroupsTodo != 0) {
        return;
    }

    if (groupsSetSuccess) {
        Q_ASSERT(contactManager->state() != ContactListStateSuccess);

        if (introspectGroupsPendingOp) {
            // Will emit stateChanged() signal when the op is finished in idle
            // callback. This is to ensure FeatureRosterGroups is marked ready.
            connect(introspectGroupsPendingOp,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(setStateSuccess()));
        } else {
            setStateSuccess();
        }

        groupsSetSuccess = false;
    }

    setContactListGroupChannelsReady();
    if (introspectGroupsPendingOp) {
        introspectGroupsPendingOp->setFinished();
        introspectGroupsPendingOp = 0;
    }
    pendingContactListGroupChannels.clear();
}

void ContactManager::Roster::setContactListGroupChannelsReady()
{
    Q_ASSERT(usingFallbackContactList == true);
    Q_ASSERT(contactListGroupChannels.isEmpty());

    foreach (const ChannelPtr &contactListGroupChannel, pendingContactListGroupChannels) {
        addContactListGroupChannel(contactListGroupChannel);
    }
}

QString ContactManager::Roster::addContactListGroupChannel(const ChannelPtr &contactListGroupChannel)
{
    QString id = contactListGroupChannel->immutableProperties().value(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
    contactListGroupChannels.insert(id, contactListGroupChannel);
    connect(contactListGroupChannel.data(),
            SIGNAL(groupMembersChanged(
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Channel::GroupMemberChangeDetails)),
            SLOT(onContactListGroupMembersChanged(
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Contacts,
                   Tp::Channel::GroupMemberChangeDetails)));
    connect(contactListGroupChannel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onContactListGroupRemoved(Tp::DBusProxy*,QString,QString)));

    foreach (const ContactPtr &contact, contactListGroupChannel->groupContacts()) {
        contact->setAddedToGroup(id);
    }

    return id;
}

/**** ContactManager::Roster::ChannelInfo ****/
QString ContactManager::Roster::ChannelInfo::identifierForType(Type type)
{
    static QString identifiers[LastType] = {
        QLatin1String("subscribe"),
        QLatin1String("publish"),
        QLatin1String("stored"),
        QLatin1String("deny"),
    };
    return identifiers[type];
}

uint ContactManager::Roster::ChannelInfo::typeForIdentifier(const QString &identifier)
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

/**** ContactManager::Roster::RemoveGroupOp ****/
ContactManager::Roster::RemoveGroupOp::RemoveGroupOp(const ChannelPtr &channel)
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

void ContactManager::Roster::RemoveGroupOp::onContactsRemoved(PendingOperation *op)
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

void ContactManager::Roster::RemoveGroupOp::onChannelClosed(PendingOperation *op)
{
    if (!op->isError()) {
        setFinished();
    } else {
        setFinishedWithError(op->errorName(), op->errorMessage());
    }
}

} // Tp
