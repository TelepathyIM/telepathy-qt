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

#include <TelepathyQt4/Client/DBusProxy>

#include "TelepathyQt4/Client/_gen/dbus-proxy.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnectionInterface>

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

struct StatefulDBusProxy::Private
{
    // Public object
    StatefulDBusProxy& parent;

    QString invalidationReason;
    QString invalidationMessage;

    Private(StatefulDBusProxy& p)
        : parent(p),
          invalidationReason(QString()),
          invalidationMessage(QString())
    {
        debug() << "Creating new StatefulDBusProxy";
    }
};

StatefulDBusProxy::StatefulDBusProxy(const QDBusConnection& dbusConnection,
        const QString& busName, const QString& objectPath, QObject* parent)
    : DBusProxy(dbusConnection, busName, objectPath, parent)
{
    // FIXME: Am I on crack?
    connect(dbusConnection.interface(), SIGNAL(serviceOwnerChanged(QString, QString, QString)),
            this, SLOT(onServiceOwnerChanged(QString, QString, QString)));
}

bool StatefulDBusProxy::isValid() const
{
    return mPriv->invalidationReason.isEmpty();
}

QString StatefulDBusProxy::invalidationReason() const
{
    return mPriv->invalidationReason;
}

QString StatefulDBusProxy::invalidationMessage() const
{
    return mPriv->invalidationMessage;
}

void StatefulDBusProxy::invalidate(const QString& reason, const QString& message)
{
    // FIXME: Which of these should be warnings instead?
    Q_ASSERT(isValid());
    Q_ASSERT(mPriv->invalidationReason.isEmpty());
    Q_ASSERT(mPriv->invalidationMessage.isEmpty());
    Q_ASSERT(!reason.isEmpty());
    // FIXME: can message be empty?

    mPriv->invalidationReason = reason;
    mPriv->invalidationMessage = message;

    Q_ASSERT(!isValid());

    // Defer emitting the invalidated signal until we next
    // return to the mainloop.
    QTimer::singleShot(0, this, SLOT(emitInvalidated()));
}

void StatefulDBusProxy::emitInvalidated()
{
    Q_ASSERT(!isValid());
    Q_ASSERT(!mPriv->invalidationReason.isEmpty());
    emit invalidated(this, mPriv->invalidationReason, mPriv->invalidationMessage);
}

void StatefulDBusProxy::onServiceOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner)
{
    if (name == busName()) {
        // FIXME: where do the error texts come from? the spec?
        invalidate("NAME_OWNER_CHANGED", "NameOwnerChanged() received for this object.");
    }
}

}
}
