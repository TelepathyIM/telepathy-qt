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

#include <TelepathyQt4/Client/PendingConnection>

#include "TelepathyQt4/Client/_gen/pending-connection.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/Connection>

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

namespace Telepathy
{
namespace Client
{

struct PendingConnection::Private
{
    Private(ConnectionManager *manager) :
        manager(manager)
    {
    }

    ConnectionManager *manager;
    QSharedPointer<Connection> connection;
    QString serviceName;
    QDBusObjectPath objectPath;
};

/**
 * \class PendingConnection
 * \ingroup clientconnection
 * \headerfile <TelepathyQt4/Client/pending-connection.h> <TelepathyQt4/Client/PendingConnection>
 *
 * Class containing the parameters of and the reply to an asynchronous connection
 * request. Instances of this class cannot be constructed directly; the only
 * way to get one is via ConnectionManager.
 */

/**
 * Construct a PendingConnection object.
 *
 * \param manager ConnectionManager to use.
 * \param protocol Name of the protocol to create the connection for.
 * \param parameters Connection parameters.
 */
PendingConnection::PendingConnection(ConnectionManager *manager,
        const QString &protocol, const QVariantMap &parameters)
    : PendingOperation(manager),
      mPriv(new Private(manager))
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            manager->baseInterface()->RequestConnection(protocol,
                parameters), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onCallFinished(QDBusPendingCallWatcher *)));
}

/**
 * Class destructor.
 */
PendingConnection::~PendingConnection()
{
    delete mPriv;
}

/**
 * Return the ConnectionManager object through which the request was made.
 *
 * \return Connection Manager object.
 */
ConnectionManager *PendingConnection::manager() const
{
    return qobject_cast<ConnectionManager *>(parent());
}

/**
 * Returns the newly created Connection object.
 *
 * \return Connection object.
 */
QSharedPointer<Connection> PendingConnection::connection() const
{
    if (!isFinished()) {
        warning() << "PendingConnection::connection called before finished, returning 0";
        return QSharedPointer<Connection>();
    } else if (!isValid()) {
        warning() << "PendingConnection::connection called when not valid, returning 0";
        return QSharedPointer<Connection>();
    }

    if (!mPriv->connection) {
        mPriv->connection = QSharedPointer<Connection>(
                new Connection(mPriv->manager->dbusConnection(),
                    mPriv->serviceName, mPriv->objectPath.path()));
    }

    return mPriv->connection;
}

/**
 * Returns the connection service name or an empty string on error.
 *
 * This method is useful for creating custom Connection objects, so instead of using
 * PendingConnection::connection, one could construct a new custom connection with
 * the service name and object path.
 *
 * \return Connection service name.
 * \sa objectPath()
 */
QString PendingConnection::serviceName() const
{
    if (!isFinished()) {
        warning() << "PendingConnection::serviceName called before finished";
    } else if (!isValid()) {
        warning() << "PendingConnection::serviceName called when not valid";
    }

    return mPriv->serviceName;
}

/**
 * Returns the connection object path or an empty string on error.
 *
 * This method is useful for creating custom Connection objects, so instead of using
 * PendingConnection::connection, one could construct a new custom connection with
 * the service name and object path.
 *
 * \return Connection object path.
 * \sa serviceName()
 */
QString PendingConnection::objectPath() const
{
    if (!isFinished()) {
        warning() << "PendingConnection::connection called before finished";
    } else if (!isValid()) {
        warning() << "PendingConnection::connection called when not valid";
    }

    return mPriv->objectPath.path();
}

void PendingConnection::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString, QDBusObjectPath> reply = *watcher;

    if (!reply.isError()) {
        mPriv->serviceName = reply.argumentAt<0>();
        mPriv->objectPath = reply.argumentAt<1>();
        debug() << "Got reply to ConnectionManager.CreateConnection - service name:" <<
            mPriv->serviceName << "- object path:" << mPriv->objectPath.path();
        setFinished();
    } else {
        debug().nospace() <<
            "CreateConnection failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
