/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt4/StreamTubeServer>

#include "TelepathyQt4/_gen/stream-tube-server.moc.hpp"

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/simple-stream-tube-handler.h"

#include <QTcpServer>

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/StreamTubeChannel>

namespace Tp
{

struct StreamTubeServer::Private
{
    Private(const ClientRegistrarPtr &registrar,
            const QStringList &services,
            const QString &maybeClientName,
            bool monitorConnections)
        : registrar(registrar),
          handler(SimpleStreamTubeHandler::create(services, true, monitorConnections)),
          clientName(maybeClientName),
          exportedPort(0)
    {
        if (clientName.isEmpty()) {
            clientName = QString::fromLatin1("TpQt4STubeServer_%1_%2")
                .arg(registrar->dbusConnection().baseService()
                        .replace(QLatin1Char(':'), QLatin1Char('_'))
                        .replace(QLatin1Char('.'), QLatin1Char('_')))
                .arg((intptr_t) this, 0, 16);
        }
    }

    ClientRegistrarPtr registrar;
    SharedPtr<SimpleStreamTubeHandler> handler;
    QString clientName;

    QHostAddress exportedAddr;
    quint16 exportedPort;
    QVariantMap exportedParams;
};

StreamTubeServerPtr StreamTubeServer::create(
        const QStringList &services,
        const QString &clientName,
        bool monitorConnections,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return create(
            QDBusConnection::sessionBus(),
            accountFactory,
            connectionFactory,
            channelFactory,
            contactFactory,
            services,
            clientName,
            monitorConnections);
}

StreamTubeServerPtr StreamTubeServer::create(
        const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const QStringList &services,
        const QString &clientName,
        bool monitorConnections)
{
    return create(
            ClientRegistrar::create(
                bus,
                accountFactory,
                connectionFactory,
                channelFactory,
                contactFactory),
            services,
            clientName,
            monitorConnections);
}

StreamTubeServerPtr StreamTubeServer::create(
        const AccountManagerPtr &accountManager,
        const QStringList &services,
        const QString &clientName,
        bool monitorConnections)
{
    return create(
            accountManager->dbusConnection(),
            accountManager->accountFactory(),
            accountManager->connectionFactory(),
            accountManager->channelFactory(),
            accountManager->contactFactory(),
            services,
            clientName,
            monitorConnections);
}

StreamTubeServerPtr StreamTubeServer::create(
        const ClientRegistrarPtr &registrar,
        const QStringList &services,
        const QString &clientName,
        bool monitorConnections)
{
    StreamTubeServerPtr server(
            new StreamTubeServer(registrar, services, clientName, monitorConnections));

    debug() << "Register StreamTubeServer with name " << server->clientName();

    if (!server->registrar()->registerClient(server->mPriv->handler, server->clientName())) {
        warning() << "StreamTubeServer" << server->clientName()
            << "registration failed, returning NULL";

        // Flag that registration failed, so we shouldn't attempt to unregister
        server->mPriv->clientName.clear();

        return StreamTubeServerPtr();
    }

    return server;
}

StreamTubeServer::StreamTubeServer(
        const ClientRegistrarPtr &registrar,
        const QStringList &services,
        const QString &clientName,
        bool monitorConnections)
: mPriv(new Private(registrar, services, clientName, monitorConnections))
{
    connect(mPriv->handler.data(),
            SIGNAL(invokedForTube(
                    Tp::AccountPtr,
                    Tp::StreamTubeChannelPtr,
                    QDateTime,
                    Tp::ChannelRequestHints)),
            SLOT(onInvokedForTube(
                    Tp::AccountPtr,
                    Tp::StreamTubeChannelPtr,
                    QDateTime,
                    Tp::ChannelRequestHints)));
}

/**
 * Class destructor.
 */
StreamTubeServer::~StreamTubeServer()
{
    if (!clientName().isNull()) {
        mPriv->registrar->unregisterClient(mPriv->handler);
    }
}

ClientRegistrarPtr StreamTubeServer::registrar() const
{
    return mPriv->registrar;
}

QString StreamTubeServer::clientName() const
{
    return mPriv->clientName;
}

bool StreamTubeServer::monitorsConnections() const
{
    return mPriv->handler->monitorsConnections();
}

QPair<QHostAddress, quint16> StreamTubeServer::exportedTcpSocketAddress() const
{
    return qMakePair(mPriv->exportedAddr, mPriv->exportedPort);
}

QVariantMap StreamTubeServer::exportedParameters() const
{
    return mPriv->exportedParams;
}

void StreamTubeServer::exportTcpSocket(
        const QHostAddress &addr,
        quint16 port,
        const QVariantMap &params)
{
    if (addr.isNull() || port == 0) {
        warning() << "Attempted to export null TCP socket address or zero port, ignoring";
        return;
    }

    mPriv->exportedAddr = addr;
    mPriv->exportedPort = port;
    mPriv->exportedParams = params;
}

void StreamTubeServer::exportTcpSocket(
        const QTcpServer *server,
        const QVariantMap &params)
{
    if (!server->isListening()) {
        warning() << "Attempted to export non-listening QTcpServer, ignoring";
        return;
    }

    if (server->serverAddress() == QHostAddress::Any) {
        return exportTcpSocket(QHostAddress::LocalHost, server->serverPort(), params);
    } else if (server->serverAddress() == QHostAddress::AnyIPv6) {
        return exportTcpSocket(QHostAddress::LocalHostIPv6, server->serverPort(), params);
    } else {
        return exportTcpSocket(server->serverAddress(), server->serverPort(), params);
    }
}

void StreamTubeServer::onInvokedForTube(
        const AccountPtr &acc,
        const StreamTubeChannelPtr &tube,
        const QDateTime &time,
        const ChannelRequestHints &hints)
{
    Q_ASSERT(tube->isRequested());

    OutgoingStreamTubeChannelPtr outgoing = OutgoingStreamTubeChannelPtr::qObjectCast(tube);

    if (!outgoing) {
        warning() << "The ChannelFactory used by StreamTubeServer must construct" <<
            "OutgoingStreamTubeChannel subclasses for Requested=true StreamTubes";
        tube->requestClose();
        return;
    } else if (mPriv->exportedAddr.isNull() || !mPriv->exportedPort) {
        warning() << "No socket exported, closing tube" << tube->objectPath();
        tube->requestClose();
        return;
    }

    // TODO: emit tubeRequested and begin tracking tube

    // TODO: connect to Offer return, and close tube emitting an error if the offer didn't succeed
    outgoing->offerTcpSocket(mPriv->exportedAddr, mPriv->exportedPort, mPriv->exportedParams);
}

} // Tp
