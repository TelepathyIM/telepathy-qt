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

#ifndef _TelepathyQt_base_connection_h_HEADER_GUARD_
#define _TelepathyQt_base_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>
#include <TelepathyQt/Callbacks>
#include <TelepathyQt/Constants>

#include <QDBusConnection>

class QString;

namespace Tp
{

class TP_QT_EXPORT BaseConnection : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnection)

public:
    static BaseConnectionPtr create(const QString &cmName, const QString &protocolName,
                                    const QVariantMap &parameters) {
        return BaseConnectionPtr(new BaseConnection(
                                     QDBusConnection::sessionBus(), cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QString &cmName,
            const QString &protocolName, const QVariantMap &parameters) {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    QDBusConnection::sessionBus(), cmName, protocolName, parameters));
    }
    static BaseConnectionPtr create(const QDBusConnection &dbusConnection,
                                    const QString &cmName, const QString &protocolName,
                                    const QVariantMap &parameters) {
        return BaseConnectionPtr(new BaseConnection(
                                     dbusConnection, cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters) {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    dbusConnection, cmName, protocolName, parameters));
    }

    virtual ~BaseConnection();

    QString cmName() const;
    QString protocolName() const;
    QVariantMap parameters() const;
    uint status() const;
    QVariantMap immutableProperties() const;

    void setStatus(uint newStatus, uint reason);

    typedef Callback4<BaseChannelPtr, const QString&, uint, uint, DBusError*> CreateChannelCallback;
    void setCreateChannelCallback(const CreateChannelCallback &cb);
    Tp::BaseChannelPtr createChannel(const QString &channelType, uint targetHandleType, uint targetHandle, uint initiatorHandle, bool suppressHandler, DBusError *error);

    typedef Callback3<UIntList, uint, const QStringList&, DBusError*> RequestHandlesCallback;
    void setRequestHandlesCallback(const RequestHandlesCallback &cb);
    UIntList requestHandles(uint handleType, const QStringList &identifiers, DBusError* error);

    //typedef Callback3<uint, const QString&, const QString&, DBusError*> SetPresenceCallback;
    //void setSetPresenceCallback(const SetPresenceCallback &cb);

    void setSelfHandle(uint selfHandle);
    uint selfHandle() const;

    typedef Callback1<void, DBusError*> ConnectCallback;
    void setConnectCallback(const ConnectCallback &cb);

    typedef Callback3<QStringList, uint, const Tp::UIntList&, DBusError*> InspectHandlesCallback;
    void setInspectHandlesCallback(const InspectHandlesCallback &cb);

    Tp::ChannelInfoList channelsInfo();
    Tp::ChannelDetailsList channelsDetails();

    BaseChannelPtr ensureChannel(const QString &channelType, uint targetHandleType,
                                 uint targetHandle, bool &yours, uint initiatorHandle, bool suppressHandler, DBusError *error);
    void addChannel(BaseChannelPtr channel);

    QList<AbstractConnectionInterfacePtr> interfaces() const;
    AbstractConnectionInterfacePtr interface(const QString  &interfaceName) const;
    bool plugInterface(const AbstractConnectionInterfacePtr &interface);

    virtual QString uniqueName() const;
    bool registerObject(DBusError *error = NULL);

Q_SIGNALS:
    void disconnected();

private Q_SLOTS:
    TP_QT_NO_EXPORT void removeChannel();

protected:
    BaseConnection(const QDBusConnection &dbusConnection,
                   const QString &cmName, const QString &protocolName,
                   const QVariantMap &parameters);

    virtual bool registerObject(const QString &busName, const QString &objectPath,
                                DBusError *error);

private:
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractConnectionInterface : public AbstractDBusServiceInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractConnectionInterface)

public:
    AbstractConnectionInterface(const QString &interfaceName);
    virtual ~AbstractConnectionInterface();

private:
    friend class BaseConnection;

    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionRequestsInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionRequestsInterface)

public:
    static BaseConnectionRequestsInterfacePtr create(BaseConnection* connection) {
        return BaseConnectionRequestsInterfacePtr(new BaseConnectionRequestsInterface(connection));
    }
    template<typename BaseConnectionRequestsInterfaceSubclass>
    static SharedPtr<BaseConnectionRequestsInterfaceSubclass> create(BaseConnection* connection) {
        return SharedPtr<BaseConnectionRequestsInterfaceSubclass>(
                   new BaseConnectionRequestsInterfaceSubclass(connection));
    }

    virtual ~BaseConnectionRequestsInterface();

    QVariantMap immutableProperties() const;

    Tp::RequestableChannelClassList requestableChannelClasses;
    void ensureChannel(const QVariantMap &request, bool &yours,
                       QDBusObjectPath &channel, QVariantMap &details, DBusError* error);
    void createChannel(const QVariantMap &request, QDBusObjectPath &channel,
                       QVariantMap &details, DBusError* error);

public Q_SLOTS:
    void newChannels(const Tp::ChannelDetailsList &channels);
    void channelClosed(const QDBusObjectPath &removed);

