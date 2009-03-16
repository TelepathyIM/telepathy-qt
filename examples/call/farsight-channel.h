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

#ifndef _TelepathyQt4_examples_call_farsight_channel_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_farsight_channel_h_HEADER_GUARD_

#include <QObject>

#include <telepathy-farsight/channel.h>

namespace Telepathy {
namespace Client {

class Connection;
class StreamedMediaChannel;

class FarsightChannel : public QObject
{
    Q_OBJECT

public:
    enum Status {
        StatusDisconnected = 0,
        StatusConnecting = 1,
        StatusConnected = 2
    };

    FarsightChannel(StreamedMediaChannel *channel, QObject *parent = 0);
    virtual ~FarsightChannel();

    Status status() const;

    // TODO add a way to change input and output devices
    //      add video support

Q_SIGNALS:
    void statusChanged(Telepathy::Client::FarsightChannel::Status status);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif
