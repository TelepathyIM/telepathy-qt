/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
 * @copyright Copyright 2013 Canonical Ltd.
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

#include "TelepathyQt/_gen/svc-channel.h"

#include <TelepathyQt/Global>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/Types>
#include "TelepathyQt/debug-internal.h"


namespace Tp
{

class TP_QT_NO_EXPORT BaseChannel::Adaptee : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString channelType READ channelType)
    Q_PROPERTY(QStringList interfaces READ interfaces)
    Q_PROPERTY(uint targetHandle READ targetHandle)
    Q_PROPERTY(QString targetID READ targetID)
    Q_PROPERTY(uint targetHandleType READ targetHandleType)
    Q_PROPERTY(bool requested READ requested)
    Q_PROPERTY(uint initiatorHandle READ initiatorHandle)
    Q_PROPERTY(QString initiatorID READ initiatorID)
public:
    Adaptee(const QDBusConnection &dbusConnection, BaseChannel *cm);
    ~Adaptee();

    QString channelType() const {
        return mChannel->channelType();
    }
    QStringList interfaces() const;
    uint targetHandle() const {
        return mChannel->targetHandle();
    }
    QString targetID() const {
        return mChannel->targetID();
    }
    uint targetHandleType() const {
        return mChannel->targetHandleType();
    }
    bool requested() const {
        return mChannel->requested();
    }
    uint initiatorHandle() const {
        return mChannel->initiatorHandle();
    }
    QString initiatorID() const {
        return mChannel->initiatorID();
    }

private Q_SLOTS:
    void close(const Tp::Service::ChannelAdaptor::CloseContextPtr &context);
    void getChannelType(const Tp::Service::ChannelAdaptor::GetChannelTypeContextPtr &context) {
        context->setFinished(channelType());
    }

    void getHandle(const Tp::Service::ChannelAdaptor::GetHandleContextPtr &context) {
        context->setFinished(targetHandleType(), targetHandle());
    }

    void getInterfaces(const Tp::Service::ChannelAdaptor::GetInterfacesContextPtr &context) {
        context->setFinished(interfaces());
    }


public:
    BaseChannel *mChannel;
    Service::ChannelAdaptor *mAdaptor;

signals:
    void closed();
};

class TP_QT_NO_EXPORT BaseChannelTextType::Adaptee : public QObject
{
    Q_OBJECT
public:
    Adaptee(BaseChannelTextType *interface);
    ~Adaptee();

public slots:
    void acknowledgePendingMessages(const Tp::UIntList &IDs, const Tp::Service::ChannelTypeTextAdaptor::AcknowledgePendingMessagesContextPtr &context);
//deprecated:
    //void getMessageTypes(const Tp::Service::ChannelTypeTextAdaptor::GetMessageTypesContextPtr &context);
    //void listPendingMessages(bool clear, const Tp::Service::ChannelTypeTextAdaptor::ListPendingMessagesContextPtr &context);
    //void send(uint type, const QString &text, const Tp::Service::ChannelTypeTextAdaptor::SendContextPtr &context);
signals:
    void lostMessage();
    void received(uint ID, uint timestamp, uint sender, uint type, uint flags, const QString &text);
    void sendError(uint error, uint timestamp, uint type, const QString &text);
    void sent(uint timestamp, uint type, const QString &text);

public:
    BaseChannelTextType *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelMessagesInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList supportedContentTypes READ supportedContentTypes)
    Q_PROPERTY(Tp::UIntList messageTypes READ messageTypes)
    Q_PROPERTY(uint messagePartSupportFlags READ messagePartSupportFlags)
    Q_PROPERTY(Tp::MessagePartListList pendingMessages READ pendingMessages)
    Q_PROPERTY(uint deliveryReportingSupport READ deliveryReportingSupport)
public:
    Adaptee(BaseChannelMessagesInterface *interface);
    ~Adaptee();

    QStringList supportedContentTypes() {
        return mInterface->supportedContentTypes();
    }
    Tp::UIntList messageTypes() {
        return mInterface->messageTypes();
    }
    uint messagePartSupportFlags() {
        return mInterface->messagePartSupportFlags();
    }
    uint deliveryReportingSupport() {
        return mInterface->deliveryReportingSupport();
    }
    Tp::MessagePartListList pendingMessages() {
        return mInterface->pendingMessages();
    }

