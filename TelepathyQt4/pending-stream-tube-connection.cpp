/**
 * This file is part of TelepathyQt4
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


#include <TelepathyQt4/PendingStreamTubeConnection>

#include "TelepathyQt4/_gen/pending-stream-tube-connection.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Types>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingStreamTubeConnection::Private
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
    bool requireCredentials;
    uchar credentialByte;
};

PendingStreamTubeConnection::Private::Private(PendingStreamTubeConnection *parent)
    : parent(parent),
      requireCredentials(false),
      credentialByte(0)
{

}

PendingStreamTubeConnection::Private::~Private()
{

}

PendingStreamTubeConnection::PendingStreamTubeConnection(
        PendingVariant *acceptOperation,
        SocketAddressType type,
        bool requireCredentials,
        uchar credentialByte,
        const IncomingStreamTubeChannelPtr &object)
    : PendingOperation(object),
      mPriv(new Private(this))
{
    mPriv->tube = object;
    mPriv->type = type;
    mPriv->requireCredentials = requireCredentials;
    mPriv->credentialByte = credentialByte;
    // Connect the pending void
    connect(acceptOperation, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onAcceptFinished(Tp::PendingOperation*)));
}

/**
 * \class PendingStreamTubeConnection
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/incoming-stream-tube-channel.h <TelepathyQt4/PendingStreamTubeConnection>
 *
 * \brief The PendingStreamTubeConnection class represents an asynchronous
 * operation for accepting a stream tube.
 *
 * When the operation is finished, you can access the resulting device
 * through device().
 * Otherwise, you can access the bare address through either tcpAddress() or
 * localAddress().
 */

PendingStreamTubeConnection::PendingStreamTubeConnection(
        const QString& errorName,
        const QString& errorMessage,
        const IncomingStreamTubeChannelPtr &object)
    : PendingOperation(object),
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
 * This method returns the address type of the opened socket.
 *
 * Calling this method when the operation has not been completed or has failed, will cause it
 * to return an unmeaningful value.
 *
 * \return The type of socket this PendingStreamTubeConnection has created
 *
 * \note This function will return a valid value only after the operation has been
 *       finished successfully.
 *
 * \see localAddress
 * \see tcpAddress
 */
SocketAddressType PendingStreamTubeConnection::addressType() const
{
    return mPriv->tube->addressType();
}

/**
 * This method returns the local address of the opened socket.
 *
 * Calling this method when the operation has not been completed or has failed, will cause it
 * to return an unmeaningful value. The same will happen if the socket which has been opened has a
 * different type from SocketAddressTypeUnix or SocketAddressTypeAbstractUnix. Use #ipAddress if
 * that is the case.
 *
 * \return The local address obtained from this PendingStreamTubeConnection as a QString,
 *         if the connection has been estabilished through a SocketAddressTypeUnix or
 *         a SocketAddressTypeAbstractUnix.
 *
 * \note This function will return a valid value only after the operation has been
 *       finished successfully.
 *
 * \see addressType
 */
QString PendingStreamTubeConnection::localAddress() const
{
    return mPriv->tube->localAddress();
}

/**
 * This method returns the IP address of the opened socket.
 *
 * Calling this method when the operation has not been completed or has failed, will cause it
 * to return an unmeaningful value. The same will happen if the socket which has been opened has a
 * different type from SocketAddressTypeIpv4 or SocketAddressTypeIPv6. Use #localAddress if
 * that is the case.
 *
 * \return The IP address and port obtained from this PendingStreamTubeConnection as a QHostAddress,
 *         if the connection has been estabilished through a SocketAddressTypeIpv4 or
 *         a SocketAddressTypeIPv6.
 *
 * \note This function will return a valid value only after the operation has been
 *       finished successfully.
 *
 * \see addressType
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
bool PendingStreamTubeConnection::requireCredentials() const
{
    return mPriv->requireCredentials;
}

/**
 * Return the credential byte to send once connecting to the socket if requireCredentials() is \c
 * true.
 *
 * \return The credential byte.
 * \sa requireCredentials()
 */
uchar PendingStreamTubeConnection::credentialByte() const
{
    return mPriv->credentialByte;
}

void PendingStreamTubeConnection::onAcceptFinished(PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Accept tube finished successfully";

    PendingVariant *pv = qobject_cast<PendingVariant *>(op);
    // Build the address
    if (mPriv->type == SocketAddressTypeIPv4) {
        SocketAddressIPv4 addr = qdbus_cast<SocketAddressIPv4>(pv->result());
        debug().nospace() << "Got address " << addr.address <<
                ":" << addr.port;
        mPriv->hostAddress = QHostAddress(addr.address);
        mPriv->port = addr.port;
    } else if (mPriv->type == SocketAddressTypeIPv6) {
        SocketAddressIPv6 addr = qdbus_cast<SocketAddressIPv6>(pv->result());
        debug().nospace() << "Got address " << addr.address <<
                ":" << addr.port;
        mPriv->hostAddress = QHostAddress(addr.address);
        mPriv->port = addr.port;
    } else {
        // Unix socket
        mPriv->socketPath = QLatin1String(qdbus_cast<QByteArray>(pv->result()));
        debug() << "Got socket " << mPriv->socketPath;
    }

    // It might have been already opened - check
    if (mPriv->tube->tubeState() == TubeChannelStateOpen) {
        onTubeStateChanged(mPriv->tube->tubeState());
    } else {
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
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
