/**
 * This file is part of TelepathyQt
 *
 * Copyright © 2009-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/Farstream/Channel>

#include "TelepathyQt/Farstream/_gen/channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/CallChannel>
#include <TelepathyQt/Connection>

#include <telepathy-farstream/telepathy-farstream.h>
#include <telepathy-glib/automatic-client-factory.h>
#include <telepathy-glib/call-channel.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/dbus.h>

namespace Tp
{
namespace Farstream
{

struct TP_QT_FS_NO_EXPORT PendingChannel::Private
{
    Private()
        : mTfChannel(0)
    {
    }

    static void onTfChannelNewFinish(GObject *sourceObject, GAsyncResult *res, gpointer userData);

    TfChannel *mTfChannel;
};

PendingChannel::PendingChannel(const CallChannelPtr &channel)
    : Tp::PendingOperation(channel),
      mPriv(new PendingChannel::Private)
{
    if (!channel->handlerStreamingRequired()) {
        warning() << "Handler streaming not required";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Handler streaming not required"));
        return;
    }

    TpDBusDaemon *dbus = tp_dbus_daemon_dup(0);
    if (!dbus) {
        warning() << "Unable to connect to D-Bus";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Unable to connect to D-Bus"));
        return;
    }

    Tp::ConnectionPtr connection = channel->connection();
    if (connection.isNull()) {
        warning() << "Connection not available";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection not available"));
        g_object_unref(dbus);
        return;
    }

    TpSimpleClientFactory *factory = (TpSimpleClientFactory *)
            tp_automatic_client_factory_new (dbus);
    if (!factory) {
        warning() << "Unable to construct TpAutomaticClientFactory";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Unable to construct TpAutomaticClientFactory"));
        g_object_unref(dbus);
        return;
    }

    TpConnection *gconnection = tp_simple_client_factory_ensure_connection (factory,
            connection->objectPath().toLatin1(), NULL, 0);
    if (!gconnection) {
        warning() << "Unable to construct TpConnection";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Unable to construct TpConnection"));
        g_object_unref(factory);
        g_object_unref(dbus);
        return;
    }

    TpChannel *gchannel = (TpChannel*) g_object_new(TP_TYPE_CALL_CHANNEL,
            "bus-name", connection->busName().toLatin1().constData(),
            "connection", gconnection,
            "dbus-daemon", dbus,
            "object-path", channel->objectPath().toLatin1().constData(),
            NULL);
    g_object_unref(factory);
    factory = 0;
    g_object_unref(dbus);
    dbus = 0;
    g_object_unref(gconnection);
    gconnection = 0;
    if (!gchannel) {
        warning() << "Unable to construct TpChannel";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Unable to construct TpChannel"));
        return;
    }

    tf_channel_new_async(gchannel, PendingChannel::Private::onTfChannelNewFinish, this);
    g_object_unref(gchannel);
}

PendingChannel::~PendingChannel()
{
    delete mPriv;
}

void PendingChannel::Private::onTfChannelNewFinish(GObject *sourceObject,
        GAsyncResult *res, gpointer userData)
{
    PendingChannel *self = reinterpret_cast<PendingChannel *>(userData);

    GError *error = NULL;
    TfChannel *ret = tf_channel_new_finish(sourceObject, res, &error);
    if (error) {
        warning() << "Fs::PendingChannel::Private::onTfChannelNewFinish: error " << error->message;
        self->setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE, QLatin1String(error->message));
        g_clear_error(&error);
        return;
    }

    self->mPriv->mTfChannel = ret;
    self->setFinished();
}

TfChannel *PendingChannel::tfChannel() const
{
    return mPriv->mTfChannel;
}

CallChannelPtr PendingChannel::callChannel() const
{
    return CallChannelPtr::staticCast(object());
}

PendingChannel *createChannel(const CallChannelPtr &channel)
{
    PendingChannel *ptf = new PendingChannel(channel);
    return ptf;
}

} // Farstream
} // Tp
