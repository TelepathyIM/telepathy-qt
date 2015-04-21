/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#ifndef _TelepathyQt_service_types_h_HEADER_GUARD_
#define _TelepathyQt_service_types_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Types>

namespace Tp
{

class AbstractProtocolInterface;
class AbstractCallContentInterface;
class AbstractConnectionInterface;
class AbstractChannelInterface;
class BaseCallContent;
class BaseCallMuteInterface;
class BaseCallContentDTMFInterface;
class BaseConnection;
class BaseConnectionRequestsInterface;
class BaseConnectionContactsInterface;
class BaseConnectionSimplePresenceInterface;
class BaseConnectionContactListInterface;
class BaseConnectionContactInfoInterface;
class BaseConnectionAddressingInterface;
class BaseConnectionAliasingInterface;
class BaseConnectionAvatarsInterface;
class BaseConnectionManager;
class BaseProtocol;
class BaseProtocolAddressingInterface;
class BaseProtocolAvatarsInterface;
class BaseProtocolPresenceInterface;
class BaseChannel;
class BaseChannelTextType;
class BaseChannelCallType;
class BaseChannelMessagesInterface;
class BaseChannelRoomListType;
class BaseChannelServerAuthenticationType;
class BaseChannelSASLAuthenticationInterface;
class BaseChannelCaptchaAuthenticationInterface;
class BaseChannelSecurableInterface;
class BaseChannelChatStateInterface;
class BaseChannelGroupInterface;
class BaseChannelRoomInterface;
class BaseChannelRoomConfigInterface;
class BaseChannelHoldInterface;
class BaseChannelMergeableConferenceInterface;
class BaseChannelSplittableInterface;
class BaseChannelSMSInterface;
class BaseChannelConferenceInterface;
class DBusService;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef SharedPtr<AbstractProtocolInterface> AbstractProtocolInterfacePtr;
typedef SharedPtr<AbstractCallContentInterface> AbstractCallContentInterfacePtr;
typedef SharedPtr<AbstractConnectionInterface> AbstractConnectionInterfacePtr;
typedef SharedPtr<AbstractChannelInterface> AbstractChannelInterfacePtr;
typedef SharedPtr<BaseCallContent> BaseCallContentPtr;
typedef SharedPtr<BaseCallContentDTMFInterface> BaseCallContentDTMFInterfacePtr;
typedef SharedPtr<BaseCallMuteInterface> BaseCallMuteInterfacePtr;
typedef SharedPtr<BaseConnection> BaseConnectionPtr;
typedef SharedPtr<BaseConnectionRequestsInterface> BaseConnectionRequestsInterfacePtr;
typedef SharedPtr<BaseConnectionContactsInterface> BaseConnectionContactsInterfacePtr;
typedef SharedPtr<BaseConnectionSimplePresenceInterface> BaseConnectionSimplePresenceInterfacePtr;
typedef SharedPtr<BaseConnectionContactListInterface> BaseConnectionContactListInterfacePtr;
typedef SharedPtr<BaseConnectionContactInfoInterface> BaseConnectionContactInfoInterfacePtr;
typedef SharedPtr<BaseConnectionAddressingInterface> BaseConnectionAddressingInterfacePtr;
typedef SharedPtr<BaseConnectionAliasingInterface> BaseConnectionAliasingInterfacePtr;
typedef SharedPtr<BaseConnectionAvatarsInterface> BaseConnectionAvatarsInterfacePtr;
typedef SharedPtr<BaseConnectionManager> BaseConnectionManagerPtr;
typedef SharedPtr<BaseProtocol> BaseProtocolPtr;
typedef SharedPtr<BaseProtocolAddressingInterface> BaseProtocolAddressingInterfacePtr;
typedef SharedPtr<BaseProtocolAvatarsInterface> BaseProtocolAvatarsInterfacePtr;
typedef SharedPtr<BaseProtocolPresenceInterface> BaseProtocolPresenceInterfacePtr;
typedef SharedPtr<BaseChannel> BaseChannelPtr;
typedef SharedPtr<BaseChannelCallType> BaseChannelCallTypePtr;
typedef SharedPtr<BaseChannelTextType> BaseChannelTextTypePtr;
typedef SharedPtr<BaseChannelMessagesInterface> BaseChannelMessagesInterfacePtr;
typedef SharedPtr<BaseChannelRoomListType> BaseChannelRoomListTypePtr;
typedef SharedPtr<BaseChannelServerAuthenticationType> BaseChannelServerAuthenticationTypePtr;
typedef SharedPtr<BaseChannelSASLAuthenticationInterface> BaseChannelSASLAuthenticationInterfacePtr;
typedef SharedPtr<BaseChannelCaptchaAuthenticationInterface> BaseChannelCaptchaAuthenticationInterfacePtr;
typedef SharedPtr<BaseChannelSecurableInterface> BaseChannelSecurableInterfacePtr;
typedef SharedPtr<BaseChannelChatStateInterface> BaseChannelChatStateInterfacePtr;
typedef SharedPtr<BaseChannelGroupInterface> BaseChannelGroupInterfacePtr;
typedef SharedPtr<BaseChannelRoomInterface> BaseChannelRoomInterfacePtr;
typedef SharedPtr<BaseChannelRoomConfigInterface> BaseChannelRoomConfigInterfacePtr;
typedef SharedPtr<BaseChannelHoldInterface> BaseChannelHoldInterfacePtr;
typedef SharedPtr<BaseChannelMergeableConferenceInterface> BaseChannelMergeableConferenceInterfacePtr;
typedef SharedPtr<BaseChannelSplittableInterface> BaseChannelSplittableInterfacePtr;
typedef SharedPtr<BaseChannelSMSInterface> BaseChannelSMSInterfacePtr;
typedef SharedPtr<BaseChannelConferenceInterface> BaseChannelConferenceInterfacePtr;
typedef SharedPtr<DBusService> DBusServicePtr;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

} // Tp

#endif
