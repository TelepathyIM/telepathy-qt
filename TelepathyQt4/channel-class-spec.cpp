/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#include <TelepathyQt4/ChannelClassSpec>

#include "TelepathyQt4/_gen/future-constants.h"
#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ChannelClassSpec::Private : public QSharedData
{
    QVariantMap props;
};

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
                props.value(TP_QT4_IFACE_CHANNEL + QLatin1String(".ChannelType"))));
    setTargetHandleType((HandleType) qdbus_cast<uint>(
                props.value(TP_QT4_IFACE_CHANNEL + QLatin1String(".TargetHandleType"))));

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
    foreach (QString key, additionalProperties.keys()) {
        setProperty(key, additionalProperties.value(key));
    }
}

ChannelClassSpec::~ChannelClassSpec()
{
}

bool ChannelClassSpec::isValid() const
{
    return mPriv.constData() != 0 && 
        !(qdbus_cast<QString>(
                    mPriv->props.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")))
                .isEmpty()) &&
        mPriv->props.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"));
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

    // Flatten the InitialAudio/Video properties from the different media interfaces to one
    // namespace - we convert back to the correct interface when this is converted back to a
    // ChannelClass for use in e.g. client channel filters

    QString propName = qualifiedName;

    // API/ABI break TODO: remove this hack when the Call.DRAFT support hack is removed from
    // StreamedMediaChannel
    if (propName == TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio")
            || propName == TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo")) {
        propName.replace(TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL,
                TP_QT4_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    }

    mPriv->props.insert(propName, value);
}

void ChannelClassSpec::unsetProperty(const QString &qualifiedName)
{
    if (mPriv.constData() == 0) {
        // No properties set for sure, so don't have to unset any
        return;
    }

    QString propName = qualifiedName;

    // API/ABI break TODO: remove this hack when the Call.DRAFT support hack is removed from
    // StreamedMediaChannel
    if (propName == TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio")
            || propName == TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo")) {
        propName.replace(TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL,
                TP_QT4_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    }

    mPriv->props.remove(propName);
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

        // API/ABI break TODO: remove this hack when the Call.DRAFT support hack is removed from
        // StreamedMediaChannel
        if (channelType() == TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL) {
            if (propName.endsWith(QLatin1String(".InitialAudio"))
                    || propName.endsWith(QLatin1String(".InitialVideo"))) {
                propName.replace(TP_QT4_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
                        TP_QT4_FUTURE_IFACE_CHANNEL_TYPE_CALL);
            }
        }

        cc.insert(propName, QDBusVariant(value));
    }

    return cc;
}

ChannelClassSpec ChannelClassSpec::textChat(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT),
                HandleTypeNone);
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA),
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

ChannelClassSpec ChannelClassSpec::roomList(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_ROOM_LIST),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER),
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

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER),
                HandleTypeContact, false);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

ChannelClassSpec ChannelClassSpec::contactSearch(const QVariantMap &additionalProperties)
{
    static ChannelClassSpec spec;

    if (!spec.isValid()) {
        spec = ChannelClassSpec(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH),
                HandleTypeNone);
    }

    if (additionalProperties.isEmpty()) {
        return spec;
    } else {
        return ChannelClassSpec(spec, additionalProperties);
    }
}

} // Tp
