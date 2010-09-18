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

#ifndef _TelepathyQt4_stream_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_stream_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/StreamTubeChannel>
#include "TelepathyQt4/tube-channel-internal.h"

#include <QtNetwork/QHostAddress>

namespace Tp {

struct TELEPATHY_QT4_NO_EXPORT StreamTubeChannel::Private
{
    enum BaseTubeType {
        NoKnownType = 0,
        OutgoingTubeType = 1,
        IncomingTubeType = 2
    };

    Private(StreamTubeChannel *parent);
    virtual ~Private();

    virtual void init();

    void extractStreamTubeProperties(const QVariantMap &props);

    static void introspectConnectionMonitoring(Private *self);
    static void introspectStreamTube(Private *self);

    UIntList connections;

    ReadinessHelper *readinessHelper;

    StreamTubeChannel *parent;

    // Properties
    SupportedSocketMap socketTypes;
    QString serviceName;

    BaseTubeType baseType;

    QPair< QHostAddress, quint16 > ipAddress;
    QString unixAddress;
    SocketAddressType addressType;

    // Private slots
    void gotStreamTubeProperties(QDBusPendingCallWatcher *watcher);
    void onConnectionClosed(uint connectionId, const QString &error, const QString &message);

    friend class IncomingStreamTubeChannel;
    friend class OutgoingStreamTubeChannel;
};

}

#endif
