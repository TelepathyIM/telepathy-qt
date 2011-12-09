/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt_connection_manager_h_HEADER_GUARD_
#define _TelepathyQt_connection_manager_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-connection-manager.h>

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/DBus>
#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/OptionalInterfaceFactory>
#include <TelepathyQt/ProtocolInfo>
#include <TelepathyQt/ProtocolParameter>
#include <TelepathyQt/ReadinessHelper>
#include <TelepathyQt/SharedPtr>
#include <TelepathyQt/Types>

namespace Tp
{

class ConnectionManagerLowlevel;
class PendingConnection;
class PendingStringList;

class TP_QT_EXPORT ConnectionManager : public StatelessDBusProxy,
                public OptionalInterfaceFactory<ConnectionManager>
{
    Q_OBJECT
    Q_DISABLE_COPY(ConnectionManager)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QStringList supportedProtocols READ supportedProtocols)
    Q_PROPERTY(ProtocolInfoList protocols READ protocols)

public:
    static const Feature FeatureCore;

    static ConnectionManagerPtr create(const QDBusConnection &bus,
            const QString &name);
    static ConnectionManagerPtr create(const QString &name,
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());
    static ConnectionManagerPtr create(const QDBusConnection &bus,
            const QString &name,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());

    virtual ~ConnectionManager();

    QString name() const;

    ConnectionFactoryConstPtr connectionFactory() const;
    ChannelFactoryConstPtr channelFactory() const;
    ContactFactoryConstPtr contactFactory() const;

    QStringList supportedProtocols() const;
    const ProtocolInfoList &protocols() const;
    bool hasProtocol(const QString &protocolName) const;
    ProtocolInfo protocol(const QString &protocolName) const;

    static PendingStringList *listNames(
            const QDBusConnection &bus = QDBusConnection::sessionBus());

#if defined(BUILDING_TP_QT) || defined(TP_QT_ENABLE_LOWLEVEL_API)
    ConnectionManagerLowlevelPtr lowlevel();
    ConnectionManagerLowlevelConstPtr lowlevel() const;
#endif

protected:
    ConnectionManager(const QDBusConnection &bus, const QString &name,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    Client::ConnectionManagerInterface *baseInterface() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void gotMainProperties(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void gotProtocolsLegacy(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotParametersLegacy(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onProtocolReady(Tp::PendingOperation *watcher);

private:
    friend class PendingConnection;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
