/**
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/FileTransferChannelCreationProperties>

#include <TelepathyQt4/Global>

#include <QSharedData>
#include <QFileInfo>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT FileTransferChannelCreationProperties::Private : public QSharedData
{
    Private(const QString &suggestedFileName, const QString &contentType,
            qulonglong size)
        : contentType(contentType),
          size(size),
          hasContentHash(false), contentHashType(FileHashTypeNone),
          hasDescription(false),
          hasLastModificationTime(false)
    {
        QFileInfo fileInfo(suggestedFileName);
        this->suggestedFileName = fileInfo.fileName();
    }

    /* mandatory parameters */
    QString suggestedFileName;
    QString contentType;
    qulonglong size;

    /* optional parameters */
    bool hasContentHash;
    FileHashType contentHashType;
    QString contentHash;

    bool hasDescription;
    QString description;

    bool hasLastModificationTime;
    QDateTime lastModificationTime;
};

/**
 * \class FileTransferChannelCreationProperties
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/file-transfer-channel-creation-properties.h <TelepathyQt4/FileTransferChannelCreationProperties>
 *
 * \brief The FileTransferChannelCreationProperties class represents the
 * properties of a file transfer channel request.
 */

FileTransferChannelCreationProperties::FileTransferChannelCreationProperties()
{
}

FileTransferChannelCreationProperties::FileTransferChannelCreationProperties(
        const QString &suggestedFileName, const QString &contentType,
        qulonglong size)
    : mPriv(new Private(suggestedFileName, contentType, size))
{
}

FileTransferChannelCreationProperties::FileTransferChannelCreationProperties(
        const FileTransferChannelCreationProperties &other)
    : mPriv(other.mPriv)
{
}


FileTransferChannelCreationProperties::~FileTransferChannelCreationProperties()
{
}

FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::operator=(
        const FileTransferChannelCreationProperties &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool FileTransferChannelCreationProperties::operator==(
        const FileTransferChannelCreationProperties &other) const
{
    return mPriv == other.mPriv;
}

FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setContentHash(
        FileHashType contentHashType, const QString &contentHash)
{
    if (!isValid()) {
        // there is no point in updating content hash if not valid, as we miss filename, content
        // type and size
        return *this;
    }

    mPriv->hasContentHash = (contentHashType != FileHashTypeNone);
    mPriv->contentHashType = contentHashType;
    mPriv->contentHash = contentHash;
    return *this;
}

FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setDescription(
        const QString &description)
{
    if (!isValid()) {
        // there is no point in updating description if not valid, as we miss filename, content
        // type and size
        return *this;
    }

    mPriv->hasDescription = (!description.isEmpty());
    mPriv->description = description;
    return *this;
}

FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setLastModificationTime(
        const QDateTime &lastModificationTime)
{
    if (!isValid()) {
        // there is no point in updating last modification time if not valid, as we miss filename,
        // content type and size
        return *this;
    }

    mPriv->hasLastModificationTime = (lastModificationTime.isValid());
    mPriv->lastModificationTime = lastModificationTime;
    return *this;
}

QString FileTransferChannelCreationProperties::suggestedFileName() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->suggestedFileName;
}

QString FileTransferChannelCreationProperties::contentType() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->contentType;
}

qulonglong FileTransferChannelCreationProperties::size() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->size;
}

bool FileTransferChannelCreationProperties::hasContentHash() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->hasContentHash;
}

FileHashType FileTransferChannelCreationProperties::contentHashType() const
{
    if (!isValid()) {
        return FileHashTypeNone;
    }

    return mPriv->contentHashType;
}

QString FileTransferChannelCreationProperties::contentHash() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->contentHash;
}

bool FileTransferChannelCreationProperties::hasDescription() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->hasDescription;
}

QString FileTransferChannelCreationProperties::description() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->description;
}

bool FileTransferChannelCreationProperties::hasLastModificationTime() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->hasLastModificationTime;
}

QDateTime FileTransferChannelCreationProperties::lastModificationTime() const
{
    if (!isValid()) {
        return QDateTime();
    }

    return mPriv->lastModificationTime;
}

} // Tp
