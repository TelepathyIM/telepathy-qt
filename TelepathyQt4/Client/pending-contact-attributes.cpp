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

#include "pending-contact-attributes.h"
#include "_gen/pending-contact-attributes.moc.hpp"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ReferencedHandles>

#include "../debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct PendingContactAttributes::Private
{
    Connection *connection;
    UIntList contactsRequested;
    QStringList interfacesRequested;
    bool shouldReference;
    ReferencedHandles validHandles;
    UIntList invalidHandles;
    ContactAttributesMap attributes;
};

PendingContactAttributes::PendingContactAttributes(Connection* connection, const UIntList &handles,
        const QStringList &interfaces, bool reference)
    : PendingOperation(connection), mPriv(new Private)
{
    debug() << "PendingContactAttributes()";

    mPriv->connection = connection;
    mPriv->contactsRequested = handles;
    mPriv->interfacesRequested = interfaces;
    mPriv->shouldReference = reference;
}

PendingContactAttributes::~PendingContactAttributes()
{
    delete mPriv;
}

Connection* PendingContactAttributes::connection() const
{
    return mPriv->connection;
}

const UIntList &PendingContactAttributes::contactsRequested() const
{
    return mPriv->contactsRequested;
}

const QStringList &PendingContactAttributes::interfacesRequested() const
{
    return mPriv->interfacesRequested;
}

bool PendingContactAttributes::shouldReference() const
{
    return mPriv->shouldReference;
}

ReferencedHandles PendingContactAttributes::validHandles() const
{
    if (!isFinished()) {
        warning() << "PendingContactAttributes::validHandles() called before finished";
    } else if (isError()) {
        warning() << "PendingContactAttributes::validHandles() called when errored";
    } else if (!shouldReference()) {
        warning() << "PendingContactAttributes::validHandles() called but weren't asked to"
                  << "reference handles";
    }

    return mPriv->validHandles;
}

UIntList PendingContactAttributes::invalidHandles() const
{
    if (!isFinished()) {
        warning() << "PendingContactAttributes::validHandles() called before finished";
    } else if (isError()) {
        warning() << "PendingContactAttributes::validHandles() called when errored";
    }

    return mPriv->invalidHandles;
}

ContactAttributesMap PendingContactAttributes::attributes() const
{
    if (!isFinished()) {
        warning() << "PendingContactAttributes::validHandles() called before finished";
    } else if (isError()) {
        warning() << "PendingContactAttributes::validHandles() called when errored";
    }

    return mPriv->attributes;
}

void PendingContactAttributes::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<ContactAttributesMap> reply = *watcher;

    debug() << "Received reply to GetContactAttributes";

    if (reply.isError()) {
        debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    } else {
        mPriv->attributes = reply.value();

        UIntList validHandles;
        foreach (uint contact, mPriv->contactsRequested) {
            if (mPriv->attributes.contains(contact)) {
                validHandles << contact;
            } else {
                mPriv->invalidHandles << contact;
            }
        }

        if (shouldReference()) {
            mPriv->validHandles = ReferencedHandles(connection(), HandleTypeContact, validHandles);
        }

        debug() << " Success:" << validHandles.size() << "valid and" << mPriv->invalidHandles.size()
            << "invalid handles";
        
        setFinished();
    }

    connection()->handleRequestLanded(HandleTypeContact);
    watcher->deleteLater();
}

void PendingContactAttributes::setUnsupported()
{
    setFinishedWithError(TELEPATHY_ERROR_NOT_IMPLEMENTED,
            "The remote object doesn't report the Contacts interface as supported");
}

} // Telepathy::Client
} // Telepathy
