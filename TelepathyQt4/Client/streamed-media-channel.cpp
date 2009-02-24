/* StreamedMedia channel client-side proxy
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt4/Client/StreamedMediaChannel>

#include "TelepathyQt4/Client/_gen/streamed-media-channel.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct StreamedMediaChannel::Private
{
    inline Private();
    inline ~Private();
};

StreamedMediaChannel::Private::Private()
{
}

StreamedMediaChannel::Private::~Private()
{
}

/**
 * \class StreamedMediaChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/streamed-media-channel.h <TelepathyQt4/Client/StreamedMediaChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the
 * StreamedMedia channel type.
 *
 * This subclass of Channel will eventually provide a high-level API for the
 * StreamedMedia interface. Until then, it's just a Channel.
 */

/**
 * Creates a StreamedMediaChannel associated with the given object on the same
 * service as the given connection.
 *
 * \param connection  Connection owning this StreamedMediaChannel, and
 *                    specifying the service.
 * \param objectPath  Path to the object on the service.
 * \param immutableProperties  The immutable properties of the channel, as
 *                             signalled by NewChannels or returned by
 *                             CreateChannel or EnsureChannel
 * \param parent      Passed to the parent class constructor.
 */
StreamedMediaChannel::StreamedMediaChannel(Connection *connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        QObject *parent)
    : Channel(connection, objectPath, immutableProperties, parent),
      mPriv(new Private())
{
}

/**
 * Class destructor.
 */
StreamedMediaChannel::~StreamedMediaChannel()
{
    delete mPriv;
}

} // Telepathy::Client
} // Telepathy
