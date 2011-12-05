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

#include "pending-file-receive.h"
#include "_gen/pending-file-receive.moc.hpp"

#include <TelepathyQt/Account>
#include <TelepathyQt/Channel>
#include <TelepathyQt/IncomingFileTransferChannel>

#include <QDebug>
#include <QFile>

PendingFileReceive::PendingFileReceive(const IncomingFileTransferChannelPtr &chan,
        const SharedPtr<RefCounted> &object)
    : PendingFileTransfer(FileTransferChannelPtr::qObjectCast(chan), object),
      mReceivingFile(false)
{
    // connect/call onTransferStateChanged here as now we are constructed, otherwise doing it in the base
    // class would only invoke the base class slot
    connect(chan.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SLOT(onTransferStateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)));
    onTransferStateChanged(chan->state(), chan->stateReason());
}

PendingFileReceive::~PendingFileReceive()
{
    mFile.close();
}

void PendingFileReceive::onTransferStateChanged(FileTransferState state,
        FileTransferStateChangeReason stateReason)
{
    PendingFileTransfer::onTransferStateChanged(state, stateReason);

    if (state == FileTransferStatePending) {
        Q_ASSERT(!mReceivingFile);
        mReceivingFile = true;

        IncomingFileTransferChannelPtr chan =
            IncomingFileTransferChannelPtr::qObjectCast(channel());
        Q_ASSERT(chan);

        QString fileName(QLatin1String("TpQtExampleFTReceiver_") + chan->fileName());
        fileName.replace(QLatin1String("/"), QLatin1String("_"));
        mFile.setFileName(fileName);

        qDebug() << "Receiving" << chan->fileName() << "from" <<
            chan->targetId() << ", saving as" << fileName;
        chan->acceptFile(0, &mFile);
    }
}
