/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/ContactCapabilities>

#include <TelepathyQt4/Types>

namespace Tp
{

/**
 * \class ContactCapabilities
 * \ingroup clientconn
 * \headerfile TelepathyQt4/contact-capabilities.h <TelepathyQt4/ContactCapabilities>
 *
 * \brief The ContactCapabilities class provides an object representing the
 * capabilities of a Contact.
 */

/**
 * Construct a new ContactCapabilities object.
 */
ContactCapabilities::ContactCapabilities(bool specificToContact)
    : CapabilitiesBase(specificToContact)
{
}

/**
 * Construct a new ContactCapabilities object using the give \a classes.
 *
 * \param classes RequestableChannelClassList representing the capabilities of a
 *                contact.
 */
ContactCapabilities::ContactCapabilities(
        const RequestableChannelClassList &classes,
        bool specificToContact)
    : CapabilitiesBase(classes, specificToContact)
{
}

/**
 * Class destructor.
 */
ContactCapabilities::~ContactCapabilities()
{
}

} // Tp
