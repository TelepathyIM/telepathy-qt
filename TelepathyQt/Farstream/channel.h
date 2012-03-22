/**
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2009-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_Farstream_channel_h_HEADER_GUARD_
#define _TelepathyQt_Farstream_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_FARSTREAM_HEADER
#error IN_TP_QT_FARSTREAM_HEADER
#endif

#include <TelepathyQt/Farstream/Global>
#include <TelepathyQt/Types>

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/RefCounted>

typedef struct _TfChannel TfChannel;

namespace Tp
{
namespace Farstream
{

class TP_QT_FS_EXPORT PendingChannel : public Tp::PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingChannel)

public:
    ~PendingChannel();

    TfChannel *tfChannel() const;
    CallChannelPtr callChannel() const;

private:
    TP_QT_FS_NO_EXPORT PendingChannel(const CallChannelPtr &channel);

    friend PendingChannel *createChannel(const CallChannelPtr &channel);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

TP_QT_FS_EXPORT PendingChannel *createChannel(const CallChannelPtr &channel);

} // Farstream
} // Tp

#endif
