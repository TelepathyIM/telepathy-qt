/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
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

#ifndef _TelepathyQt_base_channel_h_HEADER_GUARD_
#define _TelepathyQt_base_channel_h_HEADER_GUARD_

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

class TP_QT_EXPORT BaseChannel : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannel)

public:
    static BaseChannelPtr create(BaseConnection *connection, const QString &channelType,
                                 Tp::HandleType targetHandleType = Tp::HandleTypeNone, uint targetHandle = 0) {
        return BaseChannelPtr(new BaseChannel(QDBusConnection::sessionBus(), connection,
                                              channelType, targetHandleType, targetHandle));
    }

    virtual ~BaseChannel();

    QVariantMap immutableProperties() const;
    bool registerObject(DBusError *error = NULL);
    virtual QString uniqueName() const;

    QString channelType() const;
    QList<AbstractChannelInterfacePtr> interfaces() const;
    AbstractChannelInterfacePtr interface(const QString &interfaceName) const;
    uint targetHandle() const;
    QString targetID() const;
    uint targetHandleType() const;
    bool requested() const;
    uint initiatorHandle() const;
    QString initiatorID() const;
    Tp::ChannelDetails details() const;

    void setInitiatorHandle(uint initiatorHandle);
    void setInitiatorID(const QString &initiatorID);
    void setTargetID(const QString &targetID);
    void setRequested(bool requested);

    void close();

    bool plugInterface(const AbstractChannelInterfacePtr &interface);

Q_SIGNALS:
    void closed();
protected:
    BaseChannel(const QDBusConnection &dbusConnection, BaseConnection *connection,
                const QString &channelType, uint targetHandleType, uint targetHandle);
    virtual bool registerObject(const QString &busName, const QString &objectPath,
                                DBusError *error);
private:
    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractChannelInterface : public AbstractDBusServiceInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractChannelInterface)

public:
    AbstractChannelInterface(const QString &interfaceName);
    virtual ~AbstractChannelInterface();

private:
    friend class BaseChannel;

    struct Private;
    friend struct Private;
    Private *mPriv;

};

class TP_QT_EXPORT BaseChannelTextType : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelTextType)

public:
    static BaseChannelTextTypePtr create(BaseChannel* channel) {
        return BaseChannelTextTypePtr(new BaseChannelTextType(channel));
    }
    template<typename BaseChannelTextTypeSubclass>
    static SharedPtr<BaseChannelTextTypeSubclass> create(BaseChannel* channel) {
        return SharedPtr<BaseChannelTextTypeSubclass>(
                   new BaseChannelTextTypeSubclass(channel));
    }

    typedef Callback2<QDBusObjectPath, const QVariantMap&, DBusError*> CreateChannelCallback;
    CreateChannelCallback createChannel;

    typedef Callback2<bool, const QVariantMap&, DBusError*> EnsureChannelCallback;
    EnsureChannelCallback ensureChannel;

    virtual ~BaseChannelTextType();

    QVariantMap immutableProperties() const;

    Tp::RequestableChannelClassList requestableChannelClasses;

    typedef Callback1<void, QString> MessageAcknowledgedCallback;
    void setMessageAcknowledgedCallback(const MessageAcknowledgedCallback &cb);

    Tp::MessagePartListList pendingMessages();

    /* Convenience function */
    void addReceivedMessage(const Tp::MessagePartList &message);
private Q_SLOTS:
    void sent(uint timestamp, uint type, QString text);
protected:
    BaseChannelTextType(BaseChannel* channel);
    void acknowledgePendingMessages(const Tp::UIntList &IDs, DBusError* error);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelMessagesInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelMessagesInterface)

