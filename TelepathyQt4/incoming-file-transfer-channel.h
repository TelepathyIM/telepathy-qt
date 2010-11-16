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

#ifndef _TelepathyQt4_incoming_file_transfer_channel_h_HEADER_GUARD_
#define _TelepathyQt4_incoming_file_transfer_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/FileTransferChannel>

#include <QAbstractSocket>

namespace Tp
{

class TELEPATHY_QT4_EXPORT IncomingFileTransferChannel : public FileTransferChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(IncomingFileTransferChannel)

public:
    static const Feature FeatureCore;

    static IncomingFileTransferChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~IncomingFileTransferChannel();

    PendingOperation *acceptFile(qulonglong offset, QIODevice *output);

protected:
    IncomingFileTransferChannel(const ConnectionPtr &connection,
            const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = IncomingFileTransferChannel::FeatureCore);

private Q_SLOTS:
    void onAcceptFileFinished(Tp::PendingOperation *op);

    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void doTransfer();

private:
    void connectToHost();
    void setFinished();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
