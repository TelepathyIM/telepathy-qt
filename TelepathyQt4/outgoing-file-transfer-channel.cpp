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

#include <TelepathyQt4/OutgoingFileTransferChannel>

#include "TelepathyQt4/_gen/outgoing-file-transfer-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include <QIODevice>
#include <QTcpSocket>

namespace Tp
{

static const int FT_BLOCK_SIZE = 16 * 1024;

struct OutgoingFileTransferChannel::Private
{
    Private(OutgoingFileTransferChannel *parent);
    ~Private();

    // Public object
    OutgoingFileTransferChannel *parent;

    // Introspection
    QIODevice *input;
    QTcpSocket *socket;
    SocketAddressIPv4 addr;

    qint64 pos;
};

OutgoingFileTransferChannel::Private::Private(OutgoingFileTransferChannel *parent)
    : parent(parent),
      input(0),
      socket(0),
      pos(0)
{
}

OutgoingFileTransferChannel::Private::~Private()
{
}

/**
 * \class OutgoingFileTransferChannel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/outgoing-file-transfer-channel.h> <TelepathyQt4/OutgoingFileTransferChannel>
 *
 * \brief The OutgoingFileTransferChannel class provides an object representing
 * <a href="http://telepathy.freedesktop.org">Telepathy</a> file transfer
 * channel for outgoing file transfers.
 *
 * OutgoingFileTransferChannel is a high-level class representing a
 * <a href="http://telepathy.freedesktop.org">Telepathy</a> file transfer
 * channel for outgoing file transfers.
 */

const Feature OutgoingFileTransferChannel::FeatureCore = Feature(OutgoingFileTransferChannel::staticMetaObject.className(), 0);

OutgoingFileTransferChannelPtr OutgoingFileTransferChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return OutgoingFileTransferChannelPtr(new OutgoingFileTransferChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new outgoing file transfer channel associated with the given
 * \a objectPath on the same service as the given \a connection.
 *
 * \param connection Connection owning this channel, and specifying the service.
 * \param objectPath Path to the object on the service.
 * \param immutableProperties The immutable properties of the channel.
 */
OutgoingFileTransferChannel::OutgoingFileTransferChannel(
        const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : FileTransferChannel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
OutgoingFileTransferChannel::~OutgoingFileTransferChannel()
{
    delete mPriv;
}

/**
 * Provide the file for an outgoing file transfer which has been offered.
 * The state will change to %FileTransferStateOpen as soon as the transfer
 * starts.
 * The given input device should not be destroyed until the state()
 * changes to %FileTransferStateCompleted or %FileTransferStateCancelled.
 * If input is a sequential device QIODevice::isSequential(), it should be
 * closed when no more data is available, so we know when to stop reading.
 *
 * Only the primary handler of a file transfer channel may call this method.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
 *
 * \param input A QIODevice object where the data will be read from.
 * \return A PendingOperation object which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa stateChanged(), state(), stateReason()
 */
PendingOperation *OutgoingFileTransferChannel::provideFile(QIODevice *input)
{
    if (!isReady(FileTransferChannel::FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling provideFile";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel not ready");
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
    connect(input,
            SIGNAL(aboutToClose()),
            SLOT(onInputAboutToClose()));

    PendingVariant *pv = new PendingVariant(
            fileTransferInterface(BypassInterfaceCheck)->ProvideFile(
                SocketAddressTypeIPv4,
                SocketAccessControlLocalhost,
                QDBusVariant(QVariant(QString()))),
            this);
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onProvideFileFinished(Tp::PendingOperation*)));
    return pv;
}

void OutgoingFileTransferChannel::onProvideFileFinished(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Error providing file transfer " <<
            op->errorName() << ":" << op->errorMessage();
        invalidate(op->errorName(), op->errorMessage());
        return;
    }

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    mPriv->addr = qdbus_cast<SocketAddressIPv4>(pv->result());
    debug().nospace() << "Got address " << mPriv->addr.address <<
        ":" << mPriv->addr.port;

    if (state() == FileTransferStateOpen) {
        connectToHost();
    }
}

void OutgoingFileTransferChannel::connectToHost()
{
    if (isConnected() || mPriv->addr.address.isNull()) {
        return;
    }

    mPriv->socket = new QTcpSocket(this);

    connect(mPriv->socket, SIGNAL(connected()),
            SLOT(onSocketConnected()));
    connect(mPriv->socket, SIGNAL(disconnected()),
            SLOT(onSocketDisconnected()));
    connect(mPriv->socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(mPriv->socket, SIGNAL(bytesWritten(qint64)),
            SLOT(doTransfer()));

    debug().nospace() << "Connecting to host " <<
        mPriv->addr.address << ":" << mPriv->addr.port << "...";
    mPriv->socket->connectToHost(mPriv->addr.address, mPriv->addr.port);
}

void OutgoingFileTransferChannel::onSocketConnected()
{
    debug() << "Connected to host";
    setConnected();

    connect(mPriv->input, SIGNAL(readyRead()),
            SLOT(doTransfer()));

    // for non sequential devices, let's seek to the initialOffset
    if (!mPriv->input->isSequential()) {
        if (mPriv->input->seek(initialOffset())) {
            mPriv->pos = initialOffset();
        }
    }

    debug() << "Starting transfer...";
    doTransfer();
}

void OutgoingFileTransferChannel::onSocketDisconnected()
{
    debug() << "Disconnected from host";
    setFinished();
}

void OutgoingFileTransferChannel::onSocketError(QAbstractSocket::SocketError error)
{
    debug() << "Socket error" << error;
    setFinished();
}

void OutgoingFileTransferChannel::onInputAboutToClose()
{
    debug() << "Input closed";

    // in case of sequential devices, we should read everything from it and
    // write to the socket. Let's not do this for non-sequential devices as we
    // don't want to read a whole file into memory.
    if (isConnected() && mPriv->input->isSequential()) {
        QByteArray data;
        data = mPriv->input->readAll();
        mPriv->socket->write(data); // never fails
    }

    setFinished();
}

void OutgoingFileTransferChannel::doTransfer()
{
    // read FT_BLOCK_SIZE each time, as input can be a QFile, we don't want to
    // block reading the whole file
    char buffer[FT_BLOCK_SIZE];
    char *p = buffer;

    memset(buffer, 0, sizeof(buffer));
    qint64 len = mPriv->input->read(buffer, sizeof(buffer));

    bool scheduleTransfer = false;
    if (((qulonglong) mPriv->pos < initialOffset()) && (len > 0)) {
        qint64 skip = (qint64) qMin(initialOffset() - mPriv->pos,
                (qulonglong) len);

        debug() << "skipping" << skip << "bytes";
        if (skip == len) {
            // nothing to write, all data read was skipped
            // schedule a transfer, as readyRead may never be called and
            // bytesWriiten will not
            scheduleTransfer = true;
            goto end;
        }

        p += skip;
        len -= skip;
    }

    if (len > 0) {
        mPriv->socket->write(p, len); // never fails
    }

end:
    if (len == -1 || (!mPriv->input->isSequential() && mPriv->input->atEnd())) {
        // error or EOF
        setFinished();
        return;
    }

    mPriv->pos += len;

    if (scheduleTransfer) {
        QMetaObject::invokeMethod(this, SLOT(doTransfer()),
                Qt::QueuedConnection);
    }
}

void OutgoingFileTransferChannel::setFinished()
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
        disconnect(mPriv->socket, SIGNAL(bytesWritten(qint64)),
                   this, SLOT(doTransfer()));
        mPriv->socket->close();
    }

    if (mPriv->input) {
        disconnect(mPriv->input, SIGNAL(aboutToClose()),
                   this, SLOT(onInputAboutToClose()));
        disconnect(mPriv->input, SIGNAL(readyRead()),
                   this, SLOT(doTransfer()));
        mPriv->input->close();
    }

    FileTransferChannel::setFinished();
}

} // Tp
