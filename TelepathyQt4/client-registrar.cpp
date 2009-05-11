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

QHash<QPair<QString, QString>, QList<ClientHandlerAdaptor *> > ClientHandlerAdaptor::mAdaptorsForConnection;

ClientHandlerAdaptor::ClientHandlerAdaptor(const QDBusConnection &bus,
        AbstractClientHandler *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mBus(bus),
      mClient(client)
{
    QList<ClientHandlerAdaptor *> &handlerAdaptors =
        mAdaptorsForConnection[qMakePair(mBus.name(), mBus.baseService())];
    handlerAdaptors.append(this);
}

ClientHandlerAdaptor::~ClientHandlerAdaptor()
{
    QPair<QString, QString> busId = qMakePair(mBus.name(), mBus.baseService());
    QList<ClientHandlerAdaptor *> &handlerAdaptors =
        mAdaptorsForConnection[busId];
    handlerAdaptors.removeOne(this);
    if (handlerAdaptors.isEmpty()) {
        mAdaptorsForConnection.remove(busId);
    }
}

void ClientHandlerAdaptor::HandleChannels(const QDBusObjectPath &accountPath,
        const QDBusObjectPath &connectionPath,
        const Tp::ChannelDetailsList &channelDetailsList,
        const Tp::ObjectPathList &requestsSatisfied,
        qulonglong userActionTime_t,
        const QVariantMap &handlerInfo,
        const QDBusMessage &message)
{
    debug() << "HandleChannels: account:" << accountPath.path() <<
        ", connection:" << connectionPath.path();

    AccountPtr account = Account::create(mBus,
            TELEPATHY_ACCOUNT_MANAGER_BUS_NAME,
            accountPath.path());

    QString connectionBusName = connectionPath.path().mid(1).replace('/', '.');
    ConnectionPtr connection = Connection::create(mBus, connectionBusName,
            connectionPath.path());

    QList<ChannelPtr> channels;
    ChannelPtr channel;
    foreach (const ChannelDetails &channelDetails, channelDetailsList) {
        channel = Channel::create(connection, channelDetails.channel.path(),
                channelDetails.properties);
        channels.append(channel);
    }

    QList<ChannelRequestPtr> channelRequests;
    ChannelRequestPtr channelRequest;
    foreach (const QDBusObjectPath &path, requestsSatisfied) {
        channelRequest = ChannelRequest::create(mBus,
                path.path(), QVariantMap());
        channelRequests.append(channelRequest);
    }

    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    QDateTime userActionTime;
    if (userActionTime_t != 0) {
        userActionTime = QDateTime::fromTime_t((uint) userActionTime_t);
    }

    PendingClientOperation *operation = new PendingClientOperation(mBus,
            message, this);
    connect(operation,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onOperationFinished(Tp::PendingOperation*)));

    mClient->handleChannels(operation, account, connection, channels,
            channelRequests, userActionTime, handlerInfo);

    mOperations.insert(operation, channels);
}

void ClientHandlerAdaptor::onOperationFinished(PendingOperation *op)
{
    if (!op->isError()) {
        debug() << "HandleChannels operation finished successfully, "
            "updating handled channels";
        QList<ChannelPtr> channels =
            mOperations.value(dynamic_cast<PendingClientOperation*>(op));
        foreach (const ChannelPtr &channel, channels) {
            mHandledChannels.insert(channel);
            connect(channel.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SLOT(onChannelInvalidated(Tp::DBusProxy *)));
        }
    }

    mOperations.remove(dynamic_cast<PendingClientOperation*>(op));
}

void ClientHandlerAdaptor::onChannelInvalidated(DBusProxy *proxy)
{
    ChannelPtr channel(dynamic_cast<Channel*>(proxy));
    mHandledChannels.remove(channel);
}

ClientHandlerRequestsAdaptor::ClientHandlerRequestsAdaptor(
        const QDBusConnection &bus,
        AbstractClientHandler *client,
        QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mBus(bus),
      mClient(client)
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
    mClient->addRequest(ChannelRequest::create(mBus,
                request.path(), requestProperties));
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

struct ClientRegistrar::Private
{
    Private(const QDBusConnection &bus)
        : bus(bus)
    {
    }

    QDBusConnection bus;
    QHash<AbstractClientPtr, QString> clients;
    QHash<AbstractClientPtr, QObject*> clientObjects;
    QSet<QString> services;
};

QHash<QPair<QString, QString>, ClientRegistrar*> ClientRegistrar::registrarForConnection;

ClientRegistrarPtr ClientRegistrar::create()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    return create(bus);
}

