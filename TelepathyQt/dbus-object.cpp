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

#include <TelepathyQt/DBusObject>

#include "TelepathyQt/_gen/dbus-object.moc.hpp"

#include <QDBusConnection>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusObject::Private
{
    Private(const QDBusConnection &dbusConnection)
        : dbusConnection(dbusConnection)
    {
    }

    QDBusConnection dbusConnection;
    QString objectPath;
};

/**
 * \class DBusObject
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-object.h <TelepathyQt/DBusObject>
 *
 * \brief A QObject on which low-level D-Bus adaptors are plugged to provide a D-Bus object.
 */


/**
 * Construct a DBusObject that operates on the given \a dbusConnection.
 *
 * \param dbusConnection The D-Bus connection to use.
 * \param parent The QObject parent of this instance.
 */
DBusObject::DBusObject(const QDBusConnection &dbusConnection, QObject *parent)
    : QObject(parent),
      mPriv(new Private(dbusConnection))
{
}

/**
 * Class destructor.
 */
DBusObject::~DBusObject()
{
    delete mPriv;
}

void DBusObject::setObjectPath(const QString &path)
{
    mPriv->objectPath = path;
}

QString DBusObject::objectPath() const
{
    return mPriv->objectPath;
}

/**
 * Return the D-Bus connection associated with this object.
 *
 * \return The D-Bus connection associated with this object.
 */
QDBusConnection DBusObject::dbusConnection() const
{
    return mPriv->dbusConnection;
}

}
