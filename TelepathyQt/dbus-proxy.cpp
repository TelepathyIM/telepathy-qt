/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#include "config.h"

#include <TelepathyQt/DBusProxy>

#include "TelepathyQt/_gen/dbus-proxy.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusServiceWatcher>
#include <QTimer>

namespace Tp
{

// ==== DBusProxy ======================================================

// Features in TpProxy but not here:
// * tracking which interfaces we have (in tpqt, subclasses do that)
// * being Introspectable, a Peer and a Properties implementation
// * disconnecting from signals when invalidated (probably has to be in the
//   generated code)
// * making methods always raise an error when called after invalidated
//   (has to be in the generated code)

struct TP_QT_NO_EXPORT DBusProxy::Private
{
    Private(const QDBusConnection &dbusConnection, const QString &busName,
            const QString &objectPath);

    QDBusConnection dbusConnection;
    QString busName;
    QString objectPath;
    QString invalidationReason;
    QString invalidationMessage;
};

DBusProxy::Private::Private(const QDBusConnection &dbusConnection,
            const QString &busName, const QString &objectPath)
    : dbusConnection(dbusConnection),
      busName(busName),
      objectPath(objectPath)
{
    debug() << "Creating new DBusProxy";
}

/**
 * \class DBusProxy
 * \ingroup clientproxies
 * \headerfile TelepathyQt/dbus-proxy.h <TelepathyQt/DBusProxy>
 *
 * \brief The DBusProxy class is a base class representing a remote object available over D-Bus.
 *
 * All Telepathy-Qt client convenience classes that wrap Telepathy interfaces
 * inherit from this class in order to provide basic D-Bus interface
 * information.
 */

/**
 * Construct a new DBusProxy object.
 *
 * \param dbusConnection QDBusConnection to use.
 * \param busName D-Bus bus name of the service that provides the remote object.
 * \param objectPath The object path.
 * \param featureCore The object core feature.
 */
DBusProxy::DBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &objectPath, const Feature &featureCore)
    : Object(),
      ReadyObject(this, featureCore),
      mPriv(new Private(dbusConnection, busName, objectPath))
{
    if (!dbusConnection.isConnected()) {
        invalidate(TP_QT_ERROR_DISCONNECTED,
                QLatin1String("DBus connection disconnected"));
    }
}

/**
 * Class destructor.
 */
DBusProxy::~DBusProxy()
{
    delete mPriv;
}

/**
 * Return the D-Bus connection through which the remote object is
 * accessed.
 *
 * \return A QDBusConnection object.
 */
QDBusConnection DBusProxy::dbusConnection() const
{
    return mPriv->dbusConnection;
}

/**
 * Return the D-Bus object path of the remote object within the service.
 *
 * \return The D-Bus object path.
 */
QString DBusProxy::objectPath() const
{
    return mPriv->objectPath;
}

/**
 * Return the D-Bus bus name (either a unique name or a well-known
 * name) of the service that provides the remote object.
 *
 * \return The D-Bus bus name.
 */
QString DBusProxy::busName() const
{
    return mPriv->busName;
}

/**
 * Sets the D-Bus bus name. This is used by subclasses after converting
 * well-known names to unique names.
 *
 * \param busName The D-Bus bus name to set.
 */
void DBusProxy::setBusName(const QString &busName)
{
    mPriv->busName = busName;
}

/**
 * Return whether this proxy is still valid (has not emitted invalidated()).
 *
 * \return \c true if still valid, \c false otherwise.
 */
bool DBusProxy::isValid() const
{
    return mPriv->invalidationReason.isEmpty();
}

/**
 * Return the error name indicating the reason this proxy became invalid.
 *
 * \return A D-Bus error name, or QString() if this object is still valid.
 */
QString DBusProxy::invalidationReason() const
{
    return mPriv->invalidationReason;
}

/**
 * Return a debugging message indicating the reason this proxy became invalid.
 *
 * \return A debugging message, or QString() if this object is still valid.
 */
QString DBusProxy::invalidationMessage() const
{
    return mPriv->invalidationMessage;
}

/**
 * Called by subclasses when the DBusProxy should become invalid.
 *
 * This method takes care of setting the invalidationReason,
 * invalidationMessage, and emitting the invalidated signal.
 *
 * \param reason A D-Bus error name (a string in a subset of ASCII,
 *               prefixed with a reversed domain name)
 * \param message A debugging message associated with the error
 */
void DBusProxy::invalidate(const QString &reason, const QString &message)
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

void DBusProxy::invalidate(const QDBusError &error)
{
    invalidate(error.name(), error.message());
}

void DBusProxy::emitInvalidated()
{
    Q_ASSERT(!isValid());

    emit invalidated(this, mPriv->invalidationReason, mPriv->invalidationMessage);
}

