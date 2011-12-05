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

#ifndef _TelepathyQt_simple_stream_tube_handler_h_HEADER_GUARD_
#define _TelepathyQt_simple_stream_tube_handler_h_HEADER_GUARD_

#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/ChannelRequestHints>
#include <TelepathyQt/RefCounted>
#include <TelepathyQt/Types>

#include <QDateTime>
#include <QLinkedList>
#include <QHash>
#include <QQueue>
#include <QSet>

namespace Tp
{

class PendingOperation;

class TP_QT_NO_EXPORT SimpleStreamTubeHandler : public QObject,
                public AbstractClientHandler
{
    Q_OBJECT
    Q_DISABLE_COPY(SimpleStreamTubeHandler)

public:
    static SharedPtr<SimpleStreamTubeHandler> create(
            const QStringList &p2pServices,
            const QStringList &roomServices,
            bool requested,
            bool monitorConnections,
            bool bypassApproval = false);
    ~SimpleStreamTubeHandler();

    bool monitorsConnections() const
    {
        return mMonitorConnections;
    }

    bool bypassApproval() const
    {
        return mBypassApproval;
    }

    void handleChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const HandlerInfo &handlerInfo);

Q_SIGNALS:
    void invokedForTube(
            const Tp::AccountPtr &account,
            const Tp::StreamTubeChannelPtr &tube,
            const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &requestHints);
    void tubeInvalidated(
            const Tp::AccountPtr &account,
            const Tp::StreamTubeChannelPtr &tube,
            const QString &errorName,
            const QString &errorMessage);

private Q_SLOTS:
    void onReadyOpFinished(Tp::PendingOperation *);
    void onTubeInvalidated(Tp::DBusProxy *, const QString &, const QString &);

private:
    SimpleStreamTubeHandler(
            const QStringList &p2pServices,
            const QStringList &roomServices,
            bool requested,
            bool monitorConnections,
            bool bypassApproval);

    bool mMonitorConnections;

    struct InvocationData : RefCounted {
        InvocationData() : readyOp(0) {}

        PendingOperation *readyOp;
        QString error, message;

        MethodInvocationContextPtr<> ctx;
        AccountPtr acc;
        QList<StreamTubeChannelPtr> tubes;
        QDateTime time;
        ChannelRequestHints hints;
    };
    QLinkedList<SharedPtr<InvocationData> > mInvocations;
    QHash<StreamTubeChannelPtr, AccountPtr> mTubes;
    bool mBypassApproval;
};

} // Tp

#endif
