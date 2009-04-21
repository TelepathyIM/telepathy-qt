/*
 * This file is part of TelepathyQt4
 *
 * Copyright © 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2009 Nokia Corporation
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

#include <TelepathyQt4/Farsight/Channel>

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/StreamedMediaChannel>

#include <telepathy-glib/channel.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/dbus.h>

namespace Tp
{

TfChannel *createFarsightChannel(const StreamedMediaChannelPtr &channel)
{
    if (!channel->handlerStreamingRequired()) {
        warning() << "Handler streaming not required";
        return 0;
    }

    TpDBusDaemon *dbus = tp_dbus_daemon_dup(0);

    if (!dbus) {
        warning() << "Unable to connect to D-Bus";
        return 0;
    }

    ConnectionPtr connection = channel->connection();

    TpConnection *gconnection = tp_connection_new(dbus,
            connection->busName().toAscii(),
            connection->objectPath().toAscii(),
            0);
    g_object_unref(dbus);
    dbus = 0;

    if (!gconnection) {
        warning() << "Unable to construct TpConnection";
        return 0;
    }

    TpChannel *gchannel = tp_channel_new(gconnection,
            channel->objectPath().toAscii(),
            TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            (TpHandleType) channel->targetHandleType(),
            channel->targetHandle(),
            0);
    g_object_unref(gconnection);
    gconnection = 0;

    if (!gchannel) {
        warning() << "Unable to construct TpChannel";
        return 0;
    }

    TfChannel *ret = tf_channel_new(gchannel);
    g_object_unref(gchannel);
    gchannel = 0;

    return ret;
}

} // Tp
