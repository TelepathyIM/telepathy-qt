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

#ifndef _TelepathyQt4_cli_contact_h_HEADER_GUARD_
#define _TelepathyQt4_cli_contact_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>
#include <QSet>
#include <QVariantMap>

#include <TelepathyQt4/Types>

namespace Telepathy
{
namespace Client
{
class ContactManager;
class ReferencedHandles;

class Contact : public QObject
{
    Q_OBJECT

public:
    enum Feature {
        FeatureSimplePresence,
        _Padding = 0xFFFFFFFF
    };

    ContactManager *manager() const;

    ReferencedHandles handle() const;
    QString id() const;

    QSet<Feature> requestedFeatures() const;
    QSet<Feature> actualFeatures() const;

    QString presenceStatus() const;
    uint presenceType() const;
    QString presenceMessage() const;

    ~Contact();

Q_SIGNALS:
    void simplePresenceChanged(const QString &status, uint type, const QString &presenceMessage);

private:
    Q_DISABLE_COPY(Contact);

    Contact(ContactManager *manager, const ReferencedHandles &handle,
            const QSet<Feature> &requestedFeatures, const QVariantMap &attributes);

    void augment(const QSet<Feature> &requestedFeatures, const QVariantMap &attributes);
    void receiveSimplePresence(const SimplePresence &presence);

    struct Private;
    friend class ContactManager;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
