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

#ifndef _TelepathyQt4_cli_dbus_proxy_h_HEADER_GUARD_
#define _TelepathyQt4_cli_dbus_proxy_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

// FIXME: What groups should this be in/define?

#include <QDBusConnection>

namespace Telepathy
{
namespace Client
{

/**
 * \class DBusProxy
 * \ingroup FIXME: what group is it in?
 * \headerfile TelepathyQt4/Client/dbus-proxy.h <TelepathyQt4/Client/DBusProxy>
 *
 * Base class representing a remote object available over D-Bus.
 *
 * All TelepathyQt4 client convenience classes that wrap Telepathy interfaces
 * inherit from this class in order to provide basic D-Bus interface
 * information.
 *
 */
class DBusProxy : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    DBusProxy(const QDBusConnection& dbusConnection, const QString& busName,
            const QString& objectPath, QObject* parent = 0);

    /**
     * Destructor
     */
    virtual ~DBusProxy();

    /**
     * Returns the D-Bus connection through which the remote object is
     * accessed.
     *
     * \return The connection the object is associated with.
     */
    QDBusConnection dbusConnection() const;

    /**
     * Returns the D-Bus bus name (either a unique name or a well-known
     * name) of the service that provides the remote object.
     *
     * \return The service name the object is associated with.
     */
    QString busName() const;

    /**
     * Returns the D-Bus object path of the remote object within the service.
     *
     * \return The object path the object is associated with.
     */
    QString objectPath() const;

    /**
     * If this object is usable (has not emitted #invalidated()), returns
     * <code>true</code>. Otherwise returns <code>false</code>.
     *
     * \return <code>true</code> if this object is still fully usable
     */
    bool isValid() const;

    /**
     * If this object is no longer usable (has emitted #invalidated()),
     * returns the error name indicating the reason it became invalid in a
     * machine-readable way. Otherwise, returns a null QString.
     *
     * \return A D-Bus error name, or QString() if this object is still valid
     */
    QString invalidationReason() const;

    /**
     * If this object is no longer usable (has emitted #invalidated()),
     * returns a debugging message indicating the reason it became invalid.
     * Otherwise, returns a null QString.
     *
     * \return A debugging message, or QString() if this object is still valid
     */
    QString invalidationMessage() const;

Q_SIGNALS:
    /**
     * Emitted when this object is no longer usable.
     *
     * After this signal is emitted, any D-Bus method calls on the object
     * will fail, but it may be possible to retrieve information that has
     * already been retrieved and cached.
     *
     * \param proxy This proxy
     * \param errorName A D-Bus error name (a string in a subset
     *                  of ASCII, prefixed with a reversed domain name)
     * \param errorMessage A debugging message associated with the error
     */
    void invalidated(DBusProxy* proxy, QString errorName,
            QString errorMessage);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

/**
 * \class StatelessDBusProxy
 * \ingroup FIXME: what group is it in?
 * \headerfile TelepathyQt4/Client/dbus-proxy.h <TelepathyQt4/Client/DBusProxy>
 *
 * Base class representing a remote object whose API is basically stateless.
 * These objects can remain valid even if the service providing them exits
 * and is restarted.
 *
 * Examples in Telepathy include the AccountManager, Account and
 * ConnectionManager.
 */
class StatelessDBusProxy : public DBusProxy
{
    Q_OBJECT

public:
    StatelessDBusProxy(const QDBusConnection& dbusConnection,
        const QString& busName, const QString& objectPath,
        QObject* parent = 0);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

/**
 * \class StatefulDBusProxy
 * \ingroup FIXME: what group is it in?
 * \headerfile TelepathyQt4/Client/dbus-proxy.h <TelepathyQt4/Client/DBusProxy>
 *
 * Base class representing a remote object whose API is stateful. These
 * objects do not remain useful if the service providing them exits or
 * crashes, so they emit #invalidated() if this happens.
 *
 * Examples in Telepathy include the Connection and Channel.
 */
class StatefulDBusProxy : public DBusProxy
{
    Q_OBJECT

public:
    StatefulDBusProxy(const QDBusConnection& dbusConnection,
        const QString& busName, const QString& objectPath,
        QObject* parent = 0);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif
