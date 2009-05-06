/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/ClientRegistrar>
#include "TelepathyQt4/client-registrar-internal.h"

#include "TelepathyQt4/_gen/client-registrar.moc.hpp"
#include "TelepathyQt4/_gen/client-registrar-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/ClientObject>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingClientOperation>
#include <TelepathyQt4/PendingReady>

namespace Tp
{

ClientAdaptor::ClientAdaptor(const QStringList &interfaces,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mInterfaces(interfaces)
{
}

ClientAdaptor::~ClientAdaptor()
{
}

ClientHandlerAdaptor::ClientHandlerAdaptor(const QDBusConnection &bus,
        const AbstractClientHandlerPtr &client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mBus(bus),
      mClient(client),
      mProcessingHandleChannels(false)
{
}

ClientHandlerAdaptor::~ClientHandlerAdaptor()
{
}

void ClientHandlerAdaptor::HandleChannels(const QDBusObjectPath &account,
        const QDBusObjectPath &connection,
        const Tp::ChannelDetailsList &channels,
        const Tp::ObjectPathList &requestsSatisfied,
        qulonglong userActionTime,
        const QVariantMap &handlerInfo,
        const QDBusMessage &message)
{
    debug() << "HandleChannels: account:" << account.path() << ", connection:"
        << connection.path();
    mHandleChannelsQueue.enqueue(new HandleChannelsCall(mClient, account,
                connection, channels, requestsSatisfied,
                userActionTime, handlerInfo, mBus, message, this));
    processHandleChannelsQueue();
}

void ClientHandlerAdaptor::onHandleChannelsCallFinished()
{
    mHandleChannelsQueue.dequeue();
    mProcessingHandleChannels = false;
    processHandleChannelsQueue();
}

void ClientHandlerAdaptor::processHandleChannelsQueue()
{
    if (mProcessingHandleChannels || mHandleChannelsQueue.isEmpty()) {
        return;
    }

    mProcessingHandleChannels = true;
    HandleChannelsCall *call = mHandleChannelsQueue.head();
    connect(call,
            SIGNAL(finished()),
            SLOT(onHandleChannelsCallFinished()));
    call->process();
}

ClientHandlerAdaptor::HandleChannelsCall::HandleChannelsCall(
        const AbstractClientHandlerPtr &client,
        const QDBusObjectPath &account,
        const QDBusObjectPath &connection,
        const ChannelDetailsList &channels,
        const ObjectPathList &requestsSatisfied,
        qulonglong userActionTime,
        const QVariantMap &handlerInfo,
        const QDBusConnection &bus,
        const QDBusMessage &message,
        QObject *parent)
    : QObject(parent),
      mClient(client),
      mAccountPath(account),
      mConnectionPath(connection),
      mChannelDetailsList(channels),
      mRequestsSatisfied(requestsSatisfied),
      mUserActionTime(userActionTime),
      mHandlerInfo(handlerInfo),
      mBus(bus),
      mMessage(message),
      mOperation(new PendingClientOperation(bus, message, this))
{
}

ClientHandlerAdaptor::HandleChannelsCall::~HandleChannelsCall()
{
}

void ClientHandlerAdaptor::HandleChannelsCall::process()
{
    mAccount = Account::create(mBus,
            TELEPATHY_ACCOUNT_MANAGER_BUS_NAME,
            mAccountPath.path());
    connect(mAccount->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onObjectReady(Tp::PendingOperation *)));

    QString connectionBusName = mConnectionPath.path().mid(1).replace('/', '.');
    mConnection = Connection::create(mBus, connectionBusName,
            mConnectionPath.path());
    connect(mConnection->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onConnectionReady(Tp::PendingOperation *)));

    ChannelRequestPtr channelRequest;
    foreach (const QDBusObjectPath &path, mRequestsSatisfied) {
        channelRequest = ChannelRequest::create(mBus,
                path.path(), QVariantMap());
        mChannelRequests.append(channelRequest);
    }
}

void ClientHandlerAdaptor::HandleChannelsCall::onObjectReady(
        PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }
    checkFinished();
}

void ClientHandlerAdaptor::HandleChannelsCall::onConnectionReady(
        PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    ChannelPtr channel;
    foreach (const ChannelDetails &channelDetails, mChannelDetailsList) {
        channel = Channel::create(mConnection, channelDetails.channel.path(),
                channelDetails.properties);
        connect(channel->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onObjectReady(Tp::PendingOperation *)));
        mChannels.append(channel);
    }

    // just to make sure
    checkFinished();
}

