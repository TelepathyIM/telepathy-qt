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

#ifndef _TelepathyQt4_cli_pending_connection_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_connection_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingOperation>

#include <QSharedPointer>
#include <QString>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace Telepathy
{
namespace Client
{

class ConnectionManager;

class PendingConnection : public PendingOperation
{
    Q_OBJECT

public:
    ~PendingConnection();

    ConnectionManager *manager() const;
    QSharedPointer<Connection> connection() const;

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *watcher);

private:
    Q_DISABLE_COPY(PendingConnection);
    PendingConnection(ConnectionManager *manager,
            const QString &protocol, const QVariantMap &parameters);

    struct Private;
    friend struct Private;
    friend class ConnectionManager;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
