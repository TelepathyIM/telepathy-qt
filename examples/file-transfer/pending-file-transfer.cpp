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

#include "pending-file-transfer.h"
#include "_gen/pending-file-transfer.moc.hpp"

#include <TelepathyQt/Account>
#include <TelepathyQt/Channel>
#include <TelepathyQt/OutgoingFileTransferChannel>

#include <QDebug>
#include <QFile>
#include <QUrl>

PendingFileTransfer::PendingFileTransfer(const FileTransferChannelPtr &chan,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mChannel(chan)
{
    connect(chan.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));
    connect(chan.data(),
            SIGNAL(transferredBytesChanged(qulonglong)),
            SLOT(onTransferredBytesChanged(qulonglong)));
}

PendingFileTransfer::~PendingFileTransfer()
{
}

void PendingFileTransfer::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);

    qWarning() << "Error sending file, channel invalidated -" <<
        errorName << "-" << errorMessage;
    setFinishedWithError(errorName, errorMessage);
}

void PendingFileTransfer::onTransferStateChanged(FileTransferState state,
        FileTransferStateChangeReason stateReason)
{
    qDebug() << "File transfer channel state changed to" << state <<
        "with reason" << stateReason;
    switch (state) {
        case FileTransferStatePending:
        case FileTransferStateOpen:
            break;

        case FileTransferStateAccepted:
            qDebug() << "Transfer accepted!";
            break;

        case FileTransferStateCompleted:
            qDebug() << "Transfer completed!";
            setFinished();
            break;

        case FileTransferStateCancelled:
            qDebug() << "Transfer cancelled";
            setFinished();
            return;

        default:
            Q_ASSERT(false);
    }
}

void PendingFileTransfer::onTransferredBytesChanged(qulonglong count)
{
    qDebug().nospace() << "Transferred bytes " << count << " - " <<
        ((int) (((double) count / mChannel->size()) * 100)) << "% done";
}