public:
    static BaseChannelMessagesInterfacePtr create(BaseChannelTextType* textTypeInterface,
            QStringList supportedContentTypes,
            UIntList messageTypes,
            uint messagePartSupportFlags,
            uint deliveryReportingSupport) {
        return BaseChannelMessagesInterfacePtr(new BaseChannelMessagesInterface(textTypeInterface,
                                               supportedContentTypes,
                                               messageTypes,
                                               messagePartSupportFlags,
                                               deliveryReportingSupport));
    }
    template<typename BaseChannelMessagesInterfaceSubclass>
    static SharedPtr<BaseChannelMessagesInterfaceSubclass> create() {
        return SharedPtr<BaseChannelMessagesInterfaceSubclass>(
                   new BaseChannelMessagesInterfaceSubclass());
    }
    virtual ~BaseChannelMessagesInterface();

    QVariantMap immutableProperties() const;

    QStringList supportedContentTypes();
    Tp::UIntList messageTypes();
    uint messagePartSupportFlags();
    uint deliveryReportingSupport();
    Tp::MessagePartListList pendingMessages();

    void messageSent(const Tp::MessagePartList &content, uint flags, const QString &messageToken);

    typedef Callback3<QString, const Tp::MessagePartList&, uint, DBusError*> SendMessageCallback;
    void setSendMessageCallback(const SendMessageCallback &cb);
protected:
    QString sendMessage(const Tp::MessagePartList &message, uint flags, DBusError* error);
private Q_SLOTS:
    void pendingMessagesRemoved(const Tp::UIntList &messageIDs);
    void messageReceived(const Tp::MessagePartList &message);
private:
    BaseChannelMessagesInterface(BaseChannelTextType* textType,
                                 QStringList supportedContentTypes,
                                 Tp::UIntList messageTypes,
                                 uint messagePartSupportFlags,
                                 uint deliveryReportingSupport);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelRoomListType : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelRoomListType)

public:
    static BaseChannelRoomListTypePtr create(const QString &server = QString())
    {
        return BaseChannelRoomListTypePtr(new BaseChannelRoomListType(server));
    }
    template<typename BaseChannelRoomListTypeSubclass>
    static SharedPtr<BaseChannelRoomListTypeSubclass> create(const QString &server = QString())
    {
        return SharedPtr<BaseChannelRoomListTypeSubclass>(
                new BaseChannelRoomListTypeSubclass(server));
    }

    virtual ~BaseChannelRoomListType();

    QVariantMap immutableProperties() const;

    QString server() const;

    bool getListingRooms();
    void setListingRooms(bool listing);

    typedef Callback1<void, DBusError*> ListRoomsCallback;
    void setListRoomsCallback(const ListRoomsCallback &cb);
    void listRooms(DBusError *error);

    typedef Callback1<void, DBusError*> StopListingCallback;
    void setStopListingCallback(const StopListingCallback &cb);
    void stopListing(DBusError *error);

    void gotRooms(const Tp::RoomInfoList &rooms);

protected:
    BaseChannelRoomListType(const QString &server);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelServerAuthenticationType : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelServerAuthenticationType)

public:
    static BaseChannelServerAuthenticationTypePtr create(const QString &authenticationMethod) {
        return BaseChannelServerAuthenticationTypePtr(new BaseChannelServerAuthenticationType(authenticationMethod));
    }
    template<typename BaseChannelServerAuthenticationTypeSubclass>
    static SharedPtr<BaseChannelServerAuthenticationTypeSubclass> create(const QString &authenticationMethod) {
        return SharedPtr<BaseChannelServerAuthenticationTypeSubclass>(
                   new BaseChannelServerAuthenticationTypeSubclass(authenticationMethod));
    }
    virtual ~BaseChannelServerAuthenticationType();

    QVariantMap immutableProperties() const;
private Q_SLOTS:
private:
    BaseChannelServerAuthenticationType(const QString &authenticationMethod);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelCaptchaAuthenticationInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelCaptchaAuthenticationInterface)

public:
    static BaseChannelCaptchaAuthenticationInterfacePtr create(bool canRetryCaptcha) {
        return BaseChannelCaptchaAuthenticationInterfacePtr(new BaseChannelCaptchaAuthenticationInterface(canRetryCaptcha));
    }
    template<typename BaseChannelCaptchaAuthenticationInterfaceSubclass>
    static SharedPtr<BaseChannelCaptchaAuthenticationInterfaceSubclass> create(bool canRetryCaptcha) {
        return SharedPtr<BaseChannelCaptchaAuthenticationInterfaceSubclass>(
                   new BaseChannelCaptchaAuthenticationInterfaceSubclass(canRetryCaptcha));
    }
    virtual ~BaseChannelCaptchaAuthenticationInterface();

    QVariantMap immutableProperties() const;

    typedef Callback4<void, Tp::CaptchaInfoList&, uint&, QString&, DBusError*> GetCaptchasCallback;
    void setGetCaptchasCallback(const GetCaptchasCallback &cb);

    typedef Callback3<QByteArray, uint, const QString&, DBusError*> GetCaptchaDataCallback;
    void setGetCaptchaDataCallback(const GetCaptchaDataCallback &cb);

    typedef Callback2<void, const Tp::CaptchaAnswers&, DBusError*> AnswerCaptchasCallback;
    void setAnswerCaptchasCallback(const AnswerCaptchasCallback &cb);

    typedef Callback3<void, uint, const QString&, DBusError*> CancelCaptchaCallback;
    void setCancelCaptchaCallback(const CancelCaptchaCallback &cb);

    void setCaptchaStatus(uint status);
    void setCaptchaError(const QString &busName);
    void setCaptchaErrorDetails(const QVariantMap &error);
