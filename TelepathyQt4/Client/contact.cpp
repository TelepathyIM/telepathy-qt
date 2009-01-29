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

#include <TelepathyQt4/Client/Contact>
#include "TelepathyQt4/Client/_gen/contact.moc.hpp"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct Contact::Private
{
    Private(Connection *connection, const ReferencedHandles &handle)
        : connection(connection), handle(handle)
    {
    }

    Connection *connection;
    ReferencedHandles handle;
    QString id;
};

Connection *Contact::connection() const
{
    return mPriv->connection;
}

ReferencedHandles Contact::handle() const
{
    return mPriv->handle;
}

QString Contact::id() const
{
    return mPriv->id;
}

Contact::~Contact()
{
    delete mPriv;
}

Contact::Contact(Connection *connection, const ReferencedHandles &handle,
        const QVariantMap &attributes)
    : QObject(0), mPriv(new Private(connection, handle))
{
    debug() << this << "initialized with" << attributes.size() << "attributes";

    mPriv->id = qdbus_cast<QString>(attributes["org.freedesktop.Telepathy.Connection/contact-id"]);
}

} // Telepathy::Client
} // Telepathy
