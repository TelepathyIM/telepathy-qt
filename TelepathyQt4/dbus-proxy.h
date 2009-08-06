/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2009 Nokia Corporation
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

#ifndef _TelepathyQt4_dbus_proxy_h_HEADER_GUARD_
#define _TelepathyQt4_dbus_proxy_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>

class QDBusConnection;
class QDBusError;

namespace Tp
{

class DBusProxy : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DBusProxy)

public:
    DBusProxy(const QDBusConnection &dbusConnection, const QString &busName,
            const QString &objectPath, QObject *parent = 0);
    virtual ~DBusProxy();

    QDBusConnection dbusConnection() const;
    QString busName() const;
    QString objectPath() const;

    bool isValid() const;
    QString invalidationReason() const;
    QString invalidationMessage() const;

Q_SIGNALS:
    void invalidated(Tp::DBusProxy *proxy,
            const QString &errorName, const QString &errorMessage);

protected:
    void setBusName(const QString &busName);
    void invalidate(const QString &reason, const QString &message);
    void invalidate(const QDBusError &error);

private Q_SLOTS:
    void emitInvalidated();

private:
    class Private;
    friend class Private;
    Private *mPriv;
};

class StatelessDBusProxy : public DBusProxy
{
    Q_OBJECT
    Q_DISABLE_COPY(StatelessDBusProxy)

public:
    StatelessDBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &objectPath,
        QObject *parent = 0);

    virtual ~StatelessDBusProxy();

private:
    class Private;
    friend class Private;
    Private *mPriv;
};

class StatefulDBusProxy : public DBusProxy
{
    Q_OBJECT
    Q_DISABLE_COPY(StatefulDBusProxy)

public:
    StatefulDBusProxy(const QDBusConnection &dbusConnection,
        const QString &busName, const QString &objectPath,
        QObject *parent = 0);

    virtual ~StatefulDBusProxy();

private Q_SLOTS:
    void onServiceOwnerChanged(const QString &name, const QString &oldOwner,
            const QString &newOwner);

private:
    class Private;
    friend class Private;
    Private *mPriv;
};

} // Tp

#endif