public slots:
    void sendMessage(const Tp::MessagePartList &message, uint flags, const Tp::Service::ChannelInterfaceMessagesAdaptor::SendMessageContextPtr &context);
//deprecated, never implemented:
    //void getPendingMessageContent(uint messageID, const Tp::UIntList &parts, const Tp::Service::ChannelInterfaceMessagesAdaptor::GetPendingMessageContentContextPtr &context);
signals:
    void messageSent(const Tp::MessagePartList &content, uint flags, const QString &messageToken);
    void pendingMessagesRemoved(const Tp::UIntList &messageIDs);
    void messageReceived(const Tp::MessagePartList &message);

public:
    BaseChannelMessagesInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelRoomListType::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString server READ server)

public:
    Adaptee(BaseChannelRoomListType *interface);
    ~Adaptee();

    QString server() const;

private Q_SLOTS:
    void getListingRooms(
            const Tp::Service::ChannelTypeRoomListAdaptor::GetListingRoomsContextPtr &context);
    void listRooms(
            const Tp::Service::ChannelTypeRoomListAdaptor::ListRoomsContextPtr &context);
    void stopListing(
            const Tp::Service::ChannelTypeRoomListAdaptor::StopListingContextPtr &context);

Q_SIGNALS:
    void gotRooms(const Tp::RoomInfoList &rooms);
    void listingRooms(bool listing);

private:
    BaseChannelRoomListType *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelServerAuthenticationType::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString authenticationMethod READ authenticationMethod)
public:
    Adaptee(BaseChannelServerAuthenticationType *interface);
    ~Adaptee();
    QString authenticationMethod() const;
public:
    BaseChannelServerAuthenticationType *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelCaptchaAuthenticationInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool canRetryCaptcha READ canRetryCaptcha)
    Q_PROPERTY(uint captchaStatus READ captchaStatus)
    Q_PROPERTY(QString captchaError READ captchaError)
    Q_PROPERTY(QVariantMap captchaErrorDetails READ captchaErrorDetails)
public:
    Adaptee(BaseChannelCaptchaAuthenticationInterface *interface);
    ~Adaptee();
    bool canRetryCaptcha() const;
    uint captchaStatus() const;
    QString captchaError() const;
    QVariantMap captchaErrorDetails() const;
public slots:
    void getCaptchas(const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::GetCaptchasContextPtr &context);
    void getCaptchaData(uint ID, const QString &mimeType, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::GetCaptchaDataContextPtr &context);
    void answerCaptchas(const Tp::CaptchaAnswers &answers, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::AnswerCaptchasContextPtr &context);
    void cancelCaptcha(uint reason, const QString &debugMessage, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::CancelCaptchaContextPtr &context);
public:
    BaseChannelCaptchaAuthenticationInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelSASLAuthenticationInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableMechanisms READ availableMechanisms)
    Q_PROPERTY(bool hasInitialData READ hasInitialData)
    Q_PROPERTY(bool canTryAgain READ canTryAgain)
    Q_PROPERTY(uint saslStatus READ saslStatus)
    Q_PROPERTY(QString saslError READ saslError)
    Q_PROPERTY(QVariantMap saslErrorDetails READ saslErrorDetails)
    Q_PROPERTY(QString authorizationIdentity READ authorizationIdentity)
    Q_PROPERTY(QString defaultUsername READ defaultUsername)
    Q_PROPERTY(QString defaultRealm READ defaultRealm)
    Q_PROPERTY(bool maySaveResponse READ maySaveResponse)

public:
    Adaptee(BaseChannelSASLAuthenticationInterface *interface);
    ~Adaptee();

    QStringList availableMechanisms() const;
    bool hasInitialData() const;
    bool canTryAgain() const;
    uint saslStatus() const;
    QString saslError() const;
    QVariantMap saslErrorDetails() const;
    QString authorizationIdentity() const;
    QString defaultUsername() const;
    QString defaultRealm() const;
    bool maySaveResponse() const;

private Q_SLOTS:
    void startMechanism(const QString &mechanism,
            const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::StartMechanismContextPtr &context);
    void startMechanismWithData(const QString &mechanism, const QByteArray &initialData,
            const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::StartMechanismWithDataContextPtr &context);
    void respond(const QByteArray &responseData,
            const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::RespondContextPtr &context);
    void acceptSasl(
            const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::AcceptSASLContextPtr &context);
    void abortSasl(uint reason, const QString &debugMessage,
            const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::AbortSASLContextPtr &context);

