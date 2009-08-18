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

#include <TelepathyQt4/FileTransferChannel>

#include "TelepathyQt4/_gen/file-transfer-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Types>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT FileTransferChannel::Private
{
    Private(FileTransferChannel *parent);
    ~Private();

    static void introspectProperties(Private *self);

    void extractProperties(const QVariantMap &props);

    // Public object
    FileTransferChannel *parent;

    Client::ChannelTypeFileTransferInterface *fileTransferInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint pendingState;
    uint pendingStateReason;
    uint state;
    uint stateReason;
    QString contentType;
    QString fileName;
    QString contentHash;
    QString description;
    QDateTime lastModificationTime;
    FileHashType contentHashType;
    qulonglong initialOffset;
    qulonglong size;
    qulonglong transferredBytes;
    SupportedSocketMap availableSocketTypes;

    bool connected;
    bool finished;
};

FileTransferChannel::Private::Private(FileTransferChannel *parent)
    : parent(parent),
      fileTransferInterface(parent->fileTransferInterface(BypassInterfaceCheck)),
      properties(0),
      readinessHelper(parent->readinessHelper()),
      pendingState(FileTransferStateNone),
      pendingStateReason(FileTransferStateChangeReasonNone),
      state(pendingState),
      stateReason(pendingStateReason),
      contentHashType(FileHashTypeNone),
      initialOffset(0),
      size(0),
      transferredBytes(0),
      connected(false),
      finished(false)
{
    parent->connect(fileTransferInterface,
            SIGNAL(InitialOffsetDefined(qulonglong)),
            SLOT(onInitialOffsetDefined(qulonglong)));
    parent->connect(fileTransferInterface,
            SIGNAL(FileTransferStateChanged(uint, uint)),
            SLOT(onStateChanged(uint, uint)));
    parent->connect(fileTransferInterface,
            SIGNAL(TransferredBytesChanged(qulonglong)),
            SLOT(onTransferredBytesChanged(qulonglong)));

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectProperties,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
}

FileTransferChannel::Private::~Private()
{
}

void FileTransferChannel::Private::introspectProperties(
        FileTransferChannel::Private *self)
{
    if (!self->properties) {
        self->properties = self->parent->propertiesInterface();
        Q_ASSERT(self->properties != 0);
    }

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(
                    TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotProperties(QDBusPendingCallWatcher *)));
}

void FileTransferChannel::Private::extractProperties(const QVariantMap &props)
{
    pendingState = state = qdbus_cast<uint>(props["State"]);
    contentType = qdbus_cast<QString>(props["ContentType"]);
    fileName = qdbus_cast<QString>(props["Filename"]);
    contentHash = qdbus_cast<QString>(props["ContentHash"]);
    description = qdbus_cast<QString>(props["Description"]);
    lastModificationTime.setTime_t((uint) qdbus_cast<qulonglong>(props["Date"]));
    contentHashType = (FileHashType) qdbus_cast<uint>(props["ContentHashType"]);
    initialOffset = qdbus_cast<qulonglong>(props["InitialOffset"]);
    size = qdbus_cast<qulonglong>(props["Size"]);
    transferredBytes = qdbus_cast<qulonglong>(props["TransferredBytes"]);
    availableSocketTypes = qdbus_cast<SupportedSocketMap>(props["AvailableSocketTypes"]);
}

/**
 * \class FileTransferChannel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/file-transfer-channel.h> <TelepathyQt4/FileTransferChannel>
 *
 * \brief The FileTransferChannel class provides an object representing
 * <a href="http://telepathy.freedesktop.org">Telepathy</a> file transfer
 * channel.
 *
 * FileTransferChannel is a high-level class representing a
 * <a href="http://telepathy.freedesktop.org">Telepathy</a> file transfer
 * channel.

 * For more specialized file transfer classes, please refer to
 * OutgoingFileTransferChannel and IncomingFileTransferChannel.
 */

const Feature FileTransferChannel::FeatureCore = Feature(FileTransferChannel::staticMetaObject.className(), 0);

