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

#include "cli-dbus-proxy.h"

#include "debug-internal.hpp"

namespace Telepathy
{
namespace Client
{

struct DBusProxy::Private
{
    // Public object
    DBusProxy& parent;

    QDBusAbstractInterface* baseInterface;

    Private(QDBusAbstractInterface* interface, DBusProxy& p)
        : parent(p), 
          baseInterface(interface)
    {
        debug() << "Creating new DBusProxy";
    }
};

DBusProxy::DBusProxy(QDBusAbstractInterface* baseInterface, QObject* parent)
 : QObject(parent),
   mPriv(new Private(baseInterface, *this))
{

}

DBusProxy::~DBusProxy()
{
    delete mPriv;
}

QDBusConnection DBusProxy::connection() const
{
    return mPriv->baseInterface->connection();
}

QString DBusProxy::path() const
{
    return mPriv->baseInterface->path();
}

QString DBusProxy::service() const
{
    return mPriv->baseInterface->service();
}

}
}

