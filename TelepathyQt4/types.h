/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_types_h_HEADER_GUARD_
#define _TelepathyQt4_types_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/types.h>

#include <TelepathyQt4/Global>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/MethodInvocationContext>

#include <QDBusVariant>

namespace Tp
{

TELEPATHY_QT4_EXPORT void registerTypes();

template <typename T> class Filter;
template <typename T> class GenericCapabilityFilter;
template <typename T> class GenericPropertyFilter;

class AbstractClient;
class Account;
typedef GenericCapabilityFilter<Account> AccountCapabilityFilter;
class AccountFactory;
typedef Filter<Account> AccountFilter;
class AccountManager;
class AccountPropertyFilter;
class AccountSet;
class Channel;
class ChannelDispatchOperation;
class ChannelFactory;
class ChannelRequest;
class ClientObject;
class ClientRegistrar;
class Connection;
class ConnectionFactory;
class ConnectionManager;
class Contact;
class ContactFactory;
class ContactSearchChannel;
class DBusProxy;
class FileTransferChannel;
class IncomingFileTransferChannel;
class OutgoingFileTransferChannel;
class Profile;
class ProfileManager;
class RoomListChannel;
class StreamedMediaChannel;
class StreamedMediaStream;
class TextChannel;

typedef SharedPtr<AbstractClient> AbstractClientPtr;
typedef SharedPtr<Account> AccountPtr;
typedef SharedPtr<AccountCapabilityFilter> AccountCapabilityFilterPtr;
typedef SharedPtr<const AccountCapabilityFilter> AccountCapabilityFilterConstPtr;
typedef SharedPtr<AccountFactory> AccountFactoryPtr;
typedef SharedPtr<const AccountFactory> AccountFactoryConstPtr;
typedef SharedPtr<AccountFilter> AccountFilterPtr;
typedef SharedPtr<const AccountFilter> AccountFilterConstPtr;
typedef SharedPtr<AccountManager> AccountManagerPtr;
typedef SharedPtr<AccountPropertyFilter> AccountPropertyFilterPtr;
typedef SharedPtr<const AccountPropertyFilter> AccountPropertyFilterConstPtr;
typedef SharedPtr<AccountSet> AccountSetPtr;
typedef SharedPtr<Channel> ChannelPtr;
typedef SharedPtr<ChannelDispatchOperation> ChannelDispatchOperationPtr;
typedef SharedPtr<ChannelFactory> ChannelFactoryPtr;
typedef SharedPtr<const ChannelFactory> ChannelFactoryConstPtr;
typedef SharedPtr<ChannelRequest> ChannelRequestPtr;
typedef SharedPtr<ClientObject> ClientObjectPtr;
typedef SharedPtr<ClientRegistrar> ClientRegistrarPtr;
typedef SharedPtr<Connection> ConnectionPtr;
typedef SharedPtr<ConnectionFactory> ConnectionFactoryPtr;
typedef SharedPtr<const ConnectionFactory> ConnectionFactoryConstPtr;
typedef SharedPtr<ConnectionManager> ConnectionManagerPtr;
typedef SharedPtr<Contact> ContactPtr;
typedef SharedPtr<ContactFactory> ContactFactoryPtr;
typedef SharedPtr<const ContactFactory> ContactFactoryConstPtr;
typedef SharedPtr<ContactSearchChannel> ContactSearchChannelPtr;
typedef SharedPtr<DBusProxy> DBusProxyPtr;
typedef SharedPtr<FileTransferChannel> FileTransferChannelPtr;
typedef SharedPtr<IncomingFileTransferChannel> IncomingFileTransferChannelPtr;
typedef SharedPtr<OutgoingFileTransferChannel> OutgoingFileTransferChannelPtr;
typedef SharedPtr<Profile> ProfilePtr;
typedef SharedPtr<ProfileManager> ProfileManagerPtr;
typedef SharedPtr<RoomListChannel> RoomListChannelPtr;
typedef SharedPtr<StreamedMediaChannel> StreamedMediaChannelPtr;
typedef SharedPtr<StreamedMediaStream> StreamedMediaStreamPtr;
typedef SharedPtr<TextChannel> TextChannelPtr;

template<typename T1 = MethodInvocationContextTypes::Nil, typename T2 = MethodInvocationContextTypes::Nil,
         typename T3 = MethodInvocationContextTypes::Nil, typename T4 = MethodInvocationContextTypes::Nil,
         typename T5 = MethodInvocationContextTypes::Nil, typename T6 = MethodInvocationContextTypes::Nil,
         typename T7 = MethodInvocationContextTypes::Nil, typename T8 = MethodInvocationContextTypes::Nil>
class MethodInvocationContextPtr : public SharedPtr<MethodInvocationContext<T1, T2, T3, T4, T5, T6, T7, T8> >
{
public:
    inline MethodInvocationContextPtr() { }
    explicit inline MethodInvocationContextPtr(MethodInvocationContext<T1, T2, T3, T4, T5, T6, T7, T8> *d)
        : SharedPtr<MethodInvocationContext<T1, T2, T3, T4, T5, T6, T7, T8> >(d) { }
    inline MethodInvocationContextPtr(const SharedPtr<MethodInvocationContext<T1, T2, T3, T4, T5, T6, T7, T8> > &o)
        : SharedPtr<MethodInvocationContext<T1, T2, T3, T4, T5, T6, T7, T8> >(o) { }
};

class AvatarData
{
public:
    inline AvatarData(const QString &fileName, const QString &mimeType):
        fileName(fileName), mimeType(mimeType) {}
    inline AvatarData() {}
    QString fileName;
    QString mimeType;
};

} // Tp

#endif
