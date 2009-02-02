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

#include <TelepathyQt4/Client/Contact>
#include "TelepathyQt4/Client/_gen/contact.moc.hpp"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/ReferencedHandles>
#include <TelepathyQt4/Constants>

#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct Contact::Private
{
    Private(ContactManager *manager, const ReferencedHandles &handle)
        : manager(manager), handle(handle)
    {
    }

    ContactManager *manager;
    ReferencedHandles handle;
    QString id;

    QSet<Feature> requestedFeatures;
    QSet<Feature> actualFeatures;

    SimplePresence simplePresence;
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

QString Contact::presenceStatus() const
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        warning() << "Contact::presenceStatus() used on" << this
            << "for which FeatureSimplePresence hasn't been requested - returning \"unknown\"";
        return QString("");
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

Contact::~Contact()
{
    delete mPriv;
}

Contact::Contact(ContactManager *manager, const ReferencedHandles &handle,
        const QSet<Feature> &requestedFeatures, const QVariantMap &attributes)
    : QObject(0), mPriv(new Private(manager, handle))
{
    mPriv->simplePresence.type = ConnectionPresenceTypeUnknown;
    mPriv->simplePresence.status = "unknown";
    mPriv->simplePresence.statusMessage = "";

    augment(requestedFeatures, attributes);
}

void Contact::augment(const QSet<Feature> &requestedFeatures, const QVariantMap &attributes) {
    mPriv->requestedFeatures.unite(requestedFeatures);

    mPriv->id = qdbus_cast<QString>(attributes[TELEPATHY_INTERFACE_CONNECTION "/contact-id"]);

    foreach (Feature feature, requestedFeatures) {
        SimplePresence maybePresence;

        switch (feature) {
            case FeatureSimplePresence:
                maybePresence = qdbus_cast<SimplePresence>(attributes.value(
                            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE "/presence"));

                if (!maybePresence.status.isEmpty()) {
                    mPriv->simplePresence = maybePresence;
                    mPriv->actualFeatures.insert(FeatureSimplePresence);
                }
                break;

            default:
                warning() << "Unknown feature" << feature << "encountered when augmenting Contact";
                break;
        }
    }
}

} // Telepathy::Client
} // Telepathy
