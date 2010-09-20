/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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


#include <TelepathyQt4/PendingStreamTubeConnection>

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

#include <QLocalSocket>
#include <QTcpSocket>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingStreamTubeConnection::Private
{
    Private(PendingStreamTubeConnection *parent);
    virtual ~Private();

    // Public object
    PendingStreamTubeConnection *parent;

    IncomingStreamTubeChannelPtr tube;
    SocketAddressType type;
    QHostAddress hostAddress;
    quint16 port;
    QString socketPath;

    QIODevice *device;

    // Private slots
    void onAcceptFinished(Tp::PendingOperation* op);
    void onTubeStateChanged(Tp::TubeChannelState state);
    void onDeviceConnected();
    void onAbstractSocketError(QAbstractSocket::SocketError error);
    void onLocalSocketError(QLocalSocket::LocalSocketError error);
};

PendingStreamTubeConnection::Private::Private(PendingStreamTubeConnection* parent)
    : parent(parent)
    , device(0)
{

}

PendingStreamTubeConnection::Private::~Private()
{

}

PendingStreamTubeConnection::PendingStreamTubeConnection(
        PendingVariant* acceptOperation,
        SocketAddressType type,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = IncomingStreamTubeChannelPtr::dynamicCast(object);
    mPriv->type = type;

    // Connect the pending void
    connect(acceptOperation, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAcceptFinished(Tp::PendingOperation*)));
}

/**
 * \class PendingStreamTubeConnection
 * \headerfile TelepathyQt4/incoming-stream-tube-channel.h <TelepathyQt4/IncomingStreamTubeChannel>
 *
 * \brief A pending operation for accepting a stream tube
 *
 * This class represents an asynchronous operation for accepting a stream tube.
 * When the operation is finished, you can access the resulting device
 * through #device().
 * Otherwise, you can access the bare address through either #tcpAddress or #localAddress.
 */

PendingStreamTubeConnection::PendingStreamTubeConnection(
        const QString& errorName,
        const QString& errorMessage,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
    , mPriv(new PendingStreamTubeConnection::Private(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingStreamTubeConnection::~PendingStreamTubeConnection()
{
    delete mPriv;
}

/**
 * \return The opened QIODevice if the operation is finished and successful, 0 otherwise.
 */
QIODevice* PendingStreamTubeConnection::device()
{
    return mPriv->device;
}

void PendingStreamTubeConnection::Private::onAcceptFinished(PendingOperation* op)
{
    if (op->isError()) {
        parent->setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Accept tube finished successfully";

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    // Build the address
    if (type == SocketAddressTypeIPv4) {
        SocketAddressIPv4 addr = qdbus_cast<SocketAddressIPv4>(pv->result());
        debug().nospace() << "Got address " << addr.address <<
                ":" << addr.port;
        hostAddress = QHostAddress(addr.address);
        port = addr.port;
    } else if (type == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr = qdbus_cast<SocketAddressIPv6>(pv->result());
        debug().nospace() << "Got address " << addr.address <<
                ":" << addr.port;
        hostAddress = QHostAddress(addr.address);
        port = addr.port;
    } else {
        // Unix socket
        socketPath = QLatin1String(qdbus_cast<QByteArray>(pv->result()));
        debug() << "Got socket " << socketPath;
    }

    // It might have been already opened - check
    if (tube->tubeState() == TubeChannelStateOpen) {
        onTubeStateChanged(tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        parent->connect(tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                        parent, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingStreamTubeConnection::Private::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready
        // Build the IO device
        if (type == SocketAddressTypeIPv4 || type == SocketAddressTypeIPv6) {
            tube->setIpAddress(qMakePair< QHostAddress, quint16 >(hostAddress, port));
            debug() << "Building a QTcpSocket " << hostAddress << port;

            QTcpSocket *socket = new QTcpSocket(tube.data());
            socket->connectToHost(hostAddress, port);
            device = socket;

            parent->connect(socket, SIGNAL(connected()),
                            parent, SLOT(onDeviceConnected()));
            parent->connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                            parent, SLOT(onAbstractSocketError(QAbstractSocket::SocketError)));
            debug() << "QTcpSocket built";
        } else {
            // Unix socket
            tube->setLocalAddress(socketPath);

            QLocalSocket *socket = new QLocalSocket(tube.data());
            socket->connectToServer(socketPath);
            device = socket;

            // The local socket might already be connected
            if (socket->state() == QLocalSocket::ConnectedState) {
                onDeviceConnected();
            } else {
                parent->connect(socket, SIGNAL(connected()),
                                parent, SLOT(onDeviceConnected()));
                parent->connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
                                parent, SLOT(onLocalSocketError(QLocalSocket::LocalSocketError)));
            }
        }
    } else if (state != TubeChannelStateLocalPending) {
        // Something happened
        parent->setFinishedWithError(QLatin1String("Connection refused"),
                      QLatin1String("The connection to this tube was refused"));
    }
}

void PendingStreamTubeConnection::Private::onAbstractSocketError(QAbstractSocket::SocketError error)
{
    // TODO: Use error to provide more detailed error messages
    Q_UNUSED(error)
    // Failure
    device = 0;
    parent->setFinishedWithError(QLatin1String("Error while creating TCP socket"),
                      QLatin1String("Could not connect to the new socket"));
}

void PendingStreamTubeConnection::Private::onLocalSocketError(QLocalSocket::LocalSocketError error)
{
    // TODO: Use error to provide more detailed error messages
    Q_UNUSED(error)
    // Failure
    device = 0;
    parent->setFinishedWithError(QLatin1String("Error while creating Local socket"),
                      QLatin1String("Could not connect to the new socket"));
}

void PendingStreamTubeConnection::Private::onDeviceConnected()
{
    // Our IODevice is ready! Let's just set finished
    debug() << "Device connected!";
    parent->setFinished();
}


/**
 * \return The type of socket this PendingStreamTubeConnection has created
 *
 * \note This function will return a valid value only after the operation has been finished successfully
 *
 * \see localAddress
 * \see tcpAddress
 */
SocketAddressType PendingStreamTubeConnection::addressType() const
{
    return mPriv->tube->addressType();
}


/**
 * \return The local address obtained from this PendingStreamTubeConnection as a QByteArray,
 *          if the connection has been estabilished through a SocketAddressTypeUnix or
 *          a SocketAddressTypeAbstractUnix.
 *
 * \note This function will return a valid value only after the operation has been finished successfully
 *
 * \see addressType
 */
QString PendingStreamTubeConnection::localAddress() const
{
    return mPriv->tube->localAddress();
}


/**
 * \return The IP address and port obtained from this PendingStreamTubeConnection as a QHostAddress,
 *          if the connection has been estabilished through a SocketAddressTypeIpv4 or
 *          a SocketAddressTypeIPv6.
 *
 * \note This function will return a valid value only after the operation has been finished successfully
 *
 * \see addressType
 */
QPair< QHostAddress, quint16 > PendingStreamTubeConnection::ipAddress() const
{
    return mPriv->tube->ipAddress();
}

}

#include "TelepathyQt4/_gen/pending-stream-tube-connection.moc.hpp"
