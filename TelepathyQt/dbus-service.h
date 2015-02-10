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

#ifndef _TelepathyQt_dbus_service_h_HEADER_GUARD_
#define _TelepathyQt_dbus_service_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusError>
#include <TelepathyQt/Global>
#include <TelepathyQt/Object>
#include <TelepathyQt/ServiceTypes>

#include <QObject>
#include <QVariantMap>

class QDBusConnection;
class QString;

namespace Tp
{

class DBusObject;

class TP_QT_EXPORT DBusService : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(DBusService)

public:
    DBusService(const QDBusConnection &dbusConnection);
    virtual ~DBusService();

    virtual QVariantMap immutableProperties() const = 0;

    QDBusConnection dbusConnection() const;
    QString busName() const;
    QString objectPath() const;
    DBusObject *dbusObject() const;
    bool isRegistered() const;

protected:
    virtual bool registerObject(const QString &busName, const QString &objectPath,
            DBusError *error);

private:
    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractDBusServiceInterface : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractDBusServiceInterface)

public:
    AbstractDBusServiceInterface(const QString &interfaceName);
    virtual ~AbstractDBusServiceInterface();

    QString interfaceName() const;

    virtual QVariantMap immutableProperties() const = 0;

    DBusObject *dbusObject() const;
    bool isRegistered() const;

protected:
    virtual bool registerInterface(DBusObject *dbusObject);
    virtual void createAdaptor() = 0;

public:
    bool notifyPropertyChanged(const QString &propertyName, const QVariant &propertyValue);

private:
    class Private;
    friend class Private;
    Private *mPriv;
};

}

#endif
