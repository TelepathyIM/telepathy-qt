/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
 * @license LGPL 2.1
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

#include "TelepathyQt4/fake-handler-manager-internal.h"

#include "TelepathyQt4/_gen/fake-handler-manager-internal.moc.hpp"

#include <TelepathyQt4/Channel>

namespace Tp
{

FakeHandler::FakeHandler(const ClientRegistrarPtr &cr, const ChannelPtr &channel)
    : QObject(),
      mCr(cr)
{
    connect(channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated()));
    connect(channel.data(), SIGNAL(destroyed()), SLOT(onChannelDestroyed()));
}

FakeHandler::~FakeHandler()
{
}

void FakeHandler::onChannelInvalidated()
{
    disconnect(sender(), SIGNAL(destroyed()), this, SLOT(onChannelDestroyed()));
    onChannelDestroyed();
}

void FakeHandler::onChannelDestroyed()
{
    deleteLater();
}

FakeHandlerManager *FakeHandlerManager::mInstance = 0;

FakeHandlerManager *FakeHandlerManager::instance()
{
    if (!mInstance) {
        mInstance = new FakeHandlerManager();
    }
    return mInstance;
}

FakeHandlerManager::FakeHandlerManager()
    : QObject()
{
}

FakeHandlerManager::~FakeHandlerManager()
{
    mInstance = 0;
}

void FakeHandlerManager::registerHandler(const ClientRegistrarPtr &cr,
        const ChannelPtr &channel)
{
    // we need to keep one FakeHandler per ClientRegistrar as we create one client registrar per RAH
    // request and the client registrar destructor will unregister all handlers registered by it
    FakeHandler *handler = new FakeHandler(cr, channel);
    mFakeHandlers.insert(handler);
    connect(handler,
            SIGNAL(destroyed(QObject*)),
            SLOT(onFakeHandlerDestroyed(QObject*)));
}

void FakeHandlerManager::onFakeHandlerDestroyed(QObject *obj)
{
    FakeHandler *handler = (FakeHandler *) obj;

    Q_ASSERT(mFakeHandlers.contains(handler));

    mFakeHandlers.remove(handler);

    if (mFakeHandlers.isEmpty()) {
        // set mInstance to 0 here as we don't want instance() to return a already
        // deleted instance
        mInstance = 0;
        deleteLater();
    }
}

}
