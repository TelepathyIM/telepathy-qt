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

#include <TelepathyQt4/Contact>
#include "TelepathyQt4/_gen/contact.moc.hpp"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Constants>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct Contact::Private
{
    Private(ContactManager *manager, const ReferencedHandles &handle)
        : manager(manager), handle(handle), isAvatarTokenKnown(false),
          subscriptionState(PresenceStateNo), publishState(PresenceStateNo),
          blocked(false)
    {
    }

    ContactManager *manager;
    ReferencedHandles handle;
    QString id;

    QSet<Feature> requestedFeatures;
    QSet<Feature> actualFeatures;

    QString alias;
    bool isAvatarTokenKnown;
    QString avatarToken;
    SimplePresence simplePresence;

    PresenceState subscriptionState;
    PresenceState publishState;
    bool blocked;

    QSet<QString> groups;
};

ContactManager *Contact::manager() const
{
    return mPriv->manager;
}

ReferencedHandles Contact::handle() const
{
    return mPriv->handle;
}

QString Contact::id() const
{
    return mPriv->id;
}

QSet<Contact::Feature> Contact::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

QSet<Contact::Feature> Contact::actualFeatures() const
{
    return mPriv->actualFeatures;
}

QString Contact::alias() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAlias)) {
        warning() << "Contact::alias() used on" << this
            << "for which FeatureAlias hasn't been requested - returning id";
        return id();
    }

    return mPriv->alias;
}

bool Contact::isAvatarTokenKnown() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        warning() << "Contact::isAvatarTokenKnown() used on" << this
            << "for which FeatureAvatarToken hasn't been requested - returning false";
        return false;
    }

    return mPriv->isAvatarTokenKnown;
}

QString Contact::avatarToken() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        warning() << "Contact::avatarToken() used on" << this
            << "for which FeatureAvatarToken hasn't been requested - returning \"\"";
        return QString("");
    } else if (!isAvatarTokenKnown()) {
        warning() << "Contact::avatarToken() used on" << this
            << "for which the avatar token is not (yet) known - returning \"\"";
        return QString("");
    }

    return mPriv->avatarToken;
}

QString Contact::presenceStatus() const
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        warning() << "Contact::presenceStatus() used on" << this
            << "for which FeatureSimplePresence hasn't been requested - returning \"unknown\"";
        return QString("unknown");
    }

    return mPriv->simplePresence.status;
}

uint Contact::presenceType() const
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        warning() << "Contact::presenceType() used on" << this
            << "for which FeatureSimplePresence hasn't been requested - returning Unknown";
        return ConnectionPresenceTypeUnknown;
    }

    return mPriv->simplePresence.type;
}

QString Contact::presenceMessage() const
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        warning() << "Contact::presenceMessage() used on" << this
            << "for which FeatureSimplePresence hasn't been requested - returning \"\"";
        return QString("");
    }

    return mPriv->simplePresence.statusMessage;
}

Contact::PresenceState Contact::subscriptionState() const
{
    return mPriv->subscriptionState;
}

Contact::PresenceState Contact::publishState() const
{
    return mPriv->publishState;
}

PendingOperation *Contact::requestPresenceSubscription(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->requestPresenceSubscription(
            QList<ContactPtr >() << self,
            message);
}

PendingOperation *Contact::removePresenceSubscription(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->removePresenceSubscription(
            QList<ContactPtr>() << self,
            message);
}

PendingOperation *Contact::authorizePresencePublication(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->authorizePresencePublication(
            QList<ContactPtr>() << self,
            message);
}

PendingOperation *Contact::removePresencePublication(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->removePresencePublication(
            QList<ContactPtr>() << self,
            message);
}

bool Contact::isBlocked() const
{
    return mPriv->blocked;
}

PendingOperation *Contact::block(bool value)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->blockContacts(
            QList<ContactPtr>() << self,
            value);
}

QStringList Contact::groups() const
{
    return mPriv->groups.toList();
}

PendingOperation *Contact::addToGroup(const QString &group)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->groupAddContacts(
            group, QList<ContactPtr>() << self);
}

PendingOperation *Contact::removeFromGroup(const QString &group)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->groupRemoveContacts(
            group, QList<ContactPtr>() << self);
}

