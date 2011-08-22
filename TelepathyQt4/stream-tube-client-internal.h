/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt4/Account>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/StreamTubeClient>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT StreamTubeClient::TubeWrapper :
                public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TubeWrapper)

public:
    TubeWrapper(const AccountPtr &acc, const IncomingStreamTubeChannelPtr &tube, const QHostAddress &sourceAddress, quint16 sourcePort);
    TubeWrapper(const AccountPtr &acc, const IncomingStreamTubeChannelPtr &tube, bool requireCredentials);
    ~TubeWrapper() { }

    AccountPtr mAcc;
    IncomingStreamTubeChannelPtr mTube;
    QHostAddress sourceAddress;
    quint16 sourcePort;

Q_SIGNALS:
    void acceptFinished(TubeWrapper *wrapper, Tp::PendingStreamTubeConnection *conn);
    void newConnection(TubeWrapper *wrapper, uint conn);
    void connectionClosed(TubeWrapper *wrapper, uint conn, const QString &error,
            const QString &message);

private Q_SLOTS:
    void onTubeAccepted(Tp::PendingOperation *);
    void onNewConnection(uint);
    void onConnectionClosed(uint, const QString &, const QString &);
};

} // Tp