private Q_SLOTS:
private:
    BaseChannelCaptchaAuthenticationInterface(bool canRetryCaptcha);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelSASLAuthenticationInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelSASLAuthenticationInterface)

public:
    static BaseChannelSASLAuthenticationInterfacePtr create(const QStringList &availableMechanisms,
                                                            bool hasInitialData,
                                                            bool canTryAgain,
                                                            const QString &authorizationIdentity,
                                                            const QString &defaultUsername,
                                                            const QString &defaultRealm,
                                                            bool maySaveResponse)
    {
        return BaseChannelSASLAuthenticationInterfacePtr(new BaseChannelSASLAuthenticationInterface(availableMechanisms,
                                                                                                    hasInitialData,
                                                                                                    canTryAgain,
                                                                                                    authorizationIdentity,
                                                                                                    defaultUsername,
                                                                                                    defaultRealm,
                                                                                                    maySaveResponse));
    }
    template<typename BaseChannelSASLAuthenticationInterfaceSubclass>
    static SharedPtr<BaseChannelSASLAuthenticationInterfaceSubclass> create(const QStringList &availableMechanisms,
                                                                            bool hasInitialData,
                                                                            bool canTryAgain,
                                                                            const QString &authorizationIdentity,
                                                                            const QString &defaultUsername,
                                                                            const QString &defaultRealm,
                                                                            bool maySaveResponse)
    {
        return SharedPtr<BaseChannelSASLAuthenticationInterfaceSubclass>(
                new BaseChannelSASLAuthenticationInterfaceSubclass(availableMechanisms,
                                                                   hasInitialData,
                                                                   canTryAgain,
                                                                   authorizationIdentity,
                                                                   defaultUsername,
                                                                   defaultRealm,
                                                                   maySaveResponse));
    }

    virtual ~BaseChannelSASLAuthenticationInterface();

    QVariantMap immutableProperties() const;

    QStringList availableMechanisms() const;
    bool hasInitialData() const;
    bool canTryAgain() const;
    QString authorizationIdentity() const;
    QString defaultUsername() const;
    QString defaultRealm() const;
    bool maySaveResponse() const;

    uint saslStatus() const;
    void setSaslStatus(uint status, const QString &reason, const QVariantMap &details);

    QString saslError() const;
    void setSaslError(const QString &saslError);

    QVariantMap saslErrorDetails() const;
    void setSaslErrorDetails(const QVariantMap &saslErrorDetails);

    typedef Callback2<void, const QString &, DBusError*> StartMechanismCallback;
    void setStartMechanismCallback(const StartMechanismCallback &cb);
    void startMechanism(const QString &mechanism, DBusError *error);

    typedef Callback3<void, const QString &, const QByteArray &, DBusError*> StartMechanismWithDataCallback;
    void setStartMechanismWithDataCallback(const StartMechanismWithDataCallback &cb);
    void startMechanismWithData(const QString &mechanism, const QByteArray &initialData, DBusError *error);

    typedef Callback2<void, const QByteArray &, DBusError*> RespondCallback;
    void setRespondCallback(const RespondCallback &cb);
    void respond(const QByteArray &responseData, DBusError *error);

    typedef Callback1<void, DBusError*> AcceptSASLCallback;
    void setAcceptSaslCallback(const AcceptSASLCallback &cb);
    void acceptSasl(DBusError *error);

    typedef Callback3<void, uint, const QString &, DBusError*> AbortSASLCallback;
    void setAbortSaslCallback(const AbortSASLCallback &cb);
    void abortSasl(uint reason, const QString &debugMessage, DBusError *error);

    void newChallenge(const QByteArray &challengeData);