Contact::~Contact()
{
    debug() << "Contact" << id() << "destroyed";
    delete mPriv;
}

Contact::Contact(ContactManager *manager, const ReferencedHandles &handle,
        const QSet<Feature> &requestedFeatures, const QVariantMap &attributes)
    : QObject(0), mPriv(new Private(manager, handle))
{
    augment(requestedFeatures, attributes);
}

void Contact::augment(const QSet<Feature> &requestedFeatures, const QVariantMap &attributes) {
    mPriv->requestedFeatures.unite(requestedFeatures);

    mPriv->id = qdbus_cast<QString>(attributes[TELEPATHY_INTERFACE_CONNECTION "/contact-id"]);

    foreach (Feature feature, requestedFeatures) {
        QString maybeAlias;
        SimplePresence maybePresence;

        switch (feature) {
            case FeatureAlias:
                maybeAlias = qdbus_cast<QString>(attributes.value(
                            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING "/alias"));

                if (!maybeAlias.isEmpty()) {
                    receiveAlias(maybeAlias);
                } else if (mPriv->alias.isEmpty()) {
                    mPriv->alias = mPriv->id;
                }
                break;

            case FeatureAvatarToken:
                if (attributes.contains(
                            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS "/token")) {
                    receiveAvatarToken(qdbus_cast<QString>(attributes.value(
                                    TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS "/token")));
                } else {
                    if (manager()->supportedFeatures().contains(FeatureAvatarToken)) {
                        // AvatarToken being supported but not included in the mapping indicates
                        // that the avatar token is not known - however, the feature is working fine
                        mPriv->actualFeatures.insert(FeatureAvatarToken);
                    }
                    // In either case, the avatar token can't be known
                    mPriv->isAvatarTokenKnown = false;
                    mPriv->avatarToken = "";
                }
                break;

            case FeatureSimplePresence:
                maybePresence = qdbus_cast<SimplePresence>(attributes.value(
                            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE "/presence"));

                if (!maybePresence.status.isEmpty()) {
                    receiveSimplePresence(maybePresence);
                } else {
                    mPriv->simplePresence.type = ConnectionPresenceTypeUnknown;
                    mPriv->simplePresence.status = "unknown";
                    mPriv->simplePresence.statusMessage = "";
                }
                break;

            default:
                warning() << "Unknown feature" << feature << "encountered when augmenting Contact";
                break;
        }
    }
}

void Contact::receiveAlias(const QString &alias)
{
    if (!mPriv->requestedFeatures.contains(FeatureAlias)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureAlias);

    if (mPriv->alias != alias) {
        mPriv->alias = alias;
        emit aliasChanged(alias);
    }
}

void Contact::receiveAvatarToken(const QString &token)
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureAvatarToken);

    if (!mPriv->isAvatarTokenKnown || mPriv->avatarToken != token) {
        mPriv->isAvatarTokenKnown = true;
        mPriv->avatarToken = token;
        emit avatarTokenChanged(token);
    }
}

void Contact::receiveSimplePresence(const SimplePresence &presence)
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureSimplePresence);

    if (mPriv->simplePresence.status != presence.status
            || mPriv->simplePresence.statusMessage != presence.statusMessage) {
        mPriv->simplePresence = presence;
        emit simplePresenceChanged(presenceStatus(), presenceType(), presenceMessage());
    }
}

void Contact::setSubscriptionState(Contact::PresenceState state)
{
    if (mPriv->subscriptionState == state) {
        return;
    }
    mPriv->subscriptionState = state;
    emit subscriptionStateChanged(state);
}

void Contact::setPublishState(Contact::PresenceState state)
{
    if (mPriv->publishState == state) {
        return;
    }
    mPriv->publishState = state;
    emit publishStateChanged(state);
}

void Contact::setBlocked(bool value)
{
    if (mPriv->blocked == value) {
        return;
    }
    mPriv->blocked = value;
    emit blockStatusChanged(value);
}

void Contact::setAddedToGroup(const QString &group)
{
    if (!mPriv->groups.contains(group)) {
        mPriv->groups.insert(group);
        emit addedToGroup(group);
    }
}

void Contact::setRemovedFromGroup(const QString &group)
{
    if (mPriv->groups.remove(group)) {
        emit removedFromGroup(group);
    }
}

} // Tp
