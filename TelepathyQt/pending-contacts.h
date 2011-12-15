/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2011 Nokia Corporation
 * @license LGPL 2.1
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

#ifndef _TelepathyQt_pending_contacts_h_HEADER_GUARD_
#define _TelepathyQt_pending_contacts_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/PendingOperation>

#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>

#include <TelepathyQt/Types>
#include <TelepathyQt/Contact>

namespace Tp
{

class ContactManager;

class TP_QT_EXPORT PendingContacts : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingContacts);

public:
    ~PendingContacts();

    ContactManagerPtr manager() const;
    Features features() const;

    bool isForHandles() const;
    UIntList handles() const;

    bool isForIdentifiers() const;
    QStringList identifiers() const;

    bool isForVCardAddresses() const;
    QString vcardField() const;
    QStringList vcardAddresses() const;

    bool isForUris() const;
    QStringList uris() const;

    bool isUpgrade() const;
    QList<ContactPtr> contactsToUpgrade() const;

    QList<ContactPtr> contacts() const;
    UIntList invalidHandles() const;
    QStringList validIdentifiers() const;
    QHash<QString, QPair<QString, QString> > invalidIdentifiers() const;
    QStringList validVCardAddresses() const;
    QStringList invalidVCardAddresses() const;
    QStringList validUris() const;
    QStringList invalidUris() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onAttributesFinished(Tp::PendingOperation *);
    TP_QT_NO_EXPORT void onRequestHandlesFinished(Tp::PendingOperation *);
    TP_QT_NO_EXPORT void onAddressingGetContactsFinished(Tp::PendingOperation *);
    TP_QT_NO_EXPORT void onReferenceHandlesFinished(Tp::PendingOperation *);
    TP_QT_NO_EXPORT void onNestedFinished(Tp::PendingOperation *);
    TP_QT_NO_EXPORT void onInspectHandlesFinished(QDBusPendingCallWatcher *);

private:
    friend class ContactManager;

    enum RequestType
    {
        ForHandles,
        ForIdentifiers,
        ForVCardAddresses,
        ForUris,
        Upgrade
    };

    // If errorName is non-empty, these will fail instantly
    TP_QT_NO_EXPORT PendingContacts(const ContactManagerPtr &manager, const UIntList &handles,
            const Features &features,
            const Features &missingFeatures,
            const QStringList &interfaces,
            const QMap<uint, ContactPtr> &satisfyingContacts,
            const QSet<uint> &otherContacts,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    TP_QT_NO_EXPORT PendingContacts(const ContactManagerPtr &manager, const QStringList &list,
            RequestType requestType,
            const Features &features,
            const QStringList &interfaces,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    TP_QT_NO_EXPORT PendingContacts(const ContactManagerPtr &manager, const QString &vcardField,
            const QStringList &vcardAddresses,
            const Features &features,
            const QStringList &interfaces,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    TP_QT_NO_EXPORT PendingContacts(const ContactManagerPtr &manager, const QList<ContactPtr> &contacts,
            const Features &features,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());

    TP_QT_NO_EXPORT void allAttributesFetched();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