protected:
    BaseChannelSASLAuthenticationInterface(const QStringList &availableMechanisms,
                                           bool hasInitialData,
                                           bool canTryAgain,
                                           const QString &authorizationIdentity,
                                           const QString &defaultUsername,
                                           const QString &defaultRealm,
                                           bool maySaveResponse);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelSecurableInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelSecurableInterface)

public:
    static BaseChannelSecurableInterfacePtr create()
    {
        return BaseChannelSecurableInterfacePtr(new BaseChannelSecurableInterface());
    }
    template<typename BaseChannelSecurableInterfaceSubclass>
    static SharedPtr<BaseChannelSecurableInterfaceSubclass> create()
    {
        return SharedPtr<BaseChannelSecurableInterfaceSubclass>(
                new BaseChannelSecurableInterfaceSubclass());
    }

    virtual ~BaseChannelSecurableInterface();

    QVariantMap immutableProperties() const;

    bool encrypted() const;
    void setEncrypted(bool encrypted);

    bool verified() const;
    void setVerified(bool verified);

protected:
    BaseChannelSecurableInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelChatStateInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelChatStateInterface)

public:
    static BaseChannelChatStateInterfacePtr create()
    {
        return BaseChannelChatStateInterfacePtr(new BaseChannelChatStateInterface());
    }
    template<typename BaseChannelChatStateInterfaceSubclass>
    static SharedPtr<BaseChannelChatStateInterfaceSubclass> create()
    {
        return SharedPtr<BaseChannelChatStateInterfaceSubclass>(
                new BaseChannelChatStateInterfaceSubclass());
    }

    virtual ~BaseChannelChatStateInterface();

    QVariantMap immutableProperties() const;

    Tp::ChatStateMap chatStates() const;
    void setChatStates(const Tp::ChatStateMap &chatStates);

    typedef Callback2<void, uint, DBusError*> SetChatStateCallback;
    void setSetChatStateCallback(const SetChatStateCallback &cb);
    void setChatState(uint state, DBusError *error);

    void chatStateChanged(uint contact, uint state);

protected:
    BaseChannelChatStateInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelGroupInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelGroupInterface)

public:
    static BaseChannelGroupInterfacePtr create(ChannelGroupFlags initialFlags, uint selfHandle) {
        return BaseChannelGroupInterfacePtr(new BaseChannelGroupInterface(initialFlags, selfHandle));
    }
    template<typename BaseChannelGroupInterfaceSubclass>
    static SharedPtr<BaseChannelGroupInterfaceSubclass> create(ChannelGroupFlags initialFlags, uint selfHandle) {
        return SharedPtr<BaseChannelGroupInterfaceSubclass>(
                   new BaseChannelGroupInterfaceSubclass(initialFlags, selfHandle));
    }
    virtual ~BaseChannelGroupInterface();

    QVariantMap immutableProperties() const;

    typedef Callback3<void, const Tp::UIntList&, const QString&, DBusError*> RemoveMembersCallback;
    void setRemoveMembersCallback(const RemoveMembersCallback &cb);

    typedef Callback3<void, const Tp::UIntList&, const QString&, DBusError*> AddMembersCallback;
    void setAddMembersCallback(const AddMembersCallback &cb);

    /* Adds a contact to this group. No-op if already in this group */
    void addMembers(const Tp::UIntList &handles, const QStringList &identifiers);
    void removeMembers(const Tp::UIntList &handles);

private Q_SLOTS:
private:
    BaseChannelGroupInterface(ChannelGroupFlags initialFlags, uint selfHandle);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelRoomInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelRoomInterface)