protected:
    BaseConnectionRequestsInterface(BaseConnection* connection);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class TP_QT_EXPORT BaseConnectionContactsInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionContactsInterface)


public:
    static BaseConnectionContactsInterfacePtr create() {
        return BaseConnectionContactsInterfacePtr(new BaseConnectionContactsInterface());
    }
    template<typename BaseConnectionContactsInterfaceSubclass>
    static SharedPtr<BaseConnectionContactsInterfaceSubclass> create() {
        return SharedPtr<BaseConnectionContactsInterfaceSubclass>(
                   new BaseConnectionContactsInterfaceSubclass());
    }

    virtual ~BaseConnectionContactsInterface();

    QVariantMap immutableProperties() const;

    typedef Callback3<ContactAttributesMap, const Tp::UIntList&, const QStringList&, DBusError*> GetContactAttributesCallback;
    void setGetContactAttributesCallback(const GetContactAttributesCallback &cb);
    ContactAttributesMap getContactAttributes(const Tp::UIntList &handles,
            const QStringList &interfaces,
            DBusError *error);
    void setContactAttributeInterfaces(const QStringList &contactAttributeInterfaces);
protected:
    BaseConnectionContactsInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionSimplePresenceInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionSimplePresenceInterface)

public:
    static BaseConnectionSimplePresenceInterfacePtr create() {
        return BaseConnectionSimplePresenceInterfacePtr(new BaseConnectionSimplePresenceInterface());
    }
    template<typename BaseConnectionSimplePresenceInterfaceSublclass>
    static SharedPtr<BaseConnectionSimplePresenceInterfaceSublclass> create() {
        return SharedPtr<BaseConnectionSimplePresenceInterfaceSublclass>(
                   new BaseConnectionSimplePresenceInterfaceSublclass());
    }

    virtual ~BaseConnectionSimplePresenceInterface();

    QVariantMap immutableProperties() const;

    Tp::SimpleStatusSpecMap statuses() const;
    void setStatuses(const Tp::SimpleStatusSpecMap &statuses);

    int maximumStatusMessageLength() const;
    void setMaxmimumStatusMessageLength(uint maxmimumStatusMessageLength);

    typedef Callback3<uint, const QString&, const QString&, DBusError*> SetPresenceCallback;
    void setSetPresenceCallback(const SetPresenceCallback &cb);

    void setPresences(const Tp::SimpleContactPresences &presences);

protected:
    BaseConnectionSimplePresenceInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionContactListInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionContactListInterface)

