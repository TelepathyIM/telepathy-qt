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

#ifndef _TelepathyQt4_account_h_HEADER_GUARD_
#define _TelepathyQt4_account_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-account.h>

#include <TelepathyQt4/ChannelRequestHints>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ChannelDispatcherInterface>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/FileTransferChannelCreationProperties>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/Presence>
#include <TelepathyQt4/PresenceSpec>
#include <TelepathyQt4/ProtocolInfo>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/SharedPtr>

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class Account;
class Connection;
class PendingChannel;
class PendingChannelRequest;
class PendingConnection;
class PendingOperation;
class PendingReady;
class PendingStringList;

class TELEPATHY_QT4_EXPORT Account : public StatelessDBusProxy,
                public OptionalInterfaceFactory<Account>
{
    Q_OBJECT
    Q_DISABLE_COPY(Account)
    Q_PROPERTY(bool valid READ isValidAccount NOTIFY validityChanged)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY stateChanged)
    Q_PROPERTY(QString cmName READ cmName)
    Q_PROPERTY(QString protocolName READ protocolName)
    Q_PROPERTY(QString serviceName READ serviceName NOTIFY serviceNameChanged)
    Q_PROPERTY(ProfilePtr profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(QString nickname READ nickname NOTIFY nicknameChanged)
    Q_PROPERTY(AvatarSpec avatarRequirements READ avatarRequirements)
    Q_PROPERTY(Avatar avatar READ avatar NOTIFY avatarChanged)
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(ProtocolInfo protocolInfo READ protocolInfo)
    Q_PROPERTY(ConnectionCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool hasBeenOnline READ hasBeenOnline)
    Q_PROPERTY(bool connectsAutomatically READ connectsAutomatically NOTIFY connectsAutomaticallyPropertyChanged)
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(ConnectionStatusReason connectionStatusReason READ connectionStatusReason)
    Q_PROPERTY(QString connectionError READ connectionError)
    Q_PROPERTY(Tp::Connection::ErrorDetails connectionErrorDetails READ connectionErrorDetails)
    Q_PROPERTY(ConnectionPtr connection READ connection NOTIFY connectionChanged)
    Q_PROPERTY(bool changingPresence READ isChangingPresence NOTIFY changingPresence)
    Q_PROPERTY(Presence automaticPresence READ automaticPresence NOTIFY automaticPresenceChanged)
    Q_PROPERTY(Presence currentPresence READ currentPresence NOTIFY currentPresenceChanged)
    Q_PROPERTY(Presence requestedPresence READ requestedPresence NOTIFY requestedPresenceChanged)
    Q_PROPERTY(bool online READ isOnline NOTIFY onlinenessChanged)
    Q_PROPERTY(QString uniqueIdentifier READ uniqueIdentifier)
    Q_PROPERTY(QString normalizedName READ normalizedName NOTIFY normalizedNameChanged)

public:
    static const Feature FeatureCore;
    static const Feature FeatureAvatar;
    static const Feature FeatureProtocolInfo;
    static const Feature FeatureCapabilities;
    static const Feature FeatureProfile;

    static AccountPtr create(const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());
    static AccountPtr create(const QDBusConnection &bus,
            const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());
    virtual ~Account();

    ConnectionFactoryConstPtr connectionFactory() const;
    ChannelFactoryConstPtr channelFactory() const;
    ContactFactoryConstPtr contactFactory() const;

    bool isValidAccount() const;

    bool isEnabled() const;
    PendingOperation *setEnabled(bool value);

    QString cmName() const;

    QString protocolName() const;

    QString serviceName() const;
    PendingOperation *setServiceName(const QString &value);

    ProfilePtr profile() const;

    QString displayName() const;
    PendingOperation *setDisplayName(const QString &value);

    QString iconName() const;
    PendingOperation *setIconName(const QString &value);

    QString nickname() const;
    PendingOperation *setNickname(const QString &value);

    AvatarSpec avatarRequirements() const;
    // TODO: We probably want to expose the avatar file name once we have the avatar token and MC
    //       starts sharing the cache used by tp-qt4 and tp-glib and use Tp::AvatarData to represent
    //       it as used in Tp::Contact
    const Avatar &avatar() const;
    PendingOperation *setAvatar(const Avatar &avatar);

    QVariantMap parameters() const;
    PendingStringList *updateParameters(const QVariantMap &set,
            const QStringList &unset);

    ProtocolInfo protocolInfo() const;

    ConnectionCapabilities capabilities() const;

    bool connectsAutomatically() const;
    PendingOperation *setConnectsAutomatically(bool value);

    bool hasBeenOnline() const;

    ConnectionStatus connectionStatus() const;
    ConnectionStatusReason connectionStatusReason() const;
    QString connectionError() const;
    Connection::ErrorDetails connectionErrorDetails() const;
    ConnectionPtr connection() const;

    bool isChangingPresence() const;

    PresenceSpecList allowedPresenceStatuses(bool includeAllStatuses = false) const;

    // TODO: Add overload methods to set presence from a Profile::Presence
    // TODO: Add usablePresences() that would return a list of presences that could be set on the
    //       account
    Presence automaticPresence() const;
    PendingOperation *setAutomaticPresence(const Presence &presence);

    Presence currentPresence() const;

    Presence requestedPresence() const;
    PendingOperation *setRequestedPresence(const Presence &presence);

    bool isOnline() const;

    QString uniqueIdentifier() const;

    QString normalizedName() const;

    PendingOperation *reconnect();

    PendingOperation *remove();

    bool supportsRequestHints() const;
    bool requestsSucceedWithChannel() const;

    // TODO ABI break: collapse all of the overloads without a hints arg with the corresponding
    // hints versions and add a default param for the hints args

    PendingChannelRequest *ensureTextChat(
            const QString &contactIdentifier,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureTextChat(
            const QString &contactIdentifier,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureTextChat(
            const ContactPtr &contact,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureTextChat(
            const ContactPtr &contact,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureTextChatroom(
            const QString &roomName,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureTextChatroom(
            const QString &roomName,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureStreamedMediaCall(
            const QString &contactIdentifier,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureStreamedMediaCall(
            const QString &contactIdentifier,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureStreamedMediaCall(
            const ContactPtr &contact,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureStreamedMediaCall(
            const ContactPtr &contact,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureStreamedMediaAudioCall(
            const QString &contactIdentifier,
            QDateTime userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureStreamedMediaAudioCall(
            const QString &contactIdentifier,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureStreamedMediaAudioCall(
            const ContactPtr &contact,
            QDateTime userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureStreamedMediaAudioCall(
            const ContactPtr &contact,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureStreamedMediaVideoCall(
            const QString &contactIdentifier,
            bool withAudio = true,
            QDateTime userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureStreamedMediaVideoCall(
            const QString &contactIdentifier,
            bool withAudio,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureStreamedMediaVideoCall(
            const ContactPtr &contact,
            bool withAudio = true,
            QDateTime userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureStreamedMediaVideoCall(
            const ContactPtr &contact,
            bool withAudio,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *createFileTransfer(
            const QString &contactIdentifier,
            const FileTransferChannelCreationProperties &properties,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createFileTransfer(
            const QString &contactIdentifier,
            const FileTransferChannelCreationProperties &properties,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *createFileTransfer(
            const ContactPtr &contact,
            const FileTransferChannelCreationProperties &properties,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createFileTransfer(
            const ContactPtr &contact,
            const FileTransferChannelCreationProperties &properties,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *createConferenceStreamedMediaCall(
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers = QStringList(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createConferenceStreamedMediaCall(
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *createConferenceStreamedMediaCall(
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts = QList<ContactPtr>(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createConferenceStreamedMediaCall(
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *createConferenceTextChat(
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts = QList<ContactPtr>(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createConferenceTextChat(
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *createConferenceTextChat(
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers = QStringList(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createConferenceTextChat(
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    TELEPATHY_QT4_DEPRECATED PendingChannelRequest *createConferenceTextChatRoom(
            const QString &roomName,
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers = QStringList(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createConferenceTextChatroom(
            const QString &roomName,
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers = QStringList(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString(),
            const ChannelRequestHints &hints = ChannelRequestHints());

    TELEPATHY_QT4_DEPRECATED PendingChannelRequest *createConferenceTextChatRoom(
            const QString &roomName,
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts = QList<ContactPtr>(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createConferenceTextChatroom(
            const QString &roomName,
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts = QList<ContactPtr>(),
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString(),
            const ChannelRequestHints &hints = ChannelRequestHints());

    PendingChannelRequest *createContactSearch(
            const QString &server = QString(),
            uint limit = 0,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createContactSearch(
            const QString &server,
            uint limit,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    // advanced
    PendingChannelRequest *createChannel(
            const QVariantMap &requestedProperties,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *createChannel(
            const QVariantMap &requestedProperties,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannelRequest *ensureChannel(
            const QVariantMap &requestedProperties,
            const QDateTime &userActionTime = QDateTime::currentDateTime(),
            const QString &preferredHandler = QString());
    PendingChannelRequest *ensureChannel(
            const QVariantMap &requestedProperties,
            const QDateTime &userActionTime,
            const QString &preferredHandler,
            const ChannelRequestHints &hints);

    PendingChannel *createAndHandleChannel(
            const QVariantMap &requestedProperties,
            const QDateTime &userActionTime);
    PendingChannel *ensureAndHandleChannel(
            const QVariantMap &requestedProperties,
            const QDateTime &userActionTime);

Q_SIGNALS:
    void removed();
    void serviceNameChanged(const QString &serviceName);
    void profileChanged(const Tp::ProfilePtr &profile);
    void displayNameChanged(const QString &displayName);
    void iconNameChanged(const QString &iconName);
    void nicknameChanged(const QString &nickname);
    void normalizedNameChanged(const QString &normalizedName);
    void validityChanged(bool validity);
    void stateChanged(bool state);
    void capabilitiesChanged(const Tp::ConnectionCapabilities &capabilities);
    void connectsAutomaticallyPropertyChanged(bool connectsAutomatically);
    void firstOnline();
    void parametersChanged(const QVariantMap &parameters);
    void changingPresence(bool value);
    void automaticPresenceChanged(const Tp::Presence &automaticPresence);
    void currentPresenceChanged(const Tp::Presence &currentPresence);
    void requestedPresenceChanged(const Tp::Presence &requestedPresence);
    void onlinenessChanged(bool online);
    void avatarChanged(const Tp::Avatar &avatar);
    void connectionStatusChanged(Tp::ConnectionStatus status);
    void connectionChanged(const Tp::ConnectionPtr &connection);

protected:
    friend class PendingChannelRequest; // to access dispatcherInterface()

    Account(const QDBusConnection &bus,
            const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory,
            const Feature &coreFeature);

    Client::AccountInterface *baseInterface() const;
    Client::ChannelDispatcherInterface *dispatcherInterface() const;

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onDispatcherIntrospected(Tp::PendingOperation *op);
    TELEPATHY_QT4_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *);
    TELEPATHY_QT4_NO_EXPORT void gotAvatar(QDBusPendingCallWatcher *);
    TELEPATHY_QT4_NO_EXPORT void onAvatarChanged();
    TELEPATHY_QT4_NO_EXPORT void onConnectionManagerReady(Tp::PendingOperation *);
    TELEPATHY_QT4_NO_EXPORT void onConnectionReady(Tp::PendingOperation *);
    TELEPATHY_QT4_NO_EXPORT void onPropertyChanged(const QVariantMap &delta);
    TELEPATHY_QT4_NO_EXPORT void onRemoved();
    TELEPATHY_QT4_NO_EXPORT void onConnectionBuilt(Tp::PendingOperation *);

private:
    struct Private;
    friend struct Private;

    Private *mPriv;
};

} // Tp

#endif
