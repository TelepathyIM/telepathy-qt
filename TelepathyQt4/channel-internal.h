/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/PendingOperation>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT Channel::PendingLeave : public PendingOperation
{
    Q_OBJECT

public:
    PendingLeave(const ChannelPtr &channel, const QString &message,
            ChannelGroupChangeReason reason);

private Q_SLOTS:
    void onChanInvalidated(Tp::DBusProxy *proxy);
    void onRemoveFinished(Tp::PendingOperation *);
    void onCloseFinished(Tp::PendingOperation *);
};

} // Tp

#endif
