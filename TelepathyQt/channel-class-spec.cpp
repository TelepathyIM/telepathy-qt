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

#include <TelepathyQt/ChannelClassSpec>

#include "TelepathyQt/_gen/future-constants.h"

#include "TelepathyQt/debug-internal.h"

namespace Tp
{

struct TP_QT_NO_EXPORT ChannelClassSpec::Private : public QSharedData
{
    QVariantMap props;
};

/**
 * \class ChannelClassSpec
 * \ingroup wrappers
 * \headerfile TelepathyQt/channel-class-spec.h <TelepathyQt/ChannelClassSpec>
 *
 * \brief The ChannelClassSpec class represents a Telepathy channel class.
 */

ChannelClassSpec::ChannelClassSpec()
{
}

ChannelClassSpec::ChannelClassSpec(const ChannelClass &cc)
    : mPriv(new Private)
{
    foreach (QString key, cc.keys()) {
        setProperty(key, cc.value(key).variant());
    }
}

ChannelClassSpec::ChannelClassSpec(const QVariantMap &props)
    : mPriv(new Private)
{
    setChannelType(qdbus_cast<QString>(
                props.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))));
    setTargetHandleType((HandleType) qdbus_cast<uint>(
                props.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"))));

    foreach (QString propName, props.keys()) {
        setProperty(propName, props.value(propName));
    }
}

ChannelClassSpec::ChannelClassSpec(const QString &channelType, HandleType targetHandleType,
        const QVariantMap &otherProperties)
    : mPriv(new Private)
{
    setChannelType(channelType);
    setTargetHandleType(targetHandleType);
    foreach (QString key, otherProperties.keys()) {
        setProperty(key, otherProperties.value(key));
    }
}

ChannelClassSpec::ChannelClassSpec(const QString &channelType, HandleType targetHandleType,
        bool requested, const QVariantMap &otherProperties)
    : mPriv(new Private)
{
    setChannelType(channelType);
    setTargetHandleType(targetHandleType);
    setRequested(requested);
    foreach (QString key, otherProperties.keys()) {
        setProperty(key, otherProperties.value(key));
    }
}

ChannelClassSpec::ChannelClassSpec(const ChannelClassSpec &other,
        const QVariantMap &additionalProperties)
    : mPriv(other.mPriv)
{
    if (!additionalProperties.isEmpty()) {
        foreach (QString key, additionalProperties.keys()) {
            setProperty(key, additionalProperties.value(key));
        }
    }
}

ChannelClassSpec::~ChannelClassSpec()
{
}

bool ChannelClassSpec::isValid() const
{
    return mPriv.constData() != 0 &&
        !(qdbus_cast<QString>(
                    mPriv->props.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")))
                .isEmpty()) &&
        mPriv->props.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"));
}

ChannelClassSpec &ChannelClassSpec::operator=(const ChannelClassSpec &other)
{
    if (this == &other) {
        return *this;
    }

    this->mPriv = other.mPriv;
    return *this;
}

bool ChannelClassSpec::isSubsetOf(const ChannelClassSpec &other) const
{
    if (!mPriv) {
        // Invalid instances have no properties - hence they're subset of anything
        return true;
    }

    foreach (QString propName, mPriv->props.keys()) {
        if (!other.hasProperty(propName)) {
            return false;
        } else if (property(propName) != other.property(propName)) {
            return false;
        }
    }

    // other had all of the properties we have and they all had the same values

    return true;
}

bool ChannelClassSpec::matches(const QVariantMap &immutableProperties) const
{
    // We construct a ChannelClassSpec for comparison so the StreamedMedia props are normalized
    // consistently etc
    return this->isSubsetOf(ChannelClassSpec(immutableProperties));
}

bool ChannelClassSpec::hasProperty(const QString &qualifiedName) const
{
    return mPriv.constData() != 0 ? mPriv->props.contains(qualifiedName) : false;
}

QVariant ChannelClassSpec::property(const QString &qualifiedName) const
{
    return mPriv.constData() != 0 ? mPriv->props.value(qualifiedName) : QVariant();
}

void ChannelClassSpec::setProperty(const QString &qualifiedName, const QVariant &value)
{
    if (mPriv.constData() == 0) {
        mPriv = new Private;
    }

    mPriv->props.insert(qualifiedName, value);
}

void ChannelClassSpec::unsetProperty(const QString &qualifiedName)
{
    if (mPriv.constData() == 0) {
        // No properties set for sure, so don't have to unset any
        return;
    }

    mPriv->props.remove(qualifiedName);
}

QVariantMap ChannelClassSpec::allProperties() const
{
    return mPriv.constData() != 0 ? mPriv->props : QVariantMap();
}

ChannelClass ChannelClassSpec::bareClass() const
{
    ChannelClass cc;

    if (!isValid()) {
        warning() << "Tried to convert an invalid ChannelClassSpec to a ChannelClass";
        return ChannelClass();
    }

    QVariantMap props = mPriv->props;
    foreach (QString propName, props.keys()) {
        QVariant value = props.value(propName);

        cc.insert(propName, QDBusVariant(value));
    }

    return cc;
}

ChannelClassSpec ChannelClassSpec::textChat(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                HandleTypeContact);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::textChatroom(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                HandleTypeRoom);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::unnamedTextChat(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                HandleTypeNone);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::mediaCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_CALL, HandleTypeContact);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::audioCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_CALL, HandleTypeContact);
        spec.setCallInitialAudioFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::videoCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_CALL, HandleTypeContact);
        spec.setCallInitialVideoFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::videoCallWithAudio(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_CALL, HandleTypeContact);
        spec.setCallInitialAudioFlag();
        spec.setCallInitialVideoFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::streamedMediaCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeContact);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::streamedMediaAudioCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeContact);
        spec.setStreamedMediaInitialAudioFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::streamedMediaVideoCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeContact);
        spec.setStreamedMediaInitialVideoFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::streamedMediaVideoCallWithAudio(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeContact);
        spec.setStreamedMediaInitialAudioFlag();
        spec.setStreamedMediaInitialVideoFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::unnamedStreamedMediaCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeNone);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::unnamedStreamedMediaAudioCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeNone);
        spec.setStreamedMediaInitialAudioFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::unnamedStreamedMediaVideoCall(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeNone);
        spec.setStreamedMediaInitialVideoFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                HandleTypeNone);
        spec.setStreamedMediaInitialAudioFlag();
        spec.setStreamedMediaInitialVideoFlag();
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

