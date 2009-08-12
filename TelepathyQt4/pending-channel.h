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

#ifndef _TelepathyQt4_pending_channel_h_HEADER_GUARD_
#define _TelepathyQt4_pending_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/PendingOperation>

#include <QString>
#include <QVariantMap>

#include <QDBusPendingCallWatcher>

namespace Tp
{

class Connection;

class TELEPATHY_QT4_EXPORT PendingChannel : public PendingOperation
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

    QString objectPath() const;

private Q_SLOTS:
    void onCallCreateChannelFinished(QDBusPendingCallWatcher *watcher);
    void onCallEnsureChannelFinished(QDBusPendingCallWatcher *watcher);

private:
    friend class Connection;

    PendingChannel(const ConnectionPtr &connection,
            const QString &errorName, const QString &errorMessage);
    PendingChannel(const ConnectionPtr &connection,
            const QVariantMap &request, bool create);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
