/*
 * Very basic Telepathy-Qt <-> Telepathy-Farsight integration.
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "farsight-glue.h"

#include <QDebug>

#include <telepathy-farsight/channel.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/dbus.h>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/StreamedMediaChannel>

TfChannel *tfChannelFromQt(const Telepathy::Client::Connection *connection,
        const Telepathy::Client::StreamedMediaChannel *channel)
{
    TpDBusDaemon *dbus = tp_dbus_daemon_dup(0);
    if (!dbus) {
        qWarning() << "Unable to connect to D-Bus";
        return 0;
    }

    TpConnection *gconnection = tp_connection_new(dbus,
            connection->busName().toAscii(),
            connection->objectPath().toAscii(),
            0);
    g_object_unref(dbus);
    dbus = 0;

    if (!gconnection) {
        qWarning() << "Unable to construct TpConnection";
        return 0;
    }

    TpChannel *gchannel = tp_channel_new(gconnection,
            channel->objectPath().toAscii(),
            TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            TP_UNKNOWN_HANDLE_TYPE, 0, 0);
    g_object_unref(gconnection);
    gconnection = 0;

    if (!gchannel) {
        qWarning() << "Unable to construct TpChannel";
        return 0;
    }

    TfChannel *ret = tf_channel_new(gchannel);
    g_object_unref(gchannel);
    gchannel = 0;

    if (!ret) {
        qWarning() << "Unable to construct TfChannel";
        return 0;
    }
    return ret;
}
