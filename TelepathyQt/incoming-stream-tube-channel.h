/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_incoming_stream_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt_incoming_stream_tube_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/StreamTubeChannel>

#include <QtNetwork/QHostAddress>

class QIODevice;

namespace Tp
{

class PendingStreamTubeConnection;

class TP_QT_EXPORT IncomingStreamTubeChannel : public StreamTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(IncomingStreamTubeChannel)

public:
    static const Feature FeatureCore;

    static IncomingStreamTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~IncomingStreamTubeChannel();

    PendingStreamTubeConnection *acceptTubeAsTcpSocket();
    PendingStreamTubeConnection *acceptTubeAsTcpSocket(const QHostAddress &allowedAddress,
            quint16 allowedPort);
    PendingStreamTubeConnection *acceptTubeAsUnixSocket(bool requireCredentials = false);

protected:
    IncomingStreamTubeChannel(const ConnectionPtr &connection,
            const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = IncomingStreamTubeChannel::FeatureCore);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onNewLocalConnection(uint connectionId);

private:
    struct Private;
    friend class PendingStreamTubeConnection;
    friend struct Private;
    Private *mPriv;
};

}

#endif
