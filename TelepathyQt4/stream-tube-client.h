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

#ifndef _TelepathyQt4_stream_tube_client_h_HEADER_GUARD_
#define _TelepathyQt4_stream_tube_client_h_HEADER_GUARD_

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/RefCounted>
#include <TelepathyQt4/Types>

class QHostAddress;

namespace Tp
{

class PendingStreamTubeConnection;

// TODO: Turn the comments here into real doxymentation
class TELEPATHY_QT4_EXPORT StreamTubeClient : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamTubeClient)

    class TubeWrapper;

public:

    // A callback interface to allow using Port access control for incoming tubes,
    // because there we need to know the address bound to the app socket connecting to the CM
    class TcpSourceAddressGenerator
    {
    public:
        // Return qMakePair(QHostAddress::Any, 0) for "don't use Port AC for this tube", e.g.
        // because you want to make multiple connections through it
        //
        // The tube param can be used to extract the service, initiator contact and other useful
        // information for making the decision
        virtual QPair<QHostAddress /* source interface address */, quint16 /* source port */>
            nextSourceAddress(const AccountPtr &account, const IncomingStreamTubeChannelPtr &tube) const = 0;

    protected:
        ~TcpSourceAddressGenerator() {}
    };

    // The client name can be passed to allow service-activation. If service activation is not
    // desired, the name can be left out, in which case an unique name will be generated.

    // Different parameter order, because services is a mandatory param so it can't follow
    // the factory params which have default args
    static StreamTubeClientPtr create(
            const QStringList &services,
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
            const QStringList &services,
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false);

    static StreamTubeClientPtr create(
            const AccountManagerPtr &accountManager,
            const QStringList &services,
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false);

    static StreamTubeClientPtr create(
            const ClientRegistrarPtr &registrar,
            const QStringList &services,
            const QString &clientName = QString(),
            bool monitorConnections = false,
            bool bypassApproval = false);

    virtual ~StreamTubeClient();

    ClientRegistrarPtr registrar() const;
    QString clientName() const;
    bool monitorsConnections() const;

    bool acceptsAsTcp() const; // if setToAcceptAsTCP has been used last
    const TcpSourceAddressGenerator *generator() const; // warn and return NULL if !acceptsAsTCP
    bool acceptsAsUnix() const; // if setToAcceptAsUnix has been used last

    void setToAcceptAsTcp(const TcpSourceAddressGenerator *generator = 0); // 0 -> Localhost AC used
    void setToAcceptAsUnix(bool requireCredentials = false); // whether CM should req SCM_CREDENTIALS

    // This will always be populated
    QList<QPair<AccountPtr, IncomingStreamTubeChannelPtr> > tubes() const;
    QMap<QPair<AccountPtr, IncomingStreamTubeChannelPtr>, QSet<uint> > connections() const;

Q_SIGNALS:
    // These will always be emitted
    void tubeOffered(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube);
    void tubeClosed(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            const QString &error,
            const QString &message);

    // These will be emitted if a offered tube is accepted successfully, when setToAcceptAsTCP/Unix
    // has been called last
    void tubeAcceptedAsTcp(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            const QHostAddress &listenAddress,
            quint16 listenPort,
            const QHostAddress &sourceAddress, // these are populated with the source address the
            quint16 sourcePort);               // generator, if any, yieled for this tube
    void tubeAcceptedAsUnix(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            const QString &listenAddress,
            bool requiresCredentials, // this is the requireCredentials param unchanged
            uchar credentialByte);

    // These will be emitted if monitorConnections = true was passed to the create() method
    // Sadly, there is no other possible way to associate multiple connections through a single tube
    // with the actual sockets than connecting one socket at a time and waiting for newConnection()
    void newConnection(
            const Tp::AccountPtr &account,
            const Tp::IncomingStreamTubeChannelPtr &tube,
            uint connectionId);
    void connectionClosed(
            const Tp::AccountPtr &account,
            const Tp::OutgoingStreamTubeChannelPtr &tube,
            uint connectionId,
            const QString &error,
            const QString &message);

private Q_SLOTS:

    TELEPATHY_QT4_NO_EXPORT void onInvokedForTube(
            const Tp::AccountPtr &account,
            const Tp::StreamTubeChannelPtr &tube,
            const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &requestHints);
    TELEPATHY_QT4_NO_EXPORT void onAcceptFinished(TubeWrapper *, Tp::PendingStreamTubeConnection *);
    TELEPATHY_QT4_NO_EXPORT void onTubeInvalidated(Tp::DBusProxy *, const QString &, const QString &);

private:
    StreamTubeClient(
            const ClientRegistrarPtr &registrar,
            const QStringList &services,
            const QString &clientName,
            bool monitorConnections,
            bool bypassApproval);

    struct Private;
    Private *mPriv;
};

} // Tp

#endif
