/**
 * This file is part of TelepathyQt4
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

#include "TelepathyQt4/request-temporary-handler-internal.h"

#include "TelepathyQt4/_gen/request-temporary-handler-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ChannelClassSpecList>

namespace Tp
{

SharedPtr<RequestTemporaryHandler> RequestTemporaryHandler::create(const AccountPtr &account)
{
    return SharedPtr<RequestTemporaryHandler>(new RequestTemporaryHandler(account));
}

RequestTemporaryHandler::RequestTemporaryHandler(const AccountPtr &account)
    : QObject(),
      AbstractClientHandler(ChannelClassSpecList(), AbstractClientHandler::Capabilities(), false),
      mAccount(account),
      mQueueChannelReceived(true),
      dbusHandlerInvoked(false)
{
}

RequestTemporaryHandler::~RequestTemporaryHandler()
{
}

void RequestTemporaryHandler::handleChannels(
        const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const HandlerInfo &handlerInfo)
{
    Q_ASSERT(dbusHandlerInvoked);

    QString errorMessage;
    if (channels.size() != 1 || requestsSatisfied.size() != 1) {
        errorMessage = QLatin1String("Only one channel and one channel request should be given "
                "to HandleChannels");
    } else if (account != mAccount) {
        errorMessage = QLatin1String("Account received is not the same as the account which made "
                "the request");
    } else if (mChannel && mChannel != channels.first()) {
        errorMessage = QLatin1String("Received a channel that is not the same as the first "
                "one received");
    }

    if (!errorMessage.isEmpty()) {
        warning() << "Handling channel failed with" << TP_QT4_ERROR_SERVICE_CONFUSED << ":" <<
            errorMessage;

        // Only emit error if we didn't receive any channel yet.
        if (!mChannel) {
            emit error(TP_QT4_ERROR_SERVICE_CONFUSED, errorMessage);
        }
        context->setFinishedWithError(TP_QT4_ERROR_SERVICE_CONFUSED, errorMessage);
        return;
    }

    ChannelRequestPtr channelRequest = requestsSatisfied.first();

    if (!mChannel) {
        mChannel = channels.first();
        emit channelReceived(mChannel, userActionTime, channelRequest->hints());
    } else {
        if (mQueueChannelReceived) {
            mChannelReceivedQueue.enqueue(qMakePair(userActionTime, channelRequest->hints()));
        } else {
            emit channelReceived(mChannel, userActionTime, channelRequest->hints());
        }
    }

    context->setFinished();
}

void RequestTemporaryHandler::setQueueChannelReceived(bool queue)
{
    mQueueChannelReceived = queue;
    if (!queue) {
        processChannelReceivedQueue();
    }
}

void RequestTemporaryHandler::setDBusHandlerInvoked()
{
    dbusHandlerInvoked = true;
}

void RequestTemporaryHandler::setDBusHandlerErrored(const QString &errorName, const QString &errorMessage)
{
    Q_ASSERT(dbusHandlerInvoked);
    if (!mChannel) {
        emit error(errorName, errorMessage);
    }
}

void RequestTemporaryHandler::processChannelReceivedQueue()
{
    while (!mChannelReceivedQueue.isEmpty()) {
        QPair<QDateTime, ChannelRequestHints> info = mChannelReceivedQueue.dequeue();
        emit channelReceived(mChannel, info.first, info.second);
    }
}

} // Tp
