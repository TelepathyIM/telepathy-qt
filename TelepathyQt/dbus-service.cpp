/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
 * @copyright Copyright (C) 2016 George Kiagiadakis <gkiagia@tolabaki.gr>
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
#include <TelepathyQt/DBusError>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusService::Private
{
    QString busName;
};

/**
 * \class DBusService
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-service.h <TelepathyQt/DBusService>
 *
 * \brief A DBusObject providing a service with a well-known bus name
 */

/**
 * Construct a DBusService that uses the given \a dbusConnection.
 *
 * \param dbusConnection The D-Bus connection that will be used by this service.
 * \return the newly constructed DBusService in a SharedPtr
 */
DBusServicePtr DBusService::create(const QDBusConnection &dbusConnection)
{
    return DBusServicePtr(new DBusService(dbusConnection));
}

/**
 * Class constructor
 */
DBusService::DBusService(const QDBusConnection &dbusConnection)
    : DBusObject(dbusConnection),
      mPriv(new Private)
{
}

/**
 * Class destructor.
 */
DBusService::~DBusService()
{
    delete mPriv;
}

/**
 * Return the D-Bus service name of this service.
 *
 * This is only valid after this service has been registered
 * on the bus using registerService().
 *
 * \return the D-Bus service name of this service.
 */
QString DBusService::busName() const
{
    return mPriv->busName;
}

/**
 * Register this service object on the bus with the given \a busName and \a objectPath.
 *
 * \a error needs to be a valid pointer to a DBusError instance, where any
 * possible D-Bus error will be stored.
 *
 * A service may only be registered once in its lifetime.
 * Use isRegistered() to find out if it has already been registered or not.
 *
 * You normally don't need to use this method directly.
 * Subclasses should provide a simplified version of it.
 *
 * \param busName The D-Bus service name of this object.
 * \param objectPath The D-Bus object path of this object.
 * \param error A pointer to a valid DBusError instance, where any
 * possible D-Bus error will be stored.
 * \return \c true on success or \c false otherwise.
 */
bool DBusService::registerService(const QString &busName, const QString &objectPath,
        DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    if (!dbusConnection().registerService(busName)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                QString(QLatin1String("Name %1 already in use by another process"))
                    .arg(busName));
        warning() << "Unable to register service" << busName <<
            "- name already registered by another process";
        return false;
    }

    debug() << "Registered service" << busName;

    if (!registerObject(objectPath, error)) {
        return false;
    }

    mPriv->busName = busName;
    return true;
}

}
