/**
 * This file is part of TelepathyQt
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

#include "TelepathyQt/fake-handler-manager-internal.h"

#include "TelepathyQt/_gen/fake-handler-manager-internal.moc.hpp"

#include <TelepathyQt/Channel>

namespace Tp
{

FakeHandler::FakeHandler(const QDBusConnection &bus)
    : QObject(),
      mBus(bus)
{
}

FakeHandler::~FakeHandler()
{
}

ObjectPathList FakeHandler::handledChannels() const
{
    ObjectPathList ret;
    foreach (const Channel *channel, mChannels) {
        ret << QDBusObjectPath(channel->objectPath());
    }
    return ret;
}

void FakeHandler::registerChannel(const ChannelPtr &channel)
{
    if (mChannels.contains(channel.data())) {
        return;
    }

    mChannels.insert(channel.data());
    connect(channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*)));
    connect(channel.data(),
            SIGNAL(destroyed(QObject*)),
            SLOT(onChannelDestroyed(QObject*)));
}

void FakeHandler::onChannelInvalidated(DBusProxy *channel)
{
    disconnect(channel, SIGNAL(destroyed(QObject*)), this, SLOT(onChannelDestroyed(QObject*)));
    onChannelDestroyed(channel);
}

void FakeHandler::onChannelDestroyed(QObject *obj)
{
    Channel *channel = reinterpret_cast<Channel*>(obj);

    Q_ASSERT(mChannels.contains(channel));

    mChannels.remove(channel);

    if (mChannels.isEmpty()) {
        // emit invalidated here instead of relying on QObject::destroyed as FakeHandlerManager
        // may reuse this fake handler if FakeHandlerManager::registerChannel is called before the
        // slot from QObject::destroyed is invoked (deleteLater()).
        emit invalidated(this);
        deleteLater();
    }
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

ObjectPathList FakeHandlerManager::handledChannels(const QDBusConnection &bus) const
{
    QPair<QString, QString> busUniqueId(bus.name(), bus.baseService());
    if (mFakeHandlers.contains(busUniqueId)) {
        FakeHandler *fakeHandler = mFakeHandlers.value(busUniqueId);
        return fakeHandler->handledChannels();
    }
    return ObjectPathList();
}

void FakeHandlerManager::registerClientRegistrar(const ClientRegistrarPtr &cr)
{
    QDBusConnection bus(cr->dbusConnection());
    QPair<QString, QString> busUniqueId(bus.name(), bus.baseService());
    // keep one registrar around per bus so at least the handlers registered by it will be
    // around until all channels on that bus gets invalidated/destroyed
    if (!mClientRegistrars.contains(busUniqueId)) {
        mClientRegistrars.insert(busUniqueId, cr);
    }
}

void FakeHandlerManager::registerChannels(const QList<ChannelPtr> &channels)
{
    foreach (const ChannelPtr &channel, channels) {
        QDBusConnection bus(channel->dbusConnection());
        QPair<QString, QString> busUniqueId(bus.name(), bus.baseService());
        FakeHandler *fakeHandler = mFakeHandlers.value(busUniqueId);
        if (!fakeHandler) {
            fakeHandler = new FakeHandler(bus);
            mFakeHandlers.insert(busUniqueId, fakeHandler);
            connect(fakeHandler,
                    SIGNAL(invalidated(Tp::FakeHandler*)),
                    SLOT(onFakeHandlerInvalidated(Tp::FakeHandler*)));
        }
        fakeHandler->registerChannel(channel);
    }
}

void FakeHandlerManager::onFakeHandlerInvalidated(FakeHandler *fakeHandler)
{
    QDBusConnection bus(fakeHandler->dbusConnection());
    QPair<QString, QString> busUniqueId(bus.name(), bus.baseService());
    mFakeHandlers.remove(busUniqueId);

    // all channels for the bus represented by busUniqueId were already destroyed/invalidated,
    // we can now free the CR (thus the handlers registered by it) registered for that bus
    mClientRegistrars.remove(busUniqueId);

    if (mFakeHandlers.isEmpty()) {
        // set mInstance to 0 here as we don't want instance() to return a already
        // deleted instance
        mInstance = 0;
        deleteLater();
    }
}

}
