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
          isRegistered(false),
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
    bool isRegistered;

    QHostAddress exportedAddr;
    quint16 exportedPort;
    QVariantMap exportedParams;

    QHash<StreamTubeChannelPtr, QPair<QString, QString> > offerErrors;
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
    connect(mPriv->handler.data(),
            SIGNAL(tubeInvalidated(
                    Tp::AccountPtr,
                    Tp::StreamTubeChannelPtr,
                    QString,
                    QString)),
            SLOT(onTubeInvalidated(
                    Tp::AccountPtr,
                    Tp::StreamTubeChannelPtr,
                    QString,
                    QString)));
}

/**
 * Class destructor.
 */
StreamTubeServer::~StreamTubeServer()
{
    if (isRegistered()) {
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

bool StreamTubeServer::isRegistered() const
{
    return mPriv->isRegistered;
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

    if (!mPriv->isRegistered) {
        debug() << "Register StreamTubeServer with name " << clientName();

        if (registrar()->registerClient(mPriv->handler, clientName())) {
            mPriv->isRegistered = true;
        } else {
            warning() << "StreamTubeServer" << clientName()
                << "registration failed";
        }
    }
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

    if (outgoing && outgoing->isValid()) {
        emit tubeRequested(acc, outgoing, time, hints);
    }

    if (!outgoing) {
        warning() << "The ChannelFactory used by StreamTubeServer must construct" <<
            "OutgoingStreamTubeChannel subclasses for Requested=true StreamTubes";
        tube->requestClose();
        return;
    } else if (!outgoing->isValid()) {
        warning() << "A tube received by StreamTubeServer is already invalidated, ignoring";
        return;
    } else if (mPriv->exportedAddr.isNull() || !mPriv->exportedPort) {
        warning() << "No socket exported, closing tube" << tube->objectPath();
        mPriv->offerErrors.insert(tube,
                QPair<QString, QString>(
                    TP_QT4_ERROR_NOT_AVAILABLE, QLatin1String("no socket exported")));
        tube->requestClose();
        return;
    }

    debug().nospace() << "Offering socket " << mPriv->exportedAddr << ":" << mPriv->exportedPort <<
        " on tube " << tube->objectPath();

    connect(outgoing->offerTcpSocket(mPriv->exportedAddr, mPriv->exportedPort, mPriv->exportedParams),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeOffered(Tp::PendingOperation*)));

    // TODO: start monitoring connections if requested
}

void StreamTubeServer::onTubeOffered(
        Tp::PendingOperation *op)
{
    OutgoingStreamTubeChannelPtr tube = OutgoingStreamTubeChannelPtr::dynamicCast(op->_object());
    Q_ASSERT(!tube.isNull());
    AccountPtr acc = mPriv->handler->accountForTube(tube);

    if (!acc) {
        debug() << "Ignoring Offer() return for already invalidated tube\n";
        return;
    } else  if (op->isError()) {
        warning() << "Offer() failed, closing tube" << tube->objectPath() << '-' <<
            op->errorName() << ':' << op->errorMessage();
        mPriv->offerErrors.insert(tube,
                QPair<QString, QString>(op->errorName(), op->errorMessage()));
        tube->requestClose();
        return;
    }

    debug() << "Tube" << tube->objectPath() << "offered successfully";
}

void StreamTubeServer::onTubeInvalidated(
        const Tp::AccountPtr &acc,
        const StreamTubeChannelPtr &tube,
        const QString &error,
        const QString &message)
{
    debug() << "Tube" << tube->objectPath() << "invalidated with" << error << ':' << message;

    OutgoingStreamTubeChannelPtr outgoing = OutgoingStreamTubeChannelPtr::qObjectCast(tube);
    if (!outgoing) {
        // We haven't signaled tubeRequested either
        return;
    }

    if (mPriv->offerErrors.contains(outgoing)) {
        emit tubeClosed(acc, outgoing, mPriv->offerErrors.value(outgoing).first, mPriv->offerErrors.value(outgoing).second);
        mPriv->offerErrors.remove(outgoing);
    } else {
        emit tubeClosed(acc, outgoing, error, message);
    }
}

} // Tp
