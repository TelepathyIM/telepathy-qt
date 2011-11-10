/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt_pending_channel_h_HEADER_GUARD_
#define _TelepathyQt_pending_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/PendingOperation>

#include <QString>
#include <QVariantMap>

#include <QDBusPendingCallWatcher>

namespace Tp
{

class Connection;
class HandledChannelNotifier;

class TP_QT_EXPORT PendingChannel : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingChannel)

public:
    ~PendingChannel();

    ConnectionPtr connection() const;

    bool yours() const;

    const QString &channelType() const;

    uint targetHandleType() const;

    uint targetHandle() const;

    QVariantMap immutableProperties() const;

    ChannelPtr channel() const;

    HandledChannelNotifier *handledChannelNotifier() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onConnectionCreateChannelFinished(
            QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onConnectionEnsureChannelFinished(
            QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onChannelReady(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onHandlerError(const QString &errorName,
            const QString &errorMessage);
    TP_QT_NO_EXPORT void onHandlerChannelReceived(
            const Tp::ChannelPtr &channel);
    TP_QT_NO_EXPORT void onAccountCreateChannelFinished(
            Tp::PendingOperation *op);

private:
    friend class Account;
    friend class ConnectionLowlevel;

    TP_QT_NO_EXPORT PendingChannel(const ConnectionPtr &connection,
            const QString &errorName, const QString &errorMessage);
    TP_QT_NO_EXPORT PendingChannel(const ConnectionPtr &connection,
            const QVariantMap &request, bool create, int timeout = -1);
    TP_QT_NO_EXPORT PendingChannel(const AccountPtr &account,
            const QVariantMap &request, const QDateTime &userActionTime,
            bool create);
    TP_QT_NO_EXPORT PendingChannel(const QString &errorName,
            const QString &errorMessage);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
