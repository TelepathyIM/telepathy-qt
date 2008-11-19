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

// FIXME: What groups should this be in/define?

#include <QtDBus/QDBusAbstractInterface>
#include <QtDBus/QDBusConnection>

namespace Telepathy
{
namespace Client
{

/**
 * \class DBusProxy
 * \ingroup FIXME: what group is it in?
 * \headerfile TelepathyQt4/cli-dbus-proxy.h <TelepathyQt4/Client/DBusProxy>
 *
 * Base class which all TelepathyQt4 client convenience classes that wrap
 * Telepathy interfaces inherit from in order to provide basic DBus interface
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
    DBusProxy(QDBusAbstractInterface* baseInterface, QObject* parent = 0);

    /**
     * Destructor
     */
    ~DBusProxy();

    /**
     * Returns the DBus connection the base interface is associated with.
     *
     * \return The connection the interface is associated with.
     */
    QDBusConnection connection() const;

    /**
     * Returns the DBus object path the base interface is associated with.
     *
     * \return The object path the interface is associated with.
     */
    QString path() const;

    /**
     * Returns the DBus service name the base interface is associated with.
     *
     * \return The service name the interface is associated with.
     */
    QString service() const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif

