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

#include <TelepathyQt/FileTransferChannelCreationProperties>

#include <TelepathyQt/Global>

#include "TelepathyQt/debug-internal.h"

#include <QSharedData>
#include <QFileInfo>
#include <QUrl>

namespace Tp
{

struct TP_QT_NO_EXPORT FileTransferChannelCreationProperties::Private : public QSharedData
{
    Private(const QString &suggestedFileName, const QString &contentType,
            qulonglong size)
        : contentType(contentType),
          size(size),
          contentHashType(FileHashTypeNone)
    {
        QFileInfo fileInfo(suggestedFileName);
        this->suggestedFileName = fileInfo.fileName();
    }

    Private(const QString &path, const QString &contentType)
        : contentType(contentType),
          contentHashType(FileHashTypeNone)
    {
        QFileInfo fileInfo(path);

        if (fileInfo.exists()) {
            // Set mandatory parameters
            suggestedFileName = fileInfo.fileName();
            size = fileInfo.size();
            QUrl fileUri = QUrl::fromLocalFile(fileInfo.canonicalFilePath());
            uri = fileUri.toString();

            // Set optional parameters
            lastModificationTime = fileInfo.lastModified();
        } else {
            warning() << path << "is not a local file.";
        }
    }

    /* mandatory parameters */
    QString suggestedFileName;
    QString contentType;
    qulonglong size;

    /* optional parameters */
    FileHashType contentHashType;
    QString contentHash;
    QString description;
    QDateTime lastModificationTime;
    QString uri;
};

/**
 * \class FileTransferChannelCreationProperties
 * \ingroup clientchannel
 * \headerfile TelepathyQt/file-transfer-channel-creation-properties.h <TelepathyQt/FileTransferChannelCreationProperties>
 *
 * \brief The FileTransferChannelCreationProperties class represents the
 * properties of a file transfer channel request.
 */

/**
 * Create an invalid FileTransferChannelCreationProperties.
 */
FileTransferChannelCreationProperties::FileTransferChannelCreationProperties()
{
}

/**
 * Create a FileTransferChannelCreationProperties.
 *
 * If \a suggestedFileName or \a contentType are empty or if \a size is equal to
 * zero, the channel request will fail.
 * \a suggestedFileName will be cleaned of any path.
 *
 * \param suggestedFileName The name of the file on the sender's side. This is
 *                          therefore given as a suggested filename for the
 *                          receiver.
 * \param contentType The content type (MIME) of the file.
 * \param size The size of the content of the file.
 * \sa setUri()
 */
FileTransferChannelCreationProperties::FileTransferChannelCreationProperties(
        const QString &suggestedFileName, const QString &contentType,
        qulonglong size)
    : mPriv(new Private(suggestedFileName, contentType, size))
{
}

/**
 * Create a FileTransferChannelCreationProperties.
 *
 * This constructor accepts the path to a local file and sets the properties
 * that can be deducted from the file.
 * If \a path is not a local file the FileTransferChannelCreationProperties
 * will be invalid.
 *
 * \param path The path to the local file to be sent.
 */
FileTransferChannelCreationProperties::FileTransferChannelCreationProperties(
        const QString &path, const QString &contentType)
    : mPriv(new Private(path, contentType))
{
    if (mPriv->suggestedFileName.isEmpty()) {
        mPriv = QSharedDataPointer<Private>(NULL);
    }
}

/**
 * Copy constructor.
 */
FileTransferChannelCreationProperties::FileTransferChannelCreationProperties(
        const FileTransferChannelCreationProperties &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
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

/**
 * Set the content hash of the file and its type for the request.
 *
 * \param contentHashType The type of content hash.
 * \param contentHash The hash of the file, of type \a contentHashType.
 * \return This FileTransferChannelCreationProperties.
 * \sa hasContentHash(), contentHash(), contentHashType()
 */
FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setContentHash(
        FileHashType contentHashType, const QString &contentHash)
{
    if (!isValid()) {
        // there is no point in updating content hash if not valid, as we miss filename, content
        // type and size
        return *this;
    }

    mPriv->contentHashType = contentHashType;
    mPriv->contentHash = contentHash;
    return *this;
}

/**
 * Set a description of the file for the request.
 *
 * \param description The description of the file.
 * \return This FileTransferChannelCreationProperties.
 * \sa hasDescription(), description()
 */
FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setDescription(
        const QString &description)
{
    if (!isValid()) {
        // there is no point in updating description if not valid, as we miss filename, content
        // type and size
        return *this;
    }

    mPriv->description = description;
    return *this;
}

/**
 * Set the last modification time of the file for the request.
 *
 * \param lastModificationTime The last modification time of the file.
 * \return This FileTransferChannelCreationProperties.
 * \sa hasLastModificationTime(), lastModificationTime()
 */
FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setLastModificationTime(
        const QDateTime &lastModificationTime)
{
    if (!isValid()) {
        // there is no point in updating last modification time if not valid, as we miss filename,
        // content type and size
        return *this;
    }

    mPriv->lastModificationTime = lastModificationTime;
    return *this;
}

/**
 * Set the URI of the file for the request.
 *
 * \param uri The URI of the file.
 * \return This FileTransferChannelCreationProperties.
 * \sa uri()
 */
FileTransferChannelCreationProperties &FileTransferChannelCreationProperties::setUri(
        const QString &uri)
{
    if (!isValid()) {
        // there is no point in updating uri if not valid, as we miss filename, content
        // type and size
        return *this;
    }

    mPriv->uri = uri;
    return *this;
}

/**
 * Return the suggested file name for the request.
 * If the suggested file name is empty, the channel request will fail.
 *
 * \return The suggested file name for the request.
 */
QString FileTransferChannelCreationProperties::suggestedFileName() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->suggestedFileName;
}

/**
 * Return the content type (MIME) of the file for the request.
 * If the content type is empty, the channel request will fail.
 *
 * \return The content type of the file.
 */
QString FileTransferChannelCreationProperties::contentType() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->contentType;
}

