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

#include <TelepathyQt4/FileTransferChannel>

#include "TelepathyQt4/_gen/file-transfer-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include <QIODevice>
#include <QTcpSocket>

#define BUFFER_SIZE 4096

namespace Tp
{

struct FileTransferChannel::Private
{
    Private(FileTransferChannel *parent);
    ~Private();

    static void introspectProperties(Private *self);

    void extractProperties(const QVariantMap &props);

    void connectToHost();
    void startTransfer();
    void writeData();
    void changeState();
    void setFinished();

    // Public object
    FileTransferChannel *parent;

    Client::ChannelTypeFileTransferInterface *fileTransferInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    FileTransferState pendingState;
    FileTransferStateChangeReason pendingStateReason;
    FileTransferState state;
    FileTransferStateChangeReason stateReason;
    QString contentType;
    QString fileName;
    QString contentHash;
    QString description;
    QDateTime lastModificationTime;
    FileHashType contentHashType;
    qulonglong size;
    qulonglong transferredBytes;
    qulonglong initialOffset;
    qulonglong bytesWritten;

    QIODevice *input;
    QIODevice *output;
    QTcpSocket *socket;
    SocketAddressIPv4 addr;
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
      size(0),
      transferredBytes(0),
      initialOffset(0),
      bytesWritten(0),
      input(0),
      output(0),
      socket(0)
{
    parent->connect(fileTransferInterface,
            SIGNAL(FileTransferStateChanged(uint, uint)),
            SLOT(onFileTrasnferStateChanged(uint, uint)));
    parent->connect(fileTransferInterface,
            SIGNAL(TransferredBytesChanged(qulonglong)),
            SIGNAL(transferredBytesChanged(qulonglong)));
    parent->connect(fileTransferInterface,
            SIGNAL(InitialOffsetDefined(qulonglong)),
            SIGNAL(initialOffsetDefined(qulonglong)));

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
    state = (FileTransferState) qdbus_cast<uint>(props["State"]);
    pendingState = state;
    contentType = qdbus_cast<QString>(props["ContentType"]);
    fileName = qdbus_cast<QString>(props["Filename"]);
    contentHash = qdbus_cast<QString>(props["ContentHash"]);
    description = qdbus_cast<QString>(props["Description"]);
    lastModificationTime.setTime_t((uint) qdbus_cast<qulonglong>(props["Date"]));
    contentHashType = (FileHashType) qdbus_cast<uint>(props["ContentHashType"]);
    size = qdbus_cast<qulonglong>(props["Size"]);
    transferredBytes = qdbus_cast<qulonglong>(props["TransferredBytes"]);
    initialOffset = qdbus_cast<qulonglong>(props["InitialOffset"]);
}

void FileTransferChannel::Private::connectToHost()
{
    socket = new QTcpSocket(parent);

    if (parent->isRequested()) {
        output = socket;
    } else {
        input = socket;
    }

    parent->connect(socket,
            SIGNAL(connected()),
            SLOT(onSocketConnected()));
    debug().nospace() << "Connecting to host " <<
        addr.address << ":" << addr.port << "...";

    socket->connectToHost(addr.address, addr.port);
}

void FileTransferChannel::Private::startTransfer()
{
    parent->connect(input, SIGNAL(readyRead()), SLOT(writeData()));
    parent->writeData();
}

void FileTransferChannel::Private::writeData()
{
    char buffer[16 * 1024];
    qint64 len = input->read(buffer, sizeof(buffer));
    if (len > 0) {
        output->write(buffer, len); // never fails
        bytesWritten += len;
    } else if (len == -1 || (!input->isSequential() && input->atEnd())) {
        // error or EOF
        if (pendingState == FileTransferStateCompleted &&
            bytesWritten != size) {
            // the CM finished receiving the file but some error occurred
            pendingState = FileTransferStateCancelled;
            pendingStateReason = FileTransferStateChangeReasonLocalError;
            warning() << "An error occurred during the transfer";
        }
        setFinished();
        return;
    }

    if (pendingState == FileTransferStateCompleted &&
        bytesWritten == size) {
        setFinished();
        return;
    }

    // if there is no data available, writeData will be called automatically
    // when readyRead is emitted
    if (input->bytesAvailable() > 0) {
        // process more data
        QMetaObject::invokeMethod(parent, "writeData", Qt::QueuedConnection);
    }
}

void FileTransferChannel::Private::changeState()
{
    state = pendingState;
    stateReason = pendingStateReason;
    debug() << "State changed to" << state << "with reason" << stateReason;
    emit parent->stateChanged(state, stateReason);
}

void FileTransferChannel::Private::setFinished()
{
    changeState();
    input->close();
    output->close();
}

/**
 * \class FileTransferChannel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/file-transfer-channel.h> <TelepathyQt4/FileTransferChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the
 * FileTransfer channel type. These channels can be used to transfer one file
 * to or from a contact.
 *
 * This subclass of Channel will eventually provide a high-level API for the
 * FileTransfer interface. Until then, it's just a Channel.
 */

const Feature FileTransferChannel::FeatureCore = Feature(FileTransferChannel::staticMetaObject.className(), 0);

FileTransferChannelPtr FileTransferChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return FileTransferChannelPtr(new FileTransferChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Creates a FileTransferChannel associated with the given object on the same service
 * as the given connection.
 *
 * \param connection  Connection owning this FileTransferChannel, and specifying the
 *                    service.
 * \param objectPath  Path to the object on the service.
 * \param immutableProperties  The immutable properties of the channel, as
 *                             signalled by NewChannels or returned by
 *                             CreateChannel or EnsureChannel
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

FileTransferState FileTransferChannel::state() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling state";
    }

