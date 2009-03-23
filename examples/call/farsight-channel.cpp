/*
 * Very basic Telepathy-Qt <-> Telepathy-Farsight integration.
 *
 * Copyright © 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "farsight-channel.h"
#include "_gen/farsight-channel.moc.hpp"

#include "video-widget.h"

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
    GstElement *audioInput;
    GstElement *audioOutput;
    GstElement *videoInput;
    GstElement *videoTee;
    VideoWidget *videoPreview;
    VideoWidget *videoOutput;
};

FarsightChannel::Private::Private(FarsightChannel *parent,
        StreamedMediaChannel *channel)
    : parent(parent),
      channel(channel),
      status(StatusDisconnected),
      tfChannel(0),
      bus(0),
      pipeline(0),
      audioInput(0),
      audioOutput(0),
      videoInput(0),
      videoTee(0),
      videoPreview(0),
      videoOutput(0)
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

    pipeline = gst_pipeline_new(NULL);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

    audioInput = gst_element_factory_make("autoaudiosrc", NULL);
    gst_object_ref(audioInput);
    gst_object_sink(audioInput);

    audioOutput = gst_bin_new("audio-output-bin");
    GstElement *resample = gst_element_factory_make("audioresample", NULL);
    GstElement *audioSink = gst_element_factory_make("autoaudiosink", NULL);
    gst_bin_add_many(GST_BIN(audioOutput), resample, audioSink, NULL);
    gst_element_link_many(resample, audioSink, NULL);
    GstPad *sink = gst_element_get_static_pad(resample, "sink");
    GstPad *ghost = gst_ghost_pad_new("sink", sink);
    gst_element_add_pad(GST_ELEMENT(audioOutput), ghost);
    gst_object_unref(G_OBJECT(sink));
    gst_object_ref(audioOutput);
    gst_object_sink(audioOutput);

    videoInput = gst_bin_new("video-input-bin");
    GstElement *scale = gst_element_factory_make("videoscale", NULL);
    GstElement *rate = gst_element_factory_make("videorate", NULL);
    GstElement *colorspace = gst_element_factory_make("ffmpegcolorspace", NULL);
    GstElement *capsfilter = gst_element_factory_make("capsfilter", NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw-yuv",
            "width", G_TYPE_INT, 320,
            "height", G_TYPE_INT, 240,
            NULL);
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    GstElement *videoSrc = gst_element_factory_make("autovideosrc", NULL);
    // GstElement *videoSrc = gst_element_factory_make("videotestsrc", NULL);
    gst_bin_add_many(GST_BIN(videoInput), videoSrc, scale, rate,
            colorspace, capsfilter, NULL);
    gst_element_link_many(videoSrc, scale, rate, colorspace, capsfilter, NULL);
    GstPad *src = gst_element_get_static_pad(capsfilter, "src");
    ghost = gst_ghost_pad_new("src", src);
    Q_ASSERT(gst_element_add_pad(GST_ELEMENT(videoInput), ghost));
    gst_object_unref(G_OBJECT(src));
    gst_object_ref(audioInput);
    gst_object_sink(audioInput);

    videoTee = gst_element_factory_make("tee", NULL);
    gst_object_ref(videoTee);
    gst_object_sink(videoTee);

    videoPreview = new VideoWidget(bus);
    GstElement *videoPreviewElement = videoPreview->element();

    gst_bin_add_many(GST_BIN(pipeline), videoInput, videoTee,
            videoPreviewElement, NULL);
    gst_element_link_many(videoInput, videoTee,
            videoPreviewElement, NULL);

    videoOutput = new VideoWidget(bus);

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

    if (audioInput) {
        g_object_unref(audioInput);
        audioInput = 0;
    }

    if (audioOutput) {
        g_object_unref(audioOutput);
        audioOutput = 0;
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
        gst_bin_add(GST_BIN(self->pipeline), self->audioInput);
        gst_element_set_state(self->audioInput, GST_STATE_PLAYING);

        pad = gst_element_get_static_pad(self->audioInput, "src");
        gst_pad_link(pad, sink);
        break;
    case TP_MEDIA_STREAM_TYPE_VIDEO:
        pad = gst_element_get_request_pad(self->videoTee, "src%d");
        gst_pad_link(pad, sink);
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
        element = self->audioOutput;
        g_object_ref(element);
        break;
    case TP_MEDIA_STREAM_TYPE_VIDEO:
        element = self->videoOutput->element();
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

VideoWidget *FarsightChannel::videoPreview() const
{
    return mPriv->videoPreview;
}

VideoWidget *FarsightChannel::videoWidget() const
{
    return mPriv->videoOutput;
}

}
}