/**
 * \fn void DBusProxy::invalidated(Tp::DBusProxy *proxy,
 *          const QString &errorName, const QString &errorMessage)
 *
 * Emitted when this object is no longer usable.
 *
 * After this signal is emitted, any D-Bus method calls on the object
 * will fail, but it may be possible to retrieve information that has
 * already been retrieved and cached.
 *
 * \param proxy This proxy.
 * \param errorName The name of a D-Bus error describing the reason for the invalidation.
 * \param errorMessage A debugging message associated with the error.
 */

// ==== StatefulDBusProxy ==============================================

struct TP_QT_NO_EXPORT StatefulDBusProxy::Private
{
    Private(const QString &originalName)
        : originalName(originalName) {}

    QString originalName;
};

/**
 * \class StatefulDBusProxy
 * \ingroup clientproxies
 * \headerfile TelepathyQt/dbus-proxy.h <TelepathyQt/StatefulDBusProxy>
 *
 * \brief The StatefulDBusProxy class is a base class representing a remote object whose API is
 * stateful.
 *
 * These objects do not remain useful if the service providing them exits or
 * crashes, so they emit invalidated() if this happens.
 *
 * Examples include the Connection and Channel classes.
 */

/**
 * Construct a new StatefulDBusProxy object.
 *
 * \param dbusConnection QDBusConnection to use.
 * \param busName D-Bus bus name of the service that provides the remote object.
 * \param objectPath The object path.
 * \param featureCore The object core feature.
 */
StatefulDBusProxy::StatefulDBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &objectPath, const Feature &featureCore)
    : DBusProxy(dbusConnection, busName, objectPath, featureCore),
      mPriv(new Private(busName))
{
    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(busName,
            dbusConnection, QDBusServiceWatcher::WatchForUnregistration, this);
    connect(serviceWatcher,
            SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            SLOT(onServiceOwnerChanged(QString,QString,QString)));

    QString error, message;
    QString uniqueName = uniqueNameFrom(dbusConnection, busName, error, message);

    if (uniqueName.isEmpty()) {
        invalidate(error, message);
        return;
    }

    setBusName(uniqueName);
}

/**
 * Class destructor.
 */
StatefulDBusProxy::~StatefulDBusProxy()
{
    delete mPriv;
}

QString StatefulDBusProxy::uniqueNameFrom(const QDBusConnection &bus, const QString &name)
{
    QString error, message;
    QString uniqueName = uniqueNameFrom(bus, name, error, message);
    if (uniqueName.isEmpty()) {
        warning() << "StatefulDBusProxy::uniqueNameFrom(): Failed to get unique name of" << name;
        warning() << "  error:" << error << "message:" << message;
    }

    return uniqueName;
}

QString StatefulDBusProxy::uniqueNameFrom(const QDBusConnection &bus, const QString &name,
        QString &error, QString &message)
{
    if (name.startsWith(QLatin1String(":"))) {
        return name;
    }

    // For a stateful interface, it makes no sense to follow name-owner
    // changes, so we want to bind to the unique name.
    QDBusReply<QString> reply = bus.interface()->serviceOwner(name);
    if (reply.isValid()) {
        return reply.value();
    } else {
        error = reply.error().name();
        message = reply.error().message();
        return QString();
    }
}

void StatefulDBusProxy::onServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    // We only want to invalidate this object if it is not already invalidated,
    // and its (not any other object's) name owner changed signal is emitted.
    if (isValid() && name == mPriv->originalName && newOwner.isEmpty()) {
        invalidate(TP_QT_DBUS_ERROR_NAME_HAS_NO_OWNER,
                QLatin1String("Name owner lost (service crashed?)"));
    }
}

// ==== StatelessDBusProxy =============================================

/**
 * \class StatelessDBusProxy
 * \ingroup clientproxies
 * \headerfile TelepathyQt/dbus-proxy.h <TelepathyQt/DBusProxy>
 *
 * \brief The StatelessDBusProxy class is a base class representing a remote object whose API is
 * basically stateless.
 *
 * These objects can remain valid even if the service providing them exits
 * and is restarted.
 *
 * Examples include the AccountManager, Account and ConnectionManager.
 */

/**
 * Construct a new StatelessDBusProxy object.
 *
 * \param dbusConnection QDBusConnection to use.
 * \param busName D-Bus bus name of the service that provides the remote object.
 * \param objectPath The object path.
 * \param featureCore The object core feature.
 */
StatelessDBusProxy::StatelessDBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &objectPath, const Feature &featureCore)
    : DBusProxy(dbusConnection, busName, objectPath, featureCore),
      mPriv(0)
{
    if (busName.startsWith(QLatin1String(":"))) {
        warning() <<
            "Using StatelessDBusProxy for a unique name does not make sense";
    }
}

/**
 * Class destructor.
 */
StatelessDBusProxy::~StatelessDBusProxy()
{
}

} // Tp
