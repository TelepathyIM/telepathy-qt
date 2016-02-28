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

#ifndef _TelepathyQt_channel_class_spec_h_HEADER_GUARD_
#define _TelepathyQt_channel_class_spec_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>

#include <QSharedDataPointer>
#include <QVariant>
#include <QVariantMap>
#include <QPair>

namespace Tp
{

class TP_QT_EXPORT ChannelClassSpec
{
public:
    ChannelClassSpec();
    ChannelClassSpec(const ChannelClass &cc);
    ChannelClassSpec(const QVariantMap &props);
    ChannelClassSpec(const QString &channelType, HandleType targetHandleType,
            const QVariantMap &otherProperties = QVariantMap());
    ChannelClassSpec(const QString &channelType, HandleType targetHandleType, bool requested,
            const QVariantMap &otherProperties = QVariantMap());
    ChannelClassSpec(const ChannelClassSpec &other,
            const QVariantMap &additionalProperties = QVariantMap());
    ~ChannelClassSpec();

    bool isValid() const;

    ChannelClassSpec &operator=(const ChannelClassSpec &other);

    bool operator==(const ChannelClassSpec &other) const
    {
        return this->allProperties() == other.allProperties();
    }

    bool isSubsetOf(const ChannelClassSpec &other) const;
    bool matches(const QVariantMap &immutableProperties) const;