signals:
    void saslStatusChanged(uint status, const QString &reason, const QVariantMap &details);
    void newChallenge(const QByteArray &challengeData);

private:
    BaseChannelSASLAuthenticationInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelSecurableInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool encrypted READ encrypted)
    Q_PROPERTY(bool verified READ verified)

public:
    Adaptee(BaseChannelSecurableInterface *interface);
    ~Adaptee();

    bool encrypted() const;
    bool verified() const;

private:
    BaseChannelSecurableInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelChatStateInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Tp::ChatStateMap chatStates READ chatStates)

public:
    Adaptee(BaseChannelChatStateInterface *interface);
    ~Adaptee();

    Tp::ChatStateMap chatStates() const;

private Q_SLOTS:
    void setChatState(uint state,
            const Tp::Service::ChannelInterfaceChatStateAdaptor::SetChatStateContextPtr &context);

signals:
    void chatStateChanged(uint contact, uint state);

private:
    BaseChannelChatStateInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelGroupInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint groupFlags READ groupFlags)
    Q_PROPERTY(Tp::HandleOwnerMap handleOwners READ handleOwners)
    Q_PROPERTY(Tp::LocalPendingInfoList localPendingMembers READ localPendingMembers)
    Q_PROPERTY(Tp::UIntList members READ members)
    Q_PROPERTY(Tp::UIntList remotePendingMembers READ remotePendingMembers)
    Q_PROPERTY(uint selfHandle READ selfHandle)
    Q_PROPERTY(Tp::HandleIdentifierMap memberIdentifiers READ memberIdentifiers)
public:
    Adaptee(BaseChannelGroupInterface *interface);
    ~Adaptee();
    uint groupFlags() const;
    Tp::HandleOwnerMap handleOwners() const;
    Tp::LocalPendingInfoList localPendingMembers() const;
    Tp::UIntList members() const;
    Tp::UIntList remotePendingMembers() const;
    uint selfHandle() const;
    Tp::HandleIdentifierMap memberIdentifiers() const;
public slots:
    void addMembers(const Tp::UIntList &contacts, const QString &message, const Tp::Service::ChannelInterfaceGroupAdaptor::AddMembersContextPtr &context);
    void getAllMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetAllMembersContextPtr &context);
    void getGroupFlags(const Tp::Service::ChannelInterfaceGroupAdaptor::GetGroupFlagsContextPtr &context);
    void getHandleOwners(const Tp::UIntList &handles, const Tp::Service::ChannelInterfaceGroupAdaptor::GetHandleOwnersContextPtr &context);
    void getLocalPendingMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetLocalPendingMembersContextPtr &context);
    void getLocalPendingMembersWithInfo(const Tp::Service::ChannelInterfaceGroupAdaptor::GetLocalPendingMembersWithInfoContextPtr &context);
    void getMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetMembersContextPtr &context);
    void getRemotePendingMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetRemotePendingMembersContextPtr &context);
    void getSelfHandle(const Tp::Service::ChannelInterfaceGroupAdaptor::GetSelfHandleContextPtr &context);
    void removeMembers(const Tp::UIntList &contacts, const QString &message, const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersContextPtr &context);
    void removeMembersWithReason(const Tp::UIntList &contacts, const QString &message, uint reason, const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersWithReasonContextPtr &context);
signals:
    void handleOwnersChangedDetailed(const Tp::HandleOwnerMap &added, const Tp::UIntList &removed, const Tp::HandleIdentifierMap &identifiers);
    void selfContactChanged(uint selfHandle, const QString &selfID);
    void groupFlagsChanged(uint added, uint removed);
    void membersChanged(const QString &message, const Tp::UIntList &added, const Tp::UIntList &removed, const Tp::UIntList &localPending, const Tp::UIntList &remotePending, uint actor, uint reason);
    void membersChangedDetailed(const Tp::UIntList &added, const Tp::UIntList &removed, const Tp::UIntList &localPending, const Tp::UIntList &remotePending, const QVariantMap &details);
    //All other signals are deprecated
public:
    BaseChannelGroupInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelRoomInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString roomName READ roomName)
    Q_PROPERTY(QString server READ server)
    Q_PROPERTY(QString creator READ creator)
    Q_PROPERTY(uint creatorHandle READ creatorHandle)
    Q_PROPERTY(QDateTime creationTimestamp READ creationTimestamp)

