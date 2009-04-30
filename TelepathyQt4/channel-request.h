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

#ifndef _TelepathyQt4_cli_channel_request_h_HEADER_GUARD_
#define _TelepathyQt4_cli_channel_request_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-channel-request.h>

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

class ChannelRequest : public StatefulDBusProxy,
                       private OptionalInterfaceFactory<ChannelRequest>,
                       public ReadyObject,
                       public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelRequest)

public:
    static const Feature FeatureCore;

    static ChannelRequestPtr create(const QString &objectPath,
            const QVariantMap &immutableProperties);
    static ChannelRequestPtr create(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties);

    ~ChannelRequest();

    QStringList interfaces() const;

    AccountPtr account() const;

    QDateTime userActionTime() const;

    QString preferredHandler() const;

    QualifiedPropertyValueMapList requests() const;

    PendingOperation *cancel();

    template <class Interface>
    inline Interface *optionalInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && !interfaces().contains(name))
            return 0;

        // If present or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory<ChannelRequest>::interface<Interface>();
    }

    inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<Client::DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

Q_SIGNALS:
    void failed(const QString &errorName, const QString &errorMessage);
    void succeeded();

protected:
    ChannelRequest(const QString &objectPath,
            const QVariantMap &immutableProperties);
    ChannelRequest(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties);

    Client::ChannelRequestInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onAccountReady(Tp::PendingOperation *op);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
