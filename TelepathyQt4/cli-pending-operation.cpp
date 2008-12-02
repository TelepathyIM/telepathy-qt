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

#include "cli-pending-operation.h"

#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

#include "cli-pending-operation.moc.hpp"
#include "debug-internal.hpp"

namespace Telepathy
{
namespace Client
{


struct PendingOperation::Private
{
    inline Private()
      : errorName(QString()),
        errorMessage(QString()),
        finished(false)
    { }

    QString errorName;
    QString errorMessage;
    bool finished;
};


PendingOperation::PendingOperation(DBusProxy* proxy)
  : QObject(proxy),
    mPriv(new Private())
{
    connect(this, SIGNAL(destroyed(QObject*)),
        this, SLOT(selfDestroyed(QObject*)));
}


PendingOperation::~PendingOperation()
{
    delete mPriv;
}


void PendingOperation::setFinished()
{
    mPriv->finished = true;
}


void PendingOperation::setError(const QString& name, const QString& message)
{
    Q_ASSERT(!name.isEmpty());
    mPriv->errorName = name;
    mPriv->errorMessage = message;
}


void PendingOperation::setError(const QDBusError& error)
{
    setError(error.name(), error.message());
}


DBusProxy* PendingOperation::proxy() const
{
    return QObject::parent();
}


bool PendingOperation::isValid() const
{
    Q_ASSERT(mPriv->finished);
    return (mPriv->errorName.isEmpty());
}


bool PendingOperation::isFinished() const
{
    return mPriv->finished;
}


bool PendingOperation::isError() const
{
    Q_ASSERT(mPriv->finished);
    return (!mPriv->errorName.isEmpty());
}


QString PendingOperation::errorName() const
{
    Q_ASSERT(mPriv->finished);
    return mPriv->errorName;
}


QString PendingOperation::errorMessage() const
{
    Q_ASSERT(mPriv->finished);
    return mPriv->errorMessage;
}


void PendingOperation::selfDestroyed(QObject* self)
{
    // FIXME: signal finished with a synthetic error here?
    // need to work out exactly what the life-cycle is first
}


PendingVoidMethodCall::PendingVoidMethodCall(DBusProxy* proxy,
    QDBusPendingCall call)
  : PendingOperation(proxy),
    mPriv(0)
{
    connect(new QDBusPendingCallWatcher(call),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        this,
        SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}


void PendingVoidMethodCall::watcherFinished(QDBusPendingCallWatcher* watcher)
{
    setFinished();

    if (watcher->isError())
    {
        setError(watcher->error());
        Q_ASSERT(isError());
    }
    else
    {
        Q_ASSERT(isValid());
    }

    emit finished(this);
    deleteLater();
}


} // Telepathy::Client
} // Telepathy
