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

#ifndef _TelepathyQt_file_transfer_channel_h_HEADER_GUARD_
#define _TelepathyQt_file_transfer_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>

namespace Tp
{

class TP_QT_EXPORT FileTransferChannel : public Channel
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
    QString uri() const;

    FileHashType contentHashType() const;
    QString contentHash() const;

    QString description() const;

    QDateTime lastModificationTime() const;

    qulonglong initialOffset() const;

    qulonglong transferredBytes() const;

    PendingOperation *cancel();

Q_SIGNALS:
    void stateChanged(Tp::FileTransferState state,
            Tp::FileTransferStateChangeReason reason);
    void initialOffsetDefined(qulonglong initialOffset);
    void transferredBytesChanged(qulonglong count);

protected:
    FileTransferChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = FileTransferChannel::FeatureCore);

    SupportedSocketMap availableSocketTypes() const;

    virtual void connectToHost();
    bool isConnected() const;
    void setConnected();

    bool isFinished() const;
    virtual void setFinished();

private Q_SLOTS:
    TP_QT_NO_EXPORT void gotProperties(QDBusPendingCallWatcher *watcher);

    TP_QT_NO_EXPORT void changeState();
    TP_QT_NO_EXPORT void onStateChanged(uint state, uint stateReason);
    TP_QT_NO_EXPORT void onInitialOffsetDefined(qulonglong initialOffset);
    TP_QT_NO_EXPORT void onTransferredBytesChanged(qulonglong count);

protected Q_SLOTS:
    TP_QT_NO_EXPORT void onUriDefined(const QString &uri);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
