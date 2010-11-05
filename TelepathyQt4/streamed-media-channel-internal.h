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

#include "TelepathyQt4/future-internal.h"

namespace Tp
{

/* ====== PendingMediaStreams ====== */
struct TELEPATHY_QT4_NO_EXPORT PendingMediaStreams::Private
{
    Private(PendingMediaStreams *parent, const StreamedMediaChannelPtr &channel)
        : parent(parent), channel(channel), streamsReady(0)
    {
    }

    PendingMediaStreams *parent;
    StreamedMediaChannelPtr channel;
    MediaStreams streams;
    uint numStreams;
    uint streamsReady;
};

/* ====== MediaStream ====== */
struct TELEPATHY_QT4_NO_EXPORT MediaStream::Private
{
    Private(MediaStream *parent, const StreamedMediaChannelPtr &channel,
            const MediaStreamInfo &info);

    static void introspectContact(Private *self);

    PendingOperation *updateDirection(bool send, bool receive);
    SendingState localSendingStateFromDirection();
    SendingState remoteSendingStateFromDirection();

    MediaStream *parent;
    QPointer<StreamedMediaChannel> channel;
    ReadinessHelper *readinessHelper;

    uint id;
    uint type;
    uint contactHandle;
    ContactPtr contact;
    uint direction;
    uint pendingSend;
    uint state;
};

/* ====== StreamedMediaChannel ====== */
struct TELEPATHY_QT4_NO_EXPORT StreamedMediaChannel::Private
{
    Private(StreamedMediaChannel *parent);
    ~Private();

    static void introspectStreams(Private *self);
    static void introspectLocalHoldState(Private *self);

    // Public object
    StreamedMediaChannel *parent;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    MediaStreams incompleteStreams;
    MediaStreams streams;

    LocalHoldState localHoldState;
    LocalHoldStateReason localHoldStateReason;
};

} // Tp