/**
 * Return the size of the contents of the file for the request.
 * If size is zero, the channel request will fail.
 *
 * \return The size of the contents of file.
 */
qulonglong FileTransferChannelCreationProperties::size() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->size;
}

/**
 * Return whether the request will have a content hash.
 *
 * \return \c true whether it will have a content hash, \c false otherwise.
 * \sa contentHash(), contentHashType(), setContentHash()
 */
bool FileTransferChannelCreationProperties::hasContentHash() const
{
    if (!isValid()) {
        return false;
    }

    return (mPriv->contentHashType != FileHashTypeNone);
}

/**
 * Return the type of the content hash for the request.
 *
 * \return The type of the content hash.
 * \sa hasContentHash(), contentHash(), setContentHash()
 */
FileHashType FileTransferChannelCreationProperties::contentHashType() const
{
    if (!isValid()) {
        return FileHashTypeNone;
    }

    return mPriv->contentHashType;
}

/**
 * Return the content hash of the file for the request.
 *
 * \return The hash of the contents of the file transfer, of type returned by
 *         contentHashType().
 * \sa hasContentHash(), contentHashType(), setContentHash()
 */
QString FileTransferChannelCreationProperties::contentHash() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->contentHash;
}

/**
 * Return whether the request will have a descriprion.
 *
 * \return \c true whether it will have description, \c false otherwise.
 * \sa description(), setDescription()
 */
bool FileTransferChannelCreationProperties::hasDescription() const
{
    if (!isValid()) {
        return false;
    }

    return (!mPriv->description.isEmpty());
}

/**
 * Return the description of the file for the request.
 *
 * \return The description of the file.
 * \sa hasDescription(), setDescription()
 */
QString FileTransferChannelCreationProperties::description() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->description;
}

/**
 * Return whether the request will have a last modification time.
 *
 * \return \c true whether it will have a last modification time, \c false
 *         otherwise.
 * \sa lastModificationTime(), setLastModificationTime()
 */
bool FileTransferChannelCreationProperties::hasLastModificationTime() const
{
    if (!isValid()) {
        return false;
    }

    return (mPriv->lastModificationTime.isValid());
}

/**
 * Return the last modification time of the file for the request.
 *
 * \return The last modification time of the file.
 * \sa hasLastModificationTime(), setLastModificationTime()
 */
QDateTime FileTransferChannelCreationProperties::lastModificationTime() const
{
    if (!isValid()) {
        return QDateTime();
    }

    return mPriv->lastModificationTime;
}

/**
 * Return whether the request will have an URI.
 *
 * \return \c true whether it will have URI, \c false otherwise.
 * \sa uri(), setUri()
 */
bool FileTransferChannelCreationProperties::hasUri() const
{
    if (!isValid()) {
        return false;
    }

    return (!mPriv->uri.isEmpty());
}

/**
 * Return the URI of the file for the request.
 * If the URI property is empty and the file transfer is handled by an handler
 * that is not this process, then it won't be able to initiate the file
 * transfer.
 *
 * \return The URI of the file.
 * \sa setUri()
 */
QString FileTransferChannelCreationProperties::uri() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->uri;
}

QVariantMap FileTransferChannelCreationProperties::createRequest() const
{
    if (!isValid()) {
        warning() << "Invalid file transfer creation properties";
        return QVariantMap();
    }

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);

    request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Filename"),
                   suggestedFileName());
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentType"),
                   contentType());
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Size"),
                   size());

    if (hasContentHash()) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHashType"),
                       (uint) contentHashType());
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHash"),
                       contentHash());
    }

    if (hasDescription()) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Description"),
                       description());
    }

    if (hasLastModificationTime()) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Date"),
                       (qulonglong) lastModificationTime().toTime_t());
    }

    if (hasUri()) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".URI"),
                       uri());
    }

    return request;
}

QVariantMap FileTransferChannelCreationProperties::createRequest(const QString &contactIdentifier) const
{
    QVariantMap request = createRequest();

    if (!request.isEmpty()) {
        request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"), contactIdentifier);
    }

    return request;
}

QVariantMap FileTransferChannelCreationProperties::createRequest(uint handle) const
{
    QVariantMap request = createRequest();

    if (!request.isEmpty()) {
        request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"), handle);
    }

    return request;
}

} // Tp
