/*
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/CapabilitiesBase>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

namespace Tp
{

struct CapabilitiesBase::Private
{
    Private(const RequestableChannelClassList &classes);

    RequestableChannelClassList classes;
};

CapabilitiesBase::Private::Private(const RequestableChannelClassList &classes)
    : classes(classes)
{
}

CapabilitiesBase::CapabilitiesBase(const RequestableChannelClassList &classes)
    : mPriv(new Private(classes))
{
}

CapabilitiesBase::~CapabilitiesBase()
{
    delete mPriv;
}

RequestableChannelClassList CapabilitiesBase::requestableChannelClasses() const
{
    return mPriv->classes;
}

void CapabilitiesBase::setRequestableChannelClasses(
        const RequestableChannelClassList &classes)
{
    mPriv->classes = classes;
}

bool CapabilitiesBase::supportsTextChats() const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &class_, mPriv->classes) {
        if (!class_.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT &&
            targetHandleType == HandleTypeContact) {
            return true;
        }
    }
    return false;
}

bool CapabilitiesBase::supportsMediaCalls() const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &class_, mPriv->classes) {
        if (!class_.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            targetHandleType == HandleTypeContact) {
            return true;
        }
    }
    return false;
}

bool CapabilitiesBase::supportsAudioCalls() const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &class_, mPriv->classes) {
        if (!class_.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            targetHandleType == HandleTypeContact &&
            class_.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio"))) {
                return true;
        }
    }
    return false;
}

bool CapabilitiesBase::supportsVideoCalls(bool withAudio) const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &class_, mPriv->classes) {
        if (!class_.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            targetHandleType == HandleTypeContact &&
            class_.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialVideo")) &&
            (!withAudio || class_.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio")))) {
                return true;
        }
    }
    return false;
}

bool CapabilitiesBase::supportsUpgradingCalls() const
{
    QString channelType;
    foreach (const RequestableChannelClass &class_, mPriv->classes) {
        channelType = qdbus_cast<QString>(class_.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            !class_.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".ImmutableStreams"))) {
                // TODO should we test all classes that have channelType
                //      StreamedMedia or just one is fine?
                return true;
        }
    }
    return false;
}

} // Tp
