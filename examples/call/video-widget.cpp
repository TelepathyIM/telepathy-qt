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

#include "video-widget.h"
#include "_gen/video-widget.moc.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QThread>

#include <gst/interfaces/xoverlay.h>
#include <gst/farsight/fs-element-added-notifier.h>

extern void qt_x11_set_global_double_buffer(bool);

namespace Telepathy {
namespace Client {

struct VideoWidget::Private {
    Private(VideoWidget *parent, GstBus *bus);
    ~Private();

    static void onElementAdded(FsElementAddedNotifier *notifier,
            GstBin *bin, GstElement *element,
            Private *self);
    static void onSyncMessage(GstBus *bus, GstMessage *message,
            Private *self);

    VideoWidget *parent;
    GstBus *bus;
    FsElementAddedNotifier *notifier;
    GstElement *sink;
    GstElement *overlay;
};

VideoWidget::Private::Private(VideoWidget *parent, GstBus *bus)
    : parent(parent),
      bus((GstBus *) gst_object_ref(bus)),
      overlay(0)
{
    notifier = fs_element_added_notifier_new();
    g_signal_connect(notifier, "element-added",
            G_CALLBACK(&VideoWidget::Private::onElementAdded),
            this);

    sink = gst_element_factory_make("autovideosink", NULL);
    gst_object_ref(sink);
    gst_object_sink(sink);

    fs_element_added_notifier_add(notifier, GST_BIN(sink));
    gst_bus_enable_sync_message_emission(bus);

    g_signal_connect(bus, "sync-message",
            G_CALLBACK(&VideoWidget::Private::onSyncMessage),
            this);
}

VideoWidget::Private::~Private()
{
    g_object_unref(bus);
    g_object_unref(sink);
}

void VideoWidget::Private::onElementAdded(FsElementAddedNotifier *notifier,
        GstBin *bin, GstElement *element, VideoWidget::Private *self)
{
    if (!self->overlay && GST_IS_X_OVERLAY(element)) {
        self->overlay = element;
        QMetaObject::invokeMethod(self->parent, "windowExposed", Qt::QueuedConnection);
    }

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(element),
                "force-aspect-ratio")) {
        g_object_set(G_OBJECT(element), "force-aspect-ratio", TRUE, NULL);
    }
}

void VideoWidget::Private::onSyncMessage(GstBus *bus, GstMessage *message,
        VideoWidget::Private *self)
{
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_ELEMENT) {
        return;
    }

    if (GST_MESSAGE_SRC(message) != (GstObject *) self->overlay) {
        return;
    }

    const GstStructure *s = gst_message_get_structure (message);

    if (gst_structure_has_name(s, "prepare-xwindow-id") && self->overlay) {
        QMetaObject::invokeMethod(self->parent, "setOverlay", Qt::QueuedConnection);
    }
}

VideoWidget::VideoWidget(GstBus *bus, QWidget *parent)
    : QWidget(parent),
      mPriv(new Private(this, bus))
{
    qt_x11_set_global_double_buffer(false);

    QPalette palette;
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);
    setAutoFillBackground(true);
}

VideoWidget::~VideoWidget()
{
    delete mPriv;
}

GstElement *VideoWidget::element() const
{
    return mPriv->sink;
}

bool VideoWidget::eventFilter(QEvent *ev)
{
    if (ev->type() == QEvent::Show) {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_PaintOnScreen, true);
        setOverlay();
    }
    return false;
}

void VideoWidget::setOverlay()
{
    if (mPriv->overlay && GST_IS_X_OVERLAY(mPriv->overlay)) {
        WId windowId = winId();
        QApplication::syncX();
        gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(mPriv->overlay),
                windowId);
    }
    windowExposed();
}

void VideoWidget::windowExposed()
{
    QApplication::syncX();
    if (mPriv->overlay && GST_IS_X_OVERLAY(mPriv->overlay)) {
        gst_x_overlay_expose(GST_X_OVERLAY(mPriv->overlay));
    }
}

}
}
