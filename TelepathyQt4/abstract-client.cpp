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

#include <TelepathyQt4/AbstractClient>

#include <QString>

namespace Tp
{

AbstractClient::AbstractClient()
{
}

AbstractClient::~AbstractClient()
{
}

struct TELEPATHY_QT4_NO_EXPORT AbstractClientObserver::Private
{
    ChannelClassList channelFilter;
};

AbstractClientObserver::AbstractClientObserver(
        const ChannelClassList &channelFilter)
    : mPriv(new Private)
{
    mPriv->channelFilter = channelFilter;
}

AbstractClientObserver::~AbstractClientObserver()
{
    delete mPriv;
}

ChannelClassList AbstractClientObserver::observerChannelFilter() const
{
    return mPriv->channelFilter;
}

struct TELEPATHY_QT4_NO_EXPORT AbstractClientApprover::Private
{
    ChannelClassList channelFilter;
};

AbstractClientApprover::AbstractClientApprover(
        const ChannelClassList &channelFilter)
    : mPriv(new Private)
{
    mPriv->channelFilter = channelFilter;
}

AbstractClientApprover::~AbstractClientApprover()
{
    delete mPriv;
}

ChannelClassList AbstractClientApprover::approverChannelFilter() const
{
    return mPriv->channelFilter;
}

struct TELEPATHY_QT4_NO_EXPORT AbstractClientHandler::Private
{
    ChannelClassList channelFilter;
    bool wantsRequestNotification;
};

AbstractClientHandler::AbstractClientHandler(const ChannelClassList &channelFilter,
        bool wantsRequestNotification)
    : mPriv(new Private)
{
    mPriv->channelFilter = channelFilter;
    mPriv->wantsRequestNotification = wantsRequestNotification;
}

AbstractClientHandler::~AbstractClientHandler()
{
    delete mPriv;
}

ChannelClassList AbstractClientHandler::handlerChannelFilter() const
{
    return mPriv->channelFilter;
}

bool AbstractClientHandler::wantsRequestNotification() const
{
    return mPriv->wantsRequestNotification;
}

void AbstractClientHandler::addRequest(
        const ChannelRequestPtr &channelRequest)
{
    // do nothing, subclasses that want to listen requests should reimplement
    // this method
}

void AbstractClientHandler::removeRequest(
        const ChannelRequestPtr &channelRequest,
        const QString &errorName, const QString &errorMessage)
{
    // do nothing, subclasses that want to listen requests should reimplement
    // this method
}

} // Tp
