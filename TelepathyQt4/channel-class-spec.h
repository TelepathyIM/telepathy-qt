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

#ifndef _TelepathyQt4_channel_class_spec_h_HEADER_GUARD_
#define _TelepathyQt4_channel_class_spec_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Global>
#include <TelepathyQt4/Types>

#include <QSharedDataPointer>
#include <QVariant>

namespace Tp
{

class TELEPATHY_QT4_EXPORT ChannelClassSpec
{
public:
    ChannelClassSpec();
    ChannelClassSpec(const ChannelClass &cc);
    ChannelClassSpec(const QString &channelType, uint targetHandleType,
            const QVariantMap &otherProperties = QVariantMap());
    ChannelClassSpec(const QString &channelType, uint targetHandleType, bool requested,
            const QVariantMap &otherProperties = QVariantMap());
    ChannelClassSpec(const ChannelClassSpec &other,
            const QVariantMap &additionalProperties = QVariantMap());
    ~ChannelClassSpec();

    bool isValid() const;

    ChannelClassSpec &operator=(const ChannelClassSpec &other);

    bool operator<(const ChannelClassSpec &other) const;

    bool operator<=(const ChannelClassSpec &other) const
    {
        return !(other < *this);
    }

    bool operator==(const ChannelClassSpec &other) const
    {
        return !(other < *this) && !(*this < other);
    }

    bool operator>=(const ChannelClassSpec &other) const
    {
        return !(*this < other);
    }

    bool operator>(const ChannelClassSpec &other) const
    {
        return (other < *this);
    }

    bool matches(const QVariantMap &immutableProperties) const;

    QString channelType() const
    {
        return qdbus_cast<QString>(
                property(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
    }

    void setChannelType(const QString &type)
    {
        setProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                QVariant::fromValue(type));
    }

    uint targetHandleType() const
    {
        return qdbus_cast<uint>(
                property(
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
    }

    void setTargetHandleType(uint type)
    {
        setProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                QVariant::fromValue(type));
    }

    bool hasRequested() const
    {
        return hasProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested"));
    }

    bool isRequested() const
    {
        return qdbus_cast<bool>(
                property(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")));
    }

    void setRequested(bool requested)
    {
        setProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested"),
                QVariant::fromValue(requested));
    }

    void unsetRequested()
    {
        unsetProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested"));
    }

    bool hasInitialAudio() const
    {
        return qdbus_cast<bool>(
                property(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA
                        ".InitialAudio")));
    }

    void setInitialAudio()
    {
        setProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio"),
                QVariant::fromValue(true));
    }

    void unsetInitialAudio()
    {
        unsetProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA
                    ".InitialAudio"));
    }

    bool hasInitialVideo() const
    {
        return qdbus_cast<bool>(
                property(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA
                        ".InitialVideo")));
    }

    void setInitialVideo()
    {
        setProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialVideo"),
                QVariant::fromValue(true));
    }

    void unsetInitialVideo()
    {
        unsetProperty(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA
                    ".InitialVideo"));
    }

    bool hasProperty(const QString &qualifiedName) const;
    QVariant property(const QString &qualifiedName) const;

    void setProperty(const QString &qualifiedName, const QVariant &value);
    void unsetProperty(const QString &qualifiedName);

    QVariantMap allProperties() const;
    ChannelClass bareClass() const;

    static ChannelClassSpec text(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec textChatroom(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec media(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec mediaWithInitialAudio(const QVariantMap &additionalProperties =
            QVariantMap());
    static ChannelClassSpec mediaWithInitialVideo(const QVariantMap &additionalProperties =
            QVariantMap());
    // TODO: add Call when it's undrafted
    static ChannelClassSpec roomList(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec sendFile(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec receiveFile(const QVariantMap &additionalProperties = QVariantMap());
    // TODO: add dbus tubes, stream tubes when they're implemented

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TELEPATHY_QT4_EXPORT ChannelClassSpecList :
                public QList<ChannelClassSpec>
{
public:
    ChannelClassSpecList() { }

    ChannelClassSpecList(const ChannelClassSpec &spec)
    {
        append(spec);
    }

    ChannelClassSpecList(const QList<ChannelClassSpec> &other)
        : QList<ChannelClassSpec>(other)
    {
    }

    ChannelClassSpecList(const ChannelClassList &classes)
    {
        // Why doesn't Qt have range constructors like STL... stupid, so stupid.
        Q_FOREACH (const ChannelClass &cc, classes) {
            append(cc);
        }
    }
};

} // Tp

#endif
