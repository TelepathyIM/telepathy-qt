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

#ifndef _TelepathyQt_pending_stream_tube_connection_h_HEADER_GUARD_
#define _TelepathyQt_pending_stream_tube_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

#include <QPair>

class QHostAddress;

namespace Tp
{

class PendingVariant;
class IncomingStreamTubeChannel;

class TP_QT_EXPORT PendingStreamTubeConnection : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingStreamTubeConnection)

public:
    virtual ~PendingStreamTubeConnection();

    SocketAddressType addressType() const;

    QPair<QHostAddress, quint16> ipAddress() const;
    QString localAddress() const;

    bool requiresCredentials() const;
    uchar credentialByte() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onChannelInvalidated(Tp::DBusProxy *proxy,
            const QString &errorName, const QString &errorMessage);
    TP_QT_NO_EXPORT void onAcceptFinished(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onTubeStateChanged(Tp::TubeChannelState state);

private:
    TP_QT_NO_EXPORT PendingStreamTubeConnection(PendingVariant *acceptOperation,
            SocketAddressType type, bool requiresCredentials, uchar credentialByte,
            const IncomingStreamTubeChannelPtr &channel);
    TP_QT_NO_EXPORT PendingStreamTubeConnection(
            const QString &errorName, const QString &errorMessage,
            const IncomingStreamTubeChannelPtr &channel);

    struct Private;
    friend class IncomingStreamTubeChannel;
    friend struct Private;
    Private *mPriv;
};

}

#endif // TP_PENDING_STREAM_TUBE_CONNECTION_H
