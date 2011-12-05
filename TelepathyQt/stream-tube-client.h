/**
 * This file is part of TelepathyQt
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

#ifndef _TelepathyQt_stream_tube_client_h_HEADER_GUARD_
#define _TelepathyQt_stream_tube_client_h_HEADER_GUARD_

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/RefCounted>
#include <TelepathyQt/Types>

class QHostAddress;

namespace Tp
{

class PendingStreamTubeConnection;

class TP_QT_EXPORT StreamTubeClient : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamTubeClient)

    class TubeWrapper;

public:

    class TcpSourceAddressGenerator
    {
    public:
        virtual QPair<QHostAddress, quint16>
            nextSourceAddress(const AccountPtr &account, const IncomingStreamTubeChannelPtr &tube) = 0;

    protected:
        virtual ~TcpSourceAddressGenerator() {}
    };

    class Tube : public QPair<AccountPtr, IncomingStreamTubeChannelPtr>
    {
    public:
        Tube();
        Tube(const AccountPtr &account, const IncomingStreamTubeChannelPtr &channel);
        Tube(const Tube &other);
        ~Tube();

        bool isValid() const { return mPriv.constData() != 0; }

        Tube &operator=(const Tube &other);

        const AccountPtr &account() const
        {
            return first;
        }

        const IncomingStreamTubeChannelPtr &channel() const
        {
            return second;
        }

    private:
        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    static StreamTubeClientPtr create(
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false,
            const AccountFactoryConstPtr &accountFactory =
                AccountFactory::create(QDBusConnection::sessionBus()),
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());

    static StreamTubeClientPtr create(
            const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory,
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false);

    static StreamTubeClientPtr create(
            const AccountManagerPtr &accountManager,
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false);

    static StreamTubeClientPtr create(
            const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false);

    virtual ~StreamTubeClient();

    ClientRegistrarPtr registrar() const;
    QString clientName() const;
    bool isRegistered() const;
    bool monitorsConnections() const;

    bool acceptsAsTcp() const;
    TcpSourceAddressGenerator *tcpGenerator() const;
    bool acceptsAsUnix() const;

    void setToAcceptAsTcp(TcpSourceAddressGenerator *generator = 0);
    void setToAcceptAsUnix(bool requireCredentials = false);

    QList<Tube> tubes() const;
    QHash<Tube, QSet<uint> > connections() const;

Q_SIGNALS:
    void tubeOffered(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube);
    void tubeClosed(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            const QString &error,
            const QString &message);

    void tubeAcceptedAsTcp(
            const QHostAddress &listenAddress,
            quint16 listenPort,
            const QHostAddress &sourceAddress,
            quint16 sourcePort,
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube);
    void tubeAcceptedAsUnix(
            const QString &listenAddress,
            bool requiresCredentials,
            uchar credentialByte,
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube);

    void newConnection(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            uint connectionId);
    void connectionClosed(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            uint connectionId,
            const QString &error,
            const QString &message);

private Q_SLOTS:

    TP_QT_NO_EXPORT void onInvokedForTube(
            const Tp::AccountPtr &account,
            const Tp::StreamTubeChannelPtr &tube,
            const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &requestHints);

    TP_QT_NO_EXPORT void onAcceptFinished(TubeWrapper *, Tp::PendingStreamTubeConnection *);
    TP_QT_NO_EXPORT void onTubeInvalidated(Tp::DBusProxy *, const QString &, const QString &);

    TP_QT_NO_EXPORT void onNewConnection(
            TubeWrapper *wrapper,
            uint conn);
    TP_QT_NO_EXPORT void onConnectionClosed(
            TubeWrapper *wrapper,
            uint conn,
            const QString &error,
            const QString &message);

private:
    TP_QT_NO_EXPORT StreamTubeClient(
            const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices,
            const QString &clientName,
            bool monitorConnections,
            bool bypassApproval);

    struct Private;
    Private *mPriv;
};

} // Tp

#endif
