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

#include <TelepathyQt/IncomingFileTransferChannel>

#include "TelepathyQt/_gen/incoming-file-transfer-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/Types>
#include <TelepathyQt/types-internal.h>

#include <QIODevice>
#include <QTcpSocket>

namespace Tp
{

struct TP_QT_NO_EXPORT IncomingFileTransferChannel::Private
{
    Private(IncomingFileTransferChannel *parent);
    ~Private();

    // Public object
    IncomingFileTransferChannel *parent;

    Client::ChannelTypeFileTransferInterface *fileTransferInterface;

    QIODevice *output;
    QTcpSocket *socket;
    SocketAddressIPv4 addr;

    qulonglong requestedOffset;
    qint64 pos;
    bool weOpenedDevice;
};

IncomingFileTransferChannel::Private::Private(IncomingFileTransferChannel *parent)
    : parent(parent),
      fileTransferInterface(parent->interface<Client::ChannelTypeFileTransferInterface>()),
      output(0),
      socket(0),
      requestedOffset(0),
      pos(0),
      weOpenedDevice(false)
{
    parent->connect(fileTransferInterface,
            SIGNAL(URIDefined(QString)),
            SLOT(onUriDefined(QString)));
    parent->connect(fileTransferInterface,
            SIGNAL(URIDefined(QString)),
            SIGNAL(uriDefined(QString)));
}

IncomingFileTransferChannel::Private::~Private()
{
}

/**
 * \class IncomingFileTransferChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/incoming-file-transfer-channel.h <TelepathyQt/IncomingFileTransferChannel>
 *
 * \brief The IncomingFileTransferChannel class represents a Telepathy channel
 * of type FileTransfer for incoming file transfers.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * IncomingFileTransferChannel object usable.
 *
 * This is currently the same as FileTransferChannel::FeatureCore, but may change to include more.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature IncomingFileTransferChannel::FeatureCore =
    Feature(QLatin1String(FileTransferChannel::staticMetaObject.className()), 0); // FT::FeatureCore

/**
 * Create a new IncomingFileTransferChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A IncomingFileTransferChannelPtr object pointing to the newly created
 *         IncomingFileTransfer object.
 */
IncomingFileTransferChannelPtr IncomingFileTransferChannel::create(
        const ConnectionPtr &connection, const QString &objectPath,
        const QVariantMap &immutableProperties)
{
    return IncomingFileTransferChannelPtr(new IncomingFileTransferChannel(
                connection, objectPath, immutableProperties,
                IncomingFileTransferChannel::FeatureCore));
}

/**
 * Construct a new IncomingFileTransferChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on IncomingFileTransferChannel::FeatureCore.
 */
IncomingFileTransferChannel::IncomingFileTransferChannel(
        const ConnectionPtr &connection, const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : FileTransferChannel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
IncomingFileTransferChannel::~IncomingFileTransferChannel()
{
    delete mPriv;
}

/**
 * Set the URI where the file will be saved.
 *
 * This property may be set by the channel handler before calling AcceptFile to inform observers
 * where the incoming file will be saved. When the URI property is set, the signal
 * uriDefined() is emitted.
 *
 * This method requires IncomingFileTransferChannel::FeatureCore to be ready.
 *
 * \param uri The URI where the file will be saved.
 * \return A PendingOperation object which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa FileTransferChannel::uri(), uriDefined()
 */
PendingOperation *IncomingFileTransferChannel::setUri(const QString& uri)
{
    if (!isReady(FileTransferChannel::FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling setUri";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                IncomingFileTransferChannelPtr(this));
    }

    if (state() != FileTransferStatePending) {
        warning() << "setUri must be called before calling acceptFile";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Cannot set URI after calling acceptFile"),
                IncomingFileTransferChannelPtr(this));
    }

    return mPriv->fileTransferInterface->setPropertyURI(uri);
}

/**
 * Accept a file transfer that's in the #FileTransferStatePending state().
 *
 * The state will change to #FileTransferStateOpen as soon as the transfer
 * starts.
 * The given output device should not be closed/destroyed until the state()
 * changes to #FileTransferStateCompleted or #FileTransferStateCancelled.
 *
 * Only the primary handler of a file transfer channel may call this method.
 *
 * This method requires IncomingFileTransferChannel::FeatureCore to be ready.
 *
 * \param offset The desired offset in bytes where the file transfer should
 *               start. The offset is taken from the beginning of the file.
 *               Specifying an offset of zero will start the transfer from the
 *               beginning of the file. The offset that is actually given in the
 *               initialOffset() method can differ from this argument where the
 *               requested offset is not supported. (For example, some
 *               protocols do not support offsets at all so the initialOffset()
 *               will always be 0.).
 * \param output A QIODevice object where the data will be written to. The
 *               device should be ready to use when the state() changes to
 *               #FileTransferStateCompleted.
 *               If the transfer is cancelled, state() becomes
 *               #FileTransferStateCancelled, the data in \a output should be
 *               ignored
 * \return A PendingOperation object which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa FileTransferChannel::stateChanged(), FileTransferChannel::state(),
 *     FileTransferChannel::stateReason(), FileTransferChannel::initialOffset()
 */
PendingOperation *IncomingFileTransferChannel::acceptFile(qulonglong offset,
        QIODevice *output)
{
    if (!isReady(FileTransferChannel::FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling acceptFile";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                IncomingFileTransferChannelPtr(this));
    }

    // let's fail here direclty as we may only have one device to handle
    if (mPriv->output) {
        warning() << "File transfer can only be started once in the same "
            "channel";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("File transfer can only be started once in the same channel"),
                IncomingFileTransferChannelPtr(this));
    }

    if (!output->isOpen()) {
        if (output->open(QIODevice::WriteOnly)) {
            mPriv->weOpenedDevice = true;
        }
    }

    if (!output->isWritable()) {
        mPriv->weOpenedDevice = false;
        warning() << "Unable to open IO device for writing";
        return new PendingFailure(TP_QT_ERROR_PERMISSION_DENIED,
                                  QLatin1String("Unable to open IO device for writing"),
                                  IncomingFileTransferChannelPtr(this));
    }

    mPriv->output = output;

    mPriv->requestedOffset = offset;

    PendingVariant *pv = new PendingVariant(
            mPriv->fileTransferInterface->AcceptFile(SocketAddressTypeIPv4,
                SocketAccessControlLocalhost, QDBusVariant(QVariant(QString())),
                offset),
            IncomingFileTransferChannelPtr(this));
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAcceptFileFinished(Tp::PendingOperation*)));
    return pv;
}

