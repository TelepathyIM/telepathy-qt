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
#include <TelepathyQt/DBusObject>
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

/**
 * \class BaseConnection
 * \ingroup serviceconn
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for Connection implementations.
 */

/**
 * Construct a BaseConnection.
 *
 * \param dbusConnection The D-Bus connection that will be used by this object.
 * \param cmName The name of the connection manager associated with this connection.
 * \param protocolName The name of the protocol associated with this connection.
 * \param parameters The parameters of this connection.
 */
BaseConnection::BaseConnection(const QDBusConnection &dbusConnection,
        const QString &cmName, const QString &protocolName,
        const QVariantMap &parameters)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, cmName, protocolName, parameters))
{
}

/**
 * Class destructor.
 */
BaseConnection::~BaseConnection()
{
    delete mPriv;
}

/**
 * Return the name of the connection manager associated with this connection.
 *
 * \return The name of the connection manager associated with this connection.
 */
QString BaseConnection::cmName() const
{
    return mPriv->cmName;
}

/**
 * Return the name of the protocol associated with this connection.
 *
 * \return The name of the protocol associated with this connection.
 */
QString BaseConnection::protocolName() const
{
    return mPriv->protocolName;
}

/**
 * Return the parameters of this connection.
 *
 * \return The parameters of this connection.
 */
QVariantMap BaseConnection::parameters() const
{
    return mPriv->parameters;
}

/**
 * Return the immutable properties of this connection object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this connection object.
 */
QVariantMap BaseConnection::immutableProperties() const
{
    // FIXME
    return QVariantMap();
}

/**
 * Return a unique name for this connection.
 *
 * \return A unique name for this connection.
 */
QString BaseConnection::uniqueName() const
{
    return QString(QLatin1String("_%1")).arg((quintptr) this, 0, 16);
}

/**
 * Register this connection object on the bus.
 *
 * If \a error is passed, any D-Bus error that may occur will
 * be stored there.
 *
 * \param error A pointer to an empty DBusError where any
 * possible D-Bus error will be stored.
 * \return \c true on success and \c false if there was an error
 * or this connection object is already registered.
 * \sa isRegistered()
 */
bool BaseConnection::registerObject(DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

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
    QString busName = QString(QLatin1String("%1.%2.%3.%4"))
        .arg(TP_QT_CONNECTION_BUS_NAME_BASE, mPriv->cmName, escapedProtocolName, name);
    QString objectPath = QString(QLatin1String("%1/%2/%3/%4"))
        .arg(TP_QT_CONNECTION_OBJECT_PATH_BASE, mPriv->cmName, escapedProtocolName, name);
    DBusError _error;
    bool ret = registerObject(busName, objectPath, &_error);
    if (!ret && error) {
        error->set(_error.name(), _error.message());
    }
    return ret;
}

/**
 * Reimplemented from DBusService.
 */
bool BaseConnection::registerObject(const QString &busName,
        const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

/**
 * \fn void BaseConnection::disconnected()
 *
 * Emitted when this connection has been disconnected.
 */

}
