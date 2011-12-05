/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
 * @license LGPL 2.1
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

#ifndef _TelepathyQt_connection_internal_h_HEADER_GUARD_
#define _TelepathyQt_connection_internal_h_HEADER_GUARD_

#include <TelepathyQt/Connection>

#include <TelepathyQt/PendingReady>

#include <QSet>

namespace Tp
{

class TP_QT_NO_EXPORT Connection::PendingConnect : public PendingReady
{
    Q_OBJECT

public:
    PendingConnect(const ConnectionPtr &connection, const Features &requestedFeatures);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onConnectReply(QDBusPendingCallWatcher *);
    TP_QT_NO_EXPORT void onStatusChanged(Tp::ConnectionStatus newStatus);
    TP_QT_NO_EXPORT void onBecomeReadyReply(Tp::PendingOperation *);
    TP_QT_NO_EXPORT void onConnInvalidated(Tp::DBusProxy *proxy, const QString &error, const QString &message);

private:
    friend class ConnectionLowlevel;
};

class ConnectionHelper
{
public:
    static QString statusReasonToErrorName(Tp::ConnectionStatusReason reason,
        Tp::ConnectionStatus oldStatus);
};

} // Tp

#endif
