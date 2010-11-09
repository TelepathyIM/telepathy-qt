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

#ifndef _TelepathyQt4_pending_contacts_h_HEADER_GUARD_
#define _TelepathyQt4_pending_contacts_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/PendingOperation>

#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Contact>

namespace Tp
{

class ContactManager;

class TELEPATHY_QT4_EXPORT PendingContacts : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingContacts);

public:
    ~PendingContacts();

    ContactManager *manager() const;
    Features features() const;

    bool isForHandles() const;
    UIntList handles() const;

    bool isForIdentifiers() const;
    QStringList identifiers() const;

    bool isUpgrade() const;
    QList<ContactPtr> contactsToUpgrade() const;

    QList<ContactPtr> contacts() const;
    UIntList invalidHandles() const;
    QStringList validIdentifiers() const;
    QHash<QString, QPair<QString, QString> > invalidIdentifiers() const;

private Q_SLOTS:
    void onAttributesFinished(Tp::PendingOperation *);
    void onRequestHandlesFinished(Tp::PendingOperation *);
    void onReferenceHandlesFinished(Tp::PendingOperation *);
    void onNestedFinished(Tp::PendingOperation *);
    void onInspectHandlesFinished(QDBusPendingCallWatcher *);

private:
    friend class ContactManager;

    // If errorName is non-empty, these will fail instantly
    PendingContacts(ContactManager *manager, const UIntList &handles,
            const Features &features,
            const QStringList &interfaces,
            const QMap<uint, ContactPtr> &satisfyingContacts,
            const QSet<uint> &otherContacts,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    PendingContacts(ContactManager *manager, const QStringList &identifiers,
            const Features &features,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    PendingContacts(ContactManager *manager, const QList<ContactPtr> &contacts,
            const Features &features,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());

    void allAttributesFetched();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
