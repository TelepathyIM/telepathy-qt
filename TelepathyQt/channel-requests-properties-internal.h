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

#ifndef _TelepathyQt_channel_requests_properites_internal_h_HEADER_GUARD_
#define _TelepathyQt_channel_requests_properites_internal_h_HEADER_GUARD_

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

#include <QVariantMap>

namespace Tp
{

class FileTransferChannelCreationProperties;
class ConnectionCapabilities;

QVariantMap textChatCommonRequest();

QVariantMap textChatRequest(const QString &contactIdentifier);

QVariantMap textChatRequest(const Tp::ContactPtr &contact);

QVariantMap textChatroomRequest(const QString &roomName);

QVariantMap callCommonRequest(bool withAudio, const QString &audioName,
        bool withVideo, const QString &videoName);

QVariantMap audioCallRequest(const QString &contactIdentifier, const QString &contentName);

QVariantMap audioCallRequest(const Tp::ContactPtr &contact, const QString &contentName);

QVariantMap videoCallRequest(const QString &contactIdentifier, const QString &contentName);

QVariantMap videoCallRequest(const Tp::ContactPtr &contact, const QString &contentName);

QVariantMap audioVideoCallRequest(const QString &contactIdentifier,
        const QString &audioName,
        const QString &videoName);

QVariantMap audioVideoCallRequest(const Tp::ContactPtr &contact,
        const QString &audioName,
        const QString &videoName);

QVariantMap streamedMediaCallCommonRequest();

QVariantMap streamedMediaCallRequest(const QString &contactIdentifier);

QVariantMap streamedMediaCallRequest(const Tp::ContactPtr &contact);

QVariantMap streamedMediaAudioCallRequest(const QString &contactIdentifier);

QVariantMap streamedMediaAudioCallRequest(const Tp::ContactPtr &contact);

QVariantMap streamedMediaVideoCallRequest(const QString &contactIdentifier, bool withAudio);

QVariantMap streamedMediaVideoCallRequest(const Tp::ContactPtr &contact, bool withAudio);

QVariantMap fileTransferRequest(const QString &contactIdentifier,
        const Tp::FileTransferChannelCreationProperties &properties);

QVariantMap fileTransferRequest(const Tp::ContactPtr &contact,
        const Tp::FileTransferChannelCreationProperties &properties);

QVariantMap streamTubeCommonRequest(const QString &service);

QVariantMap streamTubeRequest(const QString &contactIdentifier, const QString &service);

QVariantMap streamTubeRequest(const Tp::ContactPtr &contact, const QString &service);

QVariantMap dbusTubeCommonRequest(const QString &serviceName);

QVariantMap dbusTubeRequest(const QString &contactIdentifier, const QString &serviceName);

QVariantMap dbusTubeRequest(const Tp::ContactPtr &contact, const QString &serviceName);

QVariantMap dbusTubeRoomRequest(const QString &roomName, const QString &serviceName);

QVariantMap conferenceCommonRequest(const QString &channelType, Tp::HandleType targetHandleType,
        const QList<Tp::ChannelPtr> &channels);

QVariantMap conferenceRequest(const QString &channelType, Tp::HandleType targetHandleType,
        const QList<Tp::ChannelPtr> &channels, const QStringList &initialInviteeContactsIdentifiers);

QVariantMap conferenceRequest(const QString &channelType, Tp::HandleType targetHandleType,
        const QList<Tp::ChannelPtr> &channels, const QList<Tp::ContactPtr> &initialInviteeContacts);

QVariantMap conferenceTextChatRequest(const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers);

QVariantMap conferenceTextChatRequest(const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts);

QVariantMap conferenceTextChatroomRequest(const QString &roomName,
        const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers);

QVariantMap conferenceTextChatroomRequest(const QString &roomName,
        const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts);

QVariantMap conferenceStreamedMediaCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers);

QVariantMap conferenceStreamedMediaCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts);

QVariantMap conferenceCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers);

QVariantMap conferenceCallRequest(const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ContactPtr> &initialInviteeContacts);

QVariantMap contactSearchRequest(const Tp::ConnectionCapabilities &capabilities,
        const QString &server, uint limit);

} // Tp

#endif // _TelepathyQt_channel_requests_properites_internal_h_HEADER_GUARD_
