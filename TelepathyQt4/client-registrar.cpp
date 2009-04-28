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
};

ClientRegistrarPtr ClientRegistrar::create(const QString &clientName,
        bool unique)
{
    return ClientRegistrarPtr(new ClientRegistrar(clientName, unique));
}

ClientRegistrar::ClientRegistrar(const QString &clientName,
        bool unique)
    : mPriv(new Private)
{
}

ClientRegistrar::~ClientRegistrar()
{
    delete mPriv;
}

bool ClientRegistrar::addClient(const AbstractClientPtr &client)
{
    return false;
}

bool ClientRegistrar::removeClient(const AbstractClientPtr &client)
{
    return false;
}

bool ClientRegistrar::registerClients()
{
    return false;
}

bool ClientRegistrar::registerClient(const AbstractClientPtr &client)
{
    return false;
}

void ClientRegistrar::unregisterClients()
{
}

bool ClientRegistrar::unregisterClient(const AbstractClientPtr &client)
{
    return false;
}

} // Tp
