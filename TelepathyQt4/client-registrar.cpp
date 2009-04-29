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

ClientHandlerAdaptor::ClientHandlerAdaptor(AbstractClientHandler *client)
    : QDBusAbstractAdaptor(client),
      mClient(client)
{
    setAutoRelaySignals(true);
}

ClientHandlerAdaptor::~ClientHandlerAdaptor()
{
}

void ClientHandlerAdaptor::HandleChannels(const QDBusObjectPath &account,
        const QDBusObjectPath &connection,
        const Tp::ChannelDetailsList &channels,
        const Tp::ObjectPathList &requestsSatisfied,
        qulonglong userActionTime,
        const QVariantMap &handlerInfo)
{
    mClient->handleChannels(account, connection, channels,
            requestsSatisfied, userActionTime, handlerInfo);
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
    QHash<AbstractClientPtr, QString> clients;
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
    delete mPriv;
}

QList<AbstractClientPtr> ClientRegistrar::registeredClients() const
{
    return mPriv->clients.keys();
}

bool ClientRegistrar::registerClient(const AbstractClientPtr &client,
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
                .arg((ulong) client.data()));
    }

    if (mPriv->services.contains(busName) ||
        !mPriv->bus.registerService(busName)) {
        warning() << "Unable to register client: busName" <<
            busName << "already registered";
        return false;
    }

    QStringList interfaces;

    ClientHandlerAdaptor *clientHandlerAdaptor = 0;
    AbstractClientHandler *handler =
        qobject_cast<AbstractClientHandler*>(object);
    if (handler) {
        // export o.f.T.Client.Handler
        clientHandlerAdaptor = new ClientHandlerAdaptor(handler);
        interfaces.append(
                QLatin1String("org.freedesktop.Telepathy.Client.Handler"));
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
        delete clientAdaptor;
        mPriv->bus.unregisterService(busName);
        return false;
    }

    debug() << "Client registered - busName:" << busName <<
        "objectPath:" << objectPath;

    mPriv->services.insert(busName);
    mPriv->clients.insert(client, objectPath);

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

    QString busName = objectPath.mid(1).replace('/', '.');
    if (busName != mPriv->busName) {
        // unique
        mPriv->bus.unregisterService(busName);
        mPriv->services.remove(busName);
    }

    return true;
}

void ClientRegistrar::unregisterClients()
{
    QHash<AbstractClientPtr, QString>::const_iterator end =
        mPriv->clients.constEnd();
    QHash<AbstractClientPtr, QString>::const_iterator it =
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