void ClientHandlerAdaptor::HandleChannelsCall::checkFinished()
{
    if (!mAccount->isReady() || !mConnection->isReady()) {
        return;
    }

    foreach (const ChannelPtr &channel, mChannels) {
        if (!channel->isReady()) {
            return;
        }
    }

    // FIXME: Telepathy supports 64-bit time_t, but Qt only does so on
    // ILP64 systems (e.g. sparc64, but not x86_64). If QDateTime
    // gains a fromTimestamp64 method, we should use it instead.
    QDateTime userActionTime;
    if (mUserActionTime != 0) {
        userActionTime = QDateTime::fromTime_t((uint) mUserActionTime);
    }
    mClient->handleChannels(mOperation, mAccount, mConnection, mChannels,
            mChannelRequests, userActionTime, mHandlerInfo);
    emit finished();
    deleteLater();
}

void ClientHandlerAdaptor::HandleChannelsCall::setFinishedWithError(const QString &errorName,
        const QString &errorMessage)
{
    mOperation->setFinishedWithError(errorName, errorMessage);
    emit finished();
    deleteLater();
}

ClientHandlerRequestsAdaptor::ClientHandlerRequestsAdaptor(
        const QDBusConnection &bus,
        const AbstractClientHandlerPtr &client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mBus(bus),
      mClient(client),
      mProcessingAddRequest(false)
{
}

ClientHandlerRequestsAdaptor::~ClientHandlerRequestsAdaptor()
{
}

void ClientHandlerRequestsAdaptor::AddRequest(
        const QDBusObjectPath &request,
        const QVariantMap &requestProperties,
        const QDBusMessage &message)
{
    debug() << "AddRequest:" << request.path();
    mBus.send(message.createReply());

    mAddRequestQueue.enqueue(new AddRequestCall(mClient, request,
                requestProperties, mBus, this));
    processAddRequestQueue();
}

void ClientHandlerRequestsAdaptor::RemoveRequest(
        const QDBusObjectPath &request,
        const QString &errorName, const QString &errorMessage,
        const QDBusMessage &message)
{
    debug() << "RemoveRequest:" << request.path() << "-" << errorName
        << "-" << errorMessage;
    mBus.send(message.createReply());
    mClient->removeRequest(ChannelRequest::create(mBus,
                request.path(), QVariantMap()), errorName, errorMessage);
}

void ClientHandlerRequestsAdaptor::processAddRequestQueue()
{
    if (mProcessingAddRequest || mAddRequestQueue.isEmpty()) {
        return;
    }

    mProcessingAddRequest = true;
    AddRequestCall *call = mAddRequestQueue.head();
    connect(call,
            SIGNAL(finished()),
            SLOT(onAddRequestCallFinished()));
    call->process();
}

void ClientHandlerRequestsAdaptor::onAddRequestCallFinished()
{
    mAddRequestQueue.dequeue();
    mProcessingAddRequest = false;
    processAddRequestQueue();
}

ClientHandlerRequestsAdaptor::AddRequestCall::AddRequestCall(
        const AbstractClientHandlerPtr &client,
        const QDBusObjectPath &request,
        const QVariantMap &requestProperties,
        const QDBusConnection &bus,
        QObject *parent)
    : QObject(parent),
      mClient(client),
      mRequestPath(request),
      mRequestProperties(requestProperties),
      mBus(bus)
{
}

ClientHandlerRequestsAdaptor::AddRequestCall::~AddRequestCall()
{
}

void ClientHandlerRequestsAdaptor::AddRequestCall::process()
{
    mRequest = ChannelRequest::create(mBus,
            mRequestPath.path(), mRequestProperties);
    connect(mRequest->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onChannelRequestReady(Tp::PendingOperation *)));
}

void ClientHandlerRequestsAdaptor::AddRequestCall::onChannelRequestReady(
        PendingOperation *op)
{
    if (!op->isError()) {
        mClient->addRequest(mRequest);
    } else {
        warning() << "Unable to make ChannelRequest ready:" << op->errorName()
            << "-" << op->errorMessage() << ". Ignoring AddRequest call";
    }
    emit finished();
    deleteLater();
}

struct ClientRegistrar::Private
{
    Private(const QDBusConnection &bus, const QString &clientName)
        : bus(bus), clientName(clientName)
    {
        busName = QLatin1String("org.freedesktop.Telepathy.Client.");
        busName.append(clientName);
    }

    QDBusConnection bus;
    QString busName;
    QString clientName;
    QHash<ClientObjectPtr, QString> clients;
    QSet<QString> services;
};

ClientRegistrarPtr ClientRegistrar::create(const QString &clientName)
{
    return ClientRegistrarPtr(new ClientRegistrar(clientName));
}

ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus,
        const QString &clientName)
{
    return ClientRegistrarPtr(new ClientRegistrar(bus, clientName));
}

