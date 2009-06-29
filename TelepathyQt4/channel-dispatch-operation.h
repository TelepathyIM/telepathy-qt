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

#ifndef _TelepathyQt4_cli_channel_dispatch_operation_h_HEADER_GUARD_
#define _TelepathyQt4_cli_channel_dispatch_operation_h_HEADER_GUARD_

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
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class PendingOperation;

class ChannelDispatchOperation : public StatefulDBusProxy,
                                 public OptionalInterfaceFactory<ChannelDispatchOperation>,
                                 public ReadyObject,
                                 public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelDispatchOperation)

public:
    static const Feature FeatureCore;

    static ChannelDispatchOperationPtr create(const QString &objectPath,
            const QVariantMap &immutableProperties);
    static ChannelDispatchOperationPtr create(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties);

    ~ChannelDispatchOperation();

    ConnectionPtr connection() const;

    AccountPtr account() const;

    QList<ChannelPtr> channels() const;

    QStringList possibleHandlers() const;

    PendingOperation *handleWith(const QString &handler);

    PendingOperation *claim();

    inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<Client::DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

Q_SIGNALS:
    void channelLost(const ChannelPtr &channel, const QString &errorName,
            const QString &errorMessage);

protected:
    ChannelDispatchOperation(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties);

    Client::ChannelDispatchOperationInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onConnectionReady(Tp::PendingOperation *op);
    void onAccountReady(Tp::PendingOperation *op);
    void onChannelReady(Tp::PendingOperation *op);
    void onChannelLost(const QDBusObjectPath &channelObjectPath,
        const QString &errorName, const QString &errorMessage);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