void IncomingFileTransferChannel::onAcceptFileFinished(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Error accepting file transfer " <<
            op->errorName() << ":" << op->errorMessage();
        invalidate(op->errorName(), op->errorMessage());
        return;
    }

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    mPriv->addr = qdbus_cast<SocketAddressIPv4>(pv->result());
    debug().nospace() << "Got address " << mPriv->addr.address <<
        ":" << mPriv->addr.port;

    if (state() == FileTransferStateOpen) {
        // now we have the address and we are already opened,
        // connect to host
        connectToHost();
    }
}

void IncomingFileTransferChannel::connectToHost()
{
    if (isConnected() || mPriv->addr.address.isNull()) {
        return;
    }

    // we already have initialOffsetDefined, called before State became Open, so
    // let's make sure everything is ok.
    if (initialOffset() > mPriv->requestedOffset) {
        // either the CM or the sender is doing something really wrong here,
        // cancel the transfer.
        warning() << "InitialOffset bigger than requested offset, "
            "cancelling the transfer";
        cancel();
        invalidate(TP_QT_ERROR_INCONSISTENT,
                QLatin1String("Initial offset bigger than requested offset"));
        return;
    }

    mPriv->pos = initialOffset();

    mPriv->socket = new QTcpSocket(this);

    connect(mPriv->socket, SIGNAL(connected()),
            SLOT(onSocketConnected()));
    connect(mPriv->socket, SIGNAL(disconnected()),
            SLOT(onSocketDisconnected()));
    connect(mPriv->socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(mPriv->socket, SIGNAL(readyRead()),
            SLOT(doTransfer()));

    debug().nospace() << "Connecting to host " <<
        mPriv->addr.address << ":" << mPriv->addr.port << "...";
    mPriv->socket->connectToHost(mPriv->addr.address, mPriv->addr.port);
}

void IncomingFileTransferChannel::onSocketConnected()
{
    debug() << "Connected to host";
    setConnected();

    doTransfer();
}

void IncomingFileTransferChannel::onSocketDisconnected()
{
    debug() << "Disconnected from host";
    setFinished();
}

void IncomingFileTransferChannel::onSocketError(QAbstractSocket::SocketError error)
{
    setFinished();
}

void IncomingFileTransferChannel::doTransfer()
{
    QByteArray data;
    while (mPriv->socket->bytesAvailable()) {
        data = mPriv->socket->readAll();

        // skip until we reach requetedOffset and start writing from there
        if ((qulonglong) mPriv->pos < mPriv->requestedOffset) {
            if ((qulonglong) data.length() <= (mPriv->requestedOffset - mPriv->pos)) {
                break;
            }
            data = data.mid(mPriv->requestedOffset - mPriv->pos);
        }

        mPriv->output->write(data); // never fails
    }

    mPriv->pos += data.length();
}

void IncomingFileTransferChannel::setFinished()
{
    if (isFinished()) {
        // it shouldn't happen but let's make sure
        return;
    }

    if (mPriv->socket) {
        disconnect(mPriv->socket, SIGNAL(connected()),
                   this, SLOT(onSocketConnected()));
        disconnect(mPriv->socket, SIGNAL(disconnected()),
                   this, SLOT(onSocketDisconnected()));
        disconnect(mPriv->socket, SIGNAL(error(QAbstractSocket::SocketError)),
                   this, SLOT(onSocketError(QAbstractSocket::SocketError)));
        disconnect(mPriv->socket, SIGNAL(readyRead()),
                   this, SLOT(doTransfer()));
        mPriv->socket->close();
    }

    if (mPriv->output && mPriv->weOpenedDevice) {
        mPriv->output->close();
    }

    FileTransferChannel::setFinished();
}

/**
 * \fn void IncomingFileTransferChannel::uriDefined(const QString &uri)
 *
 * Emitted when the value of uri() changes.
 *
 * \param uri The new URI of this file transfer channel.
 * \sa FileTransferChannel::uri(), setUri()
 */

} // Tp