public:
    static BaseChannelRoomInterfacePtr create(const QString &roomName,
                                              const QString &server,
                                              const QString &creator,
                                              uint creatorHandle,
                                              const QDateTime &creationTimestamp)
    {
        return BaseChannelRoomInterfacePtr(new BaseChannelRoomInterface(roomName,
                                                                        server,
                                                                        creator,
                                                                        creatorHandle,
                                                                        creationTimestamp));
    }
    template<typename BaseChannelRoomInterfaceSubclass>
    static SharedPtr<BaseChannelRoomInterfaceSubclass> create(const QString &roomName,
                                                              const QString &server,
                                                              const QString &creator,
                                                              uint creatorHandle,
                                                              const QDateTime &creationTimestamp)
    {
        return SharedPtr<BaseChannelRoomInterfaceSubclass>(
                new BaseChannelRoomInterfaceSubclass(roomName,
                                                     server,
                                                     creator,
                                                     creatorHandle,
                                                     creationTimestamp));
    }

    virtual ~BaseChannelRoomInterface();

    QVariantMap immutableProperties() const;

    QString roomName() const;
    QString server() const;
    QString creator() const;
    uint creatorHandle() const;
    QDateTime creationTimestamp() const;

protected:
    BaseChannelRoomInterface(const QString &roomName,
                             const QString &server,
                             const QString &creator,
                             uint creatorHandle,
                             const QDateTime &creationTimestamp);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelRoomConfigInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelRoomConfigInterface)

public:
    static BaseChannelRoomConfigInterfacePtr create()
    {
        return BaseChannelRoomConfigInterfacePtr(new BaseChannelRoomConfigInterface());
    }
    template<typename BaseChannelRoomConfigInterfaceSubclass>
    static SharedPtr<BaseChannelRoomConfigInterfaceSubclass> create()
    {
        return SharedPtr<BaseChannelRoomConfigInterfaceSubclass>(
                new BaseChannelRoomConfigInterfaceSubclass());
    }

    virtual ~BaseChannelRoomConfigInterface();

    QVariantMap immutableProperties() const;

    bool anonymous() const;
    void setAnonymous(bool anonymous);

    bool inviteOnly() const;
    void setInviteOnly(bool inviteOnly);

    uint limit() const;
    void setLimit(uint limit);

    bool moderated() const;
    void setModerated(bool moderated);

    QString title() const;
    void setTitle(const QString &title);

    QString description() const;
    void setDescription(const QString &description);

    bool persistent() const;
    void setPersistent(bool persistent);

    bool isPrivate() const;
    void setPrivate(bool newPrivate);

    bool passwordProtected() const;
    void setPasswordProtected(bool passwordProtected);

    QString password() const;
    void setPassword(const QString &password);

    QString passwordHint() const;
    void setPasswordHint(const QString &passwordHint);

    bool canUpdateConfiguration() const;
    void setCanUpdateConfiguration(bool canUpdateConfiguration);

    QStringList mutableProperties() const;
    void setMutableProperties(const QStringList &mutableProperties);

    bool configurationRetrieved() const;
    void setConfigurationRetrieved(bool configurationRetrieved);

    typedef Callback2<void, const QVariantMap &, DBusError*> UpdateConfigurationCallback;
    void setUpdateConfigurationCallback(const UpdateConfigurationCallback &cb);
    void updateConfiguration(const QVariantMap &properties, DBusError *error);

protected:
    BaseChannelRoomConfigInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelCallType : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelCallType)

