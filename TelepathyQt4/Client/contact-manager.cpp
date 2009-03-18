/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt4/Client/ContactManager>
#include "TelepathyQt4/Client/_gen/contact-manager.moc.hpp"

#include <QMap>
#include <QString>
#include <QSet>
#include <QWeakPointer>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingContactAttributes>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingFailure>
#include <TelepathyQt4/Client/PendingHandles>
#include <TelepathyQt4/Client/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

/**
 * \defgroup clientconn Connection proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Connection objects.
 */

namespace Telepathy
{
namespace Client
{

/**
 * \class ContactManager
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/Client/contact-manager.h> <TelepathyQt4/Client/ContactManager>
 */

struct ContactManager::Private
{
    Connection *conn;
    QMap<uint, QWeakPointer<Contact> > contacts;

    QMap<Contact::Feature, bool> tracking;
    void ensureTracking(Contact::Feature feature);

    QSet<Contact::Feature> supportedFeatures;

    QMap<uint, ContactListChannel> contactListsChannels;
    ChannelPtr subscribeChannel;
    ChannelPtr publishChannel;
    ChannelPtr storedChannel;
    ChannelPtr denyChannel;

    Contacts allKnownContacts() const;
    void updateContactsPresenceState();
};

Connection *ContactManager::connection() const
{
    return mPriv->conn;
}

bool ContactManager::isSupported() const
{
    if (!connection()->isReady()) {
        warning() << "ContactManager::isSupported() used before the connection is ready!";
        return false;
    } /* FIXME: readd this check when Connection is no longer a steaming pile of junk: else if (connection()->status() != Connection::StatusConnected) {
        warning() << "ContactManager::isSupported() used before the connection is connected!";
        return false;
    } */

    return connection()->interfaces().contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS);
}

namespace
{
QString featureToInterface(Contact::Feature feature)
{
    switch (feature) {
        case Contact::FeatureAlias:
            return TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING;
        case Contact::FeatureAvatarToken:
            return TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS;
        case Contact::FeatureSimplePresence:
            return TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE;
        default:
            warning() << "ContactManager doesn't know which interface corresponds to feature"
                << feature;
            return QString();
    }
}
}

QSet<Contact::Feature> ContactManager::supportedFeatures() const
{
    if (!isSupported()) {
        warning() << "ContactManager::supportedFeatures() used with the entire ContactManager"
            << "functionality being unsupported, returning an empty set";
        return QSet<Contact::Feature>();
    }

    if (mPriv->supportedFeatures.isEmpty()) {
        QList<Contact::Feature> allFeatures = QList<Contact::Feature>()
            << Contact::FeatureAlias
            << Contact::FeatureAvatarToken
            << Contact::FeatureSimplePresence;
        QStringList interfaces = mPriv->conn->contactAttributeInterfaces();

        foreach (Contact::Feature feature, allFeatures) {
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
    return mPriv->allKnownContacts();
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
    return mPriv->subscribeChannel &&
        mPriv->subscribeChannel->groupCanAddContacts();
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
    return mPriv->subscribeChannel &&
        (mPriv->subscribeChannel->groupFlags() &
         Telepathy::ChannelGroupFlagMessageAdd);
}

/**
 * Attempt to subscribe to the presence of the given contacts.
 *
 * This operation is sometimes called "adding contacts to the buddy
 * list" or "requesting authorization".
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
    if (!mPriv->subscribeChannel) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Cannot subscribe to contacts' presence on this protocol");
    }

    return mPriv->subscribeChannel->groupAddContacts(contacts, message);
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
    return mPriv->subscribeChannel &&
        mPriv->subscribeChannel->groupCanRemoveContacts();
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
    return mPriv->subscribeChannel &&
        (mPriv->subscribeChannel->groupFlags() &
         Telepathy::ChannelGroupFlagMessageRemove);
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
    return mPriv->subscribeChannel &&
        mPriv->subscribeChannel->groupCanRescindContacts();
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
    return mPriv->subscribeChannel &&
        (mPriv->subscribeChannel->groupFlags() &
         Telepathy::ChannelGroupFlagMessageRescind);
}

/**
 * Attempt to stop receiving the presence of the given contacts, or cancel
 * a request to subscribe to their presence that was previously sent.
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
    if (!mPriv->subscribeChannel) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Cannot subscribe to contacts' presence on this protocol");
    }

    return mPriv->subscribeChannel->groupRemoveContacts(contacts, message);
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
    // do not check for Channel::groupCanAddContacts as all contacts in local
    // pending can be added, even if the Channel::groupFlags() does not contain
    // the flag CanAdd
    return (bool) mPriv->publishChannel;
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
    return mPriv->subscribeChannel &&
        (mPriv->subscribeChannel->groupFlags() &
         Telepathy::ChannelGroupFlagMessageAccept);
}

/**
 * If the given contacts have asked the user to publish presence to them,
 * grant permission for this publication to take place.
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
    if (!mPriv->publishChannel) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Cannot control publication of presence on this protocol");
    }

    return mPriv->publishChannel->groupAddContacts(contacts, message);
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
    return mPriv->subscribeChannel &&
        (mPriv->subscribeChannel->groupFlags() &
         Telepathy::ChannelGroupFlagMessageReject);
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
    return mPriv->publishChannel &&
        mPriv->publishChannel->groupCanRemoveContacts();
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
    return mPriv->subscribeChannel &&
        (mPriv->subscribeChannel->groupFlags() &
         Telepathy::ChannelGroupFlagMessageRemove);
}

/**
 * If the given contacts have asked the user to publish presence to them,
 * deny this request (this should always succeed, unless a network error
 * occurs).
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
    if (!mPriv->publishChannel) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Cannot control publication of presence on this protocol");
    }

    return mPriv->publishChannel->groupRemoveContacts(contacts, message);
}

/**
 * Return whether this protocol has a list of blocked contacts.
 *
 * \return Whether blockContacts is likely to succeed
 */
bool ContactManager::canBlockContacts() const
{
    return (bool) mPriv->denyChannel;
}

/**
 * Set whether the given contacts are blocked. Blocked contacts cannot send
 * messages to the user; depending on the protocol, blocking a contact may
 * have other effects.
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
    if (!mPriv->denyChannel) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Cannot block contacts on this protocol");
    }

    if (value) {
        return mPriv->denyChannel->groupAddContacts(contacts);
    }
    else {
        return mPriv->denyChannel->groupRemoveContacts(contacts);
    }
}

PendingContacts *ContactManager::contactsForHandles(const UIntList &handles,
        const QSet<Contact::Feature> &features)
{
    debug() << "Building contacts for" << handles.size() << "handles" << "with" << features.size()
        << "features";

    QMap<uint, ContactPtr> satisfyingContacts;
    QSet<uint> otherContacts;
    QSet<Contact::Feature> missingFeatures;

    foreach (uint handle, handles) {
        ContactPtr contact = lookupContactByHandle(handle);
        if (contact) {
            if ((features - contact->requestedFeatures()).isEmpty()) {
                // Contact exists and has all the requested features
                satisfyingContacts.insert(handle, contact);
            } else {
                // Contact exists but is missing features
                otherContacts.insert(handle);
                missingFeatures.unite(features - contact->requestedFeatures());
            }
        } else {
            // Contact doesn't exist - we need to get all of the features (same as unite(features))
            missingFeatures = features;
            otherContacts.insert(handle);
        }
    }

    debug() << " " << satisfyingContacts.size() << "satisfying and"
                   << otherContacts.size() << "other contacts";
    debug() << " " << missingFeatures.size() << "features missing";

    QSet<Contact::Feature> supported = supportedFeatures();
    QSet<QString> interfaces;
    foreach (Contact::Feature feature, missingFeatures) {
        mPriv->ensureTracking(feature);

        if (supported.contains(feature)) {
            // Only query interfaces which are reported as supported to not get an error
            interfaces.insert(featureToInterface(feature));
        }
    }

    PendingContacts *contacts =
        new PendingContacts(this, handles, features, satisfyingContacts);

    if (!otherContacts.isEmpty()) {
        debug() << " Fetching" << interfaces.size() << "interfaces for"
                               << otherContacts.size() << "contacts";

        PendingContactAttributes *attributes =
            mPriv->conn->getContactAttributes(otherContacts.toList(), interfaces.toList(), true);

        contacts->connect(attributes,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(onAttributesFinished(Telepathy::Client::PendingOperation*)));
    } else {
        contacts->allAttributesFetched();
    }

    return contacts;
}

PendingContacts *ContactManager::contactsForHandles(const ReferencedHandles &handles,
        const QSet<Contact::Feature> &features)
{
    return contactsForHandles(handles.toList(), features);
}

PendingContacts *ContactManager::contactsForIdentifiers(const QStringList &identifiers,
        const QSet<Contact::Feature> &features)
{
    debug() << "Building contacts for" << identifiers.size() << "identifiers" << "with" << features.size()
        << "features";

    PendingHandles *handles = mPriv->conn->requestHandles(HandleTypeContact, identifiers);

    PendingContacts *contacts = new PendingContacts(this, identifiers, features);
    contacts->connect(handles,
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onHandlesFinished(Telepathy::Client::PendingOperation*)));

    return contacts;
}

PendingContacts *ContactManager::upgradeContacts(const QList<ContactPtr> &contacts,
        const QSet<Contact::Feature> &features)
{
    debug() << "Upgrading" << contacts.size() << "contacts to have at least"
                           << features.size() << "features";

    return new PendingContacts(this, contacts, features);
}

void ContactManager::onAliasesChanged(const Telepathy::AliasPairList &aliases)
{
    debug() << "Got AliasesChanged for" << aliases.size() << "contacts";

    foreach (AliasPair pair, aliases) {
        ContactPtr contact = lookupContactByHandle(pair.handle);

        if (contact) {
            contact->receiveAlias(pair.alias);
        }
    }
}

void ContactManager::onAvatarUpdated(uint handle, const QString &token)
{
    debug() << "Got AvatarUpdate for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);
    if (contact) {
        contact->receiveAvatarToken(token);
    }
}

void ContactManager::onPresencesChanged(const Telepathy::SimpleContactPresences &presences)
{
    debug() << "Got PresencesChanged for" << presences.size() << "contacts";

    foreach (uint handle, presences.keys()) {
        ContactPtr contact = lookupContactByHandle(handle);

        if (contact) {
            contact->receiveSimplePresence(presences[handle]);
        }
    }
}

void ContactManager::onSubscribeChannelMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    Q_UNUSED(details);

    if (!groupLocalPendingMembersAdded.isEmpty()) {
        warning() << "Found local pending contacts on subscribe list";
    }

    foreach (ContactPtr contact, groupMembersAdded) {
        debug() << "Contact" << contact->id() << "on subscribe list";
        contact->setSubscriptionState(Contact::PresenceStateYes);
    }

    foreach (ContactPtr contact, groupRemotePendingMembersAdded) {
        debug() << "Contact" << contact->id() << "added to subscribe list";
        contact->setSubscriptionState(Contact::PresenceStateAsk);
    }

    foreach (ContactPtr contact, groupMembersRemoved) {
        debug() << "Contact" << contact->id() << "removed from subscribe list";
        contact->setSubscriptionState(Contact::PresenceStateNo);
    }
}

void ContactManager::onPublishChannelMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    Q_UNUSED(details);

    if (!groupRemotePendingMembersAdded.isEmpty()) {
        warning() << "Found remote pending contacts on publish list";
    }

    foreach (ContactPtr contact, groupMembersAdded) {
        debug() << "Contact" << contact->id() << "on publish list";
        contact->setPublishState(Contact::PresenceStateYes);
    }

    foreach (ContactPtr contact, groupLocalPendingMembersAdded) {
        debug() << "Contact" << contact->id() << "added to publish list";
        contact->setPublishState(Contact::PresenceStateAsk);
    }

    foreach (ContactPtr contact, groupMembersRemoved) {
        debug() << "Contact" << contact->id() << "removed from publish list";
        contact->setPublishState(Contact::PresenceStateNo);
    }

    if (!groupLocalPendingMembersAdded.isEmpty()) {
        emit presencePublicationRequested(groupLocalPendingMembersAdded);
    }
}

void ContactManager::onDenyChannelMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    Q_UNUSED(details);

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

ContactManager::ContactManager(Connection *parent)
    : QObject(parent), mPriv(new Private)
{
    mPriv->conn = parent;
}

ContactManager::~ContactManager()
{
    delete mPriv;
}

ContactPtr ContactManager::ensureContact(const ReferencedHandles &handle,
        const QSet<Contact::Feature> &features, const QVariantMap &attributes)
{
    uint bareHandle = handle[0];
    ContactPtr contact = lookupContactByHandle(bareHandle);

    if (!contact) {
        contact = ContactPtr(new Contact(this, handle, features, attributes));
        mPriv->contacts.insert(bareHandle, contact);
    } else {
        contact->augment(features, attributes);
    }

    return contact;
}

void ContactManager::setContactListChannels(
        const QMap<uint, ContactListChannel> &contactListsChannels)
{
    Q_ASSERT(mPriv->contactListsChannels.isEmpty());
    mPriv->contactListsChannels = contactListsChannels;

    if (mPriv->contactListsChannels.contains(ContactListChannel::TypeSubscribe)) {
        mPriv->subscribeChannel = mPriv->contactListsChannels[ContactListChannel::TypeSubscribe].channel;
    }

    if (mPriv->contactListsChannels.contains(ContactListChannel::TypePublish)) {
        mPriv->publishChannel = mPriv->contactListsChannels[ContactListChannel::TypePublish].channel;
    }

    if (mPriv->contactListsChannels.contains(ContactListChannel::TypeStored)) {
        mPriv->storedChannel = mPriv->contactListsChannels[ContactListChannel::TypeStored].channel;
    }

    if (mPriv->contactListsChannels.contains(ContactListChannel::TypeDeny)) {
        mPriv->denyChannel = mPriv->contactListsChannels[ContactListChannel::TypeDeny].channel;
    }

    mPriv->updateContactsPresenceState();

    QMap<uint, ContactListChannel>::const_iterator i = contactListsChannels.constBegin();
    QMap<uint, ContactListChannel>::const_iterator end = contactListsChannels.constEnd();
    uint type;
    ChannelPtr channel;
    const char *method;
    while (i != end) {
        type = i.key();
        channel = i.value().channel;
        ++i;
        if (!channel) {
            continue;
        }

        if (type == ContactListChannel::TypeSubscribe) {
            method = SLOT(onSubscribeChannelMembersChanged(
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Channel::GroupMemberChangeDetails &));
        } else if (type == ContactListChannel::TypePublish) {
            method = SLOT(onPublishChannelMembersChanged(
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Channel::GroupMemberChangeDetails &));
        } else if (type == ContactListChannel::TypeDeny) {
            method = SLOT(onDenyChannelMembersChanged(
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Channel::GroupMemberChangeDetails &));
        } else {
            continue;
        }

        connect(channel.data(),
                SIGNAL(groupMembersChanged(
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Contacts &,
                        const Telepathy::Client::Channel::GroupMemberChangeDetails &)),
                method);
    }
}

ContactPtr ContactManager::lookupContactByHandle(uint handle)
{
    ContactPtr contact;

    if (mPriv->contacts.contains(handle)) {
        contact = mPriv->contacts.value(handle).toStrongRef();
        if (!contact) {
            // Dangling weak pointer, remove it
            mPriv->contacts.remove(handle);
        }
    }

    return contact;
}

void ContactManager::Private::ensureTracking(Contact::Feature feature)
{
    if (tracking[feature])
        return;

    switch (feature) {
        case Contact::FeatureAlias:
            QObject::connect(
                    conn->aliasingInterface(),
                    SIGNAL(AliasesChanged(const Telepathy::AliasPairList &)),
                    conn->contactManager(),
                    SLOT(onAliasesChanged(const Telepathy::AliasPairList &)));
            break;

        case Contact::FeatureAvatarToken:
            QObject::connect(
                    conn->avatarsInterface(),
                    SIGNAL(AvatarUpdated(uint, const QString &)),
                    conn->contactManager(),
                    SLOT(onAvatarUpdated(uint, const QString &)));
            break;

        case Contact::FeatureSimplePresence:
            QObject::connect(
                    conn->simplePresenceInterface(),
                    SIGNAL(PresencesChanged(const Telepathy::SimpleContactPresences &)),
                    conn->contactManager(),
                    SLOT(onPresencesChanged(const Telepathy::SimpleContactPresences &)));
            break;

        default:
            warning() << " Unknown feature" << feature
                << "when trying to figure out how to connect change notification!";
            return;
    }

    tracking[feature] = true;
}

Contacts ContactManager::Private::allKnownContacts() const
{
    Contacts contacts;
    foreach (const ContactListChannel &contactListChannel, contactListsChannels) {
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

void ContactManager::Private::updateContactsPresenceState()
{
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

    Contacts denyContacts;
    if (denyChannel) {
        denyContacts = denyChannel->groupContacts();
    }

    if (!subscribeChannel && !publishChannel && !denyChannel) {
        return;
    }

    Contacts contacts = allKnownContacts();
    foreach (ContactPtr contact, contacts) {
        if (subscribeChannel) {
            // not in "subscribe" -> No, in "subscribe" lp -> Ask, in "subscribe" current -> Yes
            if (subscribeContacts.contains(contact)) {
                contact->setSubscriptionState(Contact::PresenceStateYes);
            } else if (subscribeContactsRP.contains(contact)) {
                contact->setSubscriptionState(Contact::PresenceStateAsk);
            } else {
                contact->setSubscriptionState(Contact::PresenceStateNo);
            }
        }

        if (publishChannel) {
            // not in "publish" -> No, in "subscribe" rp -> Ask, in "publish" current -> Yes
            if (publishContacts.contains(contact)) {
                contact->setPublishState(Contact::PresenceStateYes);
            } else if (publishContactsLP.contains(contact)) {
                contact->setPublishState(Contact::PresenceStateAsk);
            } else {
                contact->setPublishState(Contact::PresenceStateNo);
            }
        }

        if (denyChannel) {
            if (denyContacts.contains(contact)) {
                contact->setBlocked(true);
            }
        }
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


}
}
