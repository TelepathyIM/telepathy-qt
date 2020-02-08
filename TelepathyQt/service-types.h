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

class AbstractCallContentInterface;
class AbstractChannelInterface;
class AbstractConnectionInterface;
class AbstractProtocolInterface;
class BaseCallContent;
class BaseCallContentDTMFInterface;
class BaseCallMuteInterface;
class BaseChannel;
class BaseChannelCallType;
class BaseChannelCaptchaAuthenticationInterface;
class BaseChannelChatStateInterface;
class BaseChannelConferenceInterface;
class BaseChannelFileTransferType;
class BaseChannelGroupInterface;
class BaseChannelHoldInterface;
class BaseChannelMergeableConferenceInterface;
class BaseChannelMessagesInterface;
class BaseChannelRoomConfigInterface;
class BaseChannelRoomInterface;
class BaseChannelRoomListType;
class BaseChannelSASLAuthenticationInterface;
class BaseChannelSMSInterface;
class BaseChannelSecurableInterface;
class BaseChannelServerAuthenticationType;
class BaseChannelSplittableInterface;
class BaseChannelTextType;
class BaseConnection;
class BaseConnectionAddressingInterface;
class BaseConnectionAliasingInterface;
class BaseConnectionAvatarsInterface;
class BaseConnectionClientTypesInterface;
class BaseConnectionContactCapabilitiesInterface;
class BaseConnectionContactGroupsInterface;
class BaseConnectionContactInfoInterface;
class BaseConnectionContactListInterface;
class BaseConnectionContactsInterface;
class BaseConnectionManager;
class BaseConnectionRequestsInterface;
class BaseConnectionSimplePresenceInterface;
class BaseProtocol;
class BaseProtocolAddressingInterface;
class BaseProtocolAvatarsInterface;
class BaseProtocolPresenceInterface;
class DBusService;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef SharedPtr<AbstractCallContentInterface> AbstractCallContentInterfacePtr;
typedef SharedPtr<AbstractChannelInterface> AbstractChannelInterfacePtr;
typedef SharedPtr<AbstractConnectionInterface> AbstractConnectionInterfacePtr;
typedef SharedPtr<AbstractProtocolInterface> AbstractProtocolInterfacePtr;
typedef SharedPtr<BaseCallContent> BaseCallContentPtr;
typedef SharedPtr<BaseCallContentDTMFInterface> BaseCallContentDTMFInterfacePtr;
typedef SharedPtr<BaseCallMuteInterface> BaseCallMuteInterfacePtr;
typedef SharedPtr<BaseChannel> BaseChannelPtr;
typedef SharedPtr<BaseChannelCallType> BaseChannelCallTypePtr;
typedef SharedPtr<BaseChannelCaptchaAuthenticationInterface> BaseChannelCaptchaAuthenticationInterfacePtr;
typedef SharedPtr<BaseChannelChatStateInterface> BaseChannelChatStateInterfacePtr;
typedef SharedPtr<BaseChannelConferenceInterface> BaseChannelConferenceInterfacePtr;
typedef SharedPtr<BaseChannelFileTransferType> BaseChannelFileTransferTypePtr;
typedef SharedPtr<BaseChannelGroupInterface> BaseChannelGroupInterfacePtr;
typedef SharedPtr<BaseChannelHoldInterface> BaseChannelHoldInterfacePtr;
typedef SharedPtr<BaseChannelMergeableConferenceInterface> BaseChannelMergeableConferenceInterfacePtr;
typedef SharedPtr<BaseChannelMessagesInterface> BaseChannelMessagesInterfacePtr;
typedef SharedPtr<BaseChannelRoomConfigInterface> BaseChannelRoomConfigInterfacePtr;
typedef SharedPtr<BaseChannelRoomInterface> BaseChannelRoomInterfacePtr;
typedef SharedPtr<BaseChannelRoomListType> BaseChannelRoomListTypePtr;
typedef SharedPtr<BaseChannelSASLAuthenticationInterface> BaseChannelSASLAuthenticationInterfacePtr;
typedef SharedPtr<BaseChannelSMSInterface> BaseChannelSMSInterfacePtr;
typedef SharedPtr<BaseChannelSecurableInterface> BaseChannelSecurableInterfacePtr;
typedef SharedPtr<BaseChannelServerAuthenticationType> BaseChannelServerAuthenticationTypePtr;
typedef SharedPtr<BaseChannelSplittableInterface> BaseChannelSplittableInterfacePtr;
typedef SharedPtr<BaseChannelTextType> BaseChannelTextTypePtr;
typedef SharedPtr<BaseConnection> BaseConnectionPtr;
typedef SharedPtr<BaseConnectionAddressingInterface> BaseConnectionAddressingInterfacePtr;
typedef SharedPtr<BaseConnectionAliasingInterface> BaseConnectionAliasingInterfacePtr;
typedef SharedPtr<BaseConnectionAvatarsInterface> BaseConnectionAvatarsInterfacePtr;
typedef SharedPtr<BaseConnectionClientTypesInterface> BaseConnectionClientTypesInterfacePtr;
typedef SharedPtr<BaseConnectionContactCapabilitiesInterface> BaseConnectionContactCapabilitiesInterfacePtr;
typedef SharedPtr<BaseConnectionContactGroupsInterface> BaseConnectionContactGroupsInterfacePtr;
typedef SharedPtr<BaseConnectionContactInfoInterface> BaseConnectionContactInfoInterfacePtr;
typedef SharedPtr<BaseConnectionContactListInterface> BaseConnectionContactListInterfacePtr;
typedef SharedPtr<BaseConnectionContactsInterface> BaseConnectionContactsInterfacePtr;
typedef SharedPtr<BaseConnectionManager> BaseConnectionManagerPtr;
typedef SharedPtr<BaseConnectionRequestsInterface> BaseConnectionRequestsInterfacePtr;
typedef SharedPtr<BaseConnectionSimplePresenceInterface> BaseConnectionSimplePresenceInterfacePtr;
typedef SharedPtr<BaseProtocol> BaseProtocolPtr;
typedef SharedPtr<BaseProtocolAddressingInterface> BaseProtocolAddressingInterfacePtr;
typedef SharedPtr<BaseProtocolAvatarsInterface> BaseProtocolAvatarsInterfacePtr;
typedef SharedPtr<BaseProtocolPresenceInterface> BaseProtocolPresenceInterfacePtr;
typedef SharedPtr<DBusService> DBusServicePtr;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

} // Tp

#endif
