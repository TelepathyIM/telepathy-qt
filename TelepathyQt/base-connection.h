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

#include <TelepathyQt/AvatarSpec>
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
                                    const QVariantMap &parameters,
                                    const QDBusConnection &dbusConnection = QDBusConnection::sessionBus())
    {
        return BaseConnectionPtr(new BaseConnection(
                                     dbusConnection, cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QString &cmName,
            const QString &protocolName, const QVariantMap &parameters,
            const QDBusConnection &dbusConnection = QDBusConnection::sessionBus())
    {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    dbusConnection, cmName, protocolName, parameters));
    }

    virtual ~BaseConnection();

    QString cmName() const;
    QString protocolName() const;
    QVariantMap parameters() const;
    QVariantMap immutableProperties() const;

    uint selfHandle() const;
    void setSelfHandle(uint selfHandle);

    QString selfID() const;
    void setSelfID(const QString &selfID);

    void setSelfContact(uint selfHandle, const QString &selfID);

    uint status() const;
    void setStatus(uint newStatus, uint reason);

    typedef Callback2<BaseChannelPtr, const QVariantMap &, DBusError*> CreateChannelCallback;
    void setCreateChannelCallback(const CreateChannelCallback &cb);
    BaseChannelPtr createChannel(const QVariantMap &request, bool suppressHandler, DBusError *error);

    typedef Callback1<void, DBusError*> ConnectCallback;
    void setConnectCallback(const ConnectCallback &cb);

    typedef Callback3<QStringList, uint, const Tp::UIntList &, DBusError*> InspectHandlesCallback;
    void setInspectHandlesCallback(const InspectHandlesCallback &cb);
    QStringList inspectHandles(uint handleType, const Tp::UIntList &handles, DBusError *error);

    typedef Callback3<Tp::UIntList, uint, const QStringList &, DBusError*> RequestHandlesCallback;
    void setRequestHandlesCallback(const RequestHandlesCallback &cb);
    Tp::UIntList requestHandles(uint handleType, const QStringList &identifiers, DBusError *error);

    Tp::ChannelInfoList channelsInfo();
    Tp::ChannelDetailsList channelsDetails();

    BaseChannelPtr ensureChannel(const QVariantMap &request, bool &yours, bool suppressHandler, DBusError *error);

    void addChannel(BaseChannelPtr channel, bool suppressHandler = false);

    QList<AbstractConnectionInterfacePtr> interfaces() const;
    AbstractConnectionInterfacePtr interface(const QString  &interfaceName) const;
    bool plugInterface(const AbstractConnectionInterfacePtr &interface);
    bool registerObject(DBusError *error = NULL);

    virtual QString uniqueName() const;

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

    virtual bool matchChannel(const Tp::BaseChannelPtr &channel, const QVariantMap &request, Tp::DBusError *error);

private:
    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
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

    struct Private;
    friend struct Private;
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
    static BaseConnectionSimplePresenceInterfacePtr create()
    {
        return BaseConnectionSimplePresenceInterfacePtr(new BaseConnectionSimplePresenceInterface());
    }
    template<typename BaseConnectionSimplePresenceInterfaceSubclass>
    static SharedPtr<BaseConnectionSimplePresenceInterfaceSubclass> create()
    {
        return SharedPtr<BaseConnectionSimplePresenceInterfaceSubclass>(
                new BaseConnectionSimplePresenceInterfaceSubclass());
    }

    virtual ~BaseConnectionSimplePresenceInterface();

    QVariantMap immutableProperties() const;

    Tp::SimpleStatusSpecMap statuses() const;
    void setStatuses(const Tp::SimpleStatusSpecMap &statuses);

    uint maximumStatusMessageLength() const;
    void setMaximumStatusMessageLength(uint maximumStatusMessageLength);

    typedef Callback3<uint, const QString &, const QString &, DBusError*> SetPresenceCallback;
    void setSetPresenceCallback(const SetPresenceCallback &cb);

    void setPresences(const Tp::SimpleContactPresences &presences);

    Tp::SimpleContactPresences getPresences(const Tp::UIntList &contacts);

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

class TP_QT_EXPORT BaseConnectionAvatarsInterface : public AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionAvatarsInterface)

public:
    static BaseConnectionAvatarsInterfacePtr create()
    {
        return BaseConnectionAvatarsInterfacePtr(new BaseConnectionAvatarsInterface());
    }
    template<typename BaseConnectionAvatarsInterfaceSubclass>
    static SharedPtr<BaseConnectionAvatarsInterfaceSubclass> create()
    {
        return SharedPtr<BaseConnectionAvatarsInterfaceSubclass>(
                new BaseConnectionAvatarsInterfaceSubclass());
    }

    virtual ~BaseConnectionAvatarsInterface();

    QVariantMap immutableProperties() const;

    AvatarSpec avatarDetails() const;
    void setAvatarDetails(const AvatarSpec &spec);

    typedef Callback2<Tp::AvatarTokenMap, const Tp::UIntList &, DBusError*> GetKnownAvatarTokensCallback;
    void setGetKnownAvatarTokensCallback(const GetKnownAvatarTokensCallback &cb);
    Tp::AvatarTokenMap getKnownAvatarTokens(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback2<void, const Tp::UIntList &, DBusError*> RequestAvatarsCallback;
    void setRequestAvatarsCallback(const RequestAvatarsCallback &cb);
    void requestAvatars(const Tp::UIntList &contacts, DBusError *error);

    typedef Callback3<QString, const QByteArray &, const QString &, DBusError*> SetAvatarCallback;
    void setSetAvatarCallback(const SetAvatarCallback &cb);
    QString setAvatar(const QByteArray &avatar, const QString &mimeType, DBusError *error);

    typedef Callback1<void, DBusError*> ClearAvatarCallback;
    void setClearAvatarCallback(const ClearAvatarCallback &cb);
    void clearAvatar(DBusError *error);

    void avatarUpdated(uint contact, const QString &newAvatarToken);
    void avatarRetrieved(uint contact, const QString &token, const QByteArray &avatar, const QString &type);

protected:
    BaseConnectionAvatarsInterface();

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
