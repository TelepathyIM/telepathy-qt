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
namespace Tp
{

class TELEPATHY_QT4_EXPORT OutgoingStreamTubeChannel : public StreamTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(OutgoingStreamTubeChannel)

public:
    static OutgoingStreamTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~OutgoingStreamTubeChannel();

    PendingOperation *offerTcpSocket(const QHostAddress &address, quint16 port, const QVariantMap &parameters);
    PendingOperation *offerTcpSocket(QTcpServer *server, const QVariantMap &parameters);

    PendingOperation *offerUnixSocket(const QString &socketAddress, const QVariantMap &parameters,
            bool requireCredentials = false);
    PendingOperation *offerUnixSocket(QLocalServer *server, const QVariantMap &parameters,
            bool requireCredentials = false);

    QHash< uint, Tp::ContactPtr > contactsForConnections() const;

    QHash< QPair< QHostAddress, quint16 >, uint > connectionsForSourceAddresses() const;

protected:
    OutgoingStreamTubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = StreamTubeChannel::FeatureStreamTube);

private Q_SLOTS:
    void onNewRemoteConnection(uint, const QDBusVariant &, uint);
    void onContactsRetrieved(const QUuid &, const QList<Tp::ContactPtr> &);
    void onConnectionClosed(uint, const QString &, const QString &);

private:
    struct Private;
    friend struct Private;
    Private * const mPriv;

    friend struct PendingOpenTube;
};

}

#endif
