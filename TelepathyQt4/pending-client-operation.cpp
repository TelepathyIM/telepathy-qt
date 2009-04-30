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

#include <TelepathyQt4/PendingClientOperation>

#include "TelepathyQt4/_gen/pending-client-operation.moc.hpp"

namespace Tp
{

struct PendingClientOperation::Private
{
    Private(const QDBusConnection &bus, const QDBusMessage &message)
        : bus(bus), message(message)
    {
    }

    QDBusConnection bus;
    QDBusMessage message;
};

PendingClientOperation::PendingClientOperation(const QDBusConnection &bus,
        const QDBusMessage &message, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(bus, message))
{
    message.setDelayedReply(true);
}

PendingClientOperation::~PendingClientOperation()
{
    delete mPriv;
}

void PendingClientOperation::setFinished()
{
    if (!isFinished()) {
        mPriv->bus.send(mPriv->message.createReply());
    }
    PendingOperation::setFinished();
}

void PendingClientOperation::setFinishedWithError(const QString &name,
        const QString &message)
{
    if (!isFinished()) {
        mPriv->bus.send(mPriv->message.createErrorReply(name, message));
    }
    PendingOperation::setFinishedWithError(name, message);
}

void PendingClientOperation::setFinishedWithError(const QDBusError &error)
{
    if (!isFinished()) {
        mPriv->bus.send(mPriv->message.createErrorReply(error));
    }
    PendingOperation::setFinishedWithError(error);
}

} // Tp
