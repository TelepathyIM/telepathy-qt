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

#include "ft-send-op.h"
#include "_gen/ft-send-op.moc.hpp"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/OutgoingFileTransferChannel>

#include <QDebug>
#include <QFile>
#include <QUrl>

FTSendOp::FTSendOp(const OutgoingFileTransferChannelPtr &chan,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mChan(chan),
      mSendingFile(false)
{
    mFile.setFileName(QUrl(chan->uri()).toLocalFile());
    if (!mFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Unable to open" << chan->uri() << "for reading, aborting transfer";
        setFinishedWithError(TP_QT4_ERROR_INVALID_ARGUMENT,
                QLatin1String("Unable to open file for reading"));
        return;
    }

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

FTSendOp::~FTSendOp()
{
    mFile.close();
}

void FTSendOp::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);

    qWarning() << "Error sending file, channel invalidated -" <<
        errorName << "-" << errorMessage;
    setFinishedWithError(errorName, errorMessage);
}

void FTSendOp::onStateChanged(FileTransferState state,
        FileTransferStateChangeReason stateReason)
{
    qDebug() << "File transfer channel state changed to" << state <<
        "with reason" << stateReason;
    switch (state) {
        case FileTransferStatePending:
            qDebug() << "Awaiting receiver to accept file transfer";
            break;

        case FileTransferStateAccepted:
            Q_ASSERT(!mSendingFile);
            mSendingFile = true;
            qDebug() << "Sending" << mChan->uri() << "to" << mChan->targetId();
            mChan->provideFile(&mFile);

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

void FTSendOp::onTransferredBytesChanged(qulonglong count)
{
    qDebug().nospace() << "Transferred bytes " << count << " - " <<
        ((int) (((double) count / mChan->size()) * 100)) << "% done";
}
