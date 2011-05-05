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

#include <TelepathyQt4/StreamTubeClient>

#include "TelepathyQt4/_gen/stream-tube-client.moc.hpp"

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/simple-stream-tube-handler.h"

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/StreamTubeChannel>

namespace Tp
{

struct StreamTubeClient::Private
{
    Private(const ClientRegistrarPtr &registrar,
            const QStringList &services,
            const QString &maybeClientName,
            bool monitorConnections)
        : registrar(registrar),
          handler(SimpleStreamTubeHandler::create(services, false, monitorConnections)),
          clientName(maybeClientName),
          acceptsAsTcp(false), acceptsAsUnix(false),
          tcpGenerator(0)
    {
        if (clientName.isEmpty()) {
            clientName = QString::fromLatin1("TpQt4STubeClient_%1_%2")
                .arg(registrar->dbusConnection().baseService()
                        .replace(QLatin1Char(':'), QLatin1Char('_'))
                        .replace(QLatin1Char('.'), QLatin1Char('_')))
                .arg((intptr_t) this, 0, 16);
        }
    }

    ClientRegistrarPtr registrar;
    SharedPtr<SimpleStreamTubeHandler> handler;
    QString clientName;

    bool acceptsAsTcp, acceptsAsUnix;
    const TcpSourceAddressGenerator *tcpGenerator;
};

StreamTubeClientPtr StreamTubeClient::create(
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

StreamTubeClientPtr StreamTubeClient::create(
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

StreamTubeClientPtr StreamTubeClient::create(
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

StreamTubeClientPtr StreamTubeClient::create(
        const ClientRegistrarPtr &registrar,
        const QStringList &services,
        const QString &clientName,
        bool monitorConnections)
{
    StreamTubeClientPtr client(
            new StreamTubeClient(registrar, services, clientName, monitorConnections));

    debug() << "Register StreamTubeClient with name " << client->clientName();

    if (!client->registrar()->registerClient(client->mPriv->handler, client->clientName())) {
        warning() << "StreamTubeClient" << client->clientName()
            << "registration failed, returning NULL";

        // Flag that registration failed, so we shouldn't attempt to unregister
        client->mPriv->clientName.clear();

        return StreamTubeClientPtr();
    }

    return client;
}

StreamTubeClient::StreamTubeClient(
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
StreamTubeClient::~StreamTubeClient()
{
    if (!clientName().isNull()) {
        mPriv->registrar->unregisterClient(mPriv->handler);
    }
}

ClientRegistrarPtr StreamTubeClient::registrar() const
{
    return mPriv->registrar;
}

QString StreamTubeClient::clientName() const
{
    return mPriv->clientName;
}

bool StreamTubeClient::monitorsConnections() const
{
    return mPriv->handler->monitorsConnections();
}

void StreamTubeClient::onInvokedForTube(
        const AccountPtr &acc,
        const StreamTubeChannelPtr &tube,
        const QDateTime &time,
        const ChannelRequestHints &hints)
{
    Q_ASSERT(tube->isRequested());

    IncomingStreamTubeChannelPtr outgoing = IncomingStreamTubeChannelPtr::qObjectCast(tube);

    if (!outgoing) {
        warning() << "The ChannelFactory used by StreamTubeClient must construct" <<
            "IncomingStreamTubeChannel subclasses for Requested=false StreamTubes";
        tube->requestClose();
        return;
    } else if (!mPriv->acceptsAsTcp && !mPriv->acceptsAsUnix) {
        warning() << "STubeClient not set to accept, closing tube" << tube->objectPath();
        tube->requestClose();
        return;
    }

    // TODO: emit tubeOffered and begin tracking tube

    // TODO: accept the tube, connect accept result to tracking wrapper, if successful before tube
    // is invalidated, emit tubeAcceptedAsTcp/Unix, if failure close tube and emit tubeClosed with
    // the acceptTubeAs*Socket() PendingOperation error, if invalidated first, just emit tubeClosed
    // and remove from tracking
}

} // Tp
