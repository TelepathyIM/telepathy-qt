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

#ifndef _TelepathyQt4_incoming_stream_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_incoming_stream_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/IncomingStreamTubeChannel>
#include "TelepathyQt4/stream-tube-channel-internal.h"

#include <QLocalSocket>

namespace Tp {

struct TELEPATHY_QT4_NO_EXPORT IncomingStreamTubeChannel::Private
{
    Private(IncomingStreamTubeChannel *parent);
    virtual ~Private();

    // Public object
    IncomingStreamTubeChannel *parent;

    // Properties
    QIODevice *device;

    // Private slots
    void onAcceptTubeFinished(Tp::PendingOperation* op);
    void onNewLocalConnection(uint connectionId);
};

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

}

#endif
