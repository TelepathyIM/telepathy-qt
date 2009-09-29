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

#ifndef _TelepathyQt4_examples_file_transfer_receiver_channel_h_HEADER_GUARD_
#define _TelepathyQt4_examples_file_transfer_receiver_channel_h_HEADER_GUARD_

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

using namespace Tp;

namespace Tp
{
class PendingOperation;
}

class ReceiverChannel : public QObject
{
    Q_OBJECT

public:
   ReceiverChannel(const ConnectionPtr &connection,
           const QString &objectPath, const QVariantMap &immutableProperties,
           qulonglong offset);
   ~ReceiverChannel();

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onFileTransferChannelReady(Tp::PendingOperation *op);
    void onFileTransferChannelStateChanged(Tp::FileTransferState state,
            Tp::FileTransferStateChangeReason stateReason);
    void onFileTransferChannelTransferredBytesChanged(qulonglong count);
    void onInvalidated();

private:
    IncomingFileTransferChannelPtr mChan;
    QFile mFile;
    bool mCompleted;
    qulonglong mOffset;
};

#endif
