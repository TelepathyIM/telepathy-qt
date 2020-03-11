#include "channel-requests-properties-internal.h"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Channel>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/Contact>
#include <TelepathyQt/FileTransferChannelCreationProperties>
#include <TelepathyQt/ReferencedHandles>

namespace Tp
{

QVariantMap textChatCommonRequest()
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    return request;
}

QVariantMap textChatRequest(const QString &contactIdentifier)
{
    QVariantMap request = textChatCommonRequest();
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap textChatRequest(const Tp::ContactPtr &contact)
{
    QVariantMap request = textChatCommonRequest();
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap textChatroomRequest(const QString &roomName)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeRoom);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   roomName);
    return request;
}

QVariantMap callCommonRequest(bool withAudio, const QString &audioName,
        bool withVideo, const QString &videoName)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CALL);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);

    if (withAudio) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                       true);
        if (!audioName.isEmpty()) {
            request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"),
                           audioName);
        }
    }

    if (withVideo) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
                       true);
        if (!videoName.isEmpty()) {
            request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"),
                           videoName);
        }
    }

    return request;
}

QVariantMap audioCallRequest(const QString &contactIdentifier, const QString &contentName)
{
    QVariantMap request = callCommonRequest(true, contentName, false, QString());
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap audioCallRequest(const Tp::ContactPtr &contact, const QString &contentName)
{
    QVariantMap request = callCommonRequest(true, contentName, false, QString());
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap videoCallRequest(const QString &contactIdentifier, const QString &contentName)
{
    QVariantMap request = callCommonRequest(false, QString(), true, contentName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap videoCallRequest(const Tp::ContactPtr &contact, const QString &contentName)
{
    QVariantMap request = callCommonRequest(false, QString(), true, contentName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap audioVideoCallRequest(const QString &contactIdentifier,
        const QString &audioName,
        const QString &videoName)
{
    QVariantMap request = callCommonRequest(true, audioName, true, videoName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap audioVideoCallRequest(const Tp::ContactPtr &contact,
        const QString &audioName,
        const QString &videoName)
{
    QVariantMap request = callCommonRequest(true, audioName, true, videoName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap streamedMediaCallCommonRequest()
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    return request;
}

QVariantMap streamedMediaCallRequest(const QString &contactIdentifier)
{
    QVariantMap request = streamedMediaCallCommonRequest();
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap streamedMediaCallRequest(const Tp::ContactPtr &contact)
{
    QVariantMap request = streamedMediaCallCommonRequest();
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap streamedMediaAudioCallRequest(const QString &contactIdentifier)
{
    QVariantMap request = streamedMediaCallRequest(contactIdentifier);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"),
                   true);
    return request;
}

QVariantMap streamedMediaAudioCallRequest(const Tp::ContactPtr &contact)
{
    QVariantMap request = streamedMediaCallRequest(contact);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"),
                   true);
    return request;
}

QVariantMap streamedMediaVideoCallRequest(const QString &contactIdentifier, bool withAudio)
{
    QVariantMap request = streamedMediaCallRequest(contactIdentifier);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo"),
                   true);
    if (withAudio) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"),
                       true);
    }
    return request;
}

QVariantMap streamedMediaVideoCallRequest(const Tp::ContactPtr &contact, bool withAudio)
{
    QVariantMap request = streamedMediaCallRequest(contact);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialVideo"),
                   true);
    if (withAudio) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".InitialAudio"),
                       true);
    }
    return request;
}

QVariantMap fileTransferRequest(const QString &contactIdentifier,
        const Tp::FileTransferChannelCreationProperties &properties)
{
    return properties.createRequest(contactIdentifier);
}

QVariantMap fileTransferRequest(const Tp::ContactPtr &contact,
        const Tp::FileTransferChannelCreationProperties &properties)
{
    return properties.createRequest(contact ? contact->handle().at(0) : (uint) 0);
}

QVariantMap streamTubeCommonRequest(const QString &service)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE + QLatin1String(".Service"),
                   service);
    return request;
}

QVariantMap streamTubeRequest(const QString &contactIdentifier, const QString &service)
{
    QVariantMap request = streamTubeCommonRequest(service);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap streamTubeRequest(const Tp::ContactPtr &contact, const QString &service)
{
    QVariantMap request = streamTubeCommonRequest(service);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap dbusTubeCommonRequest(const QString &serviceName)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
                   serviceName);
    return request;
}