ClientRegistrar::ClientRegistrar(const QString &clientName)
    : mPriv(new Private(QDBusConnection::sessionBus(), clientName))
{
}

ClientRegistrar::ClientRegistrar(const QDBusConnection &bus,
        const QString &clientName)
    : mPriv(new Private(bus, clientName))
{
}

ClientRegistrar::~ClientRegistrar()
{
    unregisterClients();
    delete mPriv;
}

QDBusConnection ClientRegistrar::dbusConnection() const
{
    return mPriv->bus;
}

QString ClientRegistrar::clientName() const
{
    return mPriv->clientName;
}

QList<ClientObjectPtr> ClientRegistrar::registeredClients() const
{
    return mPriv->clients.keys();
}

bool ClientRegistrar::registerClient(const ClientObjectPtr &client,
        bool unique)
{
    if (!client) {
        warning() << "Unable to register a null client";
        return false;
    }

    if (mPriv->clients.contains(client)) {
        debug() << "Client already registered";
        return true;
    }

    QObject *object = client.data();
    QString busName(mPriv->busName);
    if (unique) {
        // o.f.T.Client.<unique_bus_name>_<pointer> should be enough to identify
        // an unique identifier
        busName.append(QString(".%1._%2")
                .arg(mPriv->bus.baseService()
                    .replace(':', '_')
                    .replace('.', "._"))
                .arg((intptr_t) object));
    }

    if (mPriv->services.contains(busName) ||
        !mPriv->bus.registerService(busName)) {
        warning() << "Unable to register client: busName" <<
            busName << "already registered";
        return false;
    }

    QStringList interfaces;

    ClientHandlerAdaptor *clientHandlerAdaptor = 0;
    ClientHandlerRequestsAdaptor *clientHandlerRequestsAdaptor = 0;
    AbstractClientHandlerPtr handler = client->clientHandler();
    if (handler) {
        // export o.f.T.Client.Handler
        clientHandlerAdaptor = new ClientHandlerAdaptor(mPriv->bus, handler, object);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Handler"));
        if (handler->isListeningRequests()) {
            // export o.f.T.Client.Interface.Requests
            clientHandlerRequestsAdaptor =
                new ClientHandlerRequestsAdaptor(mPriv->bus, handler, object);
            interfaces.append(
                    QLatin1String(
                        "org.freedesktop.Telepathy.Client.Interface.Requests"));
        }
    }

    // TODO add more adaptors when they exist

    if (interfaces.isEmpty()) {
        warning() << "Client does not implement any known interface";
        // cleanup
        mPriv->bus.unregisterService(busName);
        return false;
    }

    // export o.f,T,Client interface
    ClientAdaptor *clientAdaptor = new ClientAdaptor(interfaces, object);

    QString objectPath = QString("/%1").arg(busName);
    objectPath.replace('.', '/');
    if (!mPriv->bus.registerObject(objectPath, object)) {
        // this shouldn't happen, but let's make sure
        warning() << "Unable to register client: objectPath" <<
            objectPath << "already registered";
        // cleanup
        delete clientHandlerAdaptor;
        delete clientHandlerRequestsAdaptor;
        delete clientAdaptor;
        mPriv->bus.unregisterService(busName);
        return false;
    }

    debug() << "Client registered - busName:" << busName <<
        "objectPath:" << objectPath << "interfaces:" << interfaces;

    mPriv->services.insert(busName);
    mPriv->clients.insert(client, objectPath);

    return true;
}

bool ClientRegistrar::unregisterClient(const ClientObjectPtr &client)
{
    if (!mPriv->clients.contains(client)) {
        warning() << "Trying to unregister an unregistered client";
        return false;
    }

    QString objectPath = mPriv->clients.value(client);
    mPriv->bus.unregisterObject(objectPath);
    mPriv->clients.remove(client);

    QString busName = objectPath.mid(1).replace('/', '.');
    mPriv->bus.unregisterService(busName);
    mPriv->services.remove(busName);

    debug() << "Client unregistered - busName:" << busName <<
        "objectPath:" << objectPath;

    return true;
}

void ClientRegistrar::unregisterClients()
{
    QHash<ClientObjectPtr, QString>::const_iterator end =
        mPriv->clients.constEnd();
    QHash<ClientObjectPtr, QString>::const_iterator it =
        mPriv->clients.constBegin();
    while (it != end) {
        mPriv->bus.unregisterObject(it.value());
        ++it;
    }

    foreach (const QString &service, mPriv->services) {
        mPriv->bus.unregisterService(service);
    }

    mPriv->clients.clear();
    mPriv->services.clear();
}

} // Tp
