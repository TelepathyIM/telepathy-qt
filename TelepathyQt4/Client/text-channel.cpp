/* Text channel client-side proxy
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

#include <TelepathyQt4/Client/TextChannel>

#include "TelepathyQt4/Client/_gen/text-channel.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct TextChannel::Private
{
    inline Private();
    inline ~Private();
};

TextChannel::Private::Private()
{
}

TextChannel::Private::~Private()
{
}

/**
 * \class TextChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/text-channel.h <TelepathyQt4/Client/TextChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the Text
 * channel type.
 *
 * This subclass of Channel will eventually provide a high-level API for the
 * Text and Messages interface. Until then, it's just a Channel.
 */

/**
 * Creates a TextChannel associated with the given object on the same service
 * as the given connection.
 *
 * \param connection  Connection owning this TextChannel, and specifying the
 *                    service.
 * \param objectPath  Path to the object on the service.
 * \param parent      Passed to the parent class constructor.
 */
TextChannel::TextChannel(Connection *connection,
        const QString &objectPath,
        QObject *parent)
    : Channel(connection, objectPath, parent),
      mPriv(new Private())
{
}

/**
 * Class destructor.
 */
TextChannel::~TextChannel()
{
    delete mPriv;
}

} // Telepathy::Client
} // Telepathy
