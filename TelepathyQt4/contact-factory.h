/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_contact_factory_h_HEADER_GUARD_
#define _TelepathyQt4_contact_factory_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Feature>
#include <TelepathyQt4/Global>
#include <TelepathyQt4/Types>

#include <QSet>
#include <QtGlobal>

namespace Tp
{

class ContactManager;
class ReferencedHandles;

class TELEPATHY_QT4_EXPORT ContactFactory : public RefCounted
{
    Q_DISABLE_COPY(ContactFactory)

public:
    static ContactFactoryPtr create(const Features &features = Features());

    virtual ~ContactFactory();

    Features features() const;

    void addFeature(const Feature &feature);
    void addFeatures(const Features &features);

protected:
    ContactFactory(const Features &features);

    virtual ContactPtr construct(ContactManager *manager, const ReferencedHandles &handle,
            const Features &features, const QVariantMap &attributes) const;
    virtual PendingOperation *prepare(const ContactPtr &contact) const;
    virtual PendingOperation *prepare(const QList<ContactPtr> &contacts) const;

private:
    friend class ContactManager;
    friend class PendingContacts;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
