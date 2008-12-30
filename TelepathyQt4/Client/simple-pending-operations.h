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

#ifndef _TelepathyQt4_cli_pending_operations_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_operations_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_INTERNALS
#error IN_TELEPATHY_QT4_INTERNALS
#endif

#include <QObject>

#include <TelepathyQt4/Client/PendingOperation>

namespace Telepathy
{
namespace Client
{


/**
 * A %PendingOperation that is always successful.
 */
class PendingSuccess : public PendingOperation
{
    Q_OBJECT

public:
    PendingSuccess(QObject* parent)
        : PendingOperation(parent)
    {
        setFinished();
    }
};


/**
 * A %PendingOperation that always fails with the error passed to the
 * constructor.
 */
class PendingFailure : public PendingOperation
{
    Q_OBJECT

public:
    PendingFailure(QObject* parent, const QString& name,
            const QString& message)
        : PendingOperation(parent)
    {
        setFinishedWithError(name, message);
    }

    PendingFailure(QObject* parent, const QDBusError& error)
        : PendingOperation(parent)
    {
        setFinishedWithError(error);
    }
};


/**
 * Generic subclass of %PendingOperation representing a pending D-Bus method
 * call that does not return anything (or returns a result that is not
 * interesting).
 *
 * Objects of this class indicate the success or failure of the method call,
 * but if the method call succeeds, no additional information is available.
 */
class PendingVoidMethodCall : public PendingOperation
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * \param parent The object on which this pending operation takes place
     * \param call A pending call as returned by the auto-generated low level
     *             Telepathy API; if the method returns anything, the return
     *             value(s) will be ignored
     */
    PendingVoidMethodCall(QObject* parent, QDBusPendingCall call);

private Q_SLOTS:
    void watcherFinished(QDBusPendingCallWatcher*);

private:
    // just ABI padding at the moment
    struct Private;
    Private *mPriv;
};


} // Telepathy::Client
} // Telepathy

#endif
