/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
 * @license LGPL 2.1
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

#include <TelepathyQt/DBusService>

#include "TelepathyQt/_gen/dbus-service.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>

#include <QDBusConnection>
#include <QString>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusService::Private
{
    Private(DBusService *parent, const QDBusConnection &dbusConnection)
        : parent(parent),
          dbusConnection(dbusConnection),
          dbusObject(new QObject(parent)),
          registered(false)
    {
    }

    DBusService *parent;
    QDBusConnection dbusConnection;
    QString busName;
    QString objectPath;
    QObject *dbusObject;
    bool registered;
};

DBusService::DBusService(const QDBusConnection &dbusConnection)
    : mPriv(new Private(this, dbusConnection))
{
}

DBusService::~DBusService()
{
    delete mPriv;
}

QDBusConnection DBusService::dbusConnection() const
{
    return mPriv->dbusConnection;
}

QString DBusService::busName() const
{
    return mPriv->busName;
}

QString DBusService::objectPath() const
{
    return mPriv->objectPath;
}

QObject *DBusService::dbusObject() const
{
    return mPriv->dbusObject;
}

bool DBusService::isRegistered() const
{
    return mPriv->registered;
}

bool DBusService::registerObject(const QString &busName, const QString &objectPath,
        DBusError *error)
{
    if (mPriv->registered) {
        return true;
    }

    if (!mPriv->dbusConnection.registerService(busName)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                QString(QLatin1String("Name %s already in use by another process"))
                    .arg(busName));
        warning() << "Unable to register service" << busName <<
            "- name already registered by another process";
        return false;
    }

    if (!mPriv->dbusConnection.registerObject(objectPath, mPriv->dbusObject)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                QString(QLatin1String("Object at path %s already registered"))
                    .arg(objectPath));
        warning() << "Unable to register object" << objectPath <<
            "- path already registered";
        return false;
    }

    debug() << "Registered object" << objectPath << "at bus name" << busName;

    mPriv->busName = busName;
    mPriv->objectPath = objectPath;
    mPriv->registered = true;
    return true;
}

}