ClientRegistrarPtr ClientRegistrar::create(const QDBusConnection &bus)
{
    QPair<QString, QString> busId =
        qMakePair(bus.name(), bus.baseService());
    if (registrarForConnection.contains(busId)) {
        return ClientRegistrarPtr(
                registrarForConnection.value(busId));
    }
    return ClientRegistrarPtr(new ClientRegistrar(bus));
}

ClientRegistrar::ClientRegistrar(const QDBusConnection &bus)
    : mPriv(new Private(bus))
{
    registrarForConnection.insert(qMakePair(bus.name(),
                bus.baseService()), this);
}

ClientRegistrar::~ClientRegistrar()
{
    registrarForConnection.remove(qMakePair(mPriv->bus.name(),
                mPriv->bus.baseService()));
    unregisterClients();
    delete mPriv;
}

QDBusConnection ClientRegistrar::dbusConnection() const
{
    return mPriv->bus;
}

QList<AbstractClientPtr> ClientRegistrar::registeredClients() const
{
    return mPriv->clients.keys();
}

bool ClientRegistrar::registerClient(const AbstractClientPtr &client,
        const QString &clientName, bool unique)
{
    if (!client) {
        warning() << "Unable to register a null client";
        return false;
    }

    if (mPriv->clients.contains(client)) {
        debug() << "Client already registered";
        return true;
    }

    QString busName = QLatin1String("org.freedesktop.Telepathy.Client.");
    busName.append(clientName);
    if (unique) {
        // o.f.T.Client.<unique_bus_name>_<pointer> should be enough to identify
        // an unique identifier
        busName.append(QString(".%1._%2")
                .arg(mPriv->bus.baseService()
                    .replace(':', '_')
                    .replace('.', "._"))
                .arg((intptr_t) client.data()));
    }

    if (mPriv->services.contains(busName) ||
        !mPriv->bus.registerService(busName)) {
        warning() << "Unable to register client: busName" <<
            busName << "already registered";
        return false;
    }

    QObject *object = new QObject(this);
    QStringList interfaces;

    AbstractClientHandler *handler =
        dynamic_cast<AbstractClientHandler*>(client.data());

    if (handler) {
        // export o.f.T.Client.Handler
        new ClientHandlerAdaptor(mPriv->bus, handler, object);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Handler"));
        if (handler->wantsRequestNotification()) {
            // export o.f.T.Client.Interface.Requests
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
    new ClientAdaptor(interfaces, object);

    QString objectPath = QString("/%1").arg(busName);
    objectPath.replace('.', '/');
    if (!mPriv->bus.registerObject(objectPath, object)) {
        // this shouldn't happen, but let's make sure
        warning() << "Unable to register client: objectPath" <<
            objectPath << "already registered";
        // cleanup
        delete object;
        mPriv->bus.unregisterService(busName);
        return false;
    }

    debug() << "Client registered - busName:" << busName <<
        "objectPath:" << objectPath << "interfaces:" << interfaces;

    mPriv->services.insert(busName);
    mPriv->clients.insert(client, objectPath);
    mPriv->clientObjects.insert(client, object);

    return true;
}

bool ClientRegistrar::unregisterClient(const AbstractClientPtr &client)
{
    if (!mPriv->clients.contains(client)) {
        warning() << "Trying to unregister an unregistered client";
        return false;
    }

    QString objectPath = mPriv->clients.value(client);
    mPriv->bus.unregisterObject(objectPath);
    mPriv->clients.remove(client);
    QObject *object = mPriv->clientObjects.value(client);
    // delete object here and it's children (adaptors), to make sure if adaptor
    // is keeping a static list of adaptors per connection, the list is updated.
    delete object;
    mPriv->clientObjects.remove(client);

    QString busName = objectPath.mid(1).replace('/', '.');
    mPriv->bus.unregisterService(busName);
    mPriv->services.remove(busName);

    debug() << "Client unregistered - busName:" << busName <<
        "objectPath:" << objectPath;

    return true;
}

void ClientRegistrar::unregisterClients()
{
    // copy the hash as it will be modified
    QHash<AbstractClientPtr, QString> clients = mPriv->clients;

    QHash<AbstractClientPtr, QString>::const_iterator end =
        clients.constEnd();
    QHash<AbstractClientPtr, QString>::const_iterator it =
        clients.constBegin();
    while (it != end) {
        unregisterClient(it.key());
        ++it;
    }
}

} // Tp
