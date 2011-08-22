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

#ifndef _TelepathyQt4_stream_tube_server_h_HEADER_GUARD_
#define _TelepathyQt4_stream_tube_server_h_HEADER_GUARD_

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/RefCounted>
#include <TelepathyQt4/Types>

class QHostAddress;
class QTcpServer;

namespace Tp
{

// TODO: Turn the comments here into real doxymentation
class TELEPATHY_QT4_EXPORT StreamTubeServer : public QObject, public RefCounted
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
                    const ChannelRequestHints &hints) const = 0;

    protected:
        virtual ~ParametersGenerator() {}
    };

    // The client name can be passed to allow service-activation. If service activation is not
    // desired, the name can be left out, in which case an unique name will be generated.

    // Different parameter order, because name and service are mandatory params so they can't follow
    // the factory params which have default args
    static StreamTubeServerPtr create(
            const QStringList &services,
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
            const QStringList &services,
            const QString &clientName = QString(),
            bool monitorConnections = false);

    static StreamTubeServerPtr create(
            const AccountManagerPtr &accountManager,
            const QStringList &services,
            const QString &clientName = QString(),
            bool monitorConnections = false);

    static StreamTubeServerPtr create(
            const ClientRegistrarPtr &registrar,
            const QStringList &services,
            const QString &clientName = QString(),
            bool monitorConnections = false);

    virtual ~StreamTubeServer();

    ClientRegistrarPtr registrar() const;
    QString clientName() const;
    bool isRegistered() const;
    bool monitorsConnections() const;

    // Recovery getters for the setters below
    QPair<QHostAddress, quint16> exportedTcpSocketAddress() const;
    QVariantMap exportedParameters() const;

    // These change the exported (Offer'ed) service to the given one for all future handled tubes
    // The first call to one of these actually registers the Handler
    //
    // The exported socket can be changed an arbitrary amount of times, as many networked applications
    // have a runtime setting for their listen port. This will of course only affect the handling of
    // future tubes.
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
            const ParametersGenerator *generator);
    void exportTcpSocket(
            const QTcpServer *server,
            const ParametersGenerator *generator);

    // TODO: Add Unix sockets if needed (are there other common services one might want to export
    // listening on Unix sockets and not necessarily on TCP than X11 and perhaps CUPS?)

    // This will always be populated
    QList<QPair<AccountPtr, OutgoingStreamTubeChannelPtr> > tubes() const;

    // This will be populated if monitorConnections = true and a TCP socket has been exported
    //
    // If the CM doesn't support Port AC (TCP), an invalid source address will be used as a key,
    // with multiple connections as values using insertMulti
    //
    // Given how complex the map becomes to relay all the necessary information, should we perhaps
    // leave this out? Then we wouldn't have full state-recoverability, though, and clients would
    // likely implement similar monster maps themselves.
    //
    // Including the tube channels themselves would complicate this even more, and wouldn't really
    // even help in any use case that I can think of.
    QHash<QPair<QHostAddress /* sourceAddress */, quint16 /* sourcePort */>, QPair<AccountPtr, ContactPtr> > tcpConnections() const;

Q_SIGNALS:

    // These will always be emitted
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

    // These will be emitted if monitorConnections = true was passed to the create() method
    // and a TCP socket is exported
    //
    // If the CM doesn't support Port access control, sourceAddress and sourcePort won't be
    // informative. At least Gabble supports it, though.
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
    TELEPATHY_QT4_NO_EXPORT void onInvokedForTube(
            const Tp::AccountPtr &account,
            const Tp::StreamTubeChannelPtr &tube,
            const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &requestHints);

    TELEPATHY_QT4_NO_EXPORT void onOfferFinished(
            TubeWrapper *wrapper,
            Tp::PendingOperation *op);
    TELEPATHY_QT4_NO_EXPORT void onTubeInvalidated(
            Tp::DBusProxy *proxy,
            const QString &error,
            const QString &message);

    TELEPATHY_QT4_NO_EXPORT void onNewConnection(
            TubeWrapper *wrapper,
            uint conn);
    TELEPATHY_QT4_NO_EXPORT void onConnectionClosed(
            TubeWrapper *wrapper,
            uint conn,
            const QString &error,
            const QString &message);

private:
    StreamTubeServer(
            const ClientRegistrarPtr &registrar,
            const QStringList &services,
            const QString &clientName,
            bool monitorConnections);

    struct Private;
    Private *mPriv;
};

} // Tp

#endif