public:
    Adaptee(BaseChannelRoomInterface *interface);
    ~Adaptee();

    QString roomName() const;
    QString server() const;
    QString creator() const;
    uint creatorHandle() const;
    QDateTime creationTimestamp() const;

private:
    BaseChannelRoomInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelRoomConfigInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool anonymous READ anonymous)
    Q_PROPERTY(bool inviteOnly READ inviteOnly)
    Q_PROPERTY(uint limit READ limit)
    Q_PROPERTY(bool moderated READ moderated)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(bool persistent READ persistent)
    Q_PROPERTY(bool private READ isPrivate)
    Q_PROPERTY(bool passwordProtected READ passwordProtected)
    Q_PROPERTY(QString password READ password)
    Q_PROPERTY(QString passwordHint READ passwordHint)
    Q_PROPERTY(bool canUpdateConfiguration READ canUpdateConfiguration)
    Q_PROPERTY(QStringList mutableProperties READ mutableProperties)
    Q_PROPERTY(bool configurationRetrieved READ configurationRetrieved)

public:
    Adaptee(BaseChannelRoomConfigInterface *interface);
    ~Adaptee();

    bool anonymous() const;
    bool inviteOnly() const;
    uint limit() const;
    bool moderated() const;
    QString title() const;
    QString description() const;
    bool persistent() const;
    bool isPrivate() const;
    bool passwordProtected() const;
    QString password() const;
    QString passwordHint() const;
    bool canUpdateConfiguration() const;
    QStringList mutableProperties() const;
    bool configurationRetrieved() const;

private Q_SLOTS:
    void updateConfiguration(const QVariantMap &properties,
            const Tp::Service::ChannelInterfaceRoomConfigAdaptor::UpdateConfigurationContextPtr &context);

private:
    BaseChannelRoomConfigInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelCallType::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Tp::ObjectPathList contents READ contents)
    Q_PROPERTY(QVariantMap callStateDetails READ callStateDetails)
    Q_PROPERTY(uint callState READ callState)
    Q_PROPERTY(uint callFlags READ callFlags)
    Q_PROPERTY(Tp::CallStateReason callStateReason READ callStateReason)
    Q_PROPERTY(bool hardwareStreaming READ hardwareStreaming)
    Q_PROPERTY(Tp::CallMemberMap callMembers READ callMembers)
    Q_PROPERTY(Tp::HandleIdentifierMap memberIdentifiers READ memberIdentifiers)
    Q_PROPERTY(uint initialTransport READ initialTransport)
    Q_PROPERTY(bool initialAudio READ initialAudio)
    Q_PROPERTY(bool initialVideo READ initialVideo)
    Q_PROPERTY(QString initialVideoName READ initialVideoName)
    Q_PROPERTY(QString initialAudioName READ initialAudioName)
    Q_PROPERTY(bool mutableContents READ mutableContents)

public:
    Adaptee(BaseChannelCallType *interface);
    ~Adaptee();

    Tp::ObjectPathList contents() const {
        return mInterface->contents();
    }

    QVariantMap callStateDetails() const {
        return mInterface->callStateDetails();
    }

    uint callState() const {
        return mInterface->callState();
    }

    uint callFlags() const {
        return mInterface->callFlags();
    }

    Tp::CallStateReason callStateReason() const {
        return mInterface->callStateReason();
    }

    bool hardwareStreaming() const {
        return mInterface->hardwareStreaming();
    }

    Tp::CallMemberMap callMembers() const {
        return mInterface->callMembers();
    }

    Tp::HandleIdentifierMap memberIdentifiers() const {
        return mInterface->memberIdentifiers();
    }

    uint initialTransport() const {
        return mInterface->initialTransport();
    }

    bool initialAudio() const {
        return mInterface->initialAudio();
    }

    bool initialVideo() const {
        return mInterface->initialVideo();
    }

    QString initialVideoName() const {
        return mInterface->initialVideoName();
    }

    QString initialAudioName() const {
        return mInterface->initialAudioName();
    }

    bool mutableContents() const {
        return mInterface->mutableContents();
    }

