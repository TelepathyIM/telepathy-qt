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

#ifndef _TelepathyQt_base_connection_h_HEADER_GUARD_
#define _TelepathyQt_base_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>

#include <QDBusConnection>

class QString;

namespace Tp
{

class TP_QT_EXPORT BaseConnection : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnection)

public:
    static BaseConnectionPtr create(const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters)
    {
        return BaseConnectionPtr(new BaseConnection(
                    QDBusConnection::sessionBus(), cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QString &cmName,
            const QString &protocolName, const QVariantMap &parameters)
    {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    QDBusConnection::sessionBus(), cmName, protocolName, parameters));
    }
    static BaseConnectionPtr create(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters)
    {
        return BaseConnectionPtr(new BaseConnection(
                    dbusConnection, cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters)
    {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    dbusConnection, cmName, protocolName, parameters));
    }

    virtual ~BaseConnection();

    QString cmName() const;
    QString protocolName() const;
    QVariantMap parameters() const;

    QVariantMap immutableProperties() const;

    virtual QString uniqueName() const;
    bool registerObject(DBusError *error = NULL);

Q_SIGNALS:
    void disconnected();

protected:
    BaseConnection(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters);

    virtual bool registerObject(const QString &busName, const QString &objectPath,
            DBusError *error);

private:
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

}

#endif