FileTransferChannelPtr FileTransferChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return FileTransferChannelPtr(new FileTransferChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new file transfer channel associated with the given \a objectPath
 * on the same service as the given \a connection.
 *
 * \param connection Connection owning this channel, and specifying the service.
 * \param objectPath Path to the object on the service.
 * \param immutableProperties The immutable properties of the channel.
 */
FileTransferChannel::FileTransferChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : Channel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
FileTransferChannel::~FileTransferChannel()
{
    delete mPriv;
}

/**
 * Return the state of the file transfer as described by the %FileTransferState
 * enum.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The state of the file transfer.
 */
FileTransferState FileTransferChannel::state() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling state";
    }

    return (FileTransferState) mPriv->state;
}

/**
 * Return the for the state change as described by the
 * %FileTransferStateChangeReason enum.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The reason for the state change.
 */
FileTransferStateChangeReason FileTransferChannel::stateReason() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling stateReason";
    }

    return (FileTransferStateChangeReason) mPriv->stateReason;
}

/**
 * Return the name of the file on the sender's side. This is given as
 * a suggested filename for the receiver. This cannot change once the channel
 * has been created.
 *
 * This property should be the basename of the file being sent. For example, if
 * the sender sends the file /home/user/monkey.pdf then this property should be
 * set to monkey.pdf.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return Suggested filename for the receiver.
 */
QString FileTransferChannel::fileName() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling fileName";
    }

    return mPriv->fileName;
}

/**
 * Return the file's MIME type. This cannot change once the channel has been
 * created.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The file's MIME type.
 */
QString FileTransferChannel::contentType() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling contentType";
    }

    return mPriv->contentType;
}

/**
 * The size of the file. This cannot change once the channel has been
 * created.
 *
 * Note that the size is not guaranteed to be exactly right for
 * incoming files. This is merely a hint and should not be used to know when the
 * transfer finished.
 *
 * For unknown sizes the return value can be UINT64_MAX.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The size of the file.
 */
qulonglong FileTransferChannel::size() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling size";
    }

    return mPriv->size;
}

/**
 * Return the type of the contentHash().
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The type of the contentHash().
 * \sa contentHash()
 */
FileHashType FileTransferChannel::contentHashType() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling contentHashType";
    }

    return mPriv->contentHashType;
}

/**
 * Return the hash of the contents of the file transfer, of type described in
 * the value of the contentHashType().
 *
 * Its value MUST correspond to the appropriate type of the contentHashType().
 * If the contentHashType() is set to %FileHashTypeNone, then the
 * returned value is an empty string.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return Hash of the contents of the file transfer.
 * \sa contentHashType()
 */
QString FileTransferChannel::contentHash() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling contentHash";
    }

    if (mPriv->contentHashType == FileHashTypeNone) {
        return QString();
    }

    return mPriv->contentHash;
}

/**
 * Return the description of the file transfer. This cannot change once the
 * channel has been created.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The description of the file transfer.
 */
QString FileTransferChannel::description() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling description";
    }

    return mPriv->description;
}

/**
 * Return the last modification time of the file being transferred. This cannot
 * change once the channel has been created.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The last modification time of the file being transferred.
 */
QDateTime FileTransferChannel::lastModificationTime() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling lastModificationTime";
    }

    return mPriv->lastModificationTime;
}

/**
 * Return the offset in bytes from which the file will be sent.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return The offset in bytes from where the file should be sent.
 */
qulonglong FileTransferChannel::initialOffset() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling initialOffset";
    }

    return mPriv->initialOffset;
}

/**
 * Return the number of bytes that have been transferred.
 * This will be updated as the file transfer continues.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return Number of bytes that have been transferred.
 * \sa transferredBytesChanged()
 */
qulonglong FileTransferChannel::transferredBytes() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling transferredBytes";
    }

    return mPriv->transferredBytes;
}

/**
 * Return a mapping from address types (members of SocketAddressType) to arrays
 * of access-control type (members of SocketAccessControl) that the connection
 * manager supports for sockets with that address type. For simplicity, if a CM
 * supports offering a particular type of file transfer, it is assumed to
 * support accepting it. All connection Managers support at least
 * SocketAddressTypeIPv4.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \return A mapping from address types to arrays of access-control type.
 * \sa transferredBytesChanged()
 */
SupportedSocketMap FileTransferChannel::availableSocketTypes() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling availableSocketTypes";
    }

    return mPriv->availableSocketTypes;
}


