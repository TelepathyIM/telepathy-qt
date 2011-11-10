/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt_pending_channel_request_h_HEADER_GUARD_
#define _TelepathyQt_pending_channel_request_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace Tp
{

class Account;
class ChannelRequestHints;

class TP_QT_EXPORT PendingChannelRequest : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingChannelRequest)

public:
    ~PendingChannelRequest();

    AccountPtr account() const;

    ChannelRequestPtr channelRequest() const;

    PendingOperation *cancel();

Q_SIGNALS:
    void channelRequestCreated(const Tp::ChannelRequestPtr &channelRequest);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onWatcherFinished(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onProceedOperationFinished(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onCancelOperationFinished(Tp::PendingOperation *op);

private:
    friend class Account;

    TP_QT_NO_EXPORT PendingChannelRequest(const AccountPtr &account,
            const QVariantMap &requestedProperties, const QDateTime &userActionTime,
            const QString &preferredHandler, bool create, const ChannelRequestHints &hints);
    TP_QT_NO_EXPORT PendingChannelRequest(const AccountPtr &account,
            const QString &errorName, const QString &errorMessage);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
