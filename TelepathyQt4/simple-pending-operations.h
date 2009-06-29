/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2009 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_pending_operations_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_operations_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>

#include <TelepathyQt4/PendingOperation>

namespace Tp
{

class PendingSuccess : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingSuccess)

public:
    PendingSuccess(QObject *parent)
        : PendingOperation(parent)
    {
        setFinished();
    }
};

class PendingFailure : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingFailure)

public:
    PendingFailure(QObject *parent, const QString &name,
            const QString &message)
        : PendingOperation(parent)
    {
        setFinishedWithError(name, message);
    }

    PendingFailure(QObject *parent, const QDBusError &error)
        : PendingOperation(parent)
    {
        setFinishedWithError(error);
    }
};

class PendingVoidMethodCall : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingVoidMethodCall)

public:
    PendingVoidMethodCall(QObject *parent, QDBusPendingCall call);

private Q_SLOTS:
    void watcherFinished(QDBusPendingCallWatcher*);

private:
    struct Private;
    Private *mPriv;
};

} // Tp

#endif
