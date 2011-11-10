/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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


#include <TelepathyQt/PendingStreamTubeConnection>

#include "TelepathyQt/_gen/pending-stream-tube-connection.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/IncomingStreamTubeChannel>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/Types>
#include "TelepathyQt/types-internal.h"

namespace Tp
{

struct TP_QT_NO_EXPORT PendingStreamTubeConnection::Private
{
    Private(PendingStreamTubeConnection *parent);
    ~Private();

    // Public object
    PendingStreamTubeConnection *parent;

    IncomingStreamTubeChannelPtr tube;
    SocketAddressType type;
    QHostAddress hostAddress;
    quint16 port;
    QString socketPath;
    bool requiresCredentials;
    uchar credentialByte;
};

PendingStreamTubeConnection::Private::Private(PendingStreamTubeConnection *parent)
    : parent(parent),
      requiresCredentials(false),
      credentialByte(0)
{

}

PendingStreamTubeConnection::Private::~Private()
{
}

/**
 * \class PendingStreamTubeConnection
 * \ingroup clientchannel
 * \headerfile TelepathyQt/incoming-stream-tube-channel.h <TelepathyQt/PendingStreamTubeConnection>
 *
 * \brief The PendingStreamTubeConnection class represents an asynchronous
 * operation for accepting an incoming stream tube.
 *
 * See \ref async_model
 */

PendingStreamTubeConnection::PendingStreamTubeConnection(
        PendingVariant *acceptOperation,
        SocketAddressType type,
        bool requiresCredentials,
        uchar credentialByte,
        const IncomingStreamTubeChannelPtr &channel)
    : PendingOperation(channel),
      mPriv(new Private(this))
{
    mPriv->tube = channel;
    mPriv->type = type;
    mPriv->requiresCredentials = requiresCredentials;
    mPriv->credentialByte = credentialByte;

    /* keep track of channel invalidation */
    connect(channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));

    debug() << "Calling StreamTube.Accept";
    if (acceptOperation->isFinished()) {
        onAcceptFinished(acceptOperation);
    } else {
        connect(acceptOperation,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAcceptFinished(Tp::PendingOperation*)));
    }
}

PendingStreamTubeConnection::PendingStreamTubeConnection(
        const QString& errorName,
        const QString& errorMessage,
        const IncomingStreamTubeChannelPtr &channel)
    : PendingOperation(channel),
      mPriv(new PendingStreamTubeConnection::Private(this))
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
 * Return the type of the opened stream tube socket.
 *
 * \return The socket type as #SocketAddressType.
 * \see localAddress(), ipAddress()
 */
SocketAddressType PendingStreamTubeConnection::addressType() const
{
    return mPriv->tube->addressType();
}

/**
 * Return the local address of the opened stream tube socket.
 *
 * This method will return a meaningful value only if the incoming stream tube
 * was accepted as an Unix socket.
 *
 * \return Unix socket address if using an Unix socket,
 *         or an undefined value otherwise.
 * \see addressType(), ipAddress()
 */
QString PendingStreamTubeConnection::localAddress() const
{
    return mPriv->tube->localAddress();
}

/**
 * Return the IP address/port combination of the opened stream tube socket.
 *
 * This method will return a meaningful value only if the incoming stream tube
 * was accepted as a TCP socket.
 *
 * \return Pair of IP address as QHostAddress and port if using a TCP socket,
 *         or an undefined value otherwise.
 * \see addressType(), localAddress()
 */
QPair<QHostAddress, quint16> PendingStreamTubeConnection::ipAddress() const
{
    return mPriv->tube->ipAddress();
}

/**
 * Return whether sending a credential byte once connecting to the socket is required.
 *
 * Note that if this method returns \c true, one should send a SCM_CREDS or SCM_CREDENTIALS
 * and the credentialByte() once connected. If SCM_CREDS or SCM_CREDENTIALS cannot be sent,
 * the credentialByte() should still be sent.
 *
 * \return \c true if sending credentials is required, \c false otherwise.
 * \sa credentialByte()
 */
bool PendingStreamTubeConnection::requiresCredentials() const
{
    return mPriv->requiresCredentials;
}

/**
 * Return the credential byte to send once connecting to the socket if requiresCredentials() is \c
 * true.
 *
 * \return The credential byte.
 * \sa requiresCredentials()
 */
uchar PendingStreamTubeConnection::credentialByte() const
{
    return mPriv->credentialByte;
}

void PendingStreamTubeConnection::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);

    if (isFinished()) {
        return;
    }

    warning().nospace() << "StreamTube.Accept failed because channel was invalidated with " <<
        errorName << ": " << errorMessage;

    setFinishedWithError(errorName, errorMessage);
}

void PendingStreamTubeConnection::onAcceptFinished(PendingOperation *op)
{
    if (isFinished()) {
        return;
    }

    if (op->isError()) {
        warning().nospace() << "StreamTube.Accept failed with " <<
            op->errorName() << ": " << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "StreamTube.Accept returned successfully";

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    // Build the address
    if (mPriv->type == SocketAddressTypeIPv4) {
        SocketAddressIPv4 addr = qdbus_cast<SocketAddressIPv4>(pv->result());
        debug().nospace() << "Got address " << addr.address << ":" << addr.port;
        mPriv->hostAddress = QHostAddress(addr.address);
        mPriv->port = addr.port;
    } else if (mPriv->type == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr = qdbus_cast<SocketAddressIPv6>(pv->result());
        debug().nospace() << "Got address " << addr.address << ":" << addr.port;
        mPriv->hostAddress = QHostAddress(addr.address);
        mPriv->port = addr.port;
    } else {
        // Unix socket
        mPriv->socketPath = QLatin1String(qdbus_cast<QByteArray>(pv->result()));
        debug() << "Got socket " << mPriv->socketPath;
    }

    // It might have been already opened - check
    if (mPriv->tube->state() == TubeChannelStateOpen) {
        onTubeStateChanged(mPriv->tube->state());
    } else {
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(), SIGNAL(stateChanged(Tp::TubeChannelState)),
                this, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingStreamTubeConnection::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready, populate its properties
        if (mPriv->type == SocketAddressTypeIPv4 || mPriv->type == SocketAddressTypeIPv6) {
            mPriv->tube->setIpAddress(qMakePair<QHostAddress, quint16>(mPriv->hostAddress,
                    mPriv->port));
        } else {
            // Unix socket
            mPriv->tube->setLocalAddress(mPriv->socketPath);
        }

        // Mark the operation as finished
        setFinished();
    } else if (state != TubeChannelStateLocalPending) {
        // Something happened
        setFinishedWithError(QLatin1String("Connection refused"),
                QLatin1String("The connection to this tube was refused"));
    }
}

}
