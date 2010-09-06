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

#include "TelepathyQt4/channel-factory.h"

#include "TelepathyQt4/_gen/future-constants.h"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/FileTransferChannel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/OutgoingFileTransferChannel>
#include <TelepathyQt4/RoomListChannel>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/TextChannel>

namespace Tp
{

/**
 * Create a new ChannelFactory object for the given \a bus.
 *
 * The returned factory will construct channel subclasses provided by TelepathyQt4 as appropriate
 * for the channel immutable properties, but not make any features ready.
 *
 * \param bus The QDBusConnection the proxies constructed using this factory should use.
 * \return An ChannelFactoryPtr pointing to the newly created factory.
 */
ChannelFactoryPtr ChannelFactory::create(const QDBusConnection &bus)
{
    return ChannelFactoryPtr(new ChannelFactory(bus));
}

ChannelFactory::ChannelFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus)
{
}

ChannelFactory::~ChannelFactory()
{
}

PendingReady *ChannelFactory::proxy(const ConnectionPtr &connection, const QString &channelPath,
        const QVariantMap &immutableProperties) const
{
    SharedPtr<RefCounted> proxy = cachedProxy(connection->busName(), channelPath);
    if (!proxy) {
        proxy = create(connection, channelPath, immutableProperties);
    }

    return nowHaveProxy(proxy);
}

ChannelPtr ChannelFactory::create(const ConnectionPtr &connection,
        const QString &channelPath, const QVariantMap &immutableProperties)
{
    QString channelType = immutableProperties.value(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
    if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)) {
        return TextChannel::create(connection, channelPath, immutableProperties);
    }
    else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) ||
             channelType == QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL)) {
        return StreamedMediaChannel::create(connection, channelPath, immutableProperties);
    }
    else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_ROOM_LIST)) {
        return RoomListChannel::create(connection, channelPath, immutableProperties);
    }
    else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER)) {
        if (immutableProperties.contains(QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".Requested"))) {
            bool requested = immutableProperties.value(QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
            if (requested) {
                return OutgoingFileTransferChannel::create(connection, channelPath,
                        immutableProperties);
            } else {
                return IncomingFileTransferChannel::create(connection, channelPath,
                        immutableProperties);
            }
        } else {
            warning() << "Trying to create a channel of type FileTransfer "
                "without the " TELEPATHY_INTERFACE_CHANNEL ".Requested "
                "property set in immutableProperties, returning a "
                "FileTransferChannel instance";
            return FileTransferChannel::create(connection, channelPath,
                    immutableProperties);
        }
    }

    // ContactList, old-style Tubes, or a future channel type
    return Channel::create(connection, channelPath, immutableProperties);
}

QString ChannelFactory::finalBusNameFrom(const QString &uniqueOrWellKnown) const
{
    return StatefulDBusProxy::uniqueNameFrom(dbusConnection(), uniqueOrWellKnown);
}

Features ChannelFactory::featuresFor(const SharedPtr<RefCounted> &proxy) const
{
    // TODO return whatever the user / defaults has specified
    return Features();
}

} // Tp
