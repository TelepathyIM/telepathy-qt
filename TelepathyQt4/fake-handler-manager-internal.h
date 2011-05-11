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

#ifndef _TelepathyQt4_fake_handler_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt4_fake_handler_manager_internal_h_HEADER_GUARD_

#include <QObject>

#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Types>

namespace Tp
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS

class FakeHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FakeHandler)

public:
    FakeHandler(const ClientRegistrarPtr &cr, const ChannelPtr &channel);
    ~FakeHandler();

private Q_SLOTS:
    void onChannelInvalidated();
    void onChannelDestroyed();

private:
    ClientRegistrarPtr mCr;
};

class FakeHandlerManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FakeHandlerManager)

public:
    static FakeHandlerManager *instance();

    ~FakeHandlerManager();

    void registerHandler(const ClientRegistrarPtr &registrar,
            const ChannelPtr &channel);

private Q_SLOTS:
    void onFakeHandlerDestroyed(QObject *obj);

private:
    FakeHandlerManager();

    static FakeHandlerManager *mInstance;
    QSet<FakeHandler *> mFakeHandlers;
};

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // Tp

#endif
