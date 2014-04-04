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
    Private(const TpDBus::RequestableChannelClass &rcc)
        : rcc(rcc) {}

    TpDBus::RequestableChannelClass rcc;
};

/**
 * \class RequestableChannelClassSpec
 * \ingroup wrappers
 * \headerfile TelepathyQt/requestable-channel-class-spec.h <TelepathyQt/RequestableChannelClassSpec>
 *
 * \brief The RequestableChannelClassSpec class represents a Telepathy
 * requestable channel class.
 */

RequestableChannelClassSpec::RequestableChannelClassSpec(const TpDBus::RequestableChannelClass &rcc)
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
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::textChatroom()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeRoom);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::audioCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialAudio"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialAudioName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::audioCallWithVideoAllowed()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialAudio"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialAudioName"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialVideo"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialVideoName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::videoCall()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialVideo"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialVideoName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::videoCallWithAudioAllowed()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialVideo"),
                true);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialVideoName"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialAudio"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL1 + QLatin1String(".InitialAudioName"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::fileTransfer()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChat()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE1 + QLatin1String(".InitialChannels"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChatWithInvitees()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE1 + QLatin1String(".InitialChannels"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE1 + QLatin1String(".InitialInviteeHandles"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChatroom()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeRoom);

        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE1 + QLatin1String(".InitialChannels"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::conferenceTextChatroomWithInvitees()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeRoom);

        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE1 + QLatin1String(".InitialChannels"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE1 + QLatin1String(".InitialInviteeHandles"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearch()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1);
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearchWithSpecificServer()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1 + QLatin1String(".Server"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearchWithLimit()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1 + QLatin1String(".Limit"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::contactSearchWithSpecificServerAndLimit()
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1 + QLatin1String(".Server"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH1 + QLatin1String(".Limit"));
        spec = RequestableChannelClassSpec(rcc);
    }

    return spec;
}

RequestableChannelClassSpec RequestableChannelClassSpec::dbusTube(const QString &serviceName)
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    if (serviceName.isEmpty()) {
        return spec;
    }

    TpDBus::RequestableChannelClass rcc = spec.bareClass();
    rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE1 + QLatin1String(".ServiceName"),
            serviceName);
    return RequestableChannelClassSpec(rcc);
}

RequestableChannelClassSpec RequestableChannelClassSpec::streamTube(const QString &service)
{
    static RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        TpDBus::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE1);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"),
                (uint) EntityTypeContact);
        spec = RequestableChannelClassSpec(rcc);
    }

    if (service.isEmpty()) {
        return spec;
    }

    TpDBus::RequestableChannelClass rcc = spec.bareClass();
    rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE1 + QLatin1String(".Service"),
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

bool RequestableChannelClassSpec::hasTargetEntityType() const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType"));
}

EntityType RequestableChannelClassSpec::targetEntityType() const
{
    if (!hasTargetEntityType()) {
        return (EntityType) -1;
    }
    return (EntityType) qdbus_cast<uint>(mPriv->rcc.fixedProperties.value(
                TP_QT_IFACE_CHANNEL + QLatin1String(".TargetEntityType")));
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

TpDBus::RequestableChannelClass RequestableChannelClassSpec::bareClass() const
{
    return isValid() ? mPriv->rcc : TpDBus::RequestableChannelClass();
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
