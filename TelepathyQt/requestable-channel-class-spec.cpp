/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#include <TelepathyQt/RequestableChannelClassSpec>

namespace Tp
{

struct TP_QT_NO_EXPORT RequestableChannelClassSpec::Private : public QSharedData
{
    Private(const RequestableChannelClass &rcc)
        : rcc(rcc) {}

    RequestableChannelClass rcc;
};

/**
 * \class RequestableChannelClassSpec
 * \ingroup wrappers
 * \headerfile TelepathyQt/requestable-channel-class-spec.h <TelepathyQt/RequestableChannelClassSpec>
 *
 * \brief The RequestableChannelClassSpec class represents a Telepathy
 * requestable channel class.
 */

RequestableChannelClassSpec::RequestableChannelClassSpec(const RequestableChannelClass &rcc)
    : mPriv(new Private(rcc))
{
}

RequestableChannelClassSpec::RequestableChannelClassSpec()
{
}

RequestableChannelClassSpec::RequestableChannelClassSpec(const RequestableChannelClassSpec &other)
    : mPriv(other.mPriv)
{
}

RequestableChannelClassSpec::~RequestableChannelClassSpec()
{
}

RequestableChannelClassSpec RequestableChannelClassSpec::textChat()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::textChatroom()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeRoom);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::audioCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::audioCallWithVideoAllowed()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::videoCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::videoCallWithAudioAllowed()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::streamedMediaCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::streamedMediaAudioCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::streamedMediaVideoCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::streamedMediaVideoCallWithAudio()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::fileTransfer()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChat()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChatWithInvitees()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeHandles"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChatroom()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeRoom);

        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChatroomWithInvitees()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeRoom);

        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeHandles"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceStreamedMediaCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceStreamedMediaCallWithInvitees()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeHandles"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearch()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearchWithSpecificServer()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH + QLatin1String(".Server"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearchWithLimit()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH + QLatin1String(".Limit"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearchWithSpecificServerAndLimit()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH + QLatin1String(".Server"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH + QLatin1String(".Limit"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::dbusTube(const QString &serviceName)
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    if (serviceName.isEmpty()) {
        return spec;
    }

    RequestableChannelClass rcc = spec.bareClass();
    rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
            serviceName);
    return RequestableChannelClassSpec(rcc);
}

RequestableChannelClassSpec RequestableChannelClassSpec::streamTube(const QString &service)
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    if (service.isEmpty()) {
        return spec;
    }

    RequestableChannelClass rcc = spec.bareClass();
    rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE + QLatin1String(".Service"),
            service);
    return RequestableChannelClassSpec(rcc);
}

RequestableChannelClassSpec &RequestableChannelClassSpec::operator=(const RequestableChannelClassSpec &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool RequestableChannelClassSpec::operator==(const RequestableChannelClassSpec &other) const
{
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return true;
        }
        return false;
    }

    return mPriv->rcc == other.mPriv->rcc;
}

bool RequestableChannelClassSpec::supports(const RequestableChannelClassSpec &other) const
{
    if (!isValid()) {
        return false;
    }

    if (mPriv->rcc.fixedProperties == other.fixedProperties()) {
        foreach (const QString &prop, other.allowedProperties()) {
            if (!mPriv->rcc.allowedProperties.contains(prop)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

QString RequestableChannelClassSpec::channelType() const
{
    if (!isValid()) {
        return QString();
    }
    return qdbus_cast<QString>(mPriv->rcc.fixedProperties.value(
                TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")));
}

bool RequestableChannelClassSpec::hasTargetHandleType() const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"));
}

HandleType RequestableChannelClassSpec::targetHandleType() const
{
    if (!hasTargetHandleType()) {
        return (HandleType) -1;
    }
    return (HandleType) qdbus_cast<uint>(mPriv->rcc.fixedProperties.value(
                TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")));
}

bool RequestableChannelClassSpec::hasFixedProperty(const QString &name) const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(name);
}

QVariant RequestableChannelClassSpec::fixedProperty(const QString &name) const
{
    if (!isValid()) {
        return QVariant();
    }
    return mPriv->rcc.fixedProperties.value(name);
}

QVariantMap RequestableChannelClassSpec::fixedProperties() const
{
    if (!isValid()) {
        return QVariantMap();
    }
    return mPriv->rcc.fixedProperties;
}

bool RequestableChannelClassSpec::allowsProperty(const QString &name) const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.allowedProperties.contains(name);
}

QStringList RequestableChannelClassSpec::allowedProperties() const
{
    if (!isValid()) {
        return QStringList();
    }
    return mPriv->rcc.allowedProperties;
}

RequestableChannelClass RequestableChannelClassSpec::bareClass() const
{
    return isValid() ? mPriv->rcc : RequestableChannelClass();
}

/**
 * \class RequestableChannelClassSpecList
 * \ingroup wrappers
 * \headerfile TelepathyQt/requestable-channel-class-spec.h <TelepathyQt/RequestableChannelClassSpecList>
 *
 * \brief The RequestableChannelClassSpecList class represents a list of
 * RequestableChannelClassSpec.
 */

} // Tp
