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
        // TODO: when more features are added, add the translation here too
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
        QSharedPointer<Contact> contact = mPriv->contacts[handle].toStrongRef();
        if (contact) {
            if (true /* TODO: contact->hasFeatures(features) */) {
                // Contact exists and has all the requested features
                satisfyingContacts.insert(handle, contact);
            } else {
                // Contact exists but is missing features
                otherContacts.insert(handle);
                missingFeatures.unite(features /* TODO:- contact->features() */);
            }
        } else {
            // Contact doesn't exist - we need to get all of the features (same as unite(features))
            missingFeatures = features;
            // It might be a weak pointer with 0 refs, make sure to remove it
            mPriv->contacts.remove(handle);
            // In either case, the contact needs to be fetched
            otherContacts.insert(handle);
        }
    }

    debug() << " " << satisfyingContacts.size() << "satisfying and"
                   << otherContacts.size() << "other contacts";
    debug() << " " << missingFeatures.size() << "features missing";

    QSet<QString> interfaces;
    foreach (Contact::Feature feature, missingFeatures) {
        interfaces.insert(featureToInterface(feature));
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
        const QVariantMap &attributes) {
    uint bareHandle = handle[0];
    QSharedPointer<Contact> contact = mPriv->contacts.value(bareHandle).toStrongRef();

    if (!contact) {
        contact = QSharedPointer<Contact>(new Contact(this, handle, attributes));
        mPriv->contacts.insert(bareHandle, contact);
    } else {
        contact->augment(attributes);
    }

    return contact;
}

}
}
