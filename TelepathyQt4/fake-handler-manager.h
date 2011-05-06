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

#ifndef _TelepathyQt4_fake_handler_manager_h_HEADER_GUARD_
#define _TelepathyQt4_fake_handler_manager_h_HEADER_GUARD_

#include <QObject>

#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Types>

namespace Tp
{

class FakeHandler : public QObject
{
    Q_OBJECT

public:

    FakeHandler();

    void addChannel(const ChannelPtr &channel,
                    const ClientRegistrarPtr &registrar);

private Q_SLOTS:

    void onChannelInvalidated();

private:

    int mNumChannels;
    ClientRegistrarPtr mRegistrar;

};


class FakeHandlerManager : public QObject
{
    Q_OBJECT

public:

    static FakeHandlerManager *instance();

    void registerHandler(const QString &connectionName,
                         const ChannelPtr &channel,
                         const ClientRegistrarPtr &registrar);

private:

    FakeHandlerManager();

    QHash<QString, FakeHandler *> mFakeHandlers;

};

}

#endif // _TelepathyQt4_fake_handler_manager_h_HEADER_GUARD_
