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

ChannelPtr ChannelFactory::create(const ConnectionPtr &connection,
        const QString &channelPath, const QVariantMap &immutableProperties)
{
    QString channelType = immutableProperties.value(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
    if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) {
        return ChannelPtr(dynamic_cast<Channel*>(
                    TextChannel::create(connection,
                        channelPath, immutableProperties).data()));
    }
    else if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) {
        return ChannelPtr(dynamic_cast<Channel*>(
                    StreamedMediaChannel::create(connection,
                        channelPath, immutableProperties).data()));
    }
    else if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_ROOM_LIST) {
        return ChannelPtr(dynamic_cast<Channel*>(
                    RoomListChannel::create(connection,
                        channelPath, immutableProperties).data()));
    }
    else if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER) {
        if (immutableProperties.contains(QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".Requested"))) {
            bool requested = immutableProperties.value(QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
            if (requested) {
                return ChannelPtr(dynamic_cast<Channel*>(
                            OutgoingFileTransferChannel::create(connection,
                                channelPath, immutableProperties).data()));
            } else {
                return ChannelPtr(dynamic_cast<Channel*>(
                            IncomingFileTransferChannel::create(connection,
                                channelPath, immutableProperties).data()));
            }
        } else {
            return ChannelPtr(dynamic_cast<Channel*>(
                        FileTransferChannel::create(connection,
                            channelPath, immutableProperties).data()));
        }
    }

    // ContactList, old-style Tubes, or a future channel type
    return Channel::create(connection,
            channelPath, immutableProperties);
}

} // Tp
