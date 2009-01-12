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

#include <TelepathyQt4/Constants>

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnectionInterface>

namespace Telepathy
{
namespace Client
{

// ==== DBusProxy ======================================================

class DBusProxy::Private
{
public:
    // Public object
    DBusProxy &parent;

    QDBusConnection dbusConnection;
    QString busName;
    QString objectPath;

    Private(const QDBusConnection &, const QString &, const QString &,
            DBusProxy &);
};

DBusProxy::Private::Private(const QDBusConnection &dbusConnection,
            const QString &busName, const QString &objectPath, DBusProxy &p)
 : parent(p),
   dbusConnection(dbusConnection),
   busName(busName),
   objectPath(objectPath)
{
    debug() << "Creating new DBusProxy";
}

DBusProxy::DBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &path, QObject *parent)
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

// ==== StatelessDBusProxy =============================================

StatelessDBusProxy::StatelessDBusProxy(const QDBusConnection& dbusConnection,
        const QString &busName, const QString &objectPath, QObject *parent)
    : DBusProxy(dbusConnection, busName, objectPath, parent)
{
    if (busName.startsWith(QChar(':'))) {
        warning() <<
            "Using StatelessDBusProxy for a unique name does not make sense";
    }
}

// ==== StatefulDBusProxy ==============================================

class StatefulDBusProxy::Private
{
public:
    // Public object
    StatefulDBusProxy &parent;

    QString invalidationReason;
    QString invalidationMessage;

    Private(StatefulDBusProxy &);
};

StatefulDBusProxy::Private::Private(StatefulDBusProxy &p)
 : parent(p),
   invalidationReason(QString()),
   invalidationMessage(QString())
{
    debug() << "Creating new StatefulDBusProxy";
}

StatefulDBusProxy::StatefulDBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &objectPath, QObject *parent)
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

void StatefulDBusProxy::invalidate(const QString &reason, const QString &message)
{
    if (!isValid()) {
        debug().nospace() << "Already invalidated by "
            << mPriv->invalidationReason
            << ", not replacing with " << reason
            << " \"" << message << "\"";
        return;
    }

    Q_ASSERT(!reason.isEmpty());

    debug().nospace() << "proxy invalidated: " << reason
        << ": " << message;

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

    emit invalidated(this, mPriv->invalidationReason, mPriv->invalidationMessage);
}

void StatefulDBusProxy::onServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    // We only want to invalidate this object if it is not already invalidated,
    // and it's (not any other object's) name owner changed signal is emitted.
    if (isValid() && (name == busName())) {
        invalidate(TELEPATHY_DBUS_ERROR_NAME_HAS_NO_OWNER,
            "Name owner lost (service crashed?)");
    }
}

}
}
