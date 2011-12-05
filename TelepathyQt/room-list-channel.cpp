/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/RoomListChannel>

#include "TelepathyQt/_gen/room-list-channel.moc.hpp"
#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>

namespace Tp
{

struct TP_QT_NO_EXPORT RoomListChannel::Private
{
    inline Private();
    inline ~Private();
};

RoomListChannel::Private::Private()
{
}

RoomListChannel::Private::~Private()
{
}

/**
 * \class RoomListChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/room-list-channel.h <TelepathyQt/RoomListChannel>
 *
 * \brief The RoomListChannel class represents a Telepathy Channel of type RoomList.
 *
 * Note that this subclass of Channel will eventually provide a high-level API for the
 * RoomList interface. Until then, it's just a Channel.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Create a new RoomListChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A RoomListChannelPtr object pointing to the newly created
 *         RoomListChannel object.
 */
RoomListChannelPtr RoomListChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return RoomListChannelPtr(new RoomListChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new RoomListChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    should depend on Channel::FeatureCore.
 */
RoomListChannel::RoomListChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private())
{
}

/**
 * Class destructor.
 */
RoomListChannel::~RoomListChannel()
{
    delete mPriv;
}

} // Tp
