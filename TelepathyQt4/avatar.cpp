/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt4/AvatarSpec>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT AvatarSpec::Private : public QSharedData
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
