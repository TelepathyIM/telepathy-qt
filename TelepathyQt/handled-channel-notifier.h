/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt_handled_channel_notifier_h_HEADER_GUARD_
#define _TelepathyQt_handled_channel_notifier_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/Types>

#include <QObject>

namespace Tp
{

class ChannelRequestHints;
class RequestTemporaryHandler;

class TP_QT_EXPORT HandledChannelNotifier : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(HandledChannelNotifier)

public:
    ~HandledChannelNotifier();

    ChannelPtr channel() const;

Q_SIGNALS:
    void handledAgain(const QDateTime &userActionTime, const Tp::ChannelRequestHints &requestHints);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    void connectNotify(const QMetaMethod &signal);
#else
    void connectNotify(const char *signal);
#endif

private Q_SLOTS:
    TP_QT_NO_EXPORT void onChannelReceived(const Tp::ChannelPtr &channel,
            const QDateTime &userActionTime, const Tp::ChannelRequestHints &requestHints);
    TP_QT_NO_EXPORT void onChannelInvalidated();

private:
    friend class PendingChannel;

    HandledChannelNotifier(const ClientRegistrarPtr &cr,
            const SharedPtr<RequestTemporaryHandler> &handler);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
