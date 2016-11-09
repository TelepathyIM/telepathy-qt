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

#include <TelepathyQt/DBusObject>

#include "TelepathyQt/_gen/dbus-object.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusError>

#include <QDBusConnection>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusObject::Private
{
    Private(const QDBusConnection &dbusConnection)
        : dbusConnection(dbusConnection),
          registered(false)
    {
    }

    QDBusConnection dbusConnection;
    QString objectPath;
    bool registered;
    QHash<QString, AbstractDBusInterfaceAdapteePtr> interfaces;
};

/**
 * \class DBusObject
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-object.h <TelepathyQt/DBusObject>
 *
 * \brief An Object on which low-level D-Bus interface adaptees are plugged
 *      to provide a D-Bus object.
 */

/**
 * Construct a DBusObject that operates on the given \a dbusConnection.
 *
 * \param dbusConnection The D-Bus connection to use.
 * \return the newly constructed DBusObject in a SharedPtr
 */
DBusObjectPtr DBusObject::create(const QDBusConnection &dbusConnection)
{
    return DBusObjectPtr(new DBusObject(dbusConnection));
}

/**
 * Class constructor
 */
DBusObject::DBusObject(const QDBusConnection &dbusConnection)
    : Object(),
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

/**
 * Return the D-Bus object path of this object.
 *
 * This is only valid after this object has been registered
 * on the bus using registerObject().
 *
 * \return the D-Bus object path of this service.
 */
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

/**
 * Return whether this D-Bus object has been registered on the bus or not.
 *
 * \return \c true if the object has been registered, or \c false otherwise.
 */
bool DBusObject::isRegistered() const
{
    return mPriv->registered;
}

/**
 * Register this object on the bus with the given \a objectPath.
 *
 * \a error needs to be a valid pointer to a DBusError instance, where any
 * possible D-Bus error will be stored.
 *
 * An object may only be registered once in its lifetime.
 * Use isRegistered() to find out if it has already been registered or not.
 *
 * You normally don't need to use this method directly.
 * Subclasses should provide a simplified version of it.
 *
 * \param objectPath The D-Bus object path of this object.
 * \param error A pointer to a valid DBusError instance, where any
 * possible D-Bus error will be stored.
 * \return \c true on success or \c false otherwise.
 */
bool DBusObject::registerObject(const QString &objectPath, DBusError *error)
{
    if (mPriv->registered) {
        return true;
    }

    if (!dbusConnection().registerObject(objectPath, this)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                QString(QStringLiteral("Object at path %1 already registered"))
                    .arg(objectPath));
        warning() << "Unable to register object" << objectPath <<
            "- path already registered";
        return false;
    }

    for (const AbstractDBusInterfaceAdapteePtr &adaptee : mPriv->interfaces.values()) {
        adaptee->createAdaptor(this);
    }

    debug() << "Registered object" << objectPath;

    mPriv->objectPath = objectPath;
    mPriv->registered = true;
    return true;
}

/**
 * Return the names of all the interfaces that have been plugged on this object.
 *
 * The interface adaptee objects for these interfaces may be retrieved
 * using interfaceAdaptee().
 *
 * \return A list with the names of all interfaces that have been
 *      plugged on this object.
 */
QStringList DBusObject::interfaces() const
{
    return mPriv->interfaces.keys();
}

/**
 * Return the interface adaptee object of the plugged interface named \a interfaceName.
 * \return The interface adaptee object of \a interfaceName or an invalid
 *      SharedPtr if \a interfaceName has not been plugged yet.
 */
AbstractDBusInterfaceAdapteePtr DBusObject::interfaceAdaptee(const QString &interfaceName) const
{
    return mPriv->interfaces.value(interfaceName);
}

/**
 * Plugs the D-Bus interface represented by \a adaptee on to this object.
 *
 * An adaptee can only be plugged before registering the object on the bus.
 * The interface name is retrieved from AbstractDBusInterfaceAdaptee::interfaceName().
 * It is not allowed to register more than one adaptee for the same interface name.
 *
 * \return \c true on success, \c false otherwise
 */
bool DBusObject::plugInterfaceAdaptee(const AbstractDBusInterfaceAdapteePtr &adaptee)
{
    if (mPriv->registered) {
        warning() << "Cannot plug interface adaptee after the "
                "DBusObject has been registered";
        return false;
    }

    if (mPriv->interfaces.contains(adaptee->interfaceName())) {
        warning() << "Attempted to plug adaptee for an already plugged interface";
        return false;
    }

    if (adaptee->dbusObject()) {
        warning() << "Attempted to plug adaptee that has already been plugged "
                "on another object";
        return false;
    }

    mPriv->interfaces.insert(adaptee->interfaceName(), adaptee);
    adaptee->attachToDBusObject(this);
    return true;
}


struct TP_QT_NO_EXPORT AbstractDBusInterfaceAdaptee::Private
{
    WeakPtr<DBusObject> dbusObject;
};

/**
 * \class AbstractDBusInterfaceAdaptee
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-object.h <TelepathyQt/AbstractDBusInterfaceAdaptee>
 *
 * \brief An Object that provides API to implement a D-Bus interface on a DBusObject
 */

/**
 * Class constructor
 */
AbstractDBusInterfaceAdaptee::AbstractDBusInterfaceAdaptee()
    : mPriv(new Private)
{
}

/**
 * Class destructor
 */
AbstractDBusInterfaceAdaptee::~AbstractDBusInterfaceAdaptee()
{
    delete mPriv;
}

/**
 * Return the DBusObject that this interface adaptee instance has been plugged onto
 *
 * The DBusObject is the object on which all the interface adaptees
 * are plugged to provide an object on the D-Bus.
 *
 * \return a pointer to the DBusObject that this interface adaptee
 *      has been plugged onto, or \c NULL if this adaptee is not plugged yet
 */
DBusObjectPtr AbstractDBusInterfaceAdaptee::dbusObject() const
{
    return mPriv->dbusObject.toStrongRef();
}

/**
 * Return whether the object implementing this interface has been registered.
 *
 * \return \c true if the object has been registered, or \c false otherwise.
 * \sa DBusObject::registerObject()
 */
bool AbstractDBusInterfaceAdaptee::isRegistered() const
{
    return mPriv->dbusObject && mPriv->dbusObject.toStrongRef()->isRegistered();
}

void AbstractDBusInterfaceAdaptee::attachToDBusObject(DBusObject *object)
{
    mPriv->dbusObject = WeakPtr<DBusObject>(object);
}

/** \fn QString AbstractDBusInterfaceAdaptee::interfaceName() const
 *
 * Return the D-Bus interface name that this adaptee provides
 *
 * \return the D-Bus interface name that this adaptee provides
 */

/** \fn void AbstractDBusInterfaceAdaptee::createAdaptor(QObject *parent)
 *
 * Create the QDBusAbstractAdaptor subclass object that will provide this interface
 * on the bus.
 *
 * Subclasses need to implement this function and construct a new adaptor on
 * the heap with \a parent as its parent. This function is called internally
 * at the object's registration time, by DBusObject::registerObject().
 */

}
