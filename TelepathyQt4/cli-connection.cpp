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

#include <TelepathyQt4/Client/Connection>

#include <TelepathyQt4/_gen/cli-connection-body.hpp>
#include <TelepathyQt4/_gen/cli-connection.moc.hpp>

namespace Telepathy
{
namespace Client
{

struct Connection::Private
{
    ConnectionInterface interface;

    Private(const QDBusConnection& connection, const QString& serviceName, const QString& objectPath, QObject* parent)
        : interface(connection, serviceName, objectPath, parent) {}
};

Connection::Connection(const QString& serviceName, const QString& objectPath, QObject* parent)
    : mPriv(new Private(QDBusConnection::sessionBus(), serviceName, objectPath, parent))
{
}

Connection::Connection(const QDBusConnection& connection, const QString& serviceName, const QString& objectPath, QObject* parent)
    : mPriv(new Private(connection, serviceName, objectPath, parent))
{
}

Connection::~Connection()
{
    delete mPriv;
}

ConnectionInterface& Connection::interface()
{
    return mPriv->interface;
}

const ConnectionInterface& Connection::interface() const
{
    return mPriv->interface;
}

}
}
