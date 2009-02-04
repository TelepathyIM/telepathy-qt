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
    QSharedPointer<Contact> lookupContactByHandle(uint handle);

    QMap<Contact::Feature, bool> tracking;
    void ensureTracking(Contact::Feature feature);
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
    } else if (connection()->status() != Connection::StatusConnected) {
        warning() << "ContactManager::isSupported() used before the connection is connected!";
        return false;
    }

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

PendingContacts *ContactManager::contactsForHandles(const UIntList &handles,
        const QSet<Contact::Feature> &features)
{
    debug() << "Building contacts for" << handles.size() << "handles" << "with" << features.size()
        << "features";

    QMap<uint, QSharedPointer<Contact> > satisfyingContacts;
    QSet<uint> otherContacts;
    QSet<Contact::Feature> missingFeatures;

    foreach (uint handle, handles) {
        QSharedPointer<Contact> contact = mPriv->lookupContactByHandle(handle);
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

    QSet<QString> interfaces;
    foreach (Contact::Feature feature, missingFeatures) {
        interfaces.insert(featureToInterface(feature));
        mPriv->ensureTracking(feature);
    }

    // TODO: filter using conn->contactAttributeInterfaces() when I add that

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

PendingContacts *ContactManager::upgradeContacts(const QList<QSharedPointer<Contact> > &contacts,
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
        QSharedPointer<Contact> contact = mPriv->lookupContactByHandle(pair.handle);

        if (contact) {
            contact->receiveAlias(pair.alias);
        }
    }
}

void ContactManager::onAvatarUpdated(uint handle, const QString &token)
{
    debug() << "Got AvatarUpdate for contact with handle" << handle;

    QSharedPointer<Contact> contact = mPriv->lookupContactByHandle(handle);
    if (contact) {
        contact->receiveAvatarToken(token);
    }
}

void ContactManager::onPresencesChanged(const Telepathy::SimpleContactPresences &presences)
{
    debug() << "Got PresencesChanged for" << presences.size() << "contacts";

    foreach (uint handle, presences.keys()) {
        QSharedPointer<Contact> contact = mPriv->lookupContactByHandle(handle);

        if (contact) {
            contact->receiveSimplePresence(presences[handle]);
        }
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

QSharedPointer<Contact> ContactManager::ensureContact(const ReferencedHandles &handle,
        const QSet<Contact::Feature> &features, const QVariantMap &attributes) {
    uint bareHandle = handle[0];
    QSharedPointer<Contact> contact = mPriv->lookupContactByHandle(bareHandle);

    if (!contact) {
        contact = QSharedPointer<Contact>(new Contact(this, handle, features, attributes));
        mPriv->contacts.insert(bareHandle, contact);
    } else {
        contact->augment(features, attributes);
    }

    return contact;
}

QSharedPointer<Contact> ContactManager::Private::lookupContactByHandle(uint handle)
{
    QSharedPointer<Contact> contact;

    if (contacts.contains(handle)) {
        contact = contacts.value(handle).toStrongRef();
        if (!contact) {
            // Dangling weak pointer, remove it
            contacts.remove(handle);
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

}
}
