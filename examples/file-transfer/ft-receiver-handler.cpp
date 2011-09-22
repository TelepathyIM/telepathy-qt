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

#include "ft-receiver-handler.h"
#include "_gen/ft-receiver-handler.moc.hpp"

#include "ft-receive-op.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/MethodInvocationContext>
#include <TelepathyQt4/IncomingFileTransferChannel>

#include <QDateTime>
#include <QDebug>

FTReceiverHandler::FTReceiverHandler()
    : QObject(),
      AbstractClientHandler(ChannelClassSpecList() << ChannelClassSpec::incomingFileTransfer(),
              AbstractClientHandler::Capabilities(), false)
{
}

FTReceiverHandler::~FTReceiverHandler()
{
}

bool FTReceiverHandler::bypassApproval() const
{
    return false;
}

void FTReceiverHandler::handleChannels(const MethodInvocationContextPtr<> &context,
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

    if (chan->isRequested()) {
        qWarning() << "Channel received to handle is not an incoming file transfer channel, "
            "service confused. Ignoring channel";
        chan->requestClose();
        return;
    }

    IncomingFileTransferChannelPtr iftChan = IncomingFileTransferChannelPtr::qObjectCast(chan);
    if (!iftChan) {
        qWarning() << "Channel received to handle is not a subclass of IncomingFileTransferChannel. "
            "ChannelFactory set on this handler's account must construct IncomingFileTransferChannel "
            "subclasses for incoming channels of type FileTransfer. Ignoring channel";
        chan->requestClose();
        return;
    }

    FTReceiveOp *rop = new FTReceiveOp(iftChan,
            SharedPtr<RefCounted>::dynamicCast(AbstractClientPtr(this)));
    connect(rop,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReceiveFinished(Tp::PendingOperation*)));
}

void FTReceiverHandler::onReceiveFinished(PendingOperation *op)
{
    FTReceiveOp *rop = qobject_cast<FTReceiveOp*>(op);
    qDebug() << "Closing channel";
    rop->channel()->requestClose();
}
