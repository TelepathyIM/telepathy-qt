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

#include <TelepathyQt/FileTransferChannel>

#include "TelepathyQt/_gen/file-transfer-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT FileTransferChannel::Private
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
    QString uri;
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
      fileTransferInterface(parent->interface<Client::ChannelTypeFileTransferInterface>()),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
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
            SIGNAL(FileTransferStateChanged(uint,uint)),
            SLOT(onStateChanged(uint,uint)));
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
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(
                    TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotProperties(QDBusPendingCallWatcher*)));
}

void FileTransferChannel::Private::extractProperties(const QVariantMap &props)
{
    pendingState = state = qdbus_cast<uint>(props[QLatin1String("State")]);
    contentType = qdbus_cast<QString>(props[QLatin1String("ContentType")]);
    fileName = qdbus_cast<QString>(props[QLatin1String("Filename")]);
    uri = qdbus_cast<QString>(props[QLatin1String("URI")]);
    contentHash = qdbus_cast<QString>(props[QLatin1String("ContentHash")]);
    description = qdbus_cast<QString>(props[QLatin1String("Description")]);
    lastModificationTime.setTime_t((uint) qdbus_cast<qulonglong>(props[QLatin1String("Date")]));
    contentHashType = (FileHashType) qdbus_cast<uint>(props[QLatin1String("ContentHashType")]);
    initialOffset = qdbus_cast<qulonglong>(props[QLatin1String("InitialOffset")]);
    size = qdbus_cast<qulonglong>(props[QLatin1String("Size")]);
    transferredBytes = qdbus_cast<qulonglong>(props[QLatin1String("TransferredBytes")]);
    availableSocketTypes = qdbus_cast<SupportedSocketMap>(props[QLatin1String("AvailableSocketTypes")]);
}

/**
 * \class FileTransferChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/file-transfer-channel.h <TelepathyQt/FileTransferChannel>
 *
 * \brief The FileTransferChannel class represents a Telepathy channel of type
 * FileTransfer.
 *
 * For more specialized file transfer classes, please refer to
 * OutgoingFileTransferChannel and IncomingFileTransferChannel.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * FileTransferChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * FileTransferChannel methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature FileTransferChannel::FeatureCore = Feature(QLatin1String(FileTransferChannel::staticMetaObject.className()), 0);

/**
 * Create a new FileTransferChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A FileTransferChannelPtr object pointing to the newly created
 *         FileTransferChannel object.
 */
FileTransferChannelPtr FileTransferChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return FileTransferChannelPtr(new FileTransferChannel(connection, objectPath,
                immutableProperties, FileTransferChannel::FeatureCore));
}

/**
 * Construct a new FileTransferChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on FileTransferChannel::FeatureCore.
 */
FileTransferChannel::FileTransferChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
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
 * Return the state of the file transfer as described by #FileTransferState.
 *
 * Change notification is via the stateChanged() signal.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The state as #FileTransferState.
 * \sa stateReason()
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
 * Return the reason for the state change as described by the #FileTransferStateChangeReason.
 *
 * Change notification is via the stateChanged() signal.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The state reason as #FileTransferStateChangeReason.
 * \sa state()
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
 * a suggested filename for the receiver.
 *
 * This property should be the basename of the file being sent. For example, if
 * the sender sends the file /home/user/monkey.pdf then this property should be
 * set to monkey.pdf.
 *
 * This property cannot change once the channel has been created.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The suggested filename for the receiver.
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
 * Return the file's MIME type.
 *
 * This property cannot change once the channel has been created.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
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
 * Return the size of the file.
 *
 * Note that the size is not guaranteed to be exactly right for
 * incoming files. This is merely a hint and should not be used to know when the
 * transfer finished.
 *
 * For unknown sizes the return value can be UINT64_MAX.
 *
 * This property cannot change once the channel has been created.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The file size.
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
 * Return the URI of the file.
 *
 * On outgoing file transfers, this property cannot change after the channel
 * is requested. For incoming file transfers, this property may be set by the
 * channel handler before calling AcceptFile to inform observers where the
 * incoming file will be saved. When the URI property is set, the signal
 * IncomingFileTransferChannel::uriDefined() is emitted.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The file uri.
 * \sa IncomingFileTransferChannel::uriDefined()
 */
QString FileTransferChannel::uri() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling uri";
    }

    return mPriv->uri;
}

/**
 * Return the type of the contentHash().
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The content hash type as #FileHashType.
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
 * If the contentHashType() is set to #FileHashTypeNone, then the
 * returned value is an empty string.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The hash of the contents.
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
 * Return the description of the file transfer.
 *
 * This property cannot change once the channel has been created.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The description.
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
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The file modification time as QDateTime.
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
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The offset in bytes.
 * \sa initialOffsetDefined()
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
 *
 * Change notification is via the transferredBytesChanged() signal.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The number of bytes.
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
 * Return a mapping from address types (members of #SocketAddressType) to arrays
 * of access-control type (members of #SocketAccessControl) that the CM
 * supports for sockets with that address type.
 *
 * For simplicity, if a CM supports offering a particular type of file transfer,
 * it is assumed to support accepting it. All CMs support at least
 * SocketAddressTypeIPv4.
 *
 * This method requires FileTransferChannel::FeatureCore to be ready.
 *
 * \return The available socket types as a map from address types to arrays of access-control type.
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
 * #FileTransferStateOpen.
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
 * \return \c true if the connections has been established, \c false otherwise.
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
 * \return \c true if sending/receiving has finished, \c false otherwise.
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
 * #FileTransferStateCompleted once the state change is received.
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

void FileTransferChannel::onUriDefined(const QString &uri)
{
    mPriv->uri = uri;
    // Signal is emitted only by IncomingFileTransferChannels
}

/**
 * \fn void FileTransferChannel::stateChanged(Tp::FileTransferState state,
 *          Tp::FileTransferStateChangeReason reason)
 *
 * Emitted when the value of state() changes.
 *
 * \param state The new state of this file transfer channel.
 * \param reason The reason for the change of state.
 * \sa state()
 */

/**
 * \fn void FileTransferChannel::initialOffsetDefined(qulonglong initialOffset)
 *
 * Emitted when the initial offset for the file transfer is
 * defined.
 *
 * \param initialOffset The new initial offset for the file transfer.
 * \sa initialOffset()
 */

/**
 * \fn void FileTransferChannel::transferredBytesChanged(qulonglong count);
 *
 * Emitted when the value of transferredBytes() changes.
 *
 * \param count The new number of bytes transferred.
 * \sa transferredBytes()
 */

} // Tp
