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

#ifndef _TelepathyQt_dbus_object_h_HEADER_GUARD_
#define _TelepathyQt_dbus_object_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

#include <QObject>

class QDBusConnection;

namespace Tp
{

class TP_QT_EXPORT DBusObject : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DBusObject)

public:
    DBusObject(const QDBusConnection &dbusConnection, QObject *parent = 0);
    virtual ~DBusObject();

    QString objectPath() const;
    QDBusConnection dbusConnection() const;

protected:
    void setObjectPath(const QString &path);

private:
    class Private;
    friend class Private;
    Private *mPriv;

    friend class DBusService;
};

}

#endif
