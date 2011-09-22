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

#include "file-sender-handler.h"
#include "_gen/file-sender-handler.moc.hpp"

#include "pending-file-send.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/MethodInvocationContext>
#include <TelepathyQt4/OutgoingFileTransferChannel>

#include <QDateTime>
#include <QDebug>

FileSenderHandler::FileSenderHandler()
    : QObject(),
      AbstractClientHandler(ChannelClassSpecList(), AbstractClientHandler::Capabilities(), false)
{
}

FileSenderHandler::~FileSenderHandler()
{
}

bool FileSenderHandler::bypassApproval() const
{
    return true;
}

void FileSenderHandler::handleChannels(const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const HandlerInfo &handlerInfo)
{
    Q_ASSERT(channels.size() == 1);
    ChannelPtr chan = channels.first();

    if (!chan->isValid()) {
        qWarning() << "Channel received to handle is invalid, ignoring channel";
        context->setFinishedWithError(TP_QT4_ERROR_INVALID_ARGUMENT,
                QLatin1String("Channel received to handle is invalid"));
        return;
    }

    Q_ASSERT(chan->channelType() == TP_QT4_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    Q_ASSERT(chan->isRequested());

    OutgoingFileTransferChannelPtr oftChan = OutgoingFileTransferChannelPtr::qObjectCast(chan);
    Q_ASSERT(oftChan);

    if (oftChan->uri().isEmpty()) {
        qWarning() << "Received an outgoing file transfer channel with uri undefined, "
            "aborting file transfer";
        chan->requestClose();
        context->setFinishedWithError(TP_QT4_ERROR_INVALID_ARGUMENT,
                QLatin1String("Outgoing file transfer channel received does not have the URI set"));
        return;
    }

    context->setFinished();

    PendingFileSend *sop = new PendingFileSend(oftChan,
            SharedPtr<RefCounted>::dynamicCast(AbstractClientPtr(this)));
    connect(sop,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onSendFinished(Tp::PendingOperation*)));
}

void FileSenderHandler::onSendFinished(PendingOperation *op)
{
    PendingFileSend *sop = qobject_cast<PendingFileSend*>(op);
    qDebug() << "Closing channel";
    sop->channel()->requestClose();
}
