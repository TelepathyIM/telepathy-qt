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

#ifndef _TelepathyQt_stream_tube_server_h_HEADER_GUARD_
#define _TelepathyQt_stream_tube_server_h_HEADER_GUARD_

#include <QPair>
#include <QSharedDataPointer>

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/RefCounted>
#include <TelepathyQt/Types>

class QHostAddress;
class QTcpServer;

namespace Tp
{

class TP_QT_EXPORT StreamTubeServer : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamTubeServer)

    class TubeWrapper;

public:

    class ParametersGenerator
    {
    public:
        virtual QVariantMap
            nextParameters(const AccountPtr &account, const OutgoingStreamTubeChannelPtr &tube,
                    const ChannelRequestHints &hints) = 0;

    protected:
        virtual ~ParametersGenerator() {}
    };

    class RemoteContact : public QPair<AccountPtr, ContactPtr>
    {
    public:
        RemoteContact();
        RemoteContact(const AccountPtr &account, const ContactPtr &contact);
        RemoteContact(const RemoteContact &other);
        ~RemoteContact();

        bool isValid() const { return mPriv.constData() != 0; }

        RemoteContact &operator=(const RemoteContact &other);

        const AccountPtr &account() const
        {
            return first;
        }

        const ContactPtr &contact() const
        {
            return second;
        }

    private:
        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    class Tube : public QPair<AccountPtr, OutgoingStreamTubeChannelPtr>
    {
    public:
        Tube();
        Tube(const AccountPtr &account, const OutgoingStreamTubeChannelPtr &channel);
        Tube(const Tube &other);
        ~Tube();

        bool isValid() const { return mPriv.constData() != 0; }

        Tube &operator=(const Tube &other);

        const AccountPtr &account() const
        {
            return first;
        }

        const OutgoingStreamTubeChannelPtr &channel() const
        {
            return second;
        }

    private:
        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    static StreamTubeServerPtr create(
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false,
            const AccountFactoryConstPtr &accountFactory =
                AccountFactory::create(QDBusConnection::sessionBus()),
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());

    static StreamTubeServerPtr create(
            const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory,
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false);

    static StreamTubeServerPtr create(
            const AccountManagerPtr &accountManager,
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false);

    static StreamTubeServerPtr create(
            const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices = QStringList(),
            const QString &clientName = QString(),
            bool monitorConnections = false);

    virtual ~StreamTubeServer();

    ClientRegistrarPtr registrar() const;
    QString clientName() const;
    bool isRegistered() const;
    bool monitorsConnections() const;

    QPair<QHostAddress, quint16> exportedTcpSocketAddress() const;
    QVariantMap exportedParameters() const;

    void exportTcpSocket(
            const QHostAddress &address,
            quint16 port,
            const QVariantMap &parameters = QVariantMap());
    void exportTcpSocket(
            const QTcpServer *server,
            const QVariantMap &parameters = QVariantMap());

    void exportTcpSocket(
            const QHostAddress &address,
            quint16 port,
            ParametersGenerator *generator);
    void exportTcpSocket(
            const QTcpServer *server,
            ParametersGenerator *generator);

    QList<Tube> tubes() const;

    QHash<QPair<QHostAddress, quint16>, RemoteContact> tcpConnections() const;

Q_SIGNALS:

    void tubeRequested(
            const Tp::AccountPtr &account,
            const Tp::OutgoingStreamTubeChannelPtr &tube,
            const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &hints);
    void tubeClosed(
            const Tp::AccountPtr &account,
            const Tp::OutgoingStreamTubeChannelPtr &tube,
            const QString &error,
            const QString &message);

    void newTcpConnection(
            const QHostAddress &sourceAddress,
            quint16 sourcePort,
            const Tp::AccountPtr &account,
            const Tp::ContactPtr &contact,
            const Tp::OutgoingStreamTubeChannelPtr &tube);
    void tcpConnectionClosed(
            const QHostAddress &sourceAddress,
            quint16 sourcePort,
            const Tp::AccountPtr &account,
            const Tp::ContactPtr &contact,
            const QString &error,
            const QString &message,
            const Tp::OutgoingStreamTubeChannelPtr &tube);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onInvokedForTube(
            const Tp::AccountPtr &account,
            const Tp::StreamTubeChannelPtr &tube,
            const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &requestHints);

    TP_QT_NO_EXPORT void onOfferFinished(
            TubeWrapper *wrapper,
            Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onTubeInvalidated(
            Tp::DBusProxy *proxy,
            const QString &error,
            const QString &message);

    TP_QT_NO_EXPORT void onNewConnection(
            TubeWrapper *wrapper,
            uint conn);
    TP_QT_NO_EXPORT void onConnectionClosed(
            TubeWrapper *wrapper,
            uint conn,
            const QString &error,
            const QString &message);

private:
    TP_QT_NO_EXPORT StreamTubeServer(
            const ClientRegistrarPtr &registrar,
            const QStringList &p2pServices,
            const QStringList &roomServices,
            const QString &clientName,
            bool monitorConnections);

    struct Private;
    Private *mPriv;
};

} // Tp

#endif
