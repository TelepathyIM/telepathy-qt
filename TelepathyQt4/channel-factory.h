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

#ifndef _TelepathyQt4_channel_factory_h_HEADER_GUARD_
#define _TelepathyQt4_channel_factory_h_HEADER_GUARD_

#include <TelepathyQt4/DBusProxyFactory>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

// For Q_DISABLE_COPY
#include <QtGlobal>
#include <QString>
#include <QVariantMap>

class QDBusConnection;

namespace Tp
{

class TELEPATHY_QT4_EXPORT ChannelFactory : public DBusProxyFactory
{
    Q_DISABLE_COPY(ChannelFactory)

public:
    virtual ~ChannelFactory();

    // TODO: for now, only stockFreshFactory exists so this is not quite as dynamic as it should
    // be yet - so I don't think we should even have a "create" constructor yet

    static ChannelFactoryPtr stockFreshFactory(const QDBusConnection &bus);

    PendingReady *proxy(const ConnectionPtr &connection, const QString &channelPath,
            const QVariantMap &immutableProperties) const;

    // TODO: remove and deprecate
    static ChannelPtr create(const ConnectionPtr &connection,
            const QString &channelPath, const QVariantMap &immutableProperties);
protected:
    ChannelFactory(const QDBusConnection &bus);

    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const;
    // Nothing we'd like to prepare()
    virtual Features featuresFor(const SharedPtr<RefCounted> &proxy) const;

private:
    struct Private;
    Private *mPriv;
};

} // Tp

#endif
