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

#ifndef _TelepathyQt_request_temporary_handler_internal_h_HEADER_GUARD_
#define _TelepathyQt_request_temporary_handler_internal_h_HEADER_GUARD_

#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/Account>
#include <TelepathyQt/Channel>

namespace Tp
{

class TP_QT_NO_EXPORT RequestTemporaryHandler : public QObject, public AbstractClientHandler
{
    Q_OBJECT

public:
    static SharedPtr<RequestTemporaryHandler> create(const AccountPtr &account);

    ~RequestTemporaryHandler();

    AccountPtr account() const { return mAccount; }
    ChannelPtr channel() const { return ChannelPtr(mChannel); }

    /**
     * Handlers we request ourselves never go through the approvers but this
     * handler shouldn't get any channels we didn't request - hence let's make
     * this always false to leave slightly less room for the CD to get confused and
     * give some channel we didn't request to us, without even asking an approver
     * first. Though if the CD isn't confused it shouldn't really matter - our filter
     * is empty anyway.
     */
    bool bypassApproval() const { return false; }

    void handleChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const HandlerInfo &handlerInfo);

    void setQueueChannelReceived(bool queue);

    void setDBusHandlerInvoked();
    void setDBusHandlerErrored(const QString &errorName, const QString &errorMessage);

    bool isDBusHandlerInvoked() const { return dbusHandlerInvoked; }

Q_SIGNALS:
    void error(const QString &errorName, const QString &errorMessage);
    void channelReceived(const Tp::ChannelPtr &channel, const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &requestHints);

private:
    RequestTemporaryHandler(const AccountPtr &account);

    void processChannelReceivedQueue();

    AccountPtr mAccount;
    WeakPtr<Channel> mChannel;
    bool mQueueChannelReceived;
    QQueue<QPair<QDateTime, ChannelRequestHints> > mChannelReceivedQueue;
    bool dbusHandlerInvoked;
};

} // Tp

#endif
