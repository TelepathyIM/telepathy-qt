/*
 * This file is part of TelepathyQt
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

#include "file-receiver-handler.h"
#include "_gen/file-receiver-handler.moc.hpp"

#include "pending-file-receive.h"

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/Connection>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/IncomingFileTransferChannel>

#include <QDateTime>
#include <QDebug>

FileReceiverHandler::FileReceiverHandler()
    : QObject(),
      AbstractClientHandler(ChannelClassSpecList() << ChannelClassSpec::incomingFileTransfer(),
              AbstractClientHandler::Capabilities(), false)
{
}

FileReceiverHandler::~FileReceiverHandler()
{
}

bool FileReceiverHandler::bypassApproval() const
{
    return false;
}

void FileReceiverHandler::handleChannels(const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const HandlerInfo &handlerInfo)
{
    // We should always receive one channel to handle,
    // otherwise either MC or tp-qt itself is bogus, so let's assert in case they are
    Q_ASSERT(channels.size() == 1);
    ChannelPtr chan = channels.first();

    if (!chan->isValid()) {
        qWarning() << "Channel received to handle is invalid, ignoring channel";
        context->setFinishedWithError(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Channel received to handle is invalid"));
        return;
    }

    // We should always receive incoming channels of type FileTransfer, as set by our filter,
    // otherwise either MC or tp-qt itself is bogus, so let's assert in case they are
    Q_ASSERT(chan->channelType() == TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    Q_ASSERT(!chan->isRequested());

    IncomingFileTransferChannelPtr transferChannel = IncomingFileTransferChannelPtr::qObjectCast(chan);
    Q_ASSERT(transferChannel);

    context->setFinished();

    PendingFileReceive *receiveOperation = new PendingFileReceive(transferChannel,
            SharedPtr<RefCounted>::dynamicCast(AbstractClientPtr(this)));
    connect(receiveOperation,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReceiveFinished(Tp::PendingOperation*)));
}

void FileReceiverHandler::onReceiveFinished(PendingOperation *op)
{
    PendingFileReceive *receiveOperation = qobject_cast<PendingFileReceive*>(op);
    qDebug() << "Closing channel";
    receiveOperation->channel()->requestClose();
}
