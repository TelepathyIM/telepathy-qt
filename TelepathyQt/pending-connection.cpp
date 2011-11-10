/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt/PendingConnection>

#include "TelepathyQt/_gen/pending-connection.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingReady>

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingConnection::Private
{
    ConnectionPtr connection;
};

/**
 * \class PendingConnection
 * \ingroup clientconn
 * \headerfile TelepathyQt/pending-connection.h <TelepathyQt/PendingConnection>
 *
 * \brief The PendingConnection class represents the parameters of and the reply
 * to an asynchronous connection request.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via ConnectionManager.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingConnection object.
 *
 * \param manager ConnectionManager to use.
 * \param protocol Name of the protocol to create the connection for.
 * \param parameters Connection parameters.
 */
PendingConnection::PendingConnection(const ConnectionManagerPtr &manager,
        const QString &protocol, const QVariantMap &parameters)
    : PendingOperation(manager),
      mPriv(new Private)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            manager->baseInterface()->RequestConnection(protocol,
                parameters), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onCallFinished(QDBusPendingCallWatcher*)));
}

/**
 * Construct a new PendingConnection object that will fail immediately.
 *
 * \param error Name of the error to fail with.
 * \param errorMessage Detail message for the error.
 */
PendingConnection::PendingConnection(const QString &error, const QString &errorMessage)
    : PendingOperation(ConnectionManagerPtr()),
      mPriv(new Private)
{
    setFinishedWithError(error, errorMessage);
}

/**
 * Class destructor.
 */
PendingConnection::~PendingConnection()
{
    delete mPriv;
}

/**
 * Return the connection manager through which the request was made.
 *
 * \return A pointer to the ConnectionManager object.
 */
ConnectionManagerPtr PendingConnection::manager() const
{
    return ConnectionManagerPtr(qobject_cast<ConnectionManager*>((ConnectionManager*) object().data()));
}

/**
 * Return the connection resulting from the connection request.
 *
 * \return A pointer to the Connection object.
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

    return mPriv->connection;
}

void PendingConnection::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString, QDBusObjectPath> reply = *watcher;

    if (!reply.isError()) {
        QString busName = reply.argumentAt<0>();
        QString objectPath = reply.argumentAt<1>().path();

        debug() << "Got reply to ConnectionManager.CreateConnection - bus name:" <<
            busName << "- object path:" << objectPath;

        PendingReady *readyOp = manager()->connectionFactory()->proxy(busName,
                objectPath, manager()->channelFactory(), manager()->contactFactory());
        mPriv->connection = ConnectionPtr::qObjectCast(readyOp->proxy());
        connect(readyOp,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onConnectionBuilt(Tp::PendingOperation*)));
    } else {
        debug().nospace() <<
            "CreateConnection failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingConnection::onConnectionBuilt(Tp::PendingOperation *op)
{
    Q_ASSERT(op->isFinished());

    if (op->isError()) {
        warning() << "Making connection ready using the factory failed:" <<
            op->errorName() << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        setFinished();
        debug() << "New connection" << mPriv->connection->objectPath() << "built";
    }
}

} // Tp
