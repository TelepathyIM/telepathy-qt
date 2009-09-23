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

#include <TelepathyQt4/IncomingFileTransferChannel>

#include "TelepathyQt4/_gen/incoming-file-transfer-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include <QIODevice>
#include <QTcpSocket>

namespace Tp
{

struct IncomingFileTransferChannel::Private
{
    Private(IncomingFileTransferChannel *parent);
    ~Private();

    // Public object
    IncomingFileTransferChannel *parent;

    QIODevice *output;
    QTcpSocket *socket;
    SocketAddressIPv4 addr;
};

IncomingFileTransferChannel::Private::Private(IncomingFileTransferChannel *parent)
    : parent(parent),
      output(0),
      socket(0)
{
}

IncomingFileTransferChannel::Private::~Private()
{
}

/**
 * \class IncomingFileTransferChannel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/incoming-file-transfer-channel.h> <TelepathyQt4/IncomingFileTransferChannel>
 *
 * \brief The IncomingFileTransferChannel class provides an object representing
 * <a href="http://telepathy.freedesktop.org">Telepathy</a> file transfer
 * channel for incoming file transfers.
 *
 * IncomingFileTransferChannel is a high-level class representing a
 * <a href="http://telepathy.freedesktop.org">Telepathy</a> file transfer
 * channel for incoming file transfers.
 */

const Feature IncomingFileTransferChannel::FeatureCore = Feature(IncomingFileTransferChannel::staticMetaObject.className(), 0);

IncomingFileTransferChannelPtr IncomingFileTransferChannel::create(
        const ConnectionPtr &connection, const QString &objectPath,
        const QVariantMap &immutableProperties)
{
    return IncomingFileTransferChannelPtr(new IncomingFileTransferChannel(
                connection, objectPath, immutableProperties));
}

/**
 * Construct a new incoming file transfer channel associated with the given
 * \a objectPath on the same service as the given \a connection.
 *
 * \param connection Connection owning this channel, and specifying the service.
 * \param objectPath Path to the object on the service.
 * \param immutableProperties The immutable properties of the channel.
 */
IncomingFileTransferChannel::IncomingFileTransferChannel(
        const ConnectionPtr &connection, const QString &objectPath,
        const QVariantMap &immutableProperties)
    : FileTransferChannel(connection, objectPath, immutableProperties),
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
 * Accept a file transfer that's in the %FileTransferStatePending state().
 * The state will change to %FileTransferStateOpen as soon as the transfer
 * starts.
 * The given output device should not be closed/destroyed until the state()
 * changes to %FileTransferStateCompleted or %FileTransferStateCancelled.
 *
 * Only the primary handler of a file transfer channel may call this method.
 *
 * This method requires FileTransferChannel::FeatureCore to be enabled.
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
 *               %FileTransferStateCompleted.
 *               If the transfer is cancelled, state() becomes
 *               %FileTransferStateCancelled, the data in \a output should be
 *               ignored 
 * \return A PendingOperation object which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa stateChanged(), state(), stateReason(), initialOffset()
 */
PendingOperation *IncomingFileTransferChannel::acceptFile(qulonglong offset,
        QIODevice *output)
{
    if (!isReady(FileTransferChannel::FeatureCore)) {
        warning() << "FileTransferChannel::FeatureCore must be ready before "
            "calling acceptFile";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Channel not ready");
    }

    // let's fail here direclty as we may only have one device to handle
    if (mPriv->output) {
        warning() << "File transfer can only be started once in the same "
            "channel";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "File transfer can only be started once in the same channel");
    }

    if ((!output->isOpen() && !output->open(QIODevice::WriteOnly)) &&
        (!output->isWritable())) {
        warning() << "Unable to open IO device for writing";
        return new PendingFailure(this, TELEPATHY_ERROR_PERMISSION_DENIED,
                "Unable to open IO device for writing");
    }

    mPriv->output = output;

    PendingVariant *pv = new PendingVariant(
            fileTransferInterface(BypassInterfaceCheck)->AcceptFile(SocketAddressTypeIPv4,
                SocketAccessControlLocalhost, QDBusVariant(QVariant(QString())),
                offset),
            this);
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
    debug() << "Connected to host!";
    setConnected();

    doTransfer();
}

void IncomingFileTransferChannel::onSocketDisconnected()
{
    debug() << "Disconnected from host!";
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
        mPriv->output->write(data); // never fails
    }
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

    if (mPriv->output) {
        mPriv->output->close();
    }

    FileTransferChannel::setFinished();
}

} // Tp
