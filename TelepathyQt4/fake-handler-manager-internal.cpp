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

FakeHandler::FakeHandler()
    : QObject(),
      mNumChannels(0)
{
}

void FakeHandler::addChannel(const ChannelPtr &channel,
        const ClientRegistrarPtr &registrar)
{
    if (mNumChannels <= 0) {
        mRegistrar = registrar;
    }

    mNumChannels++;

    connect(channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated()));
    connect(channel.data(), SIGNAL(destroyed()), SLOT(onChannelDestroyed()));
}

void FakeHandler::onChannelInvalidated()
{
    disconnect(sender(), SIGNAL(destroyed()), this, SLOT(onChannelDestroyed()));
    onChannelDestroyed();
}

void FakeHandler::onChannelDestroyed()
{
    mNumChannels--;
    if (mNumChannels <= 0) {
        deleteLater();
    }
}

FakeHandlerManager *FakeHandlerManager::instance()
{
    static FakeHandlerManager sInstance;
    return &sInstance;
}

FakeHandlerManager::FakeHandlerManager()
    : QObject()
{
}

void FakeHandlerManager::registerHandler(const QPair<QString, QString> &dbusConnection,
        const ChannelPtr &channel,
        const ClientRegistrarPtr &registrar)
{
    FakeHandler *handler;

    QWeakPointer<FakeHandler> weakFakeHandlerPointer = mFakeHandlers[dbusConnection];
    if (weakFakeHandlerPointer.isNull()) {
        handler = new FakeHandler;
        mFakeHandlers.insert(dbusConnection, QWeakPointer<FakeHandler>(handler));
    } else {
        handler = weakFakeHandlerPointer.data();
    }

    handler->addChannel(channel, registrar);
}

}
