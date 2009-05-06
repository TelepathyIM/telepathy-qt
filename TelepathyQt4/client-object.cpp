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

#include <TelepathyQt4/ClientObject>

#include <TelepathyQt4/AbstractClient>

namespace Tp
{

struct ClientObject::Private
{
    AbstractClientHandlerPtr clientHandler;
};

ClientObjectPtr ClientObject::create(const AbstractClientHandlerPtr &clientHandler)
{
    if (!clientHandler) {
        return ClientObjectPtr();
    }
    return ClientObjectPtr(new ClientObject(clientHandler));
}

ClientObject::ClientObject(const AbstractClientHandlerPtr &clientHandler)
    : mPriv(new Private)
{
    mPriv->clientHandler = clientHandler;
}

ClientObject::~ClientObject()
{
    delete mPriv;
}

AbstractClientHandlerPtr ClientObject::clientHandler() const
{
    return mPriv->clientHandler;
}

} // Tp
