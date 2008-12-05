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

#include "cli-connection-manager.h"

#include <TelepathyQt4/Constants>

#include "TelepathyQt4/debug-internal.hpp"

namespace Telepathy
{
namespace Client
{


struct ConnectionManager::Private
{
    ConnectionManager& parent;

    ConnectionManagerInterface* baseInterface;

    static inline QString makeBusName(const QString& name)
    {
        return QString::fromAscii(
                TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE).append(name);
    }

    static inline QString makeObjectPath(const QString& name)
    {
        return QString::fromAscii(
                TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE).append(name);
    }

    inline Private(ConnectionManager& parent)
        : parent(parent),
          baseInterface(new ConnectionManagerInterface(parent.dbusConnection(),
                      parent.busName(), parent.objectPath(), &parent))
    {
        debug() << "Creating new ConnectionManager:" << parent.busName();
    }
};


ConnectionManager::ConnectionManager(const QString& name, QObject* parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            Private::makeBusName(name), Private::makeObjectPath(name),
            parent),
      mPriv(new Private(*this))
{
}


ConnectionManager::ConnectionManager(const QDBusConnection& bus,
        const QString& name, QObject* parent)
    : StatelessDBusProxy(bus, Private::makeBusName(name),
            Private::makeObjectPath(name), parent),
      mPriv(new Private(*this))
{
}


ConnectionManager::~ConnectionManager()
{
    delete mPriv;
}


ConnectionManagerInterface* ConnectionManager::baseInterface() const
{
    return mPriv->baseInterface;
}


} // Telepathy::Client
} // Telepathy

#include <TelepathyQt4/_gen/cli-connection-manager-body.hpp>
#include <TelepathyQt4/_gen/cli-connection-manager.moc.hpp>
#include <TelepathyQt4/cli-connection-manager.moc.hpp>
