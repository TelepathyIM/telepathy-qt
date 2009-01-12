/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt4/Client/PendingHandles>
#include "_gen/pending-handles.moc.hpp"

#include "../debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct PendingHandles::Private
{
    Connection* connection;
    uint handleType;
    bool isRequest;
    QStringList namesRequested;
    UIntList handlesToReference;
    ReferencedHandles handles;
    ReferencedHandles alreadyHeld;
};

PendingHandles::PendingHandles(Connection* connection, uint handleType, const QStringList& names)
    : PendingOperation(connection), mPriv(new Private)
{
    debug() << "PendingHandles(request)";

    mPriv->connection = connection;
    mPriv->handleType = handleType;
    mPriv->isRequest = true;
    mPriv->namesRequested = names;
}

PendingHandles::PendingHandles(Connection* connection, uint handleType, const UIntList& handles, const UIntList& alreadyHeld)
    : PendingOperation(connection), mPriv(new Private)
{
    debug() << "PendingHandles(reference)";

    mPriv->connection = connection;
    mPriv->handleType = handleType;
    mPriv->isRequest = false;
    mPriv->handlesToReference = handles;
    mPriv->alreadyHeld = ReferencedHandles(connection, handleType, alreadyHeld);

    if (alreadyHeld == handles) {
        debug() << " All handles already held, finishing up instantly";
        mPriv->handles = mPriv->alreadyHeld;
        setFinished();
    }
}

PendingHandles::~PendingHandles()
{
    delete mPriv;
}

Connection* PendingHandles::connection() const
{
    return mPriv->connection;
}

uint PendingHandles::handleType() const
{
    return mPriv->handleType;
}

bool PendingHandles::isRequest() const
{
    return mPriv->isRequest;
}

bool PendingHandles::isReference() const
{
    return !isRequest();
}

const QStringList& PendingHandles::namesRequested() const
{
    return mPriv->namesRequested;
}

const UIntList& PendingHandles::handlesToReference() const
{
    return mPriv->handlesToReference;
}

ReferencedHandles PendingHandles::handles() const
{
    return mPriv->handles;
}

void PendingHandles::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    // Thanks QDBus for this the need for this error-handling code duplication
    if (mPriv->isRequest) {
        QDBusPendingReply<UIntList> reply = *watcher;

        debug() << "Received reply to RequestHandles";

        if (reply.isError()) {
            debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
            setFinishedWithError(reply.error());
        } else {
            mPriv->handles = ReferencedHandles(connection(), handleType(), reply.value());
            setFinished();
        }

        connection()->handleRequestLanded(handleType());
    } else {
        QDBusPendingReply<void> reply = *watcher;

        debug() << "Received reply to HoldHandles";

        if (reply.isError()) {
            debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
            setFinishedWithError(reply.error());
        } else {
            mPriv->handles = ReferencedHandles(connection(), handleType(), handlesToReference());
            setFinished();
        }
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
