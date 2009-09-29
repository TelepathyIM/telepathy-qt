/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include "receiver-channel.h"
#include "_gen/receiver-channel.moc.hpp"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/FileTransferChannel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <QDebug>
#include <QFile>
#include <QFileInfo>

ReceiverChannel::ReceiverChannel(const ConnectionPtr &connection, const QString &objectPath,
        const QVariantMap &immutableProperties, qulonglong offset)
    : mCompleted(false),
      mOffset(offset)
{
    mChan = IncomingFileTransferChannel::create(connection,
            objectPath, immutableProperties);
    connect(mChan.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(onInvalidated()));
    connect(mChan->becomeReady(FileTransferChannel::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onFileTransferChannelReady(Tp::PendingOperation *)));
}

ReceiverChannel::~ReceiverChannel()
{
    mFile.close();
}

void ReceiverChannel::onFileTransferChannelReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to make file transfer channel ready -" <<
            op->errorName() << ": " << op->errorMessage();
        emit finished();
        return;
    }

    qDebug() << "File transfer channel ready!";
    connect(mChan.data(),
            SIGNAL(stateChanged(Tp::FileTransferState, Tp::FileTransferStateChangeReason)),
            SLOT(onFileTransferChannelStateChanged(Tp::FileTransferState, Tp::FileTransferStateChangeReason)));
    connect(mChan.data(),
            SIGNAL(transferredBytesChanged(qulonglong)),
            SLOT(onFileTransferChannelTransferredBytesChanged(qulonglong)));
    QString fileName(QLatin1String("TelepathyQt4FTReceiverExample_") + mChan->fileName());
    fileName.replace("/", "_");
    mFile.setFileName(fileName);
    qDebug() << "Saving file as" << mFile.fileName();
    mChan->acceptFile(mOffset, &mFile);
}

void ReceiverChannel::onFileTransferChannelStateChanged(Tp::FileTransferState state,
    Tp::FileTransferStateChangeReason stateReason)
{
    qDebug() << "File transfer channel state changed to" << state <<
        "with reason" << stateReason;
    mCompleted = (state == FileTransferStateCompleted);
    if (mCompleted) {
        qDebug() << "Transfer completed, file saved at" << mFile.fileName();
        emit finished();
    }
}

void ReceiverChannel::onFileTransferChannelTransferredBytesChanged(qulonglong count)
{
    qDebug().nospace() << "Receiving " << mChan->fileName() <<
        " - transferred bytes=" << count << " (" <<
        ((int) (((double) count / mChan->size()) * 100)) << "% done)";
}

void ReceiverChannel::onInvalidated()
{
    emit finished();
}
