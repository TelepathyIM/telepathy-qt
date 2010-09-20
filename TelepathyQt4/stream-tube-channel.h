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

#ifndef _TelepathyQt4_stream_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt4_stream_tube_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/TubeChannel>

class QHostAddress;
namespace Tp {

class TELEPATHY_QT4_EXPORT StreamTubeChannel : public TubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamTubeChannel)

public:
    static const Feature FeatureStreamTube;
    static const Feature FeatureConnectionMonitoring;

    static StreamTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~StreamTubeChannel();

    QString service() const;

    bool supportsIPv4SocketsOnLocalhost() const;
    bool supportsIPv4SocketsWithSpecifiedAddress() const;

    bool supportsIPv6SocketsOnLocalhost() const;
    bool supportsIPv6SocketsWithSpecifiedAddress() const;

    bool supportsUnixSocketsOnLocalhost() const;
    bool supportsUnixSocketsWithCredentials() const;

    bool supportsAbstractUnixSocketsOnLocalhost() const;
    bool supportsAbstractUnixSocketsWithCredentials() const;

    UIntList connections() const;

    SocketAddressType addressType() const;

    QPair< QHostAddress, quint16 > ipAddress() const;
    QString localAddress() const;

Q_SIGNALS:
    void newConnection(uint connectionId);
    void connectionClosed(uint connectionId, const QString &error, const QString &message);

protected:
    StreamTubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = StreamTubeChannel::FeatureStreamTube);

    void setBaseTubeType(uint type);
    void setAddressType(SocketAddressType type);
    void setConnections(UIntList connections);
    void setIpAddress(const QPair< QHostAddress, quint16 > &address);
    void setLocalAddress(const QString &address);

private Q_SLOTS:
    void gotStreamTubeProperties(QDBusPendingCallWatcher *);
    void onConnectionClosed(uint,const QString &,const QString &);

private:
    struct Private;
    friend struct Private;

    Private * const mPriv;

};

}

#endif
