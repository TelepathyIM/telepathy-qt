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

#include "TelepathyQt4/simple-handler-internal.h"

#include "TelepathyQt4/_gen/simple-handler-internal.moc.hpp"

#include <TelepathyQt4/ChannelClassSpecList>

namespace Tp
{

SharedPtr<SimpleHandler> SimpleHandler::create(const AccountPtr &account)
{
    return SharedPtr<SimpleHandler>(new SimpleHandler(account));
}

SimpleHandler::SimpleHandler(const AccountPtr &account)
    : QObject(),
      AbstractClientHandler(ChannelClassSpecList(), AbstractClientHandler::Capabilities(), false),
      mAccount(account)
{
}

SimpleHandler::~SimpleHandler()
{
}

void SimpleHandler::handleChannels(
        const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const HandlerInfo &handlerInfo)
{
    Q_ASSERT(channels.size() == 1);

    if (account != mAccount) {
        emit error(TP_QT4_ERROR_SERVICE_CONFUSED,
                QLatin1String("Account received is not the same as the account which made the request"));
        context->setFinishedWithError(TP_QT4_ERROR_SERVICE_CONFUSED,
                QLatin1String("Account received is not the same as the account which made the request"));
        return;
    }

    if (mChannel) {
        Q_ASSERT(mChannel == channels.first());
        mChannel = channels.first();
    }

    emit channelReceived(mChannel);

    context->setFinished();
}

} // Tp
