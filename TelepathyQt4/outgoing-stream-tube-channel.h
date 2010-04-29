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

#ifndef _TelepathyQt4_outgoing_stream_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt4_outgoing_stream_tube_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/StreamTubeChannel>
#include <TelepathyQt4/PendingOperation>

class QHostAddress;
class QTcpServer;
class QLocalServer;
namespace Tp {

class OutgoingStreamTubeChannelPrivate;
class TELEPATHY_QT4_EXPORT OutgoingStreamTubeChannel : public StreamTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(OutgoingStreamTubeChannel)
    Q_DECLARE_PRIVATE(OutgoingStreamTubeChannel)

public:
//     static const Feature FeatureCore;

    static OutgoingStreamTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~OutgoingStreamTubeChannel();

    PendingOperation *offerTube(const QHostAddress &address, quint16 port, const QVariantMap &parameters,
                                SocketAccessControl accessControl = SocketAccessControlLocalhost);
    PendingOperation *offerTube(const QByteArray &socketAddress, const QVariantMap &parameters,
                                SocketAccessControl accessControl = SocketAccessControlLocalhost);
    PendingOperation *offerTube(QTcpServer *server, const QVariantMap &parameters,
                                SocketAccessControl accessControl = SocketAccessControlLocalhost);
    PendingOperation *offerTube(QLocalServer *server, const QVariantMap &parameters,
                                SocketAccessControl accessControl = SocketAccessControlLocalhost);

protected:
    OutgoingStreamTubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = StreamTubeChannel::FeatureStreamTube);

    virtual void connectNotify(const char* signal);

Q_SIGNALS:
    void newRemoteConnection(const Tp::ContactPtr &contact, const QVariant &parameter, uint connectionId);

};

}

#endif
