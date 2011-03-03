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

#ifndef _TelepathyQt4_incoming_stream_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt4_incoming_stream_tube_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/StreamTubeChannel>

#include <QtNetwork/QHostAddress>

class QIODevice;

namespace Tp
{

class PendingStreamTubeConnection;

class TELEPATHY_QT4_EXPORT IncomingStreamTubeChannel : public StreamTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(IncomingStreamTubeChannel)

public:
    static IncomingStreamTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~IncomingStreamTubeChannel();

    PendingStreamTubeConnection *acceptTubeAsTcpSocket();
    PendingStreamTubeConnection *acceptTubeAsTcpSocket(const QHostAddress &allowedAddress,
            quint16 allowedPort);
    PendingStreamTubeConnection *acceptTubeAsUnixSocket(bool requireCredentials = false);

    QIODevice *device() const;

protected:
    IncomingStreamTubeChannel(const ConnectionPtr &connection,
            const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = StreamTubeChannel::FeatureStreamTube);

private Q_SLOTS:
    void onAcceptTubeFinished(Tp::PendingOperation *op);
    void onNewLocalConnection(uint connectionId);

private:
    struct Private;
    friend class PendingStreamTubeConnection;
    friend struct Private;
    Private *mPriv;
};

}

#endif
