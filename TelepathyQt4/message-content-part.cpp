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

#include <TelepathyQt4/MessageContentPart>

#include <QSharedData>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT MessageContentPart::Private : public QSharedData
{
    Private(const MessagePart &mp)
        : mp(mp) {}

    MessagePart mp;
};

/**
 * \class MessageContentPart
 * \ingroup wrappers
 * \headerfile TelepathyQt4/message-content-part.h <TelepathyQt4/MessageContentPart>
 *
 * \brief The MessageContentPart class provides an object representing a wrapper
 * around MessagePart.
 */

MessageContentPart::MessageContentPart()
{
}

MessageContentPart::MessageContentPart(const MessagePart &mp)
    : mPriv(new Private(mp))
{
}

MessageContentPart::MessageContentPart(const MessageContentPart &other)
    : mPriv(other.mPriv)
{
}

MessageContentPart::~MessageContentPart()
{
}

MessageContentPart &MessageContentPart::operator=(const MessageContentPart &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool MessageContentPart::operator==(const MessageContentPart &other) const
{
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return true;
        }
        return false;
    }

    return mPriv->mp == other.mPriv->mp;
}

MessagePart MessageContentPart::barePart() const
{
    return isValid() ? mPriv->mp : MessagePart();
}

/**
 * \class MessageContentPartList
 * \ingroup wrappers
 * \headerfile TelepathyQt4/message-content-part.h <TelepathyQt4/MessageContentPartList>
 *
 * \brief The MessageContentPartList class an object representing a list of
 * MessageContentPart.
 */

} // Tp
