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

#ifndef _TelepathyQt_fake_handler_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt_fake_handler_manager_internal_h_HEADER_GUARD_

#include <QObject>

#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Types>

namespace Tp
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS

class FakeHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FakeHandler)

public:
    FakeHandler(const QDBusConnection &bus);
    ~FakeHandler();

    QDBusConnection dbusConnection() const { return mBus; }
    ObjectPathList handledChannels() const;
    void registerChannel(const ChannelPtr &channel);

Q_SIGNALS:
    void invalidated(Tp::FakeHandler *self);

private Q_SLOTS:
    void onChannelInvalidated(Tp::DBusProxy *channel);
    void onChannelDestroyed(QObject *channel);

private:
    QDBusConnection mBus;
    QSet<Channel*> mChannels;
};

class FakeHandlerManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FakeHandlerManager)

public:
    static FakeHandlerManager *instance();

    ~FakeHandlerManager();

    ObjectPathList handledChannels(const QDBusConnection &bus) const;
    void registerClientRegistrar(const ClientRegistrarPtr &cr);
    void registerChannels(const QList<ChannelPtr> &channels);

private Q_SLOTS:
    void onFakeHandlerInvalidated(Tp::FakeHandler *fakeHandler);

private:
    FakeHandlerManager();

    static FakeHandlerManager *mInstance;
    QHash<QPair<QString, QString>, ClientRegistrarPtr> mClientRegistrars;
    QHash<QPair<QString, QString>, FakeHandler *> mFakeHandlers;
};

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // Tp

#endif