public:
    static BaseConnectionContactListInterfacePtr create()
    {
        return BaseConnectionContactListInterfacePtr(new BaseConnectionContactListInterface());
    }
    template<typename BaseConnectionContactListInterfaceSubclass>
    static SharedPtr<BaseConnectionContactListInterfaceSubclass> create()
    {
        return SharedPtr<BaseConnectionContactListInterfaceSubclass>(
                new BaseConnectionContactListInterfaceSubclass());
    }

    virtual ~BaseConnectionContactListInterface();

    QVariantMap immutableProperties() const;

    uint contactListState() const;
    void setContactListState(uint contactListState);

    bool contactListPersists() const;
    void setContactListPersists(bool contactListPersists);

    bool canChangeContactList() const;
    void setCanChangeContactList(bool canChangeContactList);

    bool requestUsesMessage() const;
    void setRequestUsesMessage(bool requestUsesMessage);

    bool downloadAtConnection() const;
    void setDownloadAtConnection(bool downloadAtConnection);

    typedef Callback3<Tp::ContactAttributesMap, const QStringList &, bool, DBusError*> GetContactListAttributesCallback;
    void setGetContactListAttributesCallback(const GetContactListAttributesCallback &cb);
    Tp::ContactAttributesMap getContactListAttributes(const QStringList &interfaces, bool hold, DBusError *error);

    typedef Callback3<void, const Tp::UIntList &, const QString &, DBusError*> RequestSubscriptionCallback;
    void setRequestSubscriptionCallback(const RequestSubscriptionCallback &cb);
    void requestSubscription(const Tp::UIntList &contacts, const QString &message, DBusError *error);

    typedef Callback2<void, const Tp::UIntList &, DBusError*> AuthorizePublicationCallback;
    void setAuthorizePublicationCallback(const AuthorizePublicationCallback &cb);
    void authorizePublication(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<void, const Tp::UIntList &, DBusError*> RemoveContactsCallback;
    void setRemoveContactsCallback(const RemoveContactsCallback &cb);
    void removeContacts(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<void, const Tp::UIntList &, DBusError*> UnsubscribeCallback;
    void setUnsubscribeCallback(const UnsubscribeCallback &cb);
    void unsubscribe(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<void, const Tp::UIntList &, DBusError*> UnpublishCallback;
    void setUnpublishCallback(const UnpublishCallback &cb);
    void unpublish(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback1<void, DBusError*> DownloadCallback;
    void setDownloadCallback(const DownloadCallback &cb);
    void download(DBusError *error);

    void contactsChangedWithID(const Tp::ContactSubscriptionMap &changes, const Tp::HandleIdentifierMap &identifiers, const Tp::HandleIdentifierMap &removals);

protected:
    BaseConnectionContactListInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionContactInfoInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionContactInfoInterface)

public:
    static BaseConnectionContactInfoInterfacePtr create()
    {
        return BaseConnectionContactInfoInterfacePtr(new BaseConnectionContactInfoInterface());
    }
    template<typename BaseConnectionContactInfoInterfaceSubclass>
    static SharedPtr<BaseConnectionContactInfoInterfaceSubclass> create()
    {
        return SharedPtr<BaseConnectionContactInfoInterfaceSubclass>(
                new BaseConnectionContactInfoInterfaceSubclass());
    }

    virtual ~BaseConnectionContactInfoInterface();

    QVariantMap immutableProperties() const;

    Tp::ContactInfoFlags contactInfoFlags() const;
    void setContactInfoFlags(const Tp::ContactInfoFlags &contactInfoFlags);

    Tp::FieldSpecs supportedFields() const;
    void setSupportedFields(const Tp::FieldSpecs &supportedFields);

    typedef Callback2<Tp::ContactInfoMap, const Tp::UIntList &, DBusError*> GetContactInfoCallback;
    void setGetContactInfoCallback(const GetContactInfoCallback &cb);
    Tp::ContactInfoMap getContactInfo(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<void, const Tp::UIntList &, DBusError*> RefreshContactInfoCallback;
    void setRefreshContactInfoCallback(const RefreshContactInfoCallback &cb);
    void refreshContactInfo(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<Tp::ContactInfoFieldList, uint, DBusError*> RequestContactInfoCallback;
    void setRequestContactInfoCallback(const RequestContactInfoCallback &cb);
    Tp::ContactInfoFieldList requestContactInfo(uint contact, DBusError *error);

    typedef Callback2<void, const Tp::ContactInfoFieldList &, DBusError*> SetContactInfoCallback;
    void setSetContactInfoCallback(const SetContactInfoCallback &cb);
    void setContactInfo(const Tp::ContactInfoFieldList &contactInfo, DBusError *error);

    void contactInfoChanged(uint contact, const Tp::ContactInfoFieldList &contactInfo);

protected:
    BaseConnectionContactInfoInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionAddressingInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionAddressingInterface)

public:
    static BaseConnectionAddressingInterfacePtr create() {
        return BaseConnectionAddressingInterfacePtr(new BaseConnectionAddressingInterface());
    }
    template<typename BaseConnectionAddressingInterfaceSubclass>
    static SharedPtr<BaseConnectionAddressingInterfaceSubclass> create() {
        return SharedPtr<BaseConnectionAddressingInterfaceSubclass>(
                   new BaseConnectionAddressingInterfaceSubclass());
    }

    virtual ~BaseConnectionAddressingInterface();

    QVariantMap immutableProperties() const;



    typedef Callback6 < void, const QString&, const QStringList&, const QStringList&,
            Tp::AddressingNormalizationMap&, Tp::ContactAttributesMap&, DBusError* > GetContactsByVCardFieldCallback;
    void setGetContactsByVCardFieldCallback(const GetContactsByVCardFieldCallback &cb);

    typedef Callback5 < void, const QStringList&, const QStringList&,
            Tp::AddressingNormalizationMap&, Tp::ContactAttributesMap&, DBusError* > GetContactsByURICallback;
    void setGetContactsByURICallback(const GetContactsByURICallback &cb);

protected:
    BaseConnectionAddressingInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionAliasingInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionAliasingInterface)

public:
    static BaseConnectionAliasingInterfacePtr create()
    {
        return BaseConnectionAliasingInterfacePtr(new BaseConnectionAliasingInterface());
    }
    template<typename BaseConnectionAliasingInterfaceSubclass>
    static SharedPtr<BaseConnectionAliasingInterfaceSubclass> create()
    {
        return SharedPtr<BaseConnectionAliasingInterfaceSubclass>(
                new BaseConnectionAliasingInterfaceSubclass());
    }

    virtual ~BaseConnectionAliasingInterface();

    QVariantMap immutableProperties() const;

    typedef Callback1<Tp::ConnectionAliasFlags, DBusError*> GetAliasFlagsCallback;
    void setGetAliasFlagsCallback(const GetAliasFlagsCallback &cb);
    Tp::ConnectionAliasFlags getAliasFlags(DBusError *error);

    typedef Callback2<QStringList, const Tp::UIntList &, DBusError*> RequestAliasesCallback;
    void setRequestAliasesCallback(const RequestAliasesCallback &cb);
    QStringList requestAliases(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<Tp::AliasMap, const Tp::UIntList &, DBusError*> GetAliasesCallback;
    void setGetAliasesCallback(const GetAliasesCallback &cb);
    Tp::AliasMap getAliases(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<void, const Tp::AliasMap &, DBusError*> SetAliasesCallback;
    void setSetAliasesCallback(const SetAliasesCallback &cb);
    void setAliases(const Tp::AliasMap &aliases, DBusError *error);

    void aliasesChanged(const Tp::AliasPairList &aliases);

protected:
    BaseConnectionAliasingInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}

#endif
