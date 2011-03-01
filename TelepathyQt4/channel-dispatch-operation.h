/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_channel_dispatch_operation_h_HEADER_GUARD_
#define _TelepathyQt4_channel_dispatch_operation_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-channel-dispatch-operation.h>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/Feature>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class PendingOperation;

class TELEPATHY_QT4_EXPORT ChannelDispatchOperation : public StatefulDBusProxy,
                public OptionalInterfaceFactory<ChannelDispatchOperation>
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelDispatchOperation)

public:
    static const Feature FeatureCore;

    static ChannelDispatchOperationPtr create(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const QList<ChannelPtr> &initialChannels,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);
    virtual ~ChannelDispatchOperation();

    ConnectionPtr connection() const;

    AccountPtr account() const;

    QList<ChannelPtr> channels() const;

    QStringList possibleHandlers() const;

    PendingOperation *handleWith(const QString &handler);

    PendingOperation *claim();

Q_SIGNALS:
    void channelLost(const Tp::ChannelPtr &channel, const QString &errorName,
            const QString &errorMessage);

protected:
    ChannelDispatchOperation(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const QList<ChannelPtr> &initialChannels,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    Client::ChannelDispatchOperationInterface *baseInterface() const;

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onFinished();
    TELEPATHY_QT4_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *watcher);
    TELEPATHY_QT4_NO_EXPORT void onChannelLost(const QDBusObjectPath &channelObjectPath,
        const QString &errorName, const QString &errorMessage);
    TELEPATHY_QT4_NO_EXPORT void onProxiesPrepared(Tp::PendingOperation *op);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