/**
 * Cancel a file transfer.
 *
 * \return A PendingOperation object which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *FileTransferChannel::cancel()
{
    return requestClose();
}

/**
 * Protected virtual method called when the state becomes
 * %FileTransferStateOpen.
 *
 * Specialized classes should reimplement this method and call setConnected()
 * when the connection is established.
 *
 * \sa setConnected()
 */
void FileTransferChannel::connectToHost()
{
    // do nothing
}

/**
 * Return whether a connection has been established.
 *
 * \return Whether a connection has been established.
 * \sa setConnected()
 */
bool FileTransferChannel::isConnected() const
{
    return mPriv->connected;
}

/**
 * Indicate whether a connection has been established.
 *
 * Specialized classes that reimplement connectToHost() must call this method
 * once the connection has been established or setFinished() if an error
 * occurred.
 *
 * \sa isConnected(), connectToHost(), setFinished()
 */
void FileTransferChannel::setConnected()
{
    mPriv->connected = true;
}

/**
 * Return whether sending/receiving has finished.
 *
 * \return Whether sending/receiving has finished.
 */
bool FileTransferChannel::isFinished() const
{
    return mPriv->finished;
}

/**
 * Protected virtual method called when an error occurred and the transfer
 * should finish.
 *
 * Specialized classes should reimplement this method and close the IO devices
 * and do all the needed cleanup.
 *
 * Note that for specialized classes that reimplement connectToHost() and set
 * isConnected() to true, the state will not change to
 * %FileTransferStateCompleted once the state change is received.
 *
 * When finished sending/receiving the specialized class MUST call this method
 * and then the state will change to the latest pending state.
 */
void FileTransferChannel::setFinished()
{
    mPriv->finished = true;

    // do the actual state change, in case we are in
    // FileTransferStateCompleted pendingState
    changeState();
}

void FileTransferChannel::gotProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        mPriv->extractProperties(props);
        debug() << "Got reply to Properties::GetAll(FileTransferChannel)";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
    else {
        warning().nospace() << "Properties::GetAll(FileTransferChannel) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                reply.error());
    }
}

void FileTransferChannel::changeState()
{
    if (mPriv->state == mPriv->pendingState) {
        return;
    }

    mPriv->state = mPriv->pendingState;
    mPriv->stateReason = mPriv->pendingStateReason;
    emit stateChanged((FileTransferState) mPriv->state,
            (FileTransferStateChangeReason) mPriv->stateReason);
}

void FileTransferChannel::onStateChanged(uint state, uint stateReason)
{
    if (state == (uint) mPriv->pendingState) {
        return;
    }

    debug() << "File transfer state changed to" << state <<
        "with reason" << stateReason;
    mPriv->pendingState = (FileTransferState) state;
    mPriv->pendingStateReason = (FileTransferStateChangeReason) stateReason;

    switch (state) {
        case FileTransferStateOpen:
            // try to connect to host, for handlers this
            // connect to host, as the user called Accept/ProvideFile
            // and have the host addr, for observers this will do nothing and
            // everything will keep working
            connectToHost();
            changeState();
            break;
        case FileTransferStateCompleted:
            //iIf already finished sending/receiving, just change the state,
            // if not completed will only be set when:
            // IncomingChannel:
            //  - The input socket closes
            // OutgoingChannel:
            //  - Input EOF is reached or the output socket is closed
            //
            // we also check for connected as observers will never be connected
            // and finished will never be set, but we need to work anyway.
            if (mPriv->finished || !mPriv->connected) {
                changeState();
            }
            break;
        case FileTransferStateCancelled:
            // if already finished sending/receiving, just change the state,
            // if not finish now and change the state
            if (!mPriv->finished) {
                setFinished();
            } else {
                changeState();
            }
            break;
        default:
            changeState();
            break;
    }
}

void FileTransferChannel::onInitialOffsetDefined(qulonglong initialOffset)
{
    mPriv->initialOffset = initialOffset;
    emit initialOffsetDefined(initialOffset);
}

void FileTransferChannel::onTransferredBytesChanged(qulonglong count)
{
    mPriv->transferredBytes = count;
    emit transferredBytesChanged(count);
}

} // Tp
