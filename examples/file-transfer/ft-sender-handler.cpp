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

#include "ft-sender-handler.h"
#include "_gen/ft-sender-handler.moc.hpp"

#include "ft-send-op.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/MethodInvocationContext>
#include <TelepathyQt4/OutgoingFileTransferChannel>

#include <QDateTime>
#include <QDebug>

FTSenderHandler::FTSenderHandler()
    : QObject(),
      AbstractClientHandler(ChannelClassSpecList(), AbstractClientHandler::Capabilities(), false)
{
}

FTSenderHandler::~FTSenderHandler()
{
}

bool FTSenderHandler::bypassApproval() const
{
    return true;
}

void FTSenderHandler::handleChannels(const MethodInvocationContextPtr<> &context,
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

    context->setFinished();

    if (chan->channelType() != TP_QT4_IFACE_CHANNEL_TYPE_FILE_TRANSFER) {
        qWarning() << "Channel received to handle is not of type FileTransfer, service confused. "
            "Ignoring channel";
        chan->requestClose();
        return;
    }

    if (!chan->isRequested()) {
        qWarning() << "Channel received to handle is not an outgoing file transfer channel, "
            "service confused. Ignoring channel";
        chan->requestClose();
        return;
    }

    OutgoingFileTransferChannelPtr oftChan = OutgoingFileTransferChannelPtr::qObjectCast(chan);
    if (!oftChan) {
        qWarning() << "Channel received to handle is not a subclass of OutgoingFileTransferChannel. "
            "ChannelFactory set on this handler's account must construct OutgoingFileTransferChannel "
            "subclasses for outgoing channels of type FileTransfer. Ignoring channel";
        chan->requestClose();
        return;
    }

    if (oftChan->uri().isEmpty()) {
        qWarning() << "Received an outgoing file transfer channel with uri undefined, "
            "aborting file transfer";
        chan->requestClose();
        return;
    }

    FTSendOp *sop = new FTSendOp(oftChan,
            SharedPtr<RefCounted>::dynamicCast(AbstractClientPtr(this)));
    connect(sop,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onSendFinished(Tp::PendingOperation*)));
}

void FTSenderHandler::onSendFinished(PendingOperation *op)
{
    FTSendOp *sop = qobject_cast<FTSendOp*>(op);
    qDebug() << "Closing channel";
    sop->channel()->requestClose();
}
