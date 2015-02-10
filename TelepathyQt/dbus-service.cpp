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
#include <TelepathyQt/DBusObject>

#include <QDBusConnection>
#include <QString>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusService::Private
{
    Private(DBusService *parent, const QDBusConnection &dbusConnection)
        : parent(parent),
          dbusObject(new DBusObject(dbusConnection, parent)),
          registered(false)
    {
    }

    DBusService *parent;
    QString busName;
    DBusObject *dbusObject;
    bool registered;
};

/**
 * \class DBusService
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-service.h <TelepathyQt/DBusService>
 *
 * \brief Base class for D-Bus services.
 *
 * This class serves as a base for all the classes that are used to implement
 * D-Bus services.
 */

/**
 * Construct a DBusService that uses the given \a dbusConnection.
 *
 * \param dbusConnection The D-Bus connection that will be used by this service.
 */
DBusService::DBusService(const QDBusConnection &dbusConnection)
    : mPriv(new Private(this, dbusConnection))
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
 * Return the D-Bus connection associated with this service.
 *
 * \return the D-Bus connection associated with this service.
 */
QDBusConnection DBusService::dbusConnection() const
{
    return mPriv->dbusObject->dbusConnection();
}

/**
 * Return the D-Bus service name of this service.
 *
 * This is only valid after this service has been registered
 * on the bus using registerObject().
 *
 * \return the D-Bus service name of this service.
 */
QString DBusService::busName() const
{
    return mPriv->busName;
}

/**
 * Return the D-Bus object path of this service.
 *
 * This is only valid after this service has been registered
 * on the bus using registerObject().
 *
 * \return the D-Bus object path of this service.
 */
QString DBusService::objectPath() const
{
    return mPriv->dbusObject->objectPath();
}

/**
 * Return the DBusObject that is used for registering this service on the bus.
 *
 * The DBusObject is the object on which all the interface adaptors
 * for this service are plugged.
 *
 * \return a pointer to the DBusObject that is used for registering
 * this service on the bus.
 */
DBusObject *DBusService::dbusObject() const
{
    return mPriv->dbusObject;
}

/**
 * Return whether this D-Bus service has been registered on the bus or not.
 *
 * \return \c true if the service has been registered, or \c false otherwise.
 */
bool DBusService::isRegistered() const
{
    return mPriv->registered;
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
bool DBusService::registerObject(const QString &busName, const QString &objectPath,
        DBusError *error)
{
    if (mPriv->registered) {
        return true;
    }

    if (!mPriv->dbusObject->dbusConnection().registerService(busName)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                QString(QLatin1String("Name %1 already in use by another process"))
                    .arg(busName));
        warning() << "Unable to register service" << busName <<
            "- name already registered by another process";
        return false;
    }

    if (!mPriv->dbusObject->dbusConnection().registerObject(objectPath, mPriv->dbusObject)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                QString(QLatin1String("Object at path %1 already registered"))
                    .arg(objectPath));
        warning() << "Unable to register object" << objectPath <<
            "- path already registered";
        return false;
    }

    debug() << "Registered object" << objectPath << "at bus name" << busName;

    mPriv->busName = busName;
    mPriv->dbusObject->setObjectPath(objectPath);
    mPriv->registered = true;
    return true;
}

/**
 * \fn QVariantMap DBusService::immutableProperties() const
 *
 * Return the immutable properties of this D-Bus service object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this D-Bus service object.
 */

struct AbstractDBusServiceInterface::Private
{
    Private(const QString &interfaceName)
        : interfaceName(interfaceName),
          dbusObject(0),
          registered(false)
    {
    }

    QString interfaceName;
    DBusObject *dbusObject;
    bool registered;
};

/**
 * \class AbstractDBusServiceInterface
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-service.h <TelepathyQt/AbstractDBusServiceInterface>
 *
 * \brief Base class for D-Bus service interfaces.
 *
 * This class serves as a base for all the classes that are used to implement
 * interfaces that sit on top of D-Bus services.
 */

/**
 * Construct an AbstractDBusServiceInterface that implements
 * the interface with the given \a interfaceName.
 *
 * \param interfaceName The name of the interface that this class implements.
 */
AbstractDBusServiceInterface::AbstractDBusServiceInterface(const QString &interfaceName)
    : mPriv(new Private(interfaceName))
{
}

/**
 * Class destructor.
 */
AbstractDBusServiceInterface::~AbstractDBusServiceInterface()
{
    delete mPriv;
}

/**
 * Return the name of the interface that this class implements,
 * as given on the constructor.
 *
 * \return The name of the interface that this class implements.
 */
QString AbstractDBusServiceInterface::interfaceName() const
{
    return mPriv->interfaceName;
}

/**
 * Return the DBusObject on which the adaptor of this interface is plugged.
 *
 * This is only accessible after the interface has been registered
 * with registerInterface().
 *
 * \return a pointer to the DBusObject on which the adaptor
 * of this interface is plugged.
 * \sa DBusService::dbusObject()
 */
DBusObject *AbstractDBusServiceInterface::dbusObject() const
{
    return mPriv->dbusObject;
}

/**
 * Return whether this interface has been registered.
 *
 * \return \c true if the service has been registered, or \c false otherwise.
 * \sa registerInterface()
 */
bool AbstractDBusServiceInterface::isRegistered() const
{
    return mPriv->registered;
}

/**
 * Emit PropertiesChanged signal on object org.freedesktop.DBus.Properties interface
 * with the property \a propertyName.
 *
 * \param propertyName The name of the changed property.
 * \param propertyValue The actual value of the changed property.
 * \return \c false if the signal can not be emmited or \a true otherwise.
 */
bool AbstractDBusServiceInterface::notifyPropertyChanged(const QString &propertyName, const QVariant &propertyValue)
{
    if (!isRegistered()) {
        return false;
    }

    QDBusMessage signal = QDBusMessage::createSignal(dbusObject()->objectPath(),
                                                     TP_QT_IFACE_PROPERTIES,
                                                     QLatin1String("PropertiesChanged"));
    QVariantMap changedProperties;
    changedProperties.insert(propertyName, propertyValue);

    signal << interfaceName();
    signal << changedProperties;
    signal << QStringList();

    return dbusObject()->dbusConnection().send(signal);
}

/**
 * Registers this interface by plugging its adaptor
 * on the given \a dbusObject.
 *
 * \param dbusObject The object on which to plug the adaptor.
 * \return \c false if the interface has already been registered,
 * or \a true otherwise.
 * \sa isRegistered()
 */
bool AbstractDBusServiceInterface::registerInterface(DBusObject *dbusObject)
{
    if (mPriv->registered) {
        return true;
    }

    mPriv->dbusObject = dbusObject;
    createAdaptor();
    mPriv->registered = true;
    return true;
}

/**
 * \fn QVariantMap AbstractDBusServiceInterface::immutableProperties() const
 *
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */

/**
 * \fn void AbstractDBusServiceInterface::createAdaptor()
 *
 * Create the adaptor for this interface.
 *
 * Subclasses should reimplement this appropriately.
 */

}
