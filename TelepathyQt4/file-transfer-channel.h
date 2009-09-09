/* FileTransfer channel client-side proxy
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

#ifndef _TelepathyQt4_file_transfer_channel_h_HEADER_GUARD_
#define _TelepathyQt4_file_transfer_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>

namespace Tp
{

class FileTransferChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(FileTransferChannel)

public:
    static const Feature FeatureCore;

    static FileTransferChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~FileTransferChannel();

    FileTransferState state() const;
    FileTransferStateChangeReason stateReason() const;

    QString fileName() const;
    QString contentType() const;
    qulonglong size() const;

    FileHashType contentHashType() const;
    QString contentHash() const;

    QString description() const;

    QDateTime lastModificationTime() const;

    qulonglong initialOffset() const;

    qulonglong transferredBytes() const;

    PendingOperation *provideFile(QIODevice *input);
    PendingOperation *acceptFile(qulonglong offset, QIODevice *output);
    void cancel();

Q_SIGNALS:
    void stateChanged(Tp::FileTransferState state,
            Tp::FileTransferStateChangeReason reason);
    void transferredBytesChanged(qulonglong count);
    void initialOffsetDefined(qulonglong offset);

protected:
    FileTransferChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties);

private Q_SLOTS:
    void gotProperties(QDBusPendingCallWatcher *watcher);
    void onCallFinished(Tp::PendingOperation *op);
    void onFileTrasnferStateChanged(uint state, uint stateReason);
    void onSocketConnected();
    void writeData();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
