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
#include "TelepathyQt4/stream-tube-server-internal.h"

#include "TelepathyQt4/_gen/stream-tube-server.moc.hpp"
#include "TelepathyQt4/_gen/stream-tube-server-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/simple-stream-tube-handler.h"

#include <QScopedPointer>
#include <QTcpServer>

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/StreamTubeChannel>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT FixedParametersGenerator : public StreamTubeServer::ParametersGenerator
{
    public:

        FixedParametersGenerator(const QVariantMap &params) : mParams(params) {}

        QVariantMap nextParameters(const AccountPtr &, const OutgoingStreamTubeChannelPtr &,
                const ChannelRequestHints &) const
        {
            return mParams;
        }

    private:

        QVariantMap mParams;
};

struct StreamTubeServer::Private
{
    Private(const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices,
            const QString &maybeClientName,
            bool monitorConnections)
        : registrar(registrar),
          handler(SimpleStreamTubeHandler::create(p2pServices, roomServices, true, monitorConnections)),
          clientName(maybeClientName),
          isRegistered(false),
          exportedPort(0),
          generator(0)
    {
        if (clientName.isEmpty()) {
            clientName = QString::fromLatin1("TpQt4STubeServer_%1_%2")
                .arg(registrar->dbusConnection().baseService()
                        .replace(QLatin1Char(':'), QLatin1Char('_'))
                        .replace(QLatin1Char('.'), QLatin1Char('_')))
                .arg((intptr_t) this, 0, 16);
        }
    }

    void ensureRegistered()
    {
        if (isRegistered) {
            return;
        }

        debug() << "Register StreamTubeServer with name " << clientName;

        if (registrar->registerClient(handler, clientName)) {
            isRegistered = true;
        } else {
            warning() << "StreamTubeServer" << clientName
                << "registration failed";
        }
    }

    ClientRegistrarPtr registrar;
    SharedPtr<SimpleStreamTubeHandler> handler;
    QString clientName;
    bool isRegistered;

    QHostAddress exportedAddr;
    quint16 exportedPort;
    const ParametersGenerator *generator;
    QScopedPointer<FixedParametersGenerator> fixedGenerator;

    QHash<StreamTubeChannelPtr, TubeWrapper *> tubes;

};

StreamTubeServer::TubeWrapper::TubeWrapper(const AccountPtr &acc,
        const OutgoingStreamTubeChannelPtr &tube, const QHostAddress &exportedAddr,
        quint16 exportedPort, const QVariantMap &params)
    : mAcc(acc), mTube(tube), mParams(params)
{
    connect(tube->offerTcpSocket(exportedAddr, exportedPort, params),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeOffered(Tp::PendingOperation*)));
    connect(tube.data(),
            SIGNAL(newConnection(uint)),
            SLOT(onNewConnection(uint)));
    connect(tube.data(),
            SIGNAL(connectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));
}

void StreamTubeServer::TubeWrapper::onTubeOffered(Tp::PendingOperation *op)
{
    emit offerFinished(this, op);
}

void StreamTubeServer::TubeWrapper::onNewConnection(uint conn)
{
    emit newConnection(this, conn);
}

void StreamTubeServer::TubeWrapper::onConnectionClosed(uint conn, const QString &error,
        const QString &message)
{
    emit connectionClosed(this, conn, error, message);
}

StreamTubeServerPtr StreamTubeServer::create(
        const QStringList &p2pServices,
        const QStringList &roomServices,
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
            p2pServices,
            roomServices,
            clientName,
            monitorConnections);
}

StreamTubeServerPtr StreamTubeServer::create(
        const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const QStringList &p2pServices,
        const QStringList &roomServices,
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
            p2pServices,
            roomServices,
            clientName,
            monitorConnections);
}

