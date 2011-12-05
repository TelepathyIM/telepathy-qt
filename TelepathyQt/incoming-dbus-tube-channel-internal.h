/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_incoming_dbus_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt_incoming_dbus_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt/IncomingDBusTubeChannel>

namespace Tp
{

struct TP_QT_NO_EXPORT IncomingDBusTubeChannel::Private
{
public:
    Private(IncomingDBusTubeChannel* parent);
    virtual ~Private();

    // Public object
    IncomingDBusTubeChannel *parent;

    QString address;
};

}

#endif
