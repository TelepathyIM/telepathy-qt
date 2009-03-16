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

#include "farsight-channel.h"
#include "_gen/farsight-channel.moc.hpp"

#include <QDebug>

#include <telepathy-farsight/channel.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/dbus.h>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/StreamedMediaChannel>

namespace Telepathy {
namespace Client {

struct FarsightChannel::Private
{
    Private(FarsightChannel *parent, StreamedMediaChannel *channel);
    ~Private();

    static gboolean busWatch(GstBus *bus,
            GstMessage *message, FarsightChannel::Private *self);
    static void onClosed(TfChannel *tfChannel,
            FarsightChannel::Private *self);
    static void onSessionCreated(TfChannel *tfChannel,
            FsConference *conference, FsParticipant *participant,
            FarsightChannel::Private *self);
    static void onStreamCreated(TfChannel *tfChannel,
            TfStream *stream, FarsightChannel::Private *self);
    static void onSrcPadAdded(TfStream *stream,
            GstPad *pad, FsCodec *codec, FarsightChannel::Private *self);
    static gboolean onRequestResource(TfStream *stream,
            guint direction, gpointer data);

    FarsightChannel *parent;
    StreamedMediaChannel *channel;
    Status status;
    TfChannel *tfChannel;
    GstBus *bus;
    GstElement *pipeline;
    GstElement *audio_input;
    GstElement *audio_output;
};

FarsightChannel::Private::Private(FarsightChannel *parent,
        StreamedMediaChannel *channel)
    : parent(parent),
      channel(channel),
      status(StatusDisconnected),
      tfChannel(0),
      bus(0),
      pipeline(0),
      audio_input(0),
      audio_output(0)
{
    TpDBusDaemon *dbus = tp_dbus_daemon_dup(0);
    if (!dbus) {
        qWarning() << "Unable to connect to D-Bus";
        return;
    }

    Connection *connection = channel->connection();

    TpConnection *gconnection = tp_connection_new(dbus,
            connection->busName().toAscii(),
            connection->objectPath().toAscii(),
            0);
    g_object_unref(dbus);
    dbus = 0;

    if (!gconnection) {
        qWarning() << "Unable to construct TpConnection";
        return;
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
        qWarning() << "Unable to construct TpChannel";
        return;
    }

    tfChannel = tf_channel_new(gchannel);
    g_object_unref(gchannel);
    gchannel = 0;

    if (!tfChannel) {
        qWarning() << "Unable to construct TfChannel";
        return;
    }

    /* Set up the telepathy farsight channel */
    g_signal_connect(tfChannel, "closed",
        G_CALLBACK(&FarsightChannel::Private::onClosed), this);
    g_signal_connect(tfChannel, "session-created",
        G_CALLBACK(&FarsightChannel::Private::onSessionCreated), this);
    g_signal_connect(tfChannel, "stream-created",
        G_CALLBACK(&FarsightChannel::Private::onStreamCreated), this);

    pipeline = gst_pipeline_new (NULL);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

    audio_input = gst_element_factory_make("gconfaudiosrc", NULL);
    gst_object_ref(audio_input);
    gst_object_sink(audio_input);

    audio_output = gst_bin_new("bin");
    GstElement *resample = gst_element_factory_make("audioresample", NULL);
    GstElement *audio_sink = gst_element_factory_make("gconfaudiosink", NULL);
    gst_bin_add_many(GST_BIN(audio_output), resample, audio_sink, NULL);
    gst_element_link_many(resample, audio_sink, NULL);
    GstPad *sink = gst_element_get_static_pad(resample, "sink");
    GstPad *ghost = gst_ghost_pad_new("sink", sink);
    gst_element_add_pad(GST_ELEMENT(audio_output), ghost);
    gst_object_unref(G_OBJECT(sink));
    gst_object_ref(audio_output);
    gst_object_sink(audio_output);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    status = StatusConnecting;
    emit parent->statusChanged(status);
}

FarsightChannel::Private::~Private()
{
    if (tfChannel) {
        g_object_unref(tfChannel);
        tfChannel = 0;
    }

    if (bus) {
        g_object_unref(bus);
        bus = 0;
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);

    if (pipeline) {
        g_object_unref(pipeline);
        pipeline = 0;
    }

    if (audio_input) {
        g_object_unref(audio_input);
        audio_input = 0;
    }

    if (audio_output) {
        g_object_unref(audio_output);
        audio_output = 0;
    }
}

gboolean FarsightChannel::Private::busWatch(GstBus *bus,
        GstMessage *message, FarsightChannel::Private *self)
{
    tf_channel_bus_message(self->tfChannel, message);
    return TRUE;
}

void FarsightChannel::Private::onClosed(TfChannel *tfChannel,
        FarsightChannel::Private *self)
{
    self->status = StatusDisconnected;
    emit self->parent->statusChanged(self->status);
}

void FarsightChannel::Private::onSessionCreated(TfChannel *tfChannel,
        FsConference *conference, FsParticipant *participant,
        FarsightChannel::Private *self)
{
    gst_bus_add_watch(self->bus,
            (GstBusFunc) &FarsightChannel::Private::busWatch, self);

    gst_bin_add(GST_BIN(self->pipeline), GST_ELEMENT(conference));
    gst_element_set_state(GST_ELEMENT(conference), GST_STATE_PLAYING);
}

void FarsightChannel::Private::onStreamCreated(TfChannel *tfChannel,
        TfStream *stream, FarsightChannel::Private *self)
{
    guint media_type;
    GstPad *sink;

    g_signal_connect(stream, "src-pad-added",
        G_CALLBACK(&FarsightChannel::Private::onSrcPadAdded), self);
    g_signal_connect(stream, "request-resource",
        G_CALLBACK(&FarsightChannel::Private::onRequestResource), NULL);

    g_object_get(stream, "media-type", &media_type,
            "sink-pad", &sink, NULL);

    GstPad *pad;
    switch (media_type) {
    case TP_MEDIA_STREAM_TYPE_AUDIO:
        gst_bin_add(GST_BIN(self->pipeline), self->audio_input);
        gst_element_set_state(self->audio_input, GST_STATE_PLAYING);

        pad = gst_element_get_static_pad(self->audio_input, "src");
        gst_pad_link(pad, sink);
        break;
    case TP_MEDIA_STREAM_TYPE_VIDEO:
        // TODO
        break;
    default:
        Q_ASSERT(false);
    }

    gst_object_unref(sink);
}

void FarsightChannel::Private::onSrcPadAdded(TfStream *stream,
        GstPad *src, FsCodec *codec, FarsightChannel::Private *self)
{
    guint media_type;

    g_object_get(stream, "media-type", &media_type, NULL);

    GstPad *pad;
    GstElement *element = 0;

    switch (media_type) {
    case TP_MEDIA_STREAM_TYPE_AUDIO:
        element = self->audio_output;
        g_object_ref(element);
        break;
    case TP_MEDIA_STREAM_TYPE_VIDEO:
        // TODO
        return;
        break;
    default:
        Q_ASSERT(false);
    }

    gst_bin_add(GST_BIN(self->pipeline), element);

    pad = gst_element_get_static_pad(element, "sink");
    gst_element_set_state(element, GST_STATE_PLAYING);

    gst_pad_link(src, pad);

    self->status = StatusConnected;
    emit self->parent->statusChanged(self->status);
}

gboolean FarsightChannel::Private::onRequestResource(TfStream *stream,
        guint direction, gpointer data)
{
    return TRUE;
}

FarsightChannel::FarsightChannel(StreamedMediaChannel *channel, QObject *parent)
    : QObject(parent),
      mPriv(new Private(this, channel))
{
}

FarsightChannel::~FarsightChannel()
{
    delete mPriv;
}

}
}
