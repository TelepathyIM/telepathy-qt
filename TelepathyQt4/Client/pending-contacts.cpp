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
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingContactAttributes>
#include <TelepathyQt4/Client/PendingHandles>
#include <TelepathyQt4/Client/ReferencedHandles>
#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct PendingContacts::Private
{
    Private(Connection *connection, const UIntList &handles, const QSet<Contact::Feature> &features)
        : connection(connection),
          features(features),
          isForIdentifiers(false),
          handles(handles),
          nested(0)
    {
    }

    Private(Connection *connection, const QStringList &identifiers,
            const QSet<Contact::Feature> &features)
        : connection(connection),
          features(features),
          isForIdentifiers(true),
          identifiers(identifiers),
          nested(0)
    {
    }

    Connection *connection;
    QSet<Contact::Feature> features;

    bool isForIdentifiers;
    UIntList handles;
    QStringList identifiers;
    PendingContacts *nested;

    QList<QSharedPointer<Contact> > contacts;
};

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

QSet<Contact::Feature> PendingContacts::features() const
{
    return mPriv->features;
}

bool PendingContacts::isForHandles() const
{
    return !isForIdentifiers();
}

UIntList PendingContacts::handles() const
{
    if (!isForHandles()) {
        warning() << "Tried to get handles from" << this << "which is for identifiers!";
    }

    return mPriv->handles;
}

bool PendingContacts::isForIdentifiers() const
{
    return mPriv->isForIdentifiers;
}

QStringList PendingContacts::identifiers() const
{
    if (!isForIdentifiers()) {
        warning() << "Tried to get identifiers from" << this << "which is for handles!";
    }

    return mPriv->identifiers;
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

void PendingContacts::onHandlesFinished(PendingOperation *operation)
{
    PendingHandles *pendingHandles = qobject_cast<PendingHandles *>(operation);

    debug() << "Handles finished for" << this;

    if (pendingHandles->isError()) {
        debug() << " error" << operation->errorName()
                << "message" << operation->errorMessage();
        setFinishedWithError(operation->errorName(), operation->errorMessage());
        return;
    }

    debug() << " Success - doing nested contact query";

    mPriv->nested = contactManager()->contactsForHandles(pendingHandles->handles(), features());
    connect(mPriv->nested,
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onNestedFinished(Telepathy::Client::PendingOperation*)));
}

void PendingContacts::onNestedFinished(PendingOperation *operation)
{
    Q_ASSERT(operation == mPriv->nested);

    debug() << "Nested PendingContacts finished for" << this;

    if (operation->isError()) {
        debug() << " error" << operation->errorName()
                << "message" << operation->errorMessage();
        setFinishedWithError(operation->errorName(), operation->errorMessage());
        return;
    }

    mPriv->contacts = mPriv->nested->contacts();
    mPriv->nested = 0;
    setFinished();
}

PendingContacts::PendingContacts(Connection *connection,
        const UIntList &handles, const QSet<Contact::Feature> &features)
    : PendingOperation(connection),
      mPriv(new Private(connection, handles, features))
{
}

PendingContacts::PendingContacts(Connection *connection,
        const QStringList &identifiers, const QSet<Contact::Feature> &features)
    : PendingOperation(connection), mPriv(new Private(connection, identifiers, features))
{
}

} // Telepathy::Client
} // Telepathy
