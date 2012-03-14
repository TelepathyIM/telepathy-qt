/*
 * This file is part of TelepathyQt4Yell
 *
 * Copyright (C) 2009-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4Yell_Farstream_channel_h_HEADER_GUARD_
#define _TelepathyQt4Yell_Farstream_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_YELL_FARSTREAM_HEADER
#error IN_TELEPATHY_QT4_YELL_FARSTREAM_HEADER
#endif

#include <TelepathyQt4Yell/Farstream/Global>
#include <TelepathyQt4Yell/Farstream/Types>

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/RefCounted>

#include <telepathy-farstream/channel.h>

namespace Tpy
{

class TELEPATHY_QT4_YELL_FS_EXPORT PendingTfChannel : public Tp::PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingTfChannel)

public:
    ~PendingTfChannel();

    TfChannel *tfChannel() const;

private:
    friend class FarstreamChannelFactory;

    PendingTfChannel(const FarstreamChannelFactoryPtr &fsc, const CallChannelPtr &channel);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_YELL_FS_EXPORT FarstreamChannelFactory : public Tp::RefCounted
{
    Q_DISABLE_COPY(FarstreamChannelFactory)

public:
    static FarstreamChannelFactoryPtr create();
    ~FarstreamChannelFactory();

    PendingTfChannel *createTfChannel(const CallChannelPtr &channel);

private:
    FarstreamChannelFactory();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tpy

#endif