public:
    static BaseChannelCallTypePtr create(BaseChannel* channel, bool hardwareStreaming,
                                         uint initialTransport,
                                         bool initialAudio,
                                         bool initialVideo,
                                         QString initialAudioName,
                                         QString initialVideoName,
                                         bool mutableContents = false) {
        return BaseChannelCallTypePtr(new BaseChannelCallType(channel,
                                                              hardwareStreaming,
                                                              initialTransport,
                                                              initialAudio,
                                                              initialVideo,
                                                              initialAudioName,
                                                              initialVideoName,
                                                              mutableContents));
    }
    template<typename BaseChannelCallTypeSubclass>
    static SharedPtr<BaseChannelCallTypeSubclass> create(BaseChannel* channel, bool hardwareStreaming,
                                                         uint initialTransport,
                                                         bool initialAudio,
                                                         bool initialVideo,
                                                         QString initialAudioName,
                                                         QString initialVideoName,
                                                         bool mutableContents = false) {
        return SharedPtr<BaseChannelCallTypeSubclass>(
                   new BaseChannelCallTypeSubclass(channel,
                                                   hardwareStreaming,
                                                   initialTransport,
                                                   initialAudio,
                                                   initialVideo,
                                                   initialAudioName,
                                                   initialVideoName,
                                                   mutableContents));
    }

    typedef Callback2<QDBusObjectPath, const QVariantMap&, DBusError*> CreateChannelCallback;
    CreateChannelCallback createChannel;

    typedef Callback2<bool, const QVariantMap&, DBusError*> EnsureChannelCallback;
    EnsureChannelCallback ensureChannel;

    virtual ~BaseChannelCallType();

    QVariantMap immutableProperties() const;

    Tp::ObjectPathList contents();
    QVariantMap callStateDetails();
    uint callState();
    uint callFlags();
    Tp::CallStateReason callStateReason();
    bool hardwareStreaming();
    Tp::CallMemberMap callMembers();
    Tp::HandleIdentifierMap memberIdentifiers();
    uint initialTransport();
    bool initialAudio();
    bool initialVideo();
    QString initialAudioName();
    QString initialVideoName();
    bool mutableContents();

    typedef Callback1<void, DBusError*> AcceptCallback;
    void setAcceptCallback(const AcceptCallback &cb);

    typedef Callback4<void, uint, const QString &, const QString &, DBusError*> HangupCallback;
    void setHangupCallback(const HangupCallback &cb);

    typedef Callback1<void, DBusError*> SetRingingCallback;
    void setSetRingingCallback(const SetRingingCallback &cb);

    typedef Callback1<void, DBusError*> SetQueuedCallback;
    void setSetQueuedCallback(const SetQueuedCallback &cb);

    typedef Callback4<QDBusObjectPath, const QString&, const Tp::MediaStreamType&, const Tp::MediaStreamDirection&, DBusError*> AddContentCallback;
    void setAddContentCallback(const AddContentCallback &cb);

    void setCallState(const Tp::CallState &state, uint flags, const Tp::CallStateReason &stateReason, const QVariantMap &callStateDetails);
    void setMembersFlags(const Tp::CallMemberMap &flagsChanged, const Tp::HandleIdentifierMap &identifiers, const Tp::UIntList &removed, const Tp::CallStateReason &reason);
    BaseCallContentPtr addContent(const QString &name, const Tp::MediaStreamType &type, const Tp::MediaStreamDirection &direction);
    void addContent(BaseCallContentPtr content);

    Tp::RequestableChannelClassList requestableChannelClasses;

protected:
    BaseChannelCallType(BaseChannel* channel,
                        bool hardwareStreaming,
                        uint initialTransport,
                        bool initialAudio,
                        bool initialVideo,
                        QString initialAudioName,
                        QString initialVideoName,
                        bool mutableContents = false);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelHoldInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelHoldInterface)

public:
    static BaseChannelHoldInterfacePtr create() {
        return BaseChannelHoldInterfacePtr(new BaseChannelHoldInterface());
    }
    template<typename BaseChannelHoldInterfaceSubclass>
    static SharedPtr<BaseChannelHoldInterfaceSubclass> create() {
        return SharedPtr<BaseChannelHoldInterfaceSubclass>(
                   new BaseChannelHoldInterfaceSubclass());
    }
    virtual ~BaseChannelHoldInterface();

    QVariantMap immutableProperties() const;

    Tp::LocalHoldState getHoldState() const;
    Tp::LocalHoldStateReason getHoldReason() const;
    void setHoldState(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason);

    typedef Callback3<void, const Tp::LocalHoldState&, const Tp::LocalHoldStateReason &, DBusError*> SetHoldStateCallback;
    void setSetHoldStateCallback(const SetHoldStateCallback &cb);
Q_SIGNALS:
    void holdStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason);
private:
    BaseChannelHoldInterface();
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelMergeableConferenceInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelMergeableConferenceInterface)

public:
    static BaseChannelMergeableConferenceInterfacePtr create() {
        return BaseChannelMergeableConferenceInterfacePtr(new BaseChannelMergeableConferenceInterface());
    }
    template<typename BaseChannelMergeableConferenceInterfaceSubclass>
    static SharedPtr<BaseChannelMergeableConferenceInterfaceSubclass> create() {
        return SharedPtr<BaseChannelMergeableConferenceInterfaceSubclass>(
                   new BaseChannelMergeableConferenceInterfaceSubclass());
    }
    virtual ~BaseChannelMergeableConferenceInterface();

    QVariantMap immutableProperties() const;

    void merge(const QDBusObjectPath &channel);

    typedef Callback2<void, const QDBusObjectPath&, DBusError*> MergeCallback;
    void setMergeCallback(const MergeCallback &cb);
