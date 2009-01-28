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

#include <TelepathyQt4/Client/PendingContacts>
#include "TelepathyQt4/Client/_gen/pending-contacts.moc.hpp"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingContactAttributes>
#include <TelepathyQt4/Client/ReferencedHandles>
#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct PendingContacts::Private
{
    Private(Connection *connection, const UIntList &handles, const QSet<Contact::Feature> &features)
        : connection(connection), handles(handles), features(features)
    {
    }

    Connection *connection;
    UIntList handles;
    QSet<Contact::Feature> features;

    QList<QSharedPointer<Contact> > contacts;
};

PendingContacts::PendingContacts(Connection *connection,
        const UIntList &handles, const QSet<Contact::Feature> &features)
    : PendingOperation(connection),
      mPriv(new Private(connection, handles, features))
{
}

/**
 * Class destructor.
 */
PendingContacts::~PendingContacts()
{
    delete mPriv;
}

ContactManager *PendingContacts::contactManager() const
{
    return mPriv->connection->contactManager();
}

UIntList PendingContacts::handles() const
{
    return mPriv->handles;
}

QSet<Contact::Feature> PendingContacts::features() const
{
    return mPriv->features;
}

QList<QSharedPointer<Contact> > PendingContacts::contacts() const
{
    if (!isFinished()) {
        warning() << "PendingContacts::contacts() called before finished";
    } else if (isError()) {
        warning() << "PendingContacts::contacts() called when errored";
    }

    return mPriv->contacts;
}

void PendingContacts::onAttributesFinished(PendingOperation *operation)
{
    PendingContactAttributes *pendingAttributes =
        qobject_cast<PendingContactAttributes *>(operation);

    debug() << "Attributes finished for" << this;

    if (pendingAttributes->isError()) {
        debug() << " error" << pendingAttributes->errorName()
                << "message" << pendingAttributes->errorMessage();
        setFinishedWithError(pendingAttributes->errorName(), pendingAttributes->errorMessage());
        return;
    }

    ReferencedHandles validHandles = pendingAttributes->validHandles();
    ContactAttributesMap attributes = pendingAttributes->attributes();
    debug() << " Success:" << validHandles.size() << "valid handles";

    for (int i = 0; i < validHandles.size(); i++) {
        uint handle = validHandles[i];
        ReferencedHandles referenced = validHandles.mid(i, 1);
        mPriv->contacts.push_back(QSharedPointer<Contact>(new Contact(mPriv->connection, referenced,
                        attributes[handle])));
    }

    setFinished();
}

} // Telepathy::Client
} // Telepathy
