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

#ifndef _TelepathyQt4_cli_pending_contacts_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_contacts_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Client/PendingOperation>

#include <QList>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Contact>

namespace Telepathy
{
namespace Client
{

class Connection;
class ContactManager;

class PendingContacts : public PendingOperation
{
    Q_OBJECT

public:
    ~PendingContacts();

    ContactManager *contactManager() const;
    QSet<Contact::Feature> features() const;

    bool isForHandles() const;
    UIntList handles() const;

    bool isForIdentifiers() const;
    QStringList identifiers() const;

    QList<QSharedPointer<Contact> > contacts() const;

private Q_SLOTS:
    void onAttributesFinished(Telepathy::Client::PendingOperation *);
    void onHandlesFinished(Telepathy::Client::PendingOperation *);
    void onNestedFinished(Telepathy::Client::PendingOperation *);

private:
    Q_DISABLE_COPY(PendingContacts);
    PendingContacts(Connection *connection, const UIntList &handles,
            const QSet<Contact::Feature> &features);
    PendingContacts(Connection *connection, const QStringList &identifiers,
            const QSet<Contact::Feature> &features);

    struct Private;
    friend struct Private;
    friend class ContactManager;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
