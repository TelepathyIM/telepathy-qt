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

#include <TelepathyQt4/PendingConnection>

#include "TelepathyQt4/_gen/pending-connection.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/Connection>

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingConnection::Private
{
    Private(const ConnectionManagerPtr &manager) :
        manager(manager)
    {
    }

    WeakPtr<ConnectionManager> manager;
    ConnectionPtr connection;
    QString busName;
    QDBusObjectPath objectPath;
};

/**
 * \class PendingConnection
 * \ingroup clientconn
 * \headerfile TelepathyQt4/pending-connection.h <TelepathyQt4/PendingConnection>
 *
 * \brief Class containing the parameters of and the reply to an asynchronous
 * connection request. Instances of this class cannot be constructed directly;
 * the only way to get one is via ConnectionManager.
 */

/**
 * Construct a PendingConnection object.
 *
 * \param manager ConnectionManager to use.
 * \param protocol Name of the protocol to create the connection for.
 * \param parameters Connection parameters.
 */
PendingConnection::PendingConnection(const ConnectionManagerPtr &manager,
        const QString &protocol, const QVariantMap &parameters)
    : PendingOperation(manager.data()),
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
ConnectionManagerPtr PendingConnection::manager() const
{
    return ConnectionManagerPtr(mPriv->manager);
}

/**
 * Returns the newly created Connection object.
 *
 * \return Connection object.
 */
ConnectionPtr PendingConnection::connection() const
{
    if (!isFinished()) {
        warning() << "PendingConnection::connection called before finished, returning 0";
        return ConnectionPtr();
    } else if (!isValid()) {
        warning() << "PendingConnection::connection called when not valid, returning 0";
        return ConnectionPtr();
    }

    if (!mPriv->connection) {
        ConnectionManagerPtr manager(mPriv->manager);
        mPriv->connection = Connection::create(manager->dbusConnection(),
                mPriv->busName, mPriv->objectPath.path());
    }

    return mPriv->connection;
}

/**
 * Returns the connection's bus name ("service name"), or an empty string on
 * error.
 *
 * This method is useful for creating custom Connection objects: instead
 * of using PendingConnection::connection, one could construct a new custom
 * connection from the bus name and object path.
 *
 * \return Connection bus name
 * \sa objectPath()
 */
QString PendingConnection::busName() const
{
    if (!isFinished()) {
        warning() << "PendingConnection::busName called before finished";
    } else if (!isValid()) {
        warning() << "PendingConnection::busName called when not valid";
    }

    return mPriv->busName;
}

/**
 * Returns the connection's object path or an empty string on error.
 *
 * This method is useful for creating custom Connection objects: instead
 * of using PendingConnection::connection, one could construct a new custom
 * connection with the bus name and object path.
 *
 * \return Connection object path
 * \sa busName()
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
        mPriv->busName = reply.argumentAt<0>();
        mPriv->objectPath = reply.argumentAt<1>();
        debug() << "Got reply to ConnectionManager.CreateConnection - bus name:" <<
            mPriv->busName << "- object path:" << mPriv->objectPath.path();
        setFinished();
    } else {
        debug().nospace() <<
            "CreateConnection failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
