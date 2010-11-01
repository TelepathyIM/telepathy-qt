/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009-2010 Nokia Corporation
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

#ifndef _TelepathyQt4_client_registrar_h_HEADER_GUARD_
#define _TelepathyQt4_client_registrar_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/RefCounted>
#include <TelepathyQt4/Types>

#include <QDBusConnection>
#include <QObject>
#include <QString>

namespace Tp
{

class TELEPATHY_QT4_EXPORT ClientRegistrar : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ClientRegistrar)

public:
    static ClientRegistrarPtr create(
            const QDBusConnection &bus = QDBusConnection::sessionBus());

    static ClientRegistrarPtr create(
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());
    static ClientRegistrarPtr create(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    static ClientRegistrarPtr create(const AccountManagerPtr &accountManager);

    ~ClientRegistrar();

    QDBusConnection dbusConnection() const;

    AccountFactoryConstPtr accountFactory() const;
    ConnectionFactoryConstPtr connectionFactory() const;
    ChannelFactoryConstPtr channelFactory() const;
    ContactFactoryConstPtr contactFactory() const;

    QList<AbstractClientPtr> registeredClients() const;
    bool registerClient(const AbstractClientPtr &client,
            const QString &clientName, bool unique = false);
    bool unregisterClient(const AbstractClientPtr &client);
    void unregisterClients();

private:
    TELEPATHY_QT4_DEPRECATED ClientRegistrar(const QDBusConnection &bus);
    ClientRegistrar(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    struct Private;
    friend struct Private;
    Private *mPriv;

    static QHash<QPair<QString, QString>, ClientRegistrar *> registrarForConnection;
};

} // Tp

#endif
