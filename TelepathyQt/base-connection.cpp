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

#include <TelepathyQt/BaseConnection>
#include "TelepathyQt/base-connection-internal.h"

#include "TelepathyQt/_gen/base-connection.moc.hpp"
#include "TelepathyQt/_gen/base-connection-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/Utils>

#include <QString>
#include <QVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseConnection::Private
{
    Private(BaseConnection *parent, const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters)
        : parent(parent),
          cmName(cmName),
          protocolName(protocolName),
          parameters(parameters),
          adaptee(new BaseConnection::Adaptee(dbusConnection, parent))
    {
    }

    BaseConnection *parent;
    QString cmName;
    QString protocolName;
    QVariantMap parameters;

    BaseConnection::Adaptee *adaptee;
};

BaseConnection::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
        BaseConnection *connection)
    : QObject(connection),
      mConnection(connection)
{
    mAdaptor = new Service::ConnectionAdaptor(dbusConnection, this, connection->dbusObject());
}

BaseConnection::Adaptee::~Adaptee()
{
}

BaseConnection::BaseConnection(const QDBusConnection &dbusConnection,
        const QString &cmName, const QString &protocolName,
        const QVariantMap &parameters)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, cmName, protocolName, parameters))
{
}

BaseConnection::~BaseConnection()
{
    delete mPriv;
}

QString BaseConnection::protocolName() const
{
    return mPriv->protocolName;
}

QVariantMap BaseConnection::parameters() const
{
    return mPriv->parameters;
}

QString BaseConnection::uniqueName() const
{
    return QString(QLatin1String("_%1")).arg((intptr_t) this, 0, 16);
}

bool BaseConnection::registerObject(DBusError *error)
{
    if (checkValidProtocolName(mPriv->protocolName)) {
        if (error) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                mPriv->protocolName + QLatin1String("is not a valid protocol name"));
        }
        debug() << "Unable to register connection - invalid protocol name";
        return false;
    }

    QString escapedProtocolName = mPriv->protocolName;
    escapedProtocolName.replace(QLatin1Char('-'), QLatin1Char('_'));
    QString name = uniqueName();
    QString busName = QString(QLatin1String("%s.%s.%s.%s"))
        .arg(TP_QT_CONNECTION_BUS_NAME_BASE).arg(mPriv->cmName).arg(escapedProtocolName).arg(name);
    QString objectPath = QString(QLatin1String("%s/%s/%s/%s"))
        .arg(TP_QT_CONNECTION_OBJECT_PATH_BASE).arg(mPriv->cmName).arg(escapedProtocolName).arg(name);
    return registerObject(busName, objectPath, error);
}

bool BaseConnection::registerObject(const QString &busName,
        const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

}