Tp::ChannelClassSpec ChannelClassSpec::serverAuthentication(const QVariantMap &additionalProperties)
{
    static Tp::ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION,
                HandleTypeNone);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::roomList(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_ROOM_LIST,
                HandleTypeNone);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::outgoingFileTransfer(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER,
                HandleTypeContact, true);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::incomingFileTransfer(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER,
                HandleTypeContact, false);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::outgoingStreamTube(const QString &service,
                                                      const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE,
                HandleTypeContact, true);
    }

    QVariantMap props = additionalProperties;
    if (!service.isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE + QLatin1String(".Service"),
                service);
    }

    if (props.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, props);
    }
}

ChannelClassSpec ChannelClassSpec::incomingStreamTube(const QString &service,
                                                      const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE,
                HandleTypeContact, false);
    }

    QVariantMap props = additionalProperties;
    if (!service.isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE + QLatin1String(".Service"),
                service);
    }

    if (props.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, props);
    }
}

ChannelClassSpec ChannelClassSpec::outgoingRoomStreamTube(const QString &service,
                                                          const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE,
                HandleTypeRoom, true);
    }

    QVariantMap props = additionalProperties;
    if (!service.isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE + QLatin1String(".Service"),
                service);
    }

    if (props.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, props);
    }
}

ChannelClassSpec ChannelClassSpec::incomingRoomStreamTube(const QString &service,
                                                          const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE,
                HandleTypeRoom, false);
    }

    QVariantMap props = additionalProperties;
    if (!service.isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE + QLatin1String(".Service"),
                service);
    }

    if (props.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, props);
    }
}

ChannelClassSpec ChannelClassSpec::outgoingDBusTube(const QString &serviceName,
                                                    const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE,
                HandleTypeContact, true);
    }

    if (!serviceName.isEmpty()) {
        spec.setProperty(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
                         serviceName);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::incomingDBusTube(const QString &serviceName,
                                                    const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE,
                HandleTypeContact, false);
    }

    if (!serviceName.isEmpty()) {
        spec.setProperty(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
                         serviceName);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::outgoingRoomDBusTube(const QString &serviceName,
                                                        const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE,
                HandleTypeRoom, true);
    }

    QVariantMap props = additionalProperties;
    if (!serviceName.isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
                serviceName);
    }

    if (props.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, props);
    }
}

ChannelClassSpec ChannelClassSpec::incomingRoomDBusTube(const QString &serviceName,
                                                        const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE,
                HandleTypeRoom, false);
    }

    QVariantMap props = additionalProperties;
    if (!serviceName.isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
                serviceName);
    }

    if (props.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, props);
    }
}

ChannelClassSpec ChannelClassSpec::contactSearch(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.mPriv.constData()) {
        spec = ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH,
                HandleTypeNone);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

/**
 * \class ChannelClassSpecList
 * \ingroup wrappers
 * \headerfile TelepathyQt/channel-class-spec.h <TelepathyQt/ChannelClassSpecList>
 *
 * \brief The ChannelClassSpecList class represents a list of ChannelClassSpec.
 */

} // Tp