private:
    BaseChannelMergeableConferenceInterface();
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelSplittableInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelSplittableInterface)

public:
    static BaseChannelSplittableInterfacePtr create() {
        return BaseChannelSplittableInterfacePtr(new BaseChannelSplittableInterface());
    }
    template<typename BaseChannelSplittableInterfaceSubclass>
    static SharedPtr<BaseChannelSplittableInterfaceSubclass> create() {
        return SharedPtr<BaseChannelSplittableInterfaceSubclass>(
                   new BaseChannelSplittableInterfaceSubclass());
    }
    virtual ~BaseChannelSplittableInterface();

    QVariantMap immutableProperties() const;

    void split();

    typedef Callback1<void, DBusError*> SplitCallback;
    void setSplitCallback(const SplitCallback &cb);
private:
    BaseChannelSplittableInterface();
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelConferenceInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelConferenceInterface)

public:
    static BaseChannelConferenceInterfacePtr create(Tp::ObjectPathList initialChannels = Tp::ObjectPathList(),
            Tp::UIntList initialInviteeHandles = Tp::UIntList(),
            QStringList initialInviteeIDs = QStringList(),
            QString invitationMessage = QString(),
            ChannelOriginatorMap originalChannels = ChannelOriginatorMap()) {
        return BaseChannelConferenceInterfacePtr(new BaseChannelConferenceInterface(initialChannels, initialInviteeHandles, initialInviteeIDs, invitationMessage, originalChannels));
    }
    template<typename BaseChannelConferenceInterfaceSubclass>
    static SharedPtr<BaseChannelConferenceInterfaceSubclass> create(Tp::ObjectPathList initialChannels = Tp::ObjectPathList(),
            Tp::UIntList initialInviteeHandles = Tp::UIntList(),
            QStringList initialInviteeIDs = QStringList(),
            QString invitationMessage = QString(),
            ChannelOriginatorMap originalChannels = ChannelOriginatorMap()) {
        return SharedPtr<BaseChannelConferenceInterfaceSubclass>(
                   new BaseChannelConferenceInterfaceSubclass(initialChannels, initialInviteeHandles, initialInviteeIDs, invitationMessage, originalChannels));
    }
    virtual ~BaseChannelConferenceInterface();

    QVariantMap immutableProperties() const;
    Tp::ObjectPathList channels() const;
    Tp::ObjectPathList initialChannels() const;
    Tp::UIntList initialInviteeHandles() const;
    QStringList initialInviteeIDs() const;
    QString invitationMessage() const;
    ChannelOriginatorMap originalChannels() const;

    void mergeChannel(const QDBusObjectPath &channel, uint channelHandle, const QVariantMap &properties);
    void removeChannel(const QDBusObjectPath &channel, const QVariantMap &details);

private:
    BaseChannelConferenceInterface(Tp::ObjectPathList initialChannels, Tp::UIntList initialInviteeHandles, QStringList initialInviteeIDs, QString invitationMessage, ChannelOriginatorMap originalChannels);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelSMSInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelSMSInterface)

public:
    static BaseChannelSMSInterfacePtr create(bool flash, bool smsChannel) {
        return BaseChannelSMSInterfacePtr(new BaseChannelSMSInterface(flash, smsChannel));
    }
    template<typename BaseChannelSMSInterfaceSubclass>
    static SharedPtr<BaseChannelSMSInterfaceSubclass> create(bool flash, bool smsChannel) {
        return SharedPtr<BaseChannelSMSInterfaceSubclass>(
                   new BaseChannelSMSInterfaceSubclass(flash, smsChannel));
    }
    virtual ~BaseChannelSMSInterface();

    QVariantMap immutableProperties() const;

    typedef Callback2<void, const Tp::MessagePartList &, DBusError*> GetSMSLengthCallback;
    void setGetSMSLengthCallback(const GetSMSLengthCallback &cb);

    bool flash() const;
    bool smsChannel() const;

Q_SIGNALS:
    void smsChannelChanged(bool smsChannel);

private:
    BaseChannelSMSInterface(bool flash, bool smsChannel);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
#endif
