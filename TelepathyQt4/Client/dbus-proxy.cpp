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

#include "dbus-proxy.h"
#include "dbus-proxy.moc.hpp"

#include <TelepathyQt4/debug-internal.hpp>

namespace Telepathy
{
namespace Client
{

struct DBusProxy::Private
{
    // Public object
    DBusProxy& parent;

    QDBusConnection dbusConnection;
    QString busName;
    QString objectPath;

    Private(const QDBusConnection& dbusConnection, const QString& busName,
            const QString& objectPath, DBusProxy& p)
        : parent(p),
          dbusConnection(dbusConnection),
          busName(busName),
          objectPath(objectPath)
    {
        debug() << "Creating new DBusProxy";
    }
};

DBusProxy::DBusProxy(const QDBusConnection& dbusConnection,
        const QString& busName, const QString& path, QObject* parent)
 : QObject(parent),
   mPriv(new Private(dbusConnection, busName, path, *this))
{
}

DBusProxy::~DBusProxy()
{
    delete mPriv;
}

QDBusConnection DBusProxy::dbusConnection() const
{
    return mPriv->dbusConnection;
}

QString DBusProxy::objectPath() const
{
    return mPriv->objectPath;
}

QString DBusProxy::busName() const
{
    return mPriv->busName;
}

StatelessDBusProxy::StatelessDBusProxy(const QDBusConnection& dbusConnection,
        const QString& busName, const QString& objectPath, QObject* parent)
    : DBusProxy(dbusConnection, busName, objectPath, parent)
{
}

StatefulDBusProxy::StatefulDBusProxy(const QDBusConnection& dbusConnection,
        const QString& busName, const QString& objectPath, QObject* parent)
    : DBusProxy(dbusConnection, busName, objectPath, parent)
{
}

}
}
