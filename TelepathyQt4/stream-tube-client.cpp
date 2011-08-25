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

#include "TelepathyQt4/stream-tube-client-internal.h"
#include "TelepathyQt4/_gen/stream-tube-client.moc.hpp"
#include "TelepathyQt4/_gen/stream-tube-client-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/simple-stream-tube-handler.h"

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/PendingStreamTubeConnection>
#include <TelepathyQt4/StreamTubeChannel>

#include <QHash>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT StreamTubeClient::Private
{
    Private(const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices,
            const QString &maybeClientName,
            bool monitorConnections,
            bool bypassApproval)
        : registrar(registrar),
          handler(SimpleStreamTubeHandler::create(
                      p2pServices, roomServices, false, monitorConnections, bypassApproval)),
          clientName(maybeClientName),
          isRegistered(false),
          acceptsAsTcp(false), acceptsAsUnix(false),
          tcpGenerator(0), requireCredentials(false)
    {
        if (clientName.isEmpty()) {
            clientName = QString::fromLatin1("TpQt4STubeClient_%1_%2")
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

        debug() << "Register StreamTubeClient with name " << clientName;

        if (registrar->registerClient(handler, clientName)) {
            isRegistered = true;
        } else {
            warning() << "StreamTubeClient" << clientName
                << "registration failed";
        }
    }

    ClientRegistrarPtr registrar;
    SharedPtr<SimpleStreamTubeHandler> handler;
    QString clientName;
    bool isRegistered;

    bool acceptsAsTcp, acceptsAsUnix;
    const TcpSourceAddressGenerator *tcpGenerator;
    bool requireCredentials;

    QHash<StreamTubeChannelPtr, TubeWrapper *> tubes;
};

StreamTubeClient::TubeWrapper::TubeWrapper(
        const AccountPtr &acc,
        const IncomingStreamTubeChannelPtr &tube,
        const QHostAddress &sourceAddress,
        quint16 sourcePort)
    : mAcc(acc), mTube(tube), mSourceAddress(sourceAddress), mSourcePort(sourcePort)
{
    connect(tube->acceptTubeAsTcpSocket(sourceAddress, sourcePort),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeAccepted(Tp::PendingOperation*)));
    connect(tube.data(),
            SIGNAL(newConnection(uint)),
            SLOT(onNewConnection(uint)));
    connect(tube.data(),
            SIGNAL(connectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));
}

StreamTubeClient::TubeWrapper::TubeWrapper(
        const AccountPtr &acc,
        const IncomingStreamTubeChannelPtr &tube,
        bool requireCredentials)
    : mAcc(acc), mTube(tube), mSourcePort(0)
{
    connect(tube->acceptTubeAsUnixSocket(requireCredentials),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onTubeAccepted(Tp::PendingOperation*)));
    connect(tube.data(),
            SIGNAL(newConnection(uint)),
            SLOT(onNewConnection(uint)));
    connect(tube.data(),
            SIGNAL(connectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));
}

void StreamTubeClient::TubeWrapper::onTubeAccepted(Tp::PendingOperation *op)
{
    emit acceptFinished(this, qobject_cast<Tp::PendingStreamTubeConnection *>(op));
}

void StreamTubeClient::TubeWrapper::onNewConnection(uint conn)
{
    emit newConnection(this, conn);
}

void StreamTubeClient::TubeWrapper::onConnectionClosed(uint conn, const QString &error,
        const QString &message)
{
    emit connectionClosed(this, conn, error, message);
}

StreamTubeClientPtr StreamTubeClient::create(
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval,
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
            monitorConnections,
            bypassApproval);
}

StreamTubeClientPtr StreamTubeClient::create(
        const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
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
            monitorConnections,
            bypassApproval);
}

StreamTubeClientPtr StreamTubeClient::create(
        const AccountManagerPtr &accountManager,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
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
            monitorConnections,
            bypassApproval);
}

