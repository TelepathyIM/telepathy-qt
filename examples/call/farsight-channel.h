/*
 * Very basic Tp-Qt <-> Tp-Farsight integration.
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

#ifndef _TelepathyQt4_examples_call_farsight_channel_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_farsight_channel_h_HEADER_GUARD_

#include <TelepathyQt4/Types>

#include <QObject>
#include <QMetaType>

#include <telepathy-farsight/channel.h>

namespace Tp {

class Connection;
class VideoWidget;

class FarsightChannel : public QObject
{
    Q_OBJECT
    Q_ENUMS(Status)

public:
    enum Status {
        StatusDisconnected = 0,
        StatusConnecting = 1,
        StatusConnected = 2
    };

    FarsightChannel(const StreamedMediaChannelPtr &channel, QObject *parent = 0);
    virtual ~FarsightChannel();

    Status status() const;

    VideoWidget *videoPreview() const;
    VideoWidget *videoWidget() const;

    // TODO add a way to change input and output devices

Q_SIGNALS:
    void statusChanged(Tp::FarsightChannel::Status status);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::FarsightChannel::Status)

#endif