StreamTubeServerPtr StreamTubeServer::create(
        const AccountManagerPtr &accountManager,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
{
    return create(
            accountManager->dbusConnection(),
            accountManager->accountFactory(),
            accountManager->connectionFactory(),
            accountManager->channelFactory(),
            accountManager->contactFactory(),
            p2pServices,
            roomServices,
            clientName,
            monitorConnections);
}

StreamTubeServerPtr StreamTubeServer::create(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
{
    return StreamTubeServerPtr(
            new StreamTubeServer(registrar, p2pServices, roomServices, clientName,
                monitorConnections));
}

StreamTubeServer::StreamTubeServer(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections)
: mPriv(new Private(registrar, p2pServices, roomServices, clientName, monitorConnections))
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
    if (isRegistered()) {
        mPriv->registrar->unregisterClient(mPriv->handler);
    }

    foreach (TubeWrapper *wrapper, mPriv->tubes.values()) {
        wrapper->deleteLater();
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
    if (!mPriv->generator) {
        return QVariantMap();
    }

    const FixedParametersGenerator *generator =
        dynamic_cast<const FixedParametersGenerator *>(mPriv->generator);

    if (generator) {
        return generator->nextParameters(AccountPtr(), OutgoingStreamTubeChannelPtr(),
                ChannelRequestHints());
    } else {
        return QVariantMap();
    }
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

    mPriv->generator = 0;
    if (!params.isEmpty()) {
        mPriv->fixedGenerator.reset(new FixedParametersGenerator(params));
        mPriv->generator = mPriv->fixedGenerator.data();
    }

    mPriv->ensureRegistered();
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

void StreamTubeServer::exportTcpSocket(
        const QHostAddress &addr,
        quint16 port,
        const ParametersGenerator *generator)
{
    if (addr.isNull() || port == 0) {
        warning() << "Attempted to export null TCP socket address or zero port, ignoring";
        return;
    }

    mPriv->exportedAddr = addr;
    mPriv->exportedPort = port;
    mPriv->generator = generator;

    mPriv->ensureRegistered();
}

void StreamTubeServer::exportTcpSocket(
        const QTcpServer *server,
        const ParametersGenerator *generator)
{
    if (!server->isListening()) {
        warning() << "Attempted to export non-listening QTcpServer, ignoring";
        return;
    }

    if (server->serverAddress() == QHostAddress::Any) {
        return exportTcpSocket(QHostAddress::LocalHost, server->serverPort(), generator);
    } else if (server->serverAddress() == QHostAddress::AnyIPv6) {
        return exportTcpSocket(QHostAddress::LocalHostIPv6, server->serverPort(), generator);
    } else {
        return exportTcpSocket(server->serverAddress(), server->serverPort(), generator);
    }
}

QList<QPair<AccountPtr, OutgoingStreamTubeChannelPtr> > StreamTubeServer::tubes() const
{
    QList<QPair<AccountPtr, OutgoingStreamTubeChannelPtr> > tubes;

    foreach (TubeWrapper *wrapper, mPriv->tubes.values()) {
        tubes.push_back(qMakePair(wrapper->mAcc, wrapper->mTube));
    }

    return tubes;
}

QHash<QPair<QHostAddress /* sourceAddress */, quint16 /* sourcePort */>,
      QPair<AccountPtr, ContactPtr> >
    StreamTubeServer::tcpConnections() const
{
    QHash<QPair<QHostAddress /* sourceAddress */, quint16 /* sourcePort */>,
          QPair<AccountPtr, ContactPtr> > conns;
    if (!monitorsConnections()) {
        warning() << "StreamTubeServer::tcpConnections() used, but connection monitoring is disabled";
        return conns;
    }

    QPair<AccountPtr, OutgoingStreamTubeChannelPtr> tube;
    foreach (tube, tubes()) {
        if (tube.second->addressType() != SocketAddressTypeIPv4 &&
                tube.second->addressType() != SocketAddressTypeIPv6)
            continue;

        QHash<QPair<QHostAddress,quint16>, uint> srcAddrConns =
            tube.second->connectionsForSourceAddresses();
        QHash<uint, Tp::ContactPtr> connContacts =
            tube.second->contactsForConnections();
        QPair<QHostAddress, quint16> srcAddr;
        foreach(srcAddr, srcAddrConns.keys()) {
            conns.insert(srcAddr,
                    qMakePair(tube.first, connContacts.value(srcAddrConns.value(srcAddr))));
        }
    }

    return conns;
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
        emit tubeClosed(acc, outgoing,
                TP_QT4_ERROR_NOT_AVAILABLE, QLatin1String("no socket exported"));
        tube->requestClose();
        return;
    }

    if (!mPriv->tubes.contains(tube)) {
        debug().nospace() << "Offering socket " << mPriv->exportedAddr << ":" << mPriv->exportedPort
            << " on tube " << tube->objectPath();

        QVariantMap params;
        if (mPriv->generator) {
            params = mPriv->generator->nextParameters(acc, outgoing, hints);
        }

        TubeWrapper *wrapper =
            new TubeWrapper(acc, outgoing, mPriv->exportedAddr, mPriv->exportedPort, params);

        connect(wrapper,
                SIGNAL(offerFinished(TubeWrapper*,Tp::PendingOperation*)),
                SLOT(onOfferFinished(TubeWrapper*,Tp::PendingOperation*)));
        connect(tube.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                SLOT(onTubeInvalidated(Tp::DBusProxy*,QString,QString)));

        if (monitorsConnections()) {
            connect(wrapper,
                    SIGNAL(newConnection(TubeWrapper*,uint)),
                    SLOT(onNewConnection(TubeWrapper*,uint)));
            connect(wrapper,
                    SIGNAL(connectionClosed(TubeWrapper*,uint,QString,QString)),
                    SLOT(onConnectionClosed(TubeWrapper*,uint,QString,QString)));
        }

        mPriv->tubes.insert(outgoing, wrapper);
    }
}

void StreamTubeServer::onOfferFinished(
        TubeWrapper *wrapper,
        Tp::PendingOperation *op)
{
    OutgoingStreamTubeChannelPtr tube = wrapper->mTube;

    if (op->isError()) {
        warning() << "Offer() failed, closing tube" << tube->objectPath() << '-' <<
            op->errorName() << ':' << op->errorMessage();

        if (wrapper->mTube->isValid())
            wrapper->mTube->requestClose();

        wrapper->mTube->disconnect(this);
        emit tubeClosed(wrapper->mAcc, wrapper->mTube, op->errorName(), op->errorMessage());
        mPriv->tubes.remove(wrapper->mTube);
        wrapper->deleteLater();
    } else {
        debug() << "Tube" << tube->objectPath() << "offered successfully";
    }
}

void StreamTubeServer::onTubeInvalidated(
        Tp::DBusProxy *proxy,
        const QString &error,
        const QString &message)
{
    OutgoingStreamTubeChannelPtr tube(qobject_cast<OutgoingStreamTubeChannel *>(proxy));
    Q_ASSERT(!tube.isNull());

    TubeWrapper *wrapper = mPriv->tubes.value(tube);
    if (!wrapper) {
        // Offer finish with error already removed it
        return;
    }

    debug() << "Tube" << tube->objectPath() << "invalidated with" << error << ':' << message;

    emit tubeClosed(wrapper->mAcc, wrapper->mTube, error, message);
    mPriv->tubes.remove(tube);
    delete wrapper;
}

void StreamTubeServer::onNewConnection(
        TubeWrapper *wrapper,
        uint conn)
{
    Q_ASSERT(monitorsConnections());

    if (wrapper->mTube->addressType() == SocketAddressTypeIPv4
            || wrapper->mTube->addressType() == SocketAddressTypeIPv6) {
        QHash<QPair<QHostAddress,quint16>, uint> srcAddrConns =
            wrapper->mTube->connectionsForSourceAddresses();
        QHash<uint, Tp::ContactPtr> connContacts =
            wrapper->mTube->contactsForConnections();

        QPair<QHostAddress, quint16> srcAddr = srcAddrConns.key(conn);
        emit newTcpConnection(srcAddr.first, srcAddr.second, wrapper->mAcc,
                connContacts.value(conn), wrapper->mTube);
    } else {
        // No UNIX socket should ever have been offered yet
        Q_ASSERT(false);
    }
}

void StreamTubeServer::onConnectionClosed(
        TubeWrapper *wrapper,
        uint conn,
        const QString &error,
        const QString &message)
{
    Q_ASSERT(monitorsConnections());

    if (wrapper->mTube->addressType() == SocketAddressTypeIPv4
            || wrapper->mTube->addressType() == SocketAddressTypeIPv6) {
        QHash<QPair<QHostAddress,quint16>, uint> srcAddrConns =
            wrapper->mTube->connectionsForSourceAddresses();
        QHash<uint, Tp::ContactPtr> connContacts =
            wrapper->mTube->contactsForConnections();

        QPair<QHostAddress, quint16> srcAddr = srcAddrConns.key(conn);
        emit tcpConnectionClosed(srcAddr.first, srcAddr.second, wrapper->mAcc,
                connContacts.value(conn), error, message, wrapper->mTube);
    } else {
        // No UNIX socket should ever have been offered yet
        Q_ASSERT(false);
    }
}

} // Tp