StreamTubeClientPtr StreamTubeClient::create(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
{
    return StreamTubeClientPtr(
            new StreamTubeClient(registrar, p2pServices, roomServices, clientName,
                monitorConnections, bypassApproval));
}

StreamTubeClient::StreamTubeClient(
        const ClientRegistrarPtr &registrar,
        const QStringList &p2pServices,
        const QStringList &roomServices,
        const QString &clientName,
        bool monitorConnections,
        bool bypassApproval)
: mPriv(new Private(registrar, p2pServices, roomServices, clientName, monitorConnections, bypassApproval))
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
    if (isRegistered()) {
        mPriv->registrar->unregisterClient(mPriv->handler);
    }

    foreach (TubeWrapper *wrapper, mPriv->tubes.values()) {
        wrapper->deleteLater();
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

bool StreamTubeClient::isRegistered() const
{
    return mPriv->isRegistered;
}

bool StreamTubeClient::monitorsConnections() const
{
    return mPriv->handler->monitorsConnections();
}

bool StreamTubeClient::acceptsAsTcp() const
{
    return mPriv->acceptsAsTcp;
}

const StreamTubeClient::TcpSourceAddressGenerator *StreamTubeClient::generator() const
{
    if (!acceptsAsTcp()) {
        warning() << "StreamTubeClient::generator() used, but not accepting as TCP, returning 0";
        return 0;
    }

    return mPriv->tcpGenerator;
}

bool StreamTubeClient::acceptsAsUnix() const
{
    return mPriv->acceptsAsUnix;
}

void StreamTubeClient::setToAcceptAsTcp(const TcpSourceAddressGenerator *generator)
{
    mPriv->tcpGenerator = generator;
    mPriv->acceptsAsTcp = true;
    mPriv->acceptsAsUnix = false;

    mPriv->ensureRegistered();
}

void StreamTubeClient::setToAcceptAsUnix(bool requireCredentials)
{
    mPriv->tcpGenerator = 0;
    mPriv->acceptsAsTcp = false;
    mPriv->acceptsAsUnix = true;
    mPriv->requireCredentials = requireCredentials;

    mPriv->ensureRegistered();
}

QList<QPair<AccountPtr, IncomingStreamTubeChannelPtr> > StreamTubeClient::tubes() const
{
    QList<QPair<AccountPtr, IncomingStreamTubeChannelPtr> > tubes;

    foreach (TubeWrapper *wrapper, mPriv->tubes.values()) {
        tubes.push_back(qMakePair(wrapper->mAcc, wrapper->mTube));
    }

    return tubes;
}

QHash<QPair<AccountPtr, IncomingStreamTubeChannelPtr>, QSet<uint> >
    StreamTubeClient::connections() const
{
    QHash<QPair<AccountPtr, IncomingStreamTubeChannelPtr>, QSet<uint> > conns;
    if (!monitorsConnections()) {
        warning() << "StreamTubeClient::connections() used, but connection monitoring is disabled";
        return conns;
    }

    QPair<AccountPtr, IncomingStreamTubeChannelPtr> tube;
    foreach (tube, tubes()) {
        QSet<uint> tubeConns = QSet<uint>::fromList(tube.second->connections());
        if (!tubeConns.empty()) {
            conns.insert(tube, tubeConns);
        }
    }

    return conns;
}

void StreamTubeClient::onInvokedForTube(
        const AccountPtr &acc,
        const StreamTubeChannelPtr &tube,
        const QDateTime &time,
        const ChannelRequestHints &hints)
{
    Q_ASSERT(!tube->isRequested());

    if (mPriv->tubes.contains(tube)) {
        debug() << "Ignoring StreamTubeClient reinvocation for tube" << tube->objectPath();
        return;
    }

    IncomingStreamTubeChannelPtr incoming = IncomingStreamTubeChannelPtr::qObjectCast(tube);

    if (!incoming) {
        warning() << "The ChannelFactory used by StreamTubeClient must construct" <<
            "IncomingStreamTubeChannel subclasses for Requested=false StreamTubes";
        tube->requestClose();
        return;
    } else if (!mPriv->acceptsAsTcp && !mPriv->acceptsAsUnix) {
        warning() << "STubeClient not set to accept, closing tube" << tube->objectPath();
        tube->requestClose();
        return;
    }

    TubeWrapper *wrapper = 0;
    
    if (mPriv->acceptsAsTcp) {
        QPair<QHostAddress, quint16> srcAddr =
            qMakePair(QHostAddress(QHostAddress::Any), quint16(0));

        if (mPriv->tcpGenerator) {
            srcAddr = mPriv->tcpGenerator->nextSourceAddress(acc, incoming);
        }

        wrapper = new TubeWrapper(acc, incoming, srcAddr.first, srcAddr.second);
    } else {
        wrapper = new TubeWrapper(acc, incoming, mPriv->requireCredentials);
    }

    connect(wrapper,
            SIGNAL(acceptFinished(TubeWrapper*,Tp::PendingStreamTubeConnection*)),
            SLOT(onAcceptFinished(TubeWrapper*,Tp::PendingStreamTubeConnection*)));
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

    mPriv->tubes.insert(tube, wrapper);

    emit tubeOffered(acc, incoming);

    // TODO: accept the tube, connect accept result to tracking wrapper, if successful before tube
    // is invalidated, emit tubeAcceptedAsTcp/Unix, if failure close tube and emit tubeClosed with
    // the acceptTubeAs*Socket() PendingOperation error, if invalidated first, just emit tubeClosed
    // and remove from tracking
}

void StreamTubeClient::onAcceptFinished(TubeWrapper *wrapper, PendingStreamTubeConnection *conn)
{
    Q_ASSERT(wrapper != NULL);
    Q_ASSERT(conn != NULL);

    if (!mPriv->tubes.contains(wrapper->mTube)) {
        debug() << "StreamTubeClient ignoring Accept result for invalidated tube"
            << wrapper->mTube->objectPath();
        return;
    }

    if (conn->isError()) {
        warning() << "StreamTubeClient couldn't accept tube" << wrapper->mTube->objectPath() << '-'
            << conn->errorName() << ':' << conn->errorMessage();

        if (wrapper->mTube->isValid())
            wrapper->mTube->requestClose();

        wrapper->mTube->disconnect(this);
        emit tubeClosed(wrapper->mAcc, wrapper->mTube, conn->errorName(), conn->errorMessage());
        mPriv->tubes.remove(wrapper->mTube);
        wrapper->deleteLater();
        return;
    }

    debug() << "StreamTubeClient accepted tube" << wrapper->mTube->objectPath();

    if (conn->addressType() == SocketAddressTypeIPv4
            || conn->addressType() == SocketAddressTypeIPv6) {
        QPair<QHostAddress, quint16> addr = conn->ipAddress();
        emit tubeAcceptedAsTcp(wrapper->mAcc, wrapper->mTube, addr.first, addr.second,
                wrapper->mSourceAddress, wrapper->mSourcePort);
    } else {
        emit tubeAcceptedAsUnix(wrapper->mAcc, wrapper->mTube, conn->localAddress(),
                conn->requiresCredentials(), conn->credentialByte());
    }
}

void StreamTubeClient::onTubeInvalidated(Tp::DBusProxy *proxy, const QString &error,
        const QString &message)
{
    StreamTubeChannelPtr tube(qobject_cast<StreamTubeChannel *>(proxy));
    Q_ASSERT(!tube.isNull());

    TubeWrapper *wrapper = mPriv->tubes.value(tube);
    if (!wrapper) {
        // Accept finish with error already removed it
        return;
    }

    debug() << "Client StreamTube" << tube->objectPath() << "invalidated - " << error << ':'
        << message;

    emit tubeClosed(wrapper->mAcc, wrapper->mTube, error, message);
    mPriv->tubes.remove(tube);
    delete wrapper;
}

void StreamTubeClient::onNewConnection(
        TubeWrapper *wrapper,
        uint conn)
{
    Q_ASSERT(monitorsConnections());
    emit newConnection(wrapper->mAcc, wrapper->mTube, conn);
}

void StreamTubeClient::onConnectionClosed(
        TubeWrapper *wrapper,
        uint conn,
        const QString &error,
        const QString &message)
{
    Q_ASSERT(monitorsConnections());
    emit connectionClosed(wrapper->mAcc, wrapper->mTube, conn, error, message);
}

} // Tp
