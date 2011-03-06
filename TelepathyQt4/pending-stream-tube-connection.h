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

#ifndef _TelepathyQt4_pending_stream_tube_connection_h_HEADER_GUARD_
#define _TelepathyQt4_pending_stream_tube_connection_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Types>

#include <QPair>

class QHostAddress;

namespace Tp
{

class PendingVariant;
class IncomingStreamTubeChannel;

class TELEPATHY_QT4_EXPORT PendingStreamTubeConnection : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingStreamTubeConnection)

public:
    virtual ~PendingStreamTubeConnection();

    SocketAddressType addressType() const;

    QPair<QHostAddress, quint16> ipAddress() const;
    QString localAddress() const;

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onAcceptFinished(Tp::PendingOperation *op);
    TELEPATHY_QT4_NO_EXPORT void onTubeStateChanged(Tp::TubeChannelState state);

private:
    PendingStreamTubeConnection(PendingVariant *variant, SocketAddressType type,
            const IncomingStreamTubeChannelPtr &object);
    PendingStreamTubeConnection(const QString &errorName, const QString &errorMessage,
            const IncomingStreamTubeChannelPtr &object);

    struct Private;
    friend class IncomingStreamTubeChannel;
    friend struct Private;
    Private *mPriv;
};

}

#endif // TP_PENDING_STREAM_TUBE_CONNECTION_H
