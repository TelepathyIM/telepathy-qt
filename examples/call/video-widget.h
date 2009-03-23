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

#ifndef _TelepathyQt4_examples_call_video_widget_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_video_widget_h_HEADER_GUARD_

#include <QWidget>

#include <gst/gst.h>

namespace Telepathy {
namespace Client {

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget(GstBus *bus, QWidget *parent = 0);
    virtual ~VideoWidget();

    GstElement *element() const;

protected:
    bool eventFilter(QEvent *ev);

private Q_SLOTS:
    void setOverlay();
    void windowExposed();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
