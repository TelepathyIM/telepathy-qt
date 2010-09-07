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

#include <TelepathyQt4/ContactFactory>

namespace Tp
{

/**
 * \class ContactFactory
 * \headerfile TelepathyQt4/contact-factory.h <TelepathyQt4/ContactFactory>
 *
 * \brief Constructs Contact objects according to application-defined settings
 *
 * \todo Actually implement it. Currently it's just a placeholder.
 */

/**
 * Creates a new ContactFactory.
 *
 * \param features The features to make ready on constructed Contacts.
 * \returns A pointer to the created factory.
 */
ContactFactoryPtr ContactFactory::create(const QSet<Contact::Feature> &features)
{
    return ContactFactoryPtr(new ContactFactory(features));
}

/**
 * Class constructor.
 *
 * \param features The features to make ready on constructed Contacts.
 */
ContactFactory::ContactFactory(const QSet<Contact::Feature> &features)
{
}

/**
 * Class destructor.
 */
ContactFactory::~ContactFactory()
{
}

/**
 * Can be used by subclasses to override the Contact subclass constructed by the factory.
 *
 * \todo implement it...
 * \return A pointer to the constructed Contact object.
 */
ContactPtr ContactFactory::construct(Tp::ContactManager *manager, const ReferencedHandles &handle,
        const QSet<Contact::Feature> &features, const QVariantMap &attributes) const
{
    return ContactPtr();
}

/**
 * Can be used by subclasses to do arbitrary manipulation on constructed Contact objects.
 *
 * \todo implement it...
 */
PendingOperation *ContactFactory::prepare(const ContactPtr &contact) const
{
    return NULL;
}

}