QVariantMap dbusTubeRequest(const QString &contactIdentifier, const QString &serviceName)
{
    QVariantMap request = dbusTubeCommonRequest(serviceName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   contactIdentifier);
    return request;
}

QVariantMap dbusTubeRequest(const Tp::ContactPtr &contact, const QString &serviceName)
{
    QVariantMap request = dbusTubeCommonRequest(serviceName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return request;
}

QVariantMap dbusTubeRoomRequest(const QString &roomName, const QString &serviceName)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeRoom);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName"),
                   serviceName);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   roomName);
    return request;
}

QVariantMap conferenceCommonRequest(const QString &channelType, Tp::HandleType targetHandleType,
        const QList<Tp::ChannelPtr> &channels)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    if (targetHandleType != Tp::HandleTypeNone) {
        request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                       (uint) targetHandleType);
    }

    Tp::ObjectPathList objectPaths;
    for (const Tp::ChannelPtr &channel : channels) {
        objectPaths << QDBusObjectPath(channel->objectPath());
    }

    request.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"),
            QVariant::fromValue(objectPaths));
    return request;
}

QVariantMap conferenceRequest(const QString &channelType, Tp::HandleType targetHandleType,
        const QList<Tp::ChannelPtr> &channels, const QStringList &initialInviteeContactsIdentifiers)
{
    QVariantMap request = conferenceCommonRequest(channelType, targetHandleType, channels);
    if (!initialInviteeContactsIdentifiers.isEmpty()) {
        request.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeIDs"),
                initialInviteeContactsIdentifiers);
    }
    return request;
}

QVariantMap conferenceRequest(const QString &channelType, Tp::HandleType targetHandleType,
        const QList<Tp::ChannelPtr> &channels, const QList<Tp::ContactPtr> &initialInviteeContacts)
{
    QVariantMap request = conferenceCommonRequest(channelType, targetHandleType, channels);
    if (!initialInviteeContacts.isEmpty()) {
        Tp::UIntList handles;
        for (const Tp::ContactPtr &contact : initialInviteeContacts) {
            if (!contact) {
                continue;
            }
            handles << contact->handle()[0];
        }
        if (!handles.isEmpty()) {
            request.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE +
                        QLatin1String(".InitialInviteeHandles"), QVariant::fromValue(handles));
        }
    }
    return request;
}

QVariantMap conferenceTextChatRequest(const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
            Tp::HandleTypeNone, channels, initialInviteeContactsIdentifiers);
    return request;
}

QVariantMap conferenceTextChatRequest(const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
            Tp::HandleTypeNone, channels, initialInviteeContacts);
    return request;
}

QVariantMap conferenceTextChatroomRequest(const QString &roomName,
        const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
            Tp::HandleTypeRoom, channels, initialInviteeContactsIdentifiers);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"), roomName);
    return request;
}

QVariantMap conferenceTextChatroomRequest(const QString &roomName,
        const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
            Tp::HandleTypeRoom, channels, initialInviteeContacts);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"), roomName);
    return request;
}

QVariantMap conferenceStreamedMediaCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            Tp::HandleTypeNone, channels, initialInviteeContactsIdentifiers);
    return request;
}

QVariantMap conferenceStreamedMediaCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            Tp::HandleTypeNone, channels, initialInviteeContacts);
    return request;
}

QVariantMap conferenceCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_CALL,
            Tp::HandleTypeNone, channels, initialInviteeContactsIdentifiers);
    return request;
}

QVariantMap conferenceCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts)
{
    QVariantMap request = conferenceRequest(TP_QT_IFACE_CHANNEL_TYPE_CALL,
            Tp::HandleTypeNone, channels, initialInviteeContacts);
    return request;
}

QVariantMap contactSearchRequest(const ConnectionCapabilities &capabilities,
        const QString &server, uint limit)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH);
    if (capabilities.contactSearchesWithSpecificServer()) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH + QLatin1String(".Server"),
                       server);
    } else if (!server.isEmpty()) {
        warning() << "Ignoring Server parameter for contact search, since the protocol does not support it.";
    }
    if (capabilities.contactSearchesWithLimit()) {
        request.insert(TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH + QLatin1String(".Limit"), limit);
    } else if (limit > 0) {
        warning() << "Ignoring Limit parameter for contact search, since the protocol does not support it.";
    }
    return request;
}

} // Tp
