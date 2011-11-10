/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
 * @license LGPL 2.1
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

#ifndef _TelepathyQt_outgoing_file_transfer_channel_h_HEADER_GUARD_
#define _TelepathyQt_outgoing_file_transfer_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/FileTransferChannel>

#include <QAbstractSocket>

namespace Tp
{

class TP_QT_EXPORT OutgoingFileTransferChannel : public FileTransferChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(OutgoingFileTransferChannel)

public:
    static const Feature FeatureCore;

    static OutgoingFileTransferChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~OutgoingFileTransferChannel();

    PendingOperation *provideFile(QIODevice *input);

protected:
    OutgoingFileTransferChannel(const ConnectionPtr &connection,
            const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = OutgoingFileTransferChannel::FeatureCore);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onProvideFileFinished(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onSocketConnected();
    TP_QT_NO_EXPORT void onSocketDisconnected();
    TP_QT_NO_EXPORT void onSocketError(QAbstractSocket::SocketError error);
    TP_QT_NO_EXPORT void onInputAboutToClose();
    TP_QT_NO_EXPORT void doTransfer();

private:
    TP_QT_NO_EXPORT void connectToHost();
    TP_QT_NO_EXPORT void setFinished();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