    return mPriv->state;
}

FileTransferStateChangeReason FileTransferChannel::stateReason() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling stateReason";
    }

    return mPriv->stateReason;
}

QString FileTransferChannel::fileName() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling fileName";
    }

    return mPriv->fileName;
}

QString FileTransferChannel::contentType() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling contentType";
    }

    return mPriv->contentType;
}

qulonglong FileTransferChannel::size() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling size";
    }

    return mPriv->size;
}

FileHashType FileTransferChannel::contentHashType() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling contentHashType";
    }

    return mPriv->contentHashType;
}

QString FileTransferChannel::contentHash() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling contentHash";
    }

    return mPriv->contentHash;
}

QString FileTransferChannel::description() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling description";
    }

    return mPriv->description;
}

QDateTime FileTransferChannel::lastModificationTime() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling lastModificationTime";
    }

    return mPriv->lastModificationTime;
}

qulonglong FileTransferChannel::initialOffset() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling initialOffset";
    }

    return mPriv->initialOffset;
}

qulonglong FileTransferChannel::transferredBytes() const
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling transferredBytes";
    }

    return mPriv->transferredBytes;
}

PendingOperation *FileTransferChannel::provideFile(QIODevice *input)
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling provideFile";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel not ready");
    }

    if (!isRequested()) {
        warning() << "Channel must have been requested in order to start an "
            "outgoing file transfer";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_YOURS,
                "Channel must have been requested in order to start an "
                "outgoing file transfer");
    }

    // let's fail here direclty as we may only have one device to handle
    if (mPriv->input) {
        warning() << "File transfer can only be started once in the same "
            "channel";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "File transfer can only be started once in the same channel");
    }

    if ((!input->isOpen() && !input->open(QIODevice::ReadOnly)) &&
        !input->isReadable()) {
        warning() << "Unable to open IO device for reading";
        return new PendingFailure(this, TELEPATHY_ERROR_PERMISSION_DENIED,
                "Unable to open IO device for reading");
    }

    mPriv->input = input;

    PendingVariant *pv = new PendingVariant(
            mPriv->fileTransferInterface->ProvideFile(SocketAddressTypeIPv4,
                SocketAccessControlLocalhost, QDBusVariant(QVariant(QString()))),
            this);
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onCallFinished(Tp::PendingOperation*)));
    return pv;
}

PendingOperation *FileTransferChannel::acceptFile(qulonglong offset,
        QIODevice *output)
{
    if (!isReady(FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling acceptFile";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel not ready");
    }

    if (isRequested()) {
        warning() << "Channel must not have been requested in order to accept "
            "an incoming file transfer";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_YOURS,
                "Channel must not have been requested in order to accept an "
                "incoming file transfer");
    }

    // let's fail here direclty as we may only have one device to handle
    if (mPriv->output) {
        warning() << "File transfer can only be started once in the same "
            "channel";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "File transfer can only be started once in the same channel");
    }

    if ((!output->isOpen() && !output->open(QIODevice::ReadWrite)) &&
        !output->isWritable()) {
        warning() << "Unable to open IO device for writing";
        return new PendingFailure(this, TELEPATHY_ERROR_PERMISSION_DENIED,
                "Unable to open IO device for writing");
    }

    mPriv->output = output;

    PendingVariant *pv = new PendingVariant(
            mPriv->fileTransferInterface->AcceptFile(SocketAddressTypeIPv4,
                SocketAccessControlLocalhost, QDBusVariant(QVariant(QString())),
                offset),
            this);
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onCallFinished(Tp::PendingOperation*)));
    return pv;
}

void FileTransferChannel::cancel()
{
    requestClose();
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

void FileTransferChannel::onCallFinished(PendingOperation *op)
{
    if (op->isError()) {
        // fail providing file, request close
        warning() << "Error starting file transfer, "
            "closing channel...";
        requestClose();
        return;
    }

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    mPriv->addr = qdbus_cast<SocketAddressIPv4>(pv->result());
    debug().nospace() << "Got address " << mPriv->addr.address <<
        ":" << mPriv->addr.port;

    if (mPriv->pendingState == FileTransferStateOpen) {
        mPriv->connectToHost();
    }
}

void FileTransferChannel::onFileTrasnferStateChanged(uint state,
        uint stateReason)
{
    if (state == (uint) mPriv->pendingState) {
        return;
    }

    mPriv->pendingState = (FileTransferState) state;
    mPriv->pendingStateReason = (FileTransferStateChangeReason) stateReason;

    if (state == FileTransferStateOpen && !mPriv->addr.address.isNull()) {
        mPriv->connectToHost();
    }

    if ((state != FileTransferStateCompleted) ||
        (state == FileTransferStateCompleted &&
         mPriv->bytesWritten == mPriv->size)) {
        mPriv->changeState();
    }
}

void FileTransferChannel::onSocketConnected()
{
    debug() << "Connected to host!";
    mPriv->startTransfer();
}

void FileTransferChannel::writeData()
{
    mPriv->writeData();
}

} // Tp
