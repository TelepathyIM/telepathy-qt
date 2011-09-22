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

#include "pending-file-receive.h"
#include "_gen/pending-file-receive.moc.hpp"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/IncomingFileTransferChannel>

#include <QDebug>
#include <QFile>
#include <QUrl>

PendingFileReceive::PendingFileReceive(const IncomingFileTransferChannelPtr &chan,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mChan(chan),
      mReceivingFile(false)
{
    QString fileName(QLatin1String("TpQt4ExampleFTReceiver_") + mChan->fileName());
    fileName.replace(QLatin1String("/"), QLatin1String("_"));
    mFile.setFileName(fileName);

    connect(chan.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));
    connect(chan.data(),
            SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)),
            SLOT(onStateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)));
    connect(chan.data(),
            SIGNAL(transferredBytesChanged(qulonglong)),
            SLOT(onTransferredBytesChanged(qulonglong)));
    onStateChanged(mChan->state(), mChan->stateReason());
}

PendingFileReceive::~PendingFileReceive()
{
    mFile.close();
}

void PendingFileReceive::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);

    qWarning() << "Error receiving file, channel invalidated -" <<
        errorName << "-" << errorMessage;
    setFinishedWithError(errorName, errorMessage);
}

void PendingFileReceive::onStateChanged(FileTransferState state,
        FileTransferStateChangeReason stateReason)
{
    qDebug() << "File transfer channel state changed to" << state <<
        "with reason" << stateReason;
    switch (state) {
        case FileTransferStatePending:
            Q_ASSERT(!mReceivingFile);
            mReceivingFile = true;
            qDebug() << "Accepting file transfer, saving file as" << mFile.fileName();
            mChan->acceptFile(0, &mFile);
            break;

        case FileTransferStateAccepted:
            qDebug() << "Receiving" << mChan->fileName() << "from" << mChan->targetId();

        case FileTransferStateOpen:
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

void PendingFileReceive::onTransferredBytesChanged(qulonglong count)
{
    qDebug().nospace() << "Transferred bytes " << count << " - " <<
        ((int) (((double) count / mChan->size()) * 100)) << "% done";
}
