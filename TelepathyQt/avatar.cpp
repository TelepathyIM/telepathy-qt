/**
 * This file is part of TelepathyQt
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

#include <TelepathyQt/AvatarSpec>

namespace Tp
{

/**
 * \class AvatarData
 * \ingroup wrappers
 * \headerfile TelepathyQt/avatar.h <TelepathyQt/AvatarData>
 *
 * \brief The AvatarData class represents a Telepathy avatar.
 */

struct TP_QT_NO_EXPORT AvatarSpec::Private : public QSharedData
{
    Private(const QStringList &supportedMimeTypes,
            uint minHeight, uint maxHeight, uint recommendedHeight,
            uint minWidth, uint maxWidth, uint recommendedWidth,
            uint maxBytes)
        : supportedMimeTypes(supportedMimeTypes),
          minHeight(minHeight),
          maxHeight(maxHeight),
          recommendedHeight(recommendedHeight),
          minWidth(minWidth),
          maxWidth(maxWidth),
          recommendedWidth(recommendedWidth),
          maxBytes(maxBytes)
    {
    }

    QStringList supportedMimeTypes;
    uint minHeight;
    uint maxHeight;
    uint recommendedHeight;
    uint minWidth;
    uint maxWidth;
    uint recommendedWidth;
    uint maxBytes;
};

/**
 * \class AvatarSpec
 * \ingroup wrappers
 * \headerfile TelepathyQt/avatar.h <TelepathyQt/AvatarSpec>
 *
 * \brief The AvatarSpec class represents a Telepathy avatar information
 * supported by a protocol.
 */

AvatarSpec::AvatarSpec()
{
}

AvatarSpec::AvatarSpec(const QStringList &supportedMimeTypes,
            uint minHeight, uint maxHeight, uint recommendedHeight,
            uint minWidth, uint maxWidth, uint recommendedWidth,
            uint maxBytes)
    : mPriv(new Private(supportedMimeTypes, minHeight, maxHeight, recommendedHeight,
                minWidth, maxWidth, recommendedWidth, maxBytes))
{
}

AvatarSpec::AvatarSpec(const AvatarSpec &other)
    : mPriv(other.mPriv)
{
}

AvatarSpec::~AvatarSpec()
{
}

AvatarSpec &AvatarSpec::operator=(const AvatarSpec &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

QStringList AvatarSpec::supportedMimeTypes() const
{
    if (!isValid()) {
        return QStringList();
    }

    return mPriv->supportedMimeTypes;
}

uint AvatarSpec::minimumHeight() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->minHeight;
}

uint AvatarSpec::maximumHeight() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->maxHeight;
}

uint AvatarSpec::recommendedHeight() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->recommendedHeight;
}

uint AvatarSpec::minimumWidth() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->minWidth;
}

uint AvatarSpec::maximumWidth() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->maxWidth;
}

uint AvatarSpec::recommendedWidth() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->recommendedWidth;
}

uint AvatarSpec::maximumBytes() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->maxBytes;
}

} // Tp
