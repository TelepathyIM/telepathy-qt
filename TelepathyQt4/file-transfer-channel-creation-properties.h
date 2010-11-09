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

#ifndef _TelepathyQt4_file_transfer_channel_creation_properties_h_HEADER_GUARD_
#define _TelepathyQt4_file_transfer_channel_creation_properties_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Global>

#include <QDateTime>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>

namespace Tp
{

class TELEPATHY_QT4_EXPORT FileTransferChannelCreationProperties
{
public:
    FileTransferChannelCreationProperties();
    FileTransferChannelCreationProperties(const QString &suggestedFileName,
            const QString &contentType, qulonglong size);
    FileTransferChannelCreationProperties(
            const FileTransferChannelCreationProperties &other);
    ~FileTransferChannelCreationProperties();

    bool isValid() const { return mPriv.constData() != 0; }

    FileTransferChannelCreationProperties &operator=(
            const FileTransferChannelCreationProperties &other);
    bool operator==(const FileTransferChannelCreationProperties &other) const;

    FileTransferChannelCreationProperties &setContentHash(
            FileHashType contentHashType, const QString &contentHash);
    FileTransferChannelCreationProperties &setDescription(
            const QString &description);
    FileTransferChannelCreationProperties &setLastModificationTime(
            const QDateTime &lastModificationTime);

private:
    friend class Account;

    /* mandatory parameters */
    QString suggestedFileName() const;
    QString contentType() const;
    qulonglong size() const;

    /* optional parameters */
    bool hasContentHash() const;
    FileHashType contentHashType() const;
    QString contentHash() const;

    bool hasDescription() const;
    QString description() const;

    bool hasLastModificationTime() const;
    QDateTime lastModificationTime() const;

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::FileTransferChannelCreationProperties);

#endif