public slots:
    void setRinging(const Tp::Service::ChannelTypeCallAdaptor::SetRingingContextPtr &context);
    void setQueued(const Tp::Service::ChannelTypeCallAdaptor::SetQueuedContextPtr &context);
    void accept(const Tp::Service::ChannelTypeCallAdaptor::AcceptContextPtr &context);
    void hangup(uint reason, const QString &detailedHangupReason, const QString &message, const Tp::Service::ChannelTypeCallAdaptor::HangupContextPtr &context);
    void addContent(const QString &contentName, const Tp::MediaStreamType &contentType, const Tp::MediaStreamDirection &initialDirection, const Tp::Service::ChannelTypeCallAdaptor::AddContentContextPtr &context);

signals:
    void contentAdded(const QDBusObjectPath &content);
    void contentRemoved(const QDBusObjectPath &content, const Tp::CallStateReason &reason);
    void callStateChanged(uint callState, uint callFlags, const Tp::CallStateReason &stateReason, const QVariantMap &callStateDetails);
    void callMembersChanged(const Tp::CallMemberMap &flagsChanged, const Tp::HandleIdentifierMap &identifiers, const Tp::UIntList &removed, const Tp::CallStateReason &reason);

public:
    BaseChannelCallType *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelSMSInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool flash READ flash)
    Q_PROPERTY(bool smsChannel READ smsChannel)
public:
    Adaptee(BaseChannelSMSInterface *interface);
    ~Adaptee();

    bool flash() {
        return mInterface->flash();
    }

    bool smsChannel() {
        return mInterface->smsChannel();
    }

public slots:
    void getSMSLength(const Tp::MessagePartList &messages, const Tp::Service::ChannelInterfaceSMSAdaptor::GetSMSLengthContextPtr &context);
signals:
    void smsChannelChanged(bool smsChannel);
public:
    BaseChannelSMSInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelHoldInterface::Adaptee : public QObject
{
    Q_OBJECT
public:
    Adaptee(BaseChannelHoldInterface *interface);
    ~Adaptee();

public slots:
    void getHoldState(const Tp::Service::ChannelInterfaceHoldAdaptor::GetHoldStateContextPtr &context);
    void requestHold(bool hold, const Tp::Service::ChannelInterfaceHoldAdaptor::RequestHoldContextPtr &context);
signals:
    void holdStateChanged(uint holdState, uint reason);

public:
    BaseChannelHoldInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelConferenceInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Tp::ObjectPathList channels READ channels)
    Q_PROPERTY(Tp::ObjectPathList initialChannels READ initialChannels)
    Q_PROPERTY(Tp::UIntList initialInviteeHandles READ initialInviteeHandles)
    Q_PROPERTY(QStringList initialInviteeIDs READ initialInviteeIDs)
    Q_PROPERTY(QString invitationMessage READ invitationMessage)
    Q_PROPERTY(ChannelOriginatorMap originalChannels READ originalChannels)
public:
    Adaptee(BaseChannelConferenceInterface *interface);
    ~Adaptee();
    Tp::ObjectPathList channels() const {
        return mInterface->channels();
    }
    Tp::ObjectPathList initialChannels() const {
        return mInterface->initialChannels();
    }
    Tp::UIntList initialInviteeHandles() const {
        return mInterface->initialInviteeHandles();
    }
    QStringList initialInviteeIDs() const {
        return mInterface->initialInviteeIDs();
    }
    QString invitationMessage() const {
        return mInterface->invitationMessage();
    }
    ChannelOriginatorMap originalChannels() const {
        return mInterface->originalChannels();
    }

signals:
    void channelMerged(const QDBusObjectPath &channel, uint channelHandle, const QVariantMap &properties);
    void channelRemoved(const QDBusObjectPath &channel, const QVariantMap& details);

public:
    BaseChannelConferenceInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelMergeableConferenceInterface::Adaptee : public QObject
{
    Q_OBJECT
public:
    Adaptee(BaseChannelMergeableConferenceInterface *interface);
    ~Adaptee();

public slots:
    void merge(const QDBusObjectPath &channel, const Tp::Service::ChannelInterfaceMergeableConferenceAdaptor::MergeContextPtr &context);

public:
    BaseChannelMergeableConferenceInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelSplittableInterface::Adaptee : public QObject
{
    Q_OBJECT
public:
    Adaptee(BaseChannelSplittableInterface *interface);
    ~Adaptee();

public slots:
    void split(const Tp::Service::ChannelInterfaceSplittableAdaptor::SplitContextPtr &context);

public:
    BaseChannelSplittableInterface *mInterface;
};

}
