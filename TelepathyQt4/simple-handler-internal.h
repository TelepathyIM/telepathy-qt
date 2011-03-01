/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt4_simple_handler_internal_h_HEADER_GUARD_
#define _TelepathyQt4_simple_handler_internal_h_HEADER_GUARD_

#include <TelepathyQt4/AbstractClientHandler>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT SimpleHandler : public QObject, public AbstractClientHandler
{
    Q_OBJECT

public:
    static SharedPtr<SimpleHandler> create(const AccountPtr &account);

    ~SimpleHandler();

    AccountPtr account() const { return mAccount; }
    ChannelPtr channel() const { return mChannel; }

    bool bypassApproval() const { return true; }

    void handleChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const HandlerInfo &handlerInfo);

Q_SIGNALS:
    void error(const QString &errorName, const QString &errorMessage);
    void channelReceived(const Tp::ChannelPtr &channel);

private:
    SimpleHandler(const AccountPtr &account);

    AccountPtr mAccount;
    ChannelPtr mChannel;
};

} // Tp

#endif