    QString channelType() const
    {
        return qdbus_cast<QString>(
                property(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")));
    }

    void setChannelType(const QString &type)
    {
        setProperty(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                QVariant::fromValue(type));
    }

    HandleType targetHandleType() const
    {
        return (HandleType) qdbus_cast<uint>(
                property(
                    TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")));
    }

    void setTargetHandleType(HandleType type)
    {
        setProperty(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                QVariant::fromValue((uint) type));
    }

    bool hasRequested() const
    {
        return hasProperty(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"));
    }

    bool isRequested() const
    {
        return qdbus_cast<bool>(
                property(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")));
    }

    void setRequested(bool requested)
    {
        setProperty(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"),
                QVariant::fromValue(requested));
    }

    void unsetRequested()
    {
        unsetProperty(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"));
    }

    bool hasCallInitialAudioFlag() const
    {
        return qdbus_cast<bool>(
                property(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio")));
    }

    void setCallInitialAudioFlag()
    {
        setProperty(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                QVariant::fromValue(true));
    }

    void unsetCallInitialAudioFlag()
    {
        unsetProperty(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"));
    }

    bool hasCallInitialVideoFlag() const
    {
        return qdbus_cast<bool>(
                property(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo")));
    }

    void setCallInitialVideoFlag()
    {
        setProperty(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
                QVariant::fromValue(true));
    }

    void unsetCallInitialVideoFlag()
    {
        unsetProperty(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"));
    }

    TP_QT_DEPRECATED bool hasStreamedMediaInitialAudioFlag() const
    {
        return qdbus_cast<bool>(
                property(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio")));
    }

    TP_QT_DEPRECATED void setStreamedMediaInitialAudioFlag()
    {
        setProperty(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"),
                QVariant::fromValue(true));
    }

    TP_QT_DEPRECATED void unsetStreamedMediaInitialAudioFlag()
    {
        unsetProperty(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"));
    }

    TP_QT_DEPRECATED bool hasStreamedMediaInitialVideoFlag() const
    {
        return qdbus_cast<bool>(
                property(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo")));
    }

    TP_QT_DEPRECATED void setStreamedMediaInitialVideoFlag()
    {
        setProperty(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo"),
                QVariant::fromValue(true));
    }

    TP_QT_DEPRECATED void unsetStreamedMediaInitialVideoFlag()
    {
        unsetProperty(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo"));
    }

    bool hasProperty(const QString &qualifiedName) const;
    QVariant property(const QString &qualifiedName) const;

    void setProperty(const QString &qualifiedName, const QVariant &value);
    void unsetProperty(const QString &qualifiedName);

    QVariantMap allProperties() const;
    ChannelClass bareClass() const;

    static ChannelClassSpec textChat(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec textChatroom(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec unnamedTextChat(const QVariantMap &additionalProperties = QVariantMap());

    static ChannelClassSpec mediaCall(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec audioCall(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec videoCall(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec videoCallWithAudio(const QVariantMap &additionalProperties =
            QVariantMap());

    TP_QT_DEPRECATED static ChannelClassSpec streamedMediaCall(const QVariantMap &additionalProperties = QVariantMap());
    TP_QT_DEPRECATED static ChannelClassSpec streamedMediaAudioCall(const QVariantMap &additionalProperties =
            QVariantMap());
    TP_QT_DEPRECATED static ChannelClassSpec streamedMediaVideoCall(const QVariantMap &additionalProperties =
            QVariantMap());
    TP_QT_DEPRECATED static ChannelClassSpec streamedMediaVideoCallWithAudio(const QVariantMap &additionalProperties =
            QVariantMap());

    TP_QT_DEPRECATED static ChannelClassSpec unnamedStreamedMediaCall(const QVariantMap &additionalProperties =
            QVariantMap());
    TP_QT_DEPRECATED static ChannelClassSpec unnamedStreamedMediaAudioCall(const QVariantMap &additionalProperties =
            QVariantMap());
    TP_QT_DEPRECATED static ChannelClassSpec unnamedStreamedMediaVideoCall(const QVariantMap &additionalProperties =
            QVariantMap());
    TP_QT_DEPRECATED static ChannelClassSpec unnamedStreamedMediaVideoCallWithAudio(const QVariantMap &additionalProperties =
            QVariantMap());

    static ChannelClassSpec serverAuthentication(const QVariantMap &additionalProperties =
            QVariantMap());

    static ChannelClassSpec roomList(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec outgoingFileTransfer(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec incomingFileTransfer(const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec outgoingStreamTube(const QString &service = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec incomingStreamTube(const QString &service = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec outgoingRoomStreamTube(const QString &service = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec incomingRoomStreamTube(const QString &service = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec outgoingDBusTube(const QString &serviceName = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec incomingDBusTube(const QString &serviceName = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec outgoingRoomDBusTube(const QString &serviceName = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec incomingRoomDBusTube(const QString &serviceName = QString(),
            const QVariantMap &additionalProperties = QVariantMap());
    static ChannelClassSpec contactSearch(const QVariantMap &additionalProperties = QVariantMap());

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TP_QT_EXPORT ChannelClassSpecList :
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

    ChannelClassList bareClasses() const
    {
        ChannelClassList list;
        Q_FOREACH (const ChannelClassSpec &spec, *this) {
            list.append(spec.bareClass());
        }
        return list;
    }
};

inline uint qHash(const ChannelClassSpec &spec)
{
    uint ret = 0;
    QVariantMap::const_iterator it = spec.allProperties().constBegin();
    QVariantMap::const_iterator end = spec.allProperties().constEnd();
    int i = spec.allProperties().size() + 1;
    for (; it != end; ++it) {
        // all D-Bus types should be convertible to QString
        QPair<QString, QString> p(it.key(), it.value().toString());
        int h = qHash(p);
        ret ^= ((h << (2 << i)) | (h >> (2 >> i)));
        i--;
    }
    return ret;
}

inline uint qHash(const QSet<ChannelClassSpec> &specSet)
{
    int ret = 0;
    Q_FOREACH (const ChannelClassSpec &spec, specSet) {
        int h = qHash(spec);
        ret ^= h;
    }
    return ret;
}

inline uint qHash(const ChannelClassSpecList &specList)
{
    // Make it unique by converting to QSet
    QSet<ChannelClassSpec> uniqueSet = specList.toSet();
    return qHash(uniqueSet);
}

inline uint qHash(const QList<ChannelClassSpec> &specList)
{
    // Make it unique by converting to QSet
    QSet<ChannelClassSpec> uniqueSet = specList.toSet();
    return qHash(uniqueSet);
}

} // Tp

Q_DECLARE_METATYPE(Tp::ChannelClassSpec);
Q_DECLARE_METATYPE(Tp::ChannelClassSpecList);

#endif
