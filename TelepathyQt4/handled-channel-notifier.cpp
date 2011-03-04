/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt4/HandledChannelNotifier>

#include "TelepathyQt4/_gen/handled-channel-notifier.moc.hpp"

#include "TelepathyQt4/debug-internal.h"
#include "TelepathyQt4/request-temporary-handler-internal.h"

#include <TelepathyQt4/ClientRegistrar>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT HandledChannelNotifier::Private
{
    Private(const ClientRegistrarPtr &cr, const SharedPtr<RequestTemporaryHandler> &handler)
        : cr(cr),
          handler(handler)
    {
    }

    ClientRegistrarPtr cr;
    SharedPtr<RequestTemporaryHandler> handler;
};

HandledChannelNotifier::HandledChannelNotifier(const ClientRegistrarPtr &cr,
        const SharedPtr<RequestTemporaryHandler> &handler)
    : mPriv(new Private(cr, handler))
{
    connect(handler->channel().data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated()));
    connect(handler.data(),
            SIGNAL(channelReceived(Tp::ChannelPtr)),
            SIGNAL(hanldedAgain()));
}

HandledChannelNotifier::~HandledChannelNotifier()
{
    delete mPriv;
}

ChannelPtr HandledChannelNotifier::channel() const
{
    return mPriv->handler->channel();
}

void HandledChannelNotifier::onChannelInvalidated()
{
    emit finished();
    deleteLater();
}

} // Tp
