/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt/Account>

#include "TelepathyQt/_gen/account.moc.hpp"
#include "TelepathyQt/_gen/cli-account.moc.hpp"
#include "TelepathyQt/_gen/cli-account-body.hpp"

#include "TelepathyQt/debug-internal.h"

#include "TelepathyQt/connection-internal.h"

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelDispatcherInterface>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/Profile>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>

#include <QQueue>
#include <QRegExp>
#include <QSharedPointer>
#include <QTimer>
#include <QPointer>

#include <string.h>

namespace Tp
{

namespace
{

struct PresenceStatusInfo
{
    QString name;
    Tp::SimpleStatusSpec spec;
};

Tp::ConnectionPresenceType presenceTypeForStatus(const QString &status, bool &maySetOnSelf)
{
    static PresenceStatusInfo statuses[] = {
        { QLatin1String("available"), { Tp::ConnectionPresenceTypeAvailable, true, true } },
        { QLatin1String("chat"), { Tp::ConnectionPresenceTypeAvailable, true, true } },
        { QLatin1String("chatty"), { Tp::ConnectionPresenceTypeAvailable, true, true } },
        { QLatin1String("away"), { Tp::ConnectionPresenceTypeAway, true, true } },
        { QLatin1String("brb"), { Tp::ConnectionPresenceTypeAway, true, true } },
        { QLatin1String("out-to-lunch"), { Tp::ConnectionPresenceTypeAway, true, true } },
        { QLatin1String("xa"), { Tp::ConnectionPresenceTypeExtendedAway, true, true } },
        { QLatin1String("hidden"), { Tp::ConnectionPresenceTypeHidden, true, true } },
        { QLatin1String("invisible"), { Tp::ConnectionPresenceTypeHidden, true, true } },
        { QLatin1String("offline"), { Tp::ConnectionPresenceTypeOffline, true, false } },
        { QLatin1String("unknown"), { Tp::ConnectionPresenceTypeUnknown, false, false } },
        { QLatin1String("error"), { Tp::ConnectionPresenceTypeError, false, false } }
    };

    for (uint i = 0; i < sizeof(statuses) / sizeof(PresenceStatusInfo); ++i) {
        if (status == statuses[i].name) {
            maySetOnSelf = statuses[i].spec.maySetOnSelf;
            return (Tp::ConnectionPresenceType) statuses[i].spec.type;
        }
    }

    // fallback to type away if we don't know status
    maySetOnSelf = true;
    return Tp::ConnectionPresenceTypeAway;
}

Tp::PresenceSpec presenceSpecForStatus(const QString &status, bool canHaveStatusMessage)
{
    Tp::SimpleStatusSpec spec;
    spec.type = presenceTypeForStatus(status, spec.maySetOnSelf);
    spec.canHaveMessage = canHaveStatusMessage;
    return Tp::PresenceSpec(status, spec);
}

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
    foreach (const Tp::ChannelPtr &channel, channels) {
        objectPaths << QDBusObjectPath(channel->objectPath());
    }

    request.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"),
            qVariantFromValue(objectPaths));
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
        foreach (const Tp::ContactPtr &contact, initialInviteeContacts) {
            if (!contact) {
                continue;
            }
            handles << contact->handle()[0];
        }
        if (!handles.isEmpty()) {
            request.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE +
                        QLatin1String(".InitialInviteeHandles"), qVariantFromValue(handles));
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

} // anonymous namespace

struct TP_QT_NO_EXPORT Account::Private
{
    Private(Account *parent, const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory);
    ~Private();

    void init();

    static void introspectMain(Private *self);
    static void introspectAvatar(Private *self);
    static void introspectProtocolInfo(Private *self);
    static void introspectCapabilities(Private *self);

    void updateProperties(const QVariantMap &props);
    void retrieveAvatar();
    bool processConnQueue();

    bool checkCapabilitiesChanged(bool profileChanged);

    QString connectionObjectPath() const;

    // Public object
    Account *parent;

    // Factories
    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Instance of generated interface class
    Client::AccountInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    QVariantMap parameters;
    bool valid;
    bool enabled;
    bool connectsAutomatically;
    bool hasBeenOnline;
    bool changingPresence;
    QString cmName;
    QString protocolName;
    QString serviceName;
    ProfilePtr profile;
    QString displayName;
    QString nickname;
    QString iconName;
    QQueue<QString> connObjPathQueue;
    ConnectionPtr connection;
    bool mayFinishCore, coreFinished;
    QString normalizedName;
    Avatar avatar;
    ConnectionManagerPtr cm;
    ConnectionStatus connectionStatus;
    ConnectionStatusReason connectionStatusReason;
    QString connectionError;
    Connection::ErrorDetails connectionErrorDetails;
    Presence automaticPresence;
    Presence currentPresence;
    Presence requestedPresence;
    bool usingConnectionCaps;
    ConnectionCapabilities customCaps;

    // The contexts should never be removed from the map, to guarantee O(1) CD introspections per bus
    struct DispatcherContext;
    static QHash<QString, QSharedPointer<DispatcherContext> > dispatcherContexts;
    QSharedPointer<DispatcherContext> dispatcherContext;
};

struct Account::Private::DispatcherContext
{
    DispatcherContext(const QDBusConnection &bus)
        : iface(new Client::ChannelDispatcherInterface(bus, TP_QT_CHANNEL_DISPATCHER_BUS_NAME, TP_QT_CHANNEL_DISPATCHER_OBJECT_PATH)),
          introspected(false), supportsHints(false)
    {
    }

    ~DispatcherContext()
    {
        delete iface;
    }

    Client::ChannelDispatcherInterface *iface;

    bool introspected, supportsHints;
    QPointer<PendingVariant> introspectOp;

private:
    DispatcherContext(const DispatcherContext &);
    void operator=(const DispatcherContext &);
};

Account::Private::Private(Account *parent, const ConnectionFactoryConstPtr &connFactory,
        const ChannelFactoryConstPtr &chanFactory, const ContactFactoryConstPtr &contactFactory)
    : parent(parent),
      connFactory(connFactory),
      chanFactory(chanFactory),
      contactFactory(contactFactory),
      baseInterface(new Client::AccountInterface(parent)),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      valid(false),
      enabled(false),
      connectsAutomatically(false),
      hasBeenOnline(false),
      changingPresence(false),
      mayFinishCore(false),
      coreFinished(false),
      connectionStatus(ConnectionStatusDisconnected),
      connectionStatusReason(ConnectionStatusReasonNoneSpecified),
      usingConnectionCaps(false),
      dispatcherContext(dispatcherContexts.value(parent->dbusConnection().name()))
{
    // FIXME: QRegExp probably isn't the most efficient possible way to parse
    //        this :-)
    QRegExp rx(QLatin1String("^") + TP_QT_ACCOUNT_OBJECT_PATH_BASE +
                QLatin1String("/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
                "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
                "/([_A-Za-z][_A-Za-z0-9]*)"  // account-specific part
                ));

    if (rx.exactMatch(parent->objectPath())) {
        cmName = rx.cap(1);
        protocolName = rx.cap(2).replace(QLatin1Char('_'), QLatin1Char('-'));
    } else {
        warning() << "Account object path is not spec-compliant, "
            "trying again with a different account-specific part check";

        rx = QRegExp(QLatin1String("^") + TP_QT_ACCOUNT_OBJECT_PATH_BASE +
                    QLatin1String("/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
                    "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
                    "/([_A-Za-z0-9]*)"  // account-specific part
                    ));
        if (rx.exactMatch(parent->objectPath())) {
            cmName = rx.cap(1);
            protocolName = rx.cap(2).replace(QLatin1Char('_'), QLatin1Char('-'));
        } else {
            warning() << "Not a valid Account object path:" <<
                parent->objectPath();
        }
    }

    ReadinessHelper::Introspectables introspectables;

    // As Account does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableAvatar(
        QSet<uint>() << 0,                                                            // makesSenseForStatuses
        Features() << FeatureCore,                                                    // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_ACCOUNT_INTERFACE_AVATAR, // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectAvatar,
        this);
    introspectables[FeatureAvatar] = introspectableAvatar;

    ReadinessHelper::Introspectable introspectableProtocolInfo(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << FeatureCore,                                              // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectProtocolInfo,
        this);
    introspectables[FeatureProtocolInfo] = introspectableProtocolInfo;

    ReadinessHelper::Introspectable introspectableCapabilities(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << FeatureCore << FeatureProtocolInfo << FeatureProfile,     // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCapabilities,
        this);
    introspectables[FeatureCapabilities] = introspectableCapabilities;

    readinessHelper->addIntrospectables(introspectables);

    if (connFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the conn factory is not the proxy connection for"
            << parent->objectPath();
    }

    if (chanFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the channel factory is not the proxy connection for"
            << parent->objectPath();
    }

    if (!dispatcherContext) {
        dispatcherContext = QSharedPointer<DispatcherContext>(new DispatcherContext(parent->dbusConnection()));
        dispatcherContexts.insert(parent->dbusConnection().name(), dispatcherContext);
    }

    init();
}

Account::Private::~Private()
{
}

bool Account::Private::checkCapabilitiesChanged(bool profileChanged)
{
    /* when the capabilities changed:
     *
     * - We were using the connection caps and now we don't have connection or
     *   the connection we have is not connected (changed to CM caps)
     * - We were using the CM caps and now we have a connected connection
     *   (changed to new connection caps)
     */
    bool changed = false;

    if (usingConnectionCaps &&
        (parent->connection().isNull() ||
         connection->status() != ConnectionStatusConnected)) {
        usingConnectionCaps = false;
        changed = true;
    } else if (!usingConnectionCaps &&
        !parent->connection().isNull() &&
        connection->status() == ConnectionStatusConnected) {
        usingConnectionCaps = true;
        changed = true;
    } else if (!usingConnectionCaps && profileChanged) {
        changed = true;
    }

    if (changed && parent->isReady(FeatureCapabilities)) {
        emit parent->capabilitiesChanged(parent->capabilities());
    }

    return changed;
}

QString Account::Private::connectionObjectPath() const
{
    return !connection.isNull() ? connection->objectPath() : QString();
}

QHash<QString, QSharedPointer<Account::Private::DispatcherContext> > Account::Private::dispatcherContexts;

/**
 * \class Account
 * \ingroup clientaccount
 * \headerfile TelepathyQt/account.h <TelepathyQt/Account>
 *
 * \brief The Account class represents a Telepathy account.
 *
 * The remote object accessor functions on this object (isValidAccount(),
 * isEnabled(), and so on) don't make any D-Bus calls; instead, they return/use
 * values cached from a previous introspection run. The introspection process
 * populates their values in the most efficient way possible based on what the
 * service implements.
 *
 * To avoid unnecessary D-Bus traffic, some accessors only return valid
 * information after specific features have been enabled.
 * For instance, to retrieve the account protocol information, it is necessary to
 * enable the feature Account::FeatureProtocolInfo.
 * See the individual methods descriptions for more details.
 *
 * Account features can be enabled by constructing an AccountFactory and enabling
 * the desired features, and passing it to AccountManager or ClientRegistrar
 * when creating them as appropriate. However, if a particular
 * feature is only ever used in a specific circumstance, such as an user opening
 * some settings dialog separate from the general view of the application,
 * features can be later enabled as needed by calling becomeReady() with the additional
 * features, and waiting for the resulting PendingOperation to finish.
 *
 * As an addition to accessors, signals are emitted to indicate that properties have
 * changed, for example displayNameChanged(), iconNameChanged(), etc.
 *
 * Convenience methods to create channels using the channel dispatcher such as
 * ensureTextChat(), createFileTransfer() are also provided.
 *
 * If the account is deleted from the AccountManager, this object
 * will not be deleted automatically; however, it will emit invalidated()
 * with error code #TP_QT_ERROR_OBJECT_REMOVED and will cease to
 * be useful.
 *
 * \section account_usage_sec Usage
 *
 * \subsection account_create_sec Creating an account object
 *
 * The easiest way to create account objects is through AccountManager. One can
 * just use the AccountManager convenience methods such as
 * AccountManager::validAccounts() to get a list of account objects representing
 * valid accounts.
 *
 * If you already know the object path, you can just call create().
 * For example:
 *
 * \code AccountPtr acc = Account::create(busName, objectPath); \endcode
 *
 * An AccountPtr object is returned, which will automatically keep
 * track of object lifetime.
 *
 * You can also provide a D-Bus connection as a QDBusConnection:
 *
 * \code
 *
 * AccountPtr acc = Account::create(QDBusConnection::sessionBus(),
 *         busName, objectPath);
 *
 * \endcode
 *
 * \subsection account_ready_sec Making account ready to use
 *
 * An Account object needs to become ready before usage, meaning that the
 * introspection process finished and the object accessors can be used.
 *
 * To make the object ready, use becomeReady() and wait for the
 * PendingOperation::finished() signal to be emitted.
 *
 * \code
 *
 * class MyClass : public QObject
 * {
 *     QOBJECT
 *
 * public:
 *     MyClass(QObject *parent = 0);
 *     ~MyClass() { }
 *
 * private Q_SLOTS:
 *     void onAccountReady(Tp::PendingOperation*);
 *
 * private:
 *     AccountPtr acc;
 * };
 *
 * MyClass::MyClass(const QString &busName, const QString &objectPath,
 *         QObject *parent)
 *     : QObject(parent)
 *       acc(Account::create(busName, objectPath))
 * {
 *     connect(acc->becomeReady(),
 *             SIGNAL(finished(Tp::PendingOperation*)),
 *             SLOT(onAccountReady(Tp::PendingOperation*)));
 * }
 *
 * void MyClass::onAccountReady(Tp::PendingOperation *op)
 * {
 *     if (op->isError()) {
 *         qWarning() << "Account cannot become ready:" <<
 *             op->errorName() << "-" << op->errorMessage();
 *         return;
 *     }
 *
 *     // Account is now ready
 *     qDebug() << "Display name:" << acc->displayName();
 * }
 *
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the Account
 * object usable.
 *
 * Note that this feature must be enabled in order to use most Account methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature Account::FeatureCore = Feature(QLatin1String(Account::staticMetaObject.className()), 0, true);

/**
 * Feature used in order to access account avatar info.
 *
 * See avatar specific methods' documentation for more details.
 *
 * \sa avatar(), avatarChanged()
 */
const Feature Account::FeatureAvatar = Feature(QLatin1String(Account::staticMetaObject.className()), 1);

/**
 * Feature used in order to access account protocol info.
 *
 * See protocol info specific methods' documentation for more details.
 *
 * \sa protocolInfo()
 */
const Feature Account::FeatureProtocolInfo = Feature(QLatin1String(Account::staticMetaObject.className()), 2);

/**
 * Feature used in order to access account capabilities.
 *
 * Enabling this feature will also enable FeatureProtocolInfo and FeatureProfile.
 *
 * See capabilities specific methods' documentation for more details.
 *
 * \sa capabilities(), capabilitiesChanged()
 */
const Feature Account::FeatureCapabilities = Feature(QLatin1String(Account::staticMetaObject.className()), 3);

/**
 * Feature used in order to access account profile info.
 *
 * See profile specific methods' documentation for more details.
 *
 * \sa profile(), profileChanged()
 */
const Feature Account::FeatureProfile = FeatureProtocolInfo;
// FeatureProfile is the same as FeatureProtocolInfo for now, as it only needs
// the protocol info, cm name and protocol name to build a fake profile. Make it
// a full-featured feature if needed later.

/**
 * Create a new Account object using QDBusConnection::sessionBus() and the given factories.
 *
 * A warning is printed if the factories are not for QDBusConnection::sessionBus().
 *
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TP_QT_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return An AccountPtr object pointing to the newly created Account object.
 */
AccountPtr Account::create(const QString &busName, const QString &objectPath,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return AccountPtr(new Account(QDBusConnection::sessionBus(), busName, objectPath,
                connectionFactory, channelFactory, contactFactory, Account::FeatureCore));
}

/**
 * Create a new Account object using the given \a bus and the given factories.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TP_QT_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return An AccountPtr object pointing to the newly created Account object.
 */
AccountPtr Account::create(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return AccountPtr(new Account(bus, busName, objectPath, connectionFactory, channelFactory,
                contactFactory, Account::FeatureCore));
}

/**
 * Construct a new Account object using the given \a bus and the given factories.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TP_QT_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \param coreFeature The core feature of the Account subclass. The corresponding introspectable
 * should depend on Account::FeatureCore.
 */
Account::Account(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const Feature &coreFeature)
    : StatelessDBusProxy(bus, busName, objectPath, coreFeature),
      OptionalInterfaceFactory<Account>(this),
      mPriv(new Private(this, connectionFactory, channelFactory, contactFactory))
{
}

/**
 * Class destructor.
 */
Account::~Account()
{
    delete mPriv;
}

/**
 * Return the connection factory used by this account.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ConnectionFactory object.
 */
ConnectionFactoryConstPtr Account::connectionFactory() const
{
    return mPriv->connFactory;
}

/**
 * Return the channel factory used by this account.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ChannelFactory object.
 */
ChannelFactoryConstPtr Account::channelFactory() const
{
    return mPriv->chanFactory;
}

/**
 * Return the contact factory used by this account.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ContactFactory object.
 */
ContactFactoryConstPtr Account::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return whether this account is valid.
 *
 * If \c true, this account is considered by the account manager to be complete
 * and usable. If \c false, user action is required to make it usable, and it will
 * never attempt to connect. For instance, this might be caused by the absence
 * or misconfiguration of a required parameter, in which case updateParameters()
 * may be used to properly set the parameters values.
 *
 * Change notification is via the validityChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if valid, \c false otherwise.
 * \sa validityChanged(), updateParameters()
 */
bool Account::isValidAccount() const
{
    return mPriv->valid;
}

/**
 * Return whether this account is enabled.
 *
 * Change notification is via the stateChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if enabled, \c false otherwise.
 * \sa stateChanged(), setEnabled()
 */
bool Account::isEnabled() const
{
    return mPriv->enabled;
}

/**
 * Set whether this account should be enabled or disabled.
 *
 * This gives users the possibility to prevent an account from
 * being used.
 *
 * Note that changing this property won't change the validity of the account.
 *
 * \param value Whether this account should be enabled or disabled.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa stateChanged(), isEnabled()
 */
PendingOperation *Account::setEnabled(bool value)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("Enabled"),
                QDBusVariant(value)),
            AccountPtr(this));
}

/**
 * Return the connection manager name of this account.
 *
 * \return The connection manager name.
 */
QString Account::cmName() const
{
    return mPriv->cmName;
}

/**
 * Return the protocol name of this account.
 *
 * \return The protocol name.
 */
QString Account::protocolName() const
{
    return mPriv->protocolName;
}

/**
 * Return the service name of this account.
 *
 * Note that this method will fallback to protocolName() if service name
 * is not known.
 *
 * Change notification is via the serviceNameChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The service name.
 * \sa serviceNameChanged(), setServiceName(), protocolName()
 */
QString Account::serviceName() const
{
    if (mPriv->serviceName.isEmpty()) {
        return mPriv->protocolName;
    }
    return mPriv->serviceName;
}

/**
 * Set the service name of this account.
 *
 * Some protocols, like XMPP and SIP, are used by various different user-recognised brands,
 * such as Google Talk. On accounts for such services, this method can be used
 * to set the name describing the service, which must consist only of ASCII letters, numbers and
 * hyphen/minus signs, and start with a letter.
 * For the jabber protocol, one of the following service names should be used if possible:
 *
 *   google-talk (for Google's IM service)
 *   facebook (for Facebook's IM service)
 *   lj-talk (for LiveJournal's IM service)
 *
 * The service name may also be set, if appropriate, when creating the account. See
 * AccountManager::createAccount() for more details.
 *
 * Note that changing this property may also change the profile() used by this account,
 * in which case profileChanged() will be emitted in addition to serviceNameChanged(), if
 * Account::FeatureProfile is enabled.
 *
 * \param value The service name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa serviceNameChanged(), serviceName()
 */
PendingOperation *Account::setServiceName(const QString &value)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("Service"),
                QDBusVariant(value)),
            AccountPtr(this));
}

/**
 * Return the profile used by this account.
 *
 * Profiles are intended to describe variants of the basic protocols supported by Telepathy
 * connection managers.
 * An example of this would be Google Talk vs Facebook chat vs Jabber/XMPP. Google Talk is a
 * specific case of XMPP with well-known capabilities, quirks and settings, and Facebook chat is
 * a subset of the standard XMPP offering.
 *
 * This method will return the profile for this account based on the service used by it.
 *
 * Note that if a profile for serviceName() is not available, a fake profile
 * (Profile::isFake() is \c true) will be returned in case protocolInfo() is valid.
 *
 * The fake profile will contain the following info:
 *  - Profile::type() will return "IM"
 *  - Profile::provider() will return an empty string
 *  - Profile::serviceName() will return "cmName()-serviceName()"
 *  - Profile::name() and Profile::protocolName() will return protocolName()
 *  - Profile::iconName() will return "im-protocolName()"
 *  - Profile::cmName() will return cmName()
 *  - Profile::parameters() will return a list matching CM default parameters for protocol with name
 *    protocolName()
 *  - Profile::presences() will return an empty list and
 *    Profile::allowOtherPresences() will return \c true, meaning that CM
 *    presences should be used
 *  - Profile::unsupportedChannelClassSpecs() will return an empty list
 *
 * Change notification is via the profileChanged() signal.
 *
 * This method requires Account::FeatureProfile to be ready.
 *
 * \return A pointer to the Profile object.
 * \sa profileChanged(), serviceName()
 */
ProfilePtr Account::profile() const
{
    if (!isReady(FeatureProfile)) {
        warning() << "Account::profile() requires Account::FeatureProfile to be ready";
        return ProfilePtr();
    }

    if (!mPriv->profile) {
        mPriv->profile = Profile::createForServiceName(serviceName());
        if (!mPriv->profile->isValid()) {
            if (protocolInfo().isValid()) {
                mPriv->profile = ProfilePtr(new Profile(
                            QString(QLatin1String("%1-%2")).arg(mPriv->cmName).arg(serviceName()),
                            mPriv->cmName,
                            mPriv->protocolName,
                            protocolInfo()));
            } else {
                warning() << "Cannot create profile as neither a .profile is installed for service" <<
                    serviceName() << "nor protocol info can be retrieved";
            }
        }
    }
    return mPriv->profile;
}

/**
 * Return the display name of this account.
 *
 * Change notification is via the displayNameChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The display name.
 * \sa displayNameChanged(), setDisplayName()
 */
QString Account::displayName() const
{
    return mPriv->displayName;
}

/**
 * Set the display name of this account.
 *
 * The display name is the user-visible name of this account.
 * This is usually chosen by the user at account creation time.
 * See AccountManager::createAccount() for more details.
 *
 * \param value The display name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa displayNameChanged(), displayName()
 */
PendingOperation *Account::setDisplayName(const QString &value)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("DisplayName"),
                QDBusVariant(value)),
            AccountPtr(this));
}

/**
 * Return the icon name of this account.
 *
 * If the account has no icon, and Account::FeatureProfile is enabled, the icon from the result of
 * profile() will be used.
 *
 * If neither the account nor the profile has an icon, and Account::FeatureProtocolInfo is
 * enabled, the icon from protocolInfo() will be used if set.
 *
 * As a last resort, "im-" + protocolName() will be returned.
 *
 * This matches the fallbacks recommended by the \telepathy_spec.
 *
 * Change notification is via the iconNameChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The icon name.
 * \sa iconNameChanged(), setIconName()
 */
QString Account::iconName() const
{
    if (mPriv->iconName.isEmpty()) {
        if (isReady(FeatureProfile)) {
            ProfilePtr pr = profile();
            if (pr && pr->isValid()) {
                QString iconName = pr->iconName();
                if (!iconName.isEmpty()) {
                    return iconName;
                }
            }
        }

        if (isReady(FeatureProtocolInfo) && protocolInfo().isValid()) {
            return protocolInfo().iconName();
        }

        return QString(QLatin1String("im-%1")).arg(protocolName());
    }

    return mPriv->iconName;
}

/**
 * Set the icon name of this account.
 *
 * \param value The icon name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa iconNameChanged(), iconName()
 */
PendingOperation *Account::setIconName(const QString &value)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("Icon"),
                QDBusVariant(value)),
            AccountPtr(this));
}

/**
 * Return the nickname of this account.
 *
 * Change notification is via the nicknameChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The nickname.
 * \sa nicknameChanged(), setNickname()
 */
QString Account::nickname() const
{
    return mPriv->nickname;
}

/**
 * Set the nickname of this account as seen to other contacts.
 *
 * \param value The nickname of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa nicknameChanged(), nickname()
 */
PendingOperation *Account::setNickname(const QString &value)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("Nickname"),
                QDBusVariant(value)),
            AccountPtr(this));
}

/**
 * Return the avatar requirements (size limits, supported MIME types, etc)
 * for avatars passed to setAvatar() on this account.
 *
 * For now this method will only return the avatar requirements found in protocolInfo() if
 * Account::FeatureProtocolInfo is ready, otherwise an invalid AvatarSpec instance is returned.
 *
 * \return The requirements as an AvatarSpec object.
 * \sa avatar(), setAvatar()
 */
AvatarSpec Account::avatarRequirements() const
{
    // TODO Once connection has support for avatar requirements use it if the connection is usable
    ProtocolInfo pi = protocolInfo();
    if (pi.isValid()) {
        return pi.avatarRequirements();
    }
    return AvatarSpec();
}

/**
 * Return the avatar of this account.
 *
 * Change notification is via the avatarChanged() signal.
 *
 * This method requires Account::FeatureAvatar to be ready.
 *
 * \return The avatar as an Avatar object.
 * \sa avatarChanged(), setAvatar()
 */
const Avatar &Account::avatar() const
{
    if (!isReady(Features() << FeatureAvatar)) {
        warning() << "Trying to retrieve avatar from account, but "
                     "avatar is not supported or was not requested. "
                     "Use becomeReady(FeatureAvatar)";
    }

    return mPriv->avatar;
}

/**
 * Set avatar of this account as seen to other contacts.
 *
 * Note that \a avatar must match the requirements as returned by avatarRequirements().
 *
 * \param avatar The avatar of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa avatarChanged(), avatar(), avatarRequirements()
 */
PendingOperation *Account::setAvatar(const Avatar &avatar)
{
    if (!interfaces().contains(TP_QT_IFACE_ACCOUNT_INTERFACE_AVATAR)) {
        return new PendingFailure(
                TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Account does not support Avatar"),
                AccountPtr(this));
    }

    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT_INTERFACE_AVATAR,
                QLatin1String("Avatar"),
                QDBusVariant(QVariant::fromValue(avatar))),
            AccountPtr(this));
}

/**
 * Return the parameters of this account.
 *
 * The account parameters are represented as a map from connection manager parameter names
 * to their values.
 *
 * Change notification is via the parametersChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The parameters as QVariantMap.
 * \sa parametersChanged(), updateParameters()
 */
QVariantMap Account::parameters() const
{
    return mPriv->parameters;
}

/**
 * Update this account parameters.
 *
 * On success, the PendingOperation returned by this method will produce a
 * list of strings, which are the names of parameters whose changes will not
 * take effect until the account is disconnected and reconnected (for instance
 * by calling reconnect()).
 *
 * \param set Parameters to set.
 * \param unset Parameters to unset.
 * \return A PendingStringList which will emit PendingStringList::finished
 *         when the request has been made
 * \sa parametersChanged(), parameters(), reconnect()
 */
PendingStringList *Account::updateParameters(const QVariantMap &set,
        const QStringList &unset)
{
    return new PendingStringList(
            baseInterface()->UpdateParameters(set, unset),
            AccountPtr(this));
}

/**
 * Return the protocol info of this account protocol.
 *
 * This method requires Account::FeatureProtocolInfo to be ready.
 *
 * \return The protocol info as a ProtocolInfo object.
 */
ProtocolInfo Account::protocolInfo() const
{
    if (!isReady(Features() << FeatureProtocolInfo)) {
        warning() << "Trying to retrieve protocol info from account, but "
                     "protocol info is not supported or was not requested. "
                     "Use becomeReady(FeatureProtocolInfo)";
        return ProtocolInfo();
    }

    return mPriv->cm->protocol(mPriv->protocolName);
}

/**
 * Return the capabilities for this account.
 *
 * Note that this method will return the connection() capabilities if the
 * account is online and ready. If the account is disconnected, it will fallback
 * to return the subtraction of the protocolInfo() capabilities and the profile() unsupported
 * capabilities.
 *
 * Change notification is via the capabilitiesChanged() signal.
 *
 * This method requires Account::FeatureCapabilities to be ready.
 *
 * \return The capabilities as a ConnectionCapabilities object.
 * \sa capabilitiesChanged(), protocolInfo(), profile()
 */
ConnectionCapabilities Account::capabilities() const
{
    if (!isReady(FeatureCapabilities)) {
        warning() << "Trying to retrieve capabilities from account, but "
                     "FeatureCapabilities was not requested. "
                     "Use becomeReady(FeatureCapabilities)";
        return ConnectionCapabilities();
    }

    // if the connection is online and ready use its caps
    if (mPriv->connection &&
        mPriv->connection->status() == ConnectionStatusConnected) {
        return mPriv->connection->capabilities();
    }

    // if we are here it means FeatureProtocolInfo and FeatureProfile are ready, as
    // FeatureCapabilities depend on them, so let's use the subtraction of protocol info caps rccs
    // and profile unsupported rccs.
    //
    // However, if we failed to introspect the CM (eg. this is a test), then let's not try to use
    // the protocolInfo because it'll be NULL! Profile may also be NULL in case a .profile for
    // serviceName() is not present and protocolInfo is NULL.
    ProtocolInfo pi = protocolInfo();
    if (!pi.isValid()) {
        return ConnectionCapabilities();
    }
    ProfilePtr pr;
    if (isReady(FeatureProfile)) {
        pr = profile();
    }
    if (!pr || !pr->isValid()) {
        return pi.capabilities();
    }

    RequestableChannelClassSpecList piClassSpecs = pi.capabilities().allClassSpecs();
    RequestableChannelClassSpecList prUnsupportedClassSpecs = pr->unsupportedChannelClassSpecs();
    RequestableChannelClassSpecList classSpecs;
    bool unsupported;
    foreach (const RequestableChannelClassSpec &piClassSpec, piClassSpecs) {
        unsupported = false;
        foreach (const RequestableChannelClassSpec &prUnsuportedClassSpec, prUnsupportedClassSpecs) {
            // Here we check the following:
            // - If the unsupported spec has no allowed property it means it does not support any
            // class whose fixed properties match.
            //   E.g: Doesn't support any media calls, be it audio or video.
            // - If the unsupported spec has allowed properties it means it does not support a
            // specific class whose fixed properties and allowed properties should match.
            //   E.g: Doesn't support video calls but does support audio calls.
            if (prUnsuportedClassSpec.allowedProperties().isEmpty()) {
                if (piClassSpec.fixedProperties() == prUnsuportedClassSpec.fixedProperties()) {
                    unsupported = true;
                    break;
                }
            } else {
                if (piClassSpec == prUnsuportedClassSpec) {
                    unsupported = true;
                    break;
                }
            }
        }
        if (!unsupported) {
            classSpecs.append(piClassSpec);
        }
    }
    mPriv->customCaps = ConnectionCapabilities(classSpecs);
    return mPriv->customCaps;
}

/**
 * Return whether this account should be put online automatically whenever
 * possible.
 *
 * Change notification is via the connectsAutomaticallyPropertyChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if it should try to connect automatically, \c false
 *         otherwise.
 * \sa connectsAutomaticallyPropertyChanged(), setConnectsAutomatically()
 */
bool Account::connectsAutomatically() const
{
    return mPriv->connectsAutomatically;
}

/**
 * Set whether this account should be put online automatically whenever
 * possible.
 *
 * \param value Value indicating if this account should be put online whenever
 *              possible.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa connectsAutomaticallyPropertyChanged(), connectsAutomatically()
 */
PendingOperation *Account::setConnectsAutomatically(bool value)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("ConnectAutomatically"),
                QDBusVariant(value)),
            AccountPtr(this));
}

/**
 * Return whether this account has ever been put online successfully.
 *
 * This property cannot change from \c true to \c false, only from \c false to \c true.
 * When the account successfully goes online for the first time, or when it
 * is detected that this has already happened, the firstOnline() signal is
 * emitted.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if ever been online, \c false otherwise.
 */
bool Account::hasBeenOnline() const
{
    return mPriv->hasBeenOnline;
}

/**
 * Return the status of this account connection.
 *
 * Note that this method may return a different value from the one returned by Connection::status()
 * on this account connection. This happens because this value will change whenever the connection
 * status of this account connection changes and won't consider the Connection introspection
 * process. The same rationale also applies to connectionStatusReason() and
 * connectionErrorDetails().
 *
 * A valid use case for this is for account creation UIs that wish to display the accounts
 * connection status and nothing else on the connections (e.g. retrieve the contact list).
 *
 * Change notification is via the connectionStatusChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The connection status as #ConnectionStatus.
 * \sa connectionStatusChanged(), connectionStatusReason(), connectionError(),
 *     connectionErrorDetails()
 */
ConnectionStatus Account::connectionStatus() const
{
    return mPriv->connectionStatus;
}

/**
 * Return the reason for this account connection status.
 *
 * This represents the reason for the last change to connectionStatus().
 *
 * Note that this method may return a different value from the one returned by
 * Connection::statusReason() on this account connection.
 * See connectionStatus() for more details.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The connection status reason as #ConnectionStatusReason.
 * \sa connectionStatusChanged(), connectionStatus(), connectionError(), connectionErrorDetails()
 */
ConnectionStatusReason Account::connectionStatusReason() const
{
    return mPriv->connectionStatusReason;
}

/**
 * Return the D-Bus error name for the last disconnection or connection failure,
 * (in particular, #TP_QT_ERROR_CANCELLED if it was disconnected by user
 * request), or an empty string if the account is connected.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The D-Bus error name for the last disconnection or connection failure.
 * \sa connectionErrorDetails(), connectionStatus(), connectionStatusReason(),
 *     connectionStatusChanged()
 */
QString Account::connectionError() const
{
    return mPriv->connectionError;
}

/**
 * Return detailed information related to connectionError().
 *
 * Note that this method may return a different value from the one returned by
 * Connection::errorDetails() on this account connection.
 * See connectionStatus() for more details.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The connection error details as a Connection::ErrorDetails object.
 * \sa connectionError(), connectionStatus(), connectionStatusReason(), connectionStatusChanged(),
 *     Connection::ErrorDetails.
 */
Connection::ErrorDetails Account::connectionErrorDetails() const
{
    return mPriv->connectionErrorDetails;
}

/**
 * Return the object representing the connection of this account.
 *
 * Note that the Connection object returned by this method will have the
 * features set in the connectionFactory() used by this account ready.
 *
 * Change notification is via the connectionChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return A pointer to the Connection object, or a null ConnectionPtr if
 *         there is no connection currently or if an error occurred.
 * \sa connectionChanged()
 */
ConnectionPtr Account::connection() const
{
    return mPriv->connection;
}

/**
 * Return whether this account connection is changing presence.
 *
 * Change notification is via the changingPresence() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if changing presence, \c false otherwise.
 * \sa changingPresence(), currentPresenceChanged(), setRequestedPresence()
 */
bool Account::isChangingPresence() const
{
    return mPriv->changingPresence;
}

/**
 * Return a list of presences allowed by a connection to this account.
 *
 * In particular, for the statuses reported here it can be assumed that setting them as the
 * requested presence via setRequestedPresence() will eventually result in currentPresence()
 * changing to exactly said presence. Other statuses are only guaranteed to be matched as closely as
 * possible.
 *
 * The statuses can be also used for the automatic presence, as set by setAutomaticPresence(), with
 * the exception of any status specifications for which Presence::type() is
 * Tp::ConnectionPresenceTypeOffline for the Presence returned by PresenceSpec::presence().
 *
 * However, the optional parameter can be used to allow reporting also other possible presence
 * statuses on this protocol besides the others that can be set on yourself. These are purely
 * informatory, for e.g. adjusting an UI to allow all possible remote contact statuses to be
 * displayed.
 *
 * An offline presence status is always included, because it's always valid to make an account
 * offline by setting the requested presence to an offline status.
 *
 * Full functionality requires Account::FeatureProtocolInfo and Account::FeatureProfile to be ready
 * as well as connection with Connection::FeatureSimplePresence enabled. If the connection is online
 * and Connection::FeatureSimplePresence is enabled, it will return the connection allowed statuses,
 * otherwise it will return a list os statuses based on profile() and protocolInfo() information
 * if the corresponding features are enabled.
 *
 * If there's a mismatch between the presence status info provided in the .profile file and/or the
 * .manager file and what an online Connection actually reports (for example, the said data files
 * are missing or too old to include presence information), the returned value can change, in
 * particular when connectionChanged() is emitted with a connection for which Connection::status()
 * is Tp::ConnectionStatusConnected.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \param includeAllStatuses Whether the returned list will include all statuses or just the ones
 *                           that can be settable using setRequestedPresence().
 * \return The allowed statuses as a list of PresenceSpec objects.
 */
PresenceSpecList Account::allowedPresenceStatuses(bool includeAllStatuses) const
{
    QHash<QString, PresenceSpec> specMap;

    // if the connection is online and ready use it
    if (mPriv->connection &&
        mPriv->connection->status() == ConnectionStatusConnected &&
        mPriv->connection->actualFeatures().contains(Connection::FeatureSimplePresence)) {
        SimpleStatusSpecMap connectionAllowedPresences =
            mPriv->connection->lowlevel()->allowedPresenceStatuses();
        SimpleStatusSpecMap::const_iterator i = connectionAllowedPresences.constBegin();
        SimpleStatusSpecMap::const_iterator end = connectionAllowedPresences.constEnd();
        for (; i != end; ++i) {
            PresenceSpec presence = PresenceSpec(i.key(), i.value());
            specMap.insert(i.key(), presence);
        }
    } else {
        ProtocolInfo pi = protocolInfo();
        if (pi.isValid()) {
            // add all ProtocolInfo presences to the returned map
            foreach (const PresenceSpec &piPresence, pi.allowedPresenceStatuses()) {
                QString piStatus = piPresence.presence().status();
                specMap.insert(piStatus, piPresence);
            }
        }

        ProfilePtr pr;
        if (isReady(FeatureProfile)) {
            pr = profile();
        }
        if (pr && pr->isValid()) {
            // add all Profile presences to the returned map
            foreach (const Profile::Presence &prPresence, pr->presences()) {
                QString prStatus = prPresence.id();
                if (specMap.contains(prStatus)) {
                    // we already got the presence from ProtocolInfo, just update
                    // canHaveStatusMessage if needed
                    PresenceSpec presence = specMap.value(prStatus);
                    if (presence.canHaveStatusMessage() != prPresence.canHaveStatusMessage()) {
                        SimpleStatusSpec spec;
                        spec.type = presence.presence().type();
                        spec.maySetOnSelf = presence.maySetOnSelf();
                        spec.canHaveMessage = prPresence.canHaveStatusMessage();
                        specMap.insert(prStatus, PresenceSpec(prStatus, spec));
                    }
                } else {
                    // presence not found in ProtocolInfo, adding it
                    specMap.insert(prStatus, presenceSpecForStatus(prStatus,
                                prPresence.canHaveStatusMessage()));
                }
            }

            // now remove all presences that are not in the Profile, if it does
            // not allow other presences, and the ones that are disabled
            QHash<QString, PresenceSpec>::iterator i = specMap.begin();
            QHash<QString, PresenceSpec>::iterator end = specMap.end();
            while (i != end) {
                PresenceSpec presence = i.value();
                QString status = presence.presence().status();
                bool hasPresence = pr->hasPresence(status);
                Profile::Presence prPresence = pr->presence(status);
                if ((!hasPresence && !pr->allowOtherPresences()) || (hasPresence && prPresence.isDisabled())) {
                     i = specMap.erase(i);
                } else {
                     ++i;
                }
            }
        }
    }

    // filter out presences that may not be set on self if includeAllStatuses is false
    if (!includeAllStatuses) {
        QHash<QString, PresenceSpec>::iterator i = specMap.begin();
        QHash<QString, PresenceSpec>::iterator end = specMap.end();
        while (i != end) {
            PresenceSpec presence = i.value();
            if (!presence.maySetOnSelf()) {
                i = specMap.erase(i);
            } else {
                ++i;
            }
        }
    }

    if (!specMap.size()) {
        // If we didn't discover any statuses, either the protocol doesn't really support presence,
        // or we lack information (e.g. features not enabled or info not provided in the .manager or
        // .profile files). "available" - just the fact that you're online in the first place, is at
        // least a valid option for any protocol, so we'll include it as a fallback.

        specMap.insert(QLatin1String("available"),
                presenceSpecForStatus(QLatin1String("available"), false));
    }

    // We'll always include "offline". It is always valid to make an account offline via
    // setRequestedPresence().
    if (!specMap.contains(QLatin1String("offline"))) {
        specMap.insert(QLatin1String("offline"),
                presenceSpecForStatus(QLatin1String("offline"), false));
    }

    return specMap.values();
}

/**
 * Return the maximum length for a presence status message.
 *
 * If a status message set using setRequestedPresence() (or setAutomaticPresence()) is longer than
 * the maximum length allowed, the message will be truncated and
 * currentPresenceChanged() will be emitted (if setting the presence worked)
 * with the truncated message.
 *
 * Full functionality requires Connection with Connection::FeatureSimplePresence
 * enabled. If the connection is online and Connection::FeatureSimplePresence is
 * enabled, it will return the connection maximum status message length,
 * otherwise it will return 0.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The maximum length, or 0 if there is no limit.
 */
uint Account::maxPresenceStatusMessageLength() const
{
    // if the connection is online and ready use it
    if (mPriv->connection &&
        mPriv->connection->status() == ConnectionStatusConnected &&
        mPriv->connection->actualFeatures().contains(Connection::FeatureSimplePresence)) {
        return mPriv->connection->lowlevel()->maxPresenceStatusMessageLength();
    }

    return 0;
}

/**
 * Return the presence status that this account will have set on it by the
 * account manager if it brings it online automatically.
 *
 * Change notification is via the automaticPresenceChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The automatic presence as a Presence object.
 * \sa automaticPresenceChanged(), setAutomaticPresence()
 */
Presence Account::automaticPresence() const
{
    return mPriv->automaticPresence;
}

/**
 * Set the presence status that this account should have if it is brought
 * online automatically by the account manager.
 *
 * Note that changing this property won't actually change the account's status
 * until the next time it is (re)connected for some reason.
 *
 * The value of this property must be one that would be acceptable for setRequestedPresence(),
 * as returned by allowedPresenceStatuses(), with the additional restriction that the offline
 * presence cannot be used.
 *
 * \param presence The presence to set when this account is brought
 *                 online automatically by the account manager.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa automaticPresenceChanged(), automaticPresence(), setRequestedPresence()
 */
PendingOperation *Account::setAutomaticPresence(const Presence &presence)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("AutomaticPresence"),
                QDBusVariant(QVariant::fromValue(presence.barePresence()))),
            AccountPtr(this));
}

/**
 * Return the actual presence of this account.
 *
 * Change notification is via the currentPresenceChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The current presence as a Presence object.
 * \sa currentPresenceChanged(), setRequestedPresence(), requestedPresence(), automaticPresence()
 */
Presence Account::currentPresence() const
{
    return mPriv->currentPresence;
}

/**
 * Return the requested presence of this account.
 *
 * Change notification is via the requestedPresenceChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The requested presence as a Presence object.
 * \sa requestedPresenceChanged(), setRequestedPresence(), currentPresence(), automaticPresence()
 */
Presence Account::requestedPresence() const
{
    return mPriv->requestedPresence;
}

/**
 * Set the requested presence of this account.
 *
 * When the requested presence is changed, the account manager will attempt to
 * manipulate the connection to make currentPresence() match requestedPresence()
 * as closely as possible.
 *
 * \param presence The requested presence.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa requestedPresenceChanged(), currentPresence(), automaticPresence(), setAutomaticPresence()
 */
PendingOperation *Account::setRequestedPresence(const Presence &presence)
{
    return new PendingVoid(
            mPriv->properties->Set(
                TP_QT_IFACE_ACCOUNT,
                QLatin1String("RequestedPresence"),
                QDBusVariant(QVariant::fromValue(presence.barePresence()))),
            AccountPtr(this));
}

/**
 * Return whether this account is online.
 *
 * Change notification is via the onlinenessChanged() signal.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if online, otherwise \c false.
 * \sa onlinenessChanged()
 */
bool Account::isOnline() const
{
    return mPriv->currentPresence.type() != ConnectionPresenceTypeOffline;
}

/**
 * Return the unique identifier of this account.
 *
 * \return The unique identifier.
 */
QString Account::uniqueIdentifier() const
{
    QString path = objectPath();
    return path.right(path.length() -
            strlen("/org/freedesktop/Telepathy/Account/"));
}

/**
 * Return the normalized user ID of the local user of this account.
 *
 * It is unspecified whether this user ID is globally unique.
 *
 * As currently implemented, IRC user IDs are only unique within the same
 * IRCnet. On some saner protocols, the user ID includes a DNS name which
 * provides global uniqueness.
 *
 * If this value is not known yet (which will always be the case for accounts
 * that have never been online), it will be an empty string.
 *
 * It is possible that this value will change if the connection manager's
 * normalization algorithm changes.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return The normalized user ID of the local user.
 * \sa normalizedNameChanged()
 */
QString Account::normalizedName() const
{
    return mPriv->normalizedName;
}

/**
 * If this account is currently connected, disconnect and reconnect it. If it
 * is currently trying to connect, cancel the attempt to connect and start
 * another. If it is currently disconnected, do nothing.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 */
PendingOperation *Account::reconnect()
{
    return new PendingVoid(baseInterface()->Reconnect(), AccountPtr(this));
}

/**
 * Delete this account.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa removed()
 */
PendingOperation *Account::remove()
{
    return new PendingVoid(baseInterface()->Remove(), AccountPtr(this));
}

/**
 * Return whether passing hints on channel requests on this account is known to be supported.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool Account::supportsRequestHints() const
{
    return mPriv->dispatcherContext->supportsHints;
}

/**
 * Return whether the ChannelRequest::succeeded(const Tp::ChannelPtr &channel) signal is expected to
 * be emitted with a non-NULL channel parameter for requests made using this account.
 *
 * This can be used as a run-time check for the Channel Dispatcher implementation being new enough.
 * In particular, similarly old Channel Dispatchers don't support request hints either, so the
 * return value for this function and Account::supportsRequestHints() will bet he same.
 *
 * This method requires Account::FeatureCore to be ready.
 *
 * \return \c true if supported, \c false otherwise.
 */
bool Account::requestsSucceedWithChannel() const
{
    return supportsRequestHints();
}

/**
 * Start a request to ensure that a text channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to chat with.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureTextChat(
        const QString &contactIdentifier,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = textChatRequest(contactIdentifier);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a text channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to chat with.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureTextChat(
        const ContactPtr &contact,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = textChatRequest(contact);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a text chat room with the given
 * room name \a roomName exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param roomName The name of the chat room.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureTextChatroom(
        const QString &roomName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = textChatroomRequest(roomName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that an audio call channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param initialAudioContentName The name of the initial CallContent that will
 *                                be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureAudioCall(
        const QString &contactIdentifier,
        const QString &initialAudioContentName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = audioCallRequest(contactIdentifier, initialAudioContentName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that an audio call channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest* Account::ensureAudioCall(
        const ContactPtr &contact,
        const QString &initialAudioContentName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = audioCallRequest(contact, initialAudioContentName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a video call channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureVideoCall(
        const QString &contactIdentifier,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = videoCallRequest(contactIdentifier, initialVideoContentName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a video call channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to call.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureVideoCall(
        const ContactPtr &contact,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = videoCallRequest(contact, initialVideoContentName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that an audio/video call channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureAudioVideoCall(
        const QString &contactIdentifier,
        const QString &initialAudioContentName,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = audioVideoCallRequest(contactIdentifier,
            initialAudioContentName, initialVideoContentName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that an audio/video call channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureAudioVideoCall(
        const ContactPtr &contact,
        const QString &initialAudioContentName,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = audioVideoCallRequest(contact,
            initialAudioContentName, initialVideoContentName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a media channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureStreamedMediaCall(
        const QString &contactIdentifier,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamedMediaCallRequest(contactIdentifier);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a media channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureStreamedMediaCall(
        const ContactPtr &contact,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamedMediaCallRequest(contact);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that an audio call with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureStreamedMediaAudioCall(
        const QString &contactIdentifier,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamedMediaAudioCallRequest(contactIdentifier);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that an audio call with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contact The contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureStreamedMediaAudioCall(
        const ContactPtr &contact,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamedMediaAudioCallRequest(contact);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a video call with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureStreamedMediaVideoCall(
        const QString &contactIdentifier,
        bool withAudio,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamedMediaVideoCallRequest(contactIdentifier, withAudio);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to ensure that a video call with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contact The contact to call.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureStreamedMediaVideoCall(
        const ContactPtr &contact,
        bool withAudio,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamedMediaVideoCallRequest(contact, withAudio);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to create a file transfer channel with the given
 * contact \a contact.
 *
 * \param contactIdentifier The identifier of the contact to send a file.
 * \param properties The desired properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createFileTransfer(
        const QString &contactIdentifier,
        const FileTransferChannelCreationProperties &properties,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = fileTransferRequest(contactIdentifier, properties);

    if (request.isEmpty()) {
        return new PendingChannelRequest(AccountPtr(this), TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Cannot create a file transfer with invalid parameters"));
    }

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a file transfer channel with the given
 * contact \a contact.
 *
 * \param contact The contact to send a file.
 * \param properties The desired properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createFileTransfer(
        const ContactPtr &contact,
        const FileTransferChannelCreationProperties &properties,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = fileTransferRequest(contact, properties);

    if (request.isEmpty()) {
        return new PendingChannelRequest(AccountPtr(this), TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Cannot create a file transfer with invalid parameters"));
    }

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a stream tube channel with the given
 * contact identifier \a contactIdentifier.
 *
 * \param contactIdentifier The identifier of the contact to open a stream tube with.
 * \param service The stream tube service.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createStreamTube(
        const QString &contactIdentifier,
        const QString &service,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamTubeRequest(contactIdentifier, service);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a stream tube channel with the given
 * contact \a contact.
 *
 * \param contact The contact to open a stream tube with.
 * \param service The stream tube service.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createStreamTube(
        const ContactPtr &contact,
        const QString &service,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = streamTubeRequest(contact, service);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a DBus tube channel with the given
 * contact \a contactIdentifier.
 *
 * \param contactIdentifier The contact identifier of the contact to open a DBus tube with.
 * \param serviceName the service name that will be used over the
 *                    tube. It should be a well-known D-Bus service name, of the form
 *                    \c com.example.ServiceName
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createDBusTube(
        const QString &contactIdentifier,
        const QString &serviceName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = dbusTubeRequest(contactIdentifier, serviceName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}


/**
 * Start a request to create a DBus tube channel with the given
 * contact \a contact.
 *
 * \param contact The contact to open a DBus tube with.
 * \param serviceName the service name that will be used over the
 *                    tube. It should be a well-known D-Bus service name, of the form
 *                    \c com.example.ServiceName
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest* Account::createDBusTube(
        const Tp::ContactPtr& contact,
        const QString& serviceName,
        const QDateTime& userActionTime,
        const QString& preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = dbusTubeRequest(contact, serviceName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

PendingChannelRequest* Account::createDBusTubeRoom(
        const QString &room,
        const QString &serviceName,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = dbusTubeRoomRequest(room, serviceName);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
        preferredHandler, true, hints);
}

/**
 * Start a request to create a conference media call with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceStreamedMediaCall(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceStreamedMediaCallRequest(channels,
            initialInviteeContactsIdentifiers);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference media call with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceStreamedMediaCall(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceStreamedMediaCallRequest(channels, initialInviteeContacts);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference call with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceCall(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceCallRequest(channels,
            initialInviteeContactsIdentifiers);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference call with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceCall(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceCallRequest(channels, initialInviteeContacts);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference text chat with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChat(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceTextChatRequest(channels, initialInviteeContactsIdentifiers);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference text chat with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChat(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceTextChatRequest(channels, initialInviteeContacts);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference text chat room with the given
 * channels \a channels and room name \a roomName.
 *
 * \param roomName The room name.
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChatroom(
        const QString &roomName,
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceTextChatroomRequest(roomName, channels,
            initialInviteeContactsIdentifiers);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference text chat room with the given
 * channels \a channels and room name \a roomName.
 *
 * \param roomName The room name.
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChatroom(
        const QString &roomName,
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = conferenceTextChatroomRequest(roomName, channels,
            initialInviteeContacts);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to create a conference call with the given
 * channels \a channels.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceCall(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceCallRequest(channels,
            initialInviteeContactsIdentifiers);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference call with the given
 * channels \a channels.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceCall(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceCallRequest(channels, initialInviteeContacts);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a contact search channel with the given
 * server \a server and limit \a limit.
 *
 * \param server For protocols which support searching for contacts on multiple servers with
 *               different DNS names (like XMPP), the DNS name of the server to be searched,
 *               e.g. "characters.shakespeare.lit". Otherwise, an empty string.
 * \param limit The desired maximum number of results that should be returned by a doing a search.
 *              If the protocol does not support specifying a limit for the number of results
 *              returned at a time, this will be ignored.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa createChannel()
 */
PendingChannelRequest *Account::createContactSearch(
        const QString &server,
        uint limit,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    QVariantMap request = contactSearchRequest(capabilities(), server, limit);

    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to ensure that a text channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to chat with.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleTextChat(
        const QString &contactIdentifier,
        const QDateTime &userActionTime)
{
    QVariantMap request = textChatRequest(contactIdentifier);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a text channel with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to chat with.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleTextChat(
        const ContactPtr &contact,
        const QDateTime &userActionTime)
{
    QVariantMap request = textChatRequest(contact);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a text chat room with the given
 * room name \a roomName exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param roomName The name of the chat room.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleTextChatroom(
        const QString &roomName,
        const QDateTime &userActionTime)
{
    QVariantMap request = textChatroomRequest(roomName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that an audio call channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleAudioCall(
        const QString &contactIdentifier,
        const QString &initialAudioContentName,
        const QDateTime &userActionTime)
{
    QVariantMap request = audioCallRequest(contactIdentifier, initialAudioContentName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that an audio call channel with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleAudioCall(
        const ContactPtr &contact,
        const QString &initialAudioContentName,
        const QDateTime &userActionTime)
{
    QVariantMap request = audioCallRequest(contact, initialAudioContentName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a video call channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleVideoCall(
        const QString &contactIdentifier,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime)
{
    QVariantMap request = videoCallRequest(contactIdentifier, initialVideoContentName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a video call channel with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to call.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleVideoCall(
        const ContactPtr &contact,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime)
{
    QVariantMap request = videoCallRequest(contact, initialVideoContentName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that an audio/video call channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleAudioVideoCall(
        const QString &contactIdentifier,
        const QString &initialAudioContentName,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime)
{
    QVariantMap request = audioVideoCallRequest(contactIdentifier,
            initialAudioContentName, initialVideoContentName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that an audio/video call channel with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to call.
 * \param initialAudioContentName The name of the initial audio CallContent that
 *                                will be automatically added on the channel.
 * \param initialVideoContentName The name of the initial video CallContent that
 *                                will be automatically added on the channel.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleAudioVideoCall(
        const ContactPtr &contact,
        const QString &initialAudioContentName,
        const QString &initialVideoContentName,
        const QDateTime &userActionTime)
{
    QVariantMap request = audioVideoCallRequest(contact,
            initialAudioContentName, initialVideoContentName);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a media channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleStreamedMediaCall(
        const QString &contactIdentifier,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamedMediaCallRequest(contactIdentifier);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a media channel with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleStreamedMediaCall(
        const ContactPtr &contact,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamedMediaCallRequest(contact);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that an audio call with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleStreamedMediaAudioCall(
        const QString &contactIdentifier,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamedMediaAudioCallRequest(contactIdentifier);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that an audio call with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contact The contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleStreamedMediaAudioCall(
        const ContactPtr &contact,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamedMediaAudioCallRequest(contact);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a video call with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleStreamedMediaVideoCall(
        const QString &contactIdentifier,
        bool withAudio,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamedMediaVideoCallRequest(contactIdentifier, withAudio);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to ensure that a video call with the given
 * contact \a contact exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contact The contact to call.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleStreamedMediaVideoCall(
        const ContactPtr &contact,
        bool withAudio,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamedMediaVideoCallRequest(contact, withAudio);

    return ensureAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a file transfer channel with the given
 * contact \a contactIdentifier.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to send a file.
 * \param properties The desired properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleFileTransfer(
        const QString &contactIdentifier,
        const FileTransferChannelCreationProperties &properties,
        const QDateTime &userActionTime)
{
    QVariantMap request = fileTransferRequest(contactIdentifier, properties);

    if (request.isEmpty()) {
        return new PendingChannel(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Cannot create a file transfer with invalid parameters"));
    }

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a file transfer channel with the given
 * contact \a contact.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to send a file.
 * \param properties The desired properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleFileTransfer(
        const ContactPtr &contact,
        const FileTransferChannelCreationProperties &properties,
        const QDateTime &userActionTime)
{
    QVariantMap request = fileTransferRequest(contact, properties);

    if (request.isEmpty()) {
        return new PendingChannel(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Cannot create a file transfer with invalid parameters"));
    }

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a stream tube channel with the given
 * contact identifier \a contactIdentifier.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to open a stream tube with.
 * \param service The stream tube service.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleStreamTube(
        const QString &contactIdentifier,
        const QString &service,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamTubeRequest(contactIdentifier, service);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a stream tube channel with the given
 * contact \a contact.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to open a stream tube with.
 * \param service The stream tube service.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleStreamTube(
        const ContactPtr &contact,
        const QString &service,
        const QDateTime &userActionTime)
{
    QVariantMap request = streamTubeRequest(contact, service);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a DBus tube channel with the given
 * contact identifier \a contactIdentifier.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contactIdentifier The identifier of the contact to open a DBus tube with.
 * \param serviceName The DBus tube service name.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleDBusTube(
        const QString &contactIdentifier,
        const QString &serviceName,
        const QDateTime &userActionTime)
{
    QVariantMap request = dbusTubeRequest(contactIdentifier, serviceName);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a DBus tube channel with the given
 * contact \a contact.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param contact The contact to open a DBus tube with.
 * \param serviceName The DBus tube service name.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleDBusTube(
        const ContactPtr &contact,
        const QString &serviceName,
        const QDateTime &userActionTime)
{
    QVariantMap request = dbusTubeRequest(contact, serviceName);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference text chat with the given
 * channels \a channels.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceTextChat(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceTextChatRequest(channels, initialInviteeContactsIdentifiers);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference text chat with the given
 * channels \a channels.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceTextChat(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceTextChatRequest(channels, initialInviteeContacts);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference text chat room with the given
 * channels \a channels and room name \a roomName.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param roomName The room name.
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceTextChatroom(
        const QString &roomName,
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceTextChatroomRequest(roomName, channels,
            initialInviteeContactsIdentifiers);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference text chat room with the given
 * channels \a channels and room name \a roomName.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param roomName The room name.
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceTextChatroom(
        const QString &roomName,
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceTextChatroomRequest(roomName, channels,
            initialInviteeContacts);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference media call with the given
 * channels \a channels.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceStreamedMediaCall(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceStreamedMediaCallRequest(channels,
            initialInviteeContactsIdentifiers);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a conference media call with the given
 * channels \a channels.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param channels The conference channels.
 * \param initialInviteeContacts A list of additional contacts
 *                               to be invited to this
 *                               conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleConferenceStreamedMediaCall(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime)
{
    QVariantMap request = conferenceStreamedMediaCallRequest(channels, initialInviteeContacts);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a contact search channel with the given
 * server \a server and limit \a limit.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * \param server For protocols which support searching for contacts on multiple servers with
 *               different DNS names (like XMPP), the DNS name of the server to be searched,
 *               e.g. "characters.shakespeare.lit". Otherwise, an empty string.
 *               If the protocol does not support specifying a search server, this will be ignored.
 * \param limit The desired maximum number of results that should be returned by a doing a search.
 *              If the protocol does not support specifying a limit for the number of results
 *              returned at a time, this will be ignored.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel(), createAndHandleChannel()
 */
PendingChannel *Account::createAndHandleContactSearch(
        const QString &server,
        uint limit,
        const QDateTime &userActionTime)
{
    QVariantMap request = contactSearchRequest(capabilities(), server, limit);

    return createAndHandleChannel(request, userActionTime);
}

/**
 * Start a request to create a channel.
 * This initially just creates a PendingChannelRequest object,
 * which can be used to track the success or failure of the request,
 * or to cancel it.
 *
 * Helper methods for text chat, text chat room, media call and conference are
 * provided and should be used if appropriate.
 *
 * \param request A dictionary containing desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa createChannel()
 */
PendingChannelRequest *Account::createChannel(
        const QVariantMap &request,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, true, hints);
}

/**
 * Start a request to ensure that a channel exists, creating it if necessary.
 * This initially just creates a PendingChannelRequest object,
 * which can be used to track the success or failure of the request,
 * or to cancel it.
 *
 * Helper methods for text chat, text chat room, media call and conference are
 * provided and should be used if appropriate.
 *
 * \param request A dictionary containing desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param hints Arbitrary metadata which will be relayed to the handler if supported,
 *              as indicated by supportsRequestHints().
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the request has been made.
 * \sa createChannel()
 */
PendingChannelRequest *Account::ensureChannel(
        const QVariantMap &request,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        const ChannelRequestHints &hints)
{
    return new PendingChannelRequest(AccountPtr(this), request, userActionTime,
            preferredHandler, false, hints);
}

/**
 * Start a request to create channel.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * Helper methods for text chat, text chat room, media call and conference are
 * provided and should be used if appropriate.
 *
 * The caller is responsible for closing the channel with
 * Channel::requestClose() or Channel::requestLeave() when it has finished handling it.
 *
 * A possible error returned by this method is #TP_QT_ERROR_NOT_AVAILABLE, in case a conflicting
 * channel that matches \a request already exists.
 *
 * \param request A dictionary containing desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa ensureAndHandleChannel()
 */
PendingChannel *Account::createAndHandleChannel(
        const QVariantMap &request,
        const QDateTime &userActionTime)
{
    return new PendingChannel(AccountPtr(this), request, userActionTime, true);
}

/**
 * Start a request to ensure that a channel exists, creating it if necessary.
 * This initially just creates a PendingChannel object,
 * which can be used to track the success or failure of the request.
 *
 * Helper methods for text chat, text chat room, media call and conference are
 * provided and should be used if appropriate.
 *
 * The caller is responsible for closing the channel with
 * Channel::requestClose() or Channel::requestLeave() when it has finished handling it.
 *
 * A possible error returned by this method is #TP_QT_ERROR_NOT_YOURS, in case somebody else is
 * already handling a channel that matches \a request.
 *
 * \param request A dictionary containing desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         successfully, when the Channel is available for handling using
 *         PendingChannel::channel(), or with an error if one has been encountered.
 * \sa createAndHandleChannel()
 */
PendingChannel *Account::ensureAndHandleChannel(
        const QVariantMap &request,
        const QDateTime &userActionTime)
{
    return new PendingChannel(AccountPtr(this), request, userActionTime, false);
}

/**
 * Return the Client::AccountInterface interface proxy object for this account.
 * This method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing Client::AccountInterface object for this
 *         Account object.
 */
Client::AccountInterface *Account::baseInterface() const
{
    return mPriv->baseInterface;
}

/**
 * Return the Client::ChannelDispatcherInterface interface proxy object to use for requesting
 * channels on this account.
 *
 * This method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing Client::ChannelDispatcherInterface object for this
 *         Account object.
 */
Client::ChannelDispatcherInterface *Account::dispatcherInterface() const
{
    return mPriv->dispatcherContext->iface;
}

/**** Private ****/
void Account::Private::init()
{
    if (!parent->isValid()) {
        return;
    }

    parent->connect(baseInterface,
            SIGNAL(Removed()),
            SLOT(onRemoved()));
    parent->connect(baseInterface,
            SIGNAL(AccountPropertyChanged(QVariantMap)),
            SLOT(onPropertyChanged(QVariantMap)));
}

void Account::Private::introspectMain(Account::Private *self)
{
    if (self->dispatcherContext->introspected) {
        self->parent->onDispatcherIntrospected(0);
        return;
    }

    if (!self->dispatcherContext->introspectOp) {
        debug() << "Discovering if the Channel Dispatcher supports request hints";
        self->dispatcherContext->introspectOp =
            self->dispatcherContext->iface->requestPropertySupportsRequestHints();
    }

    connect(self->dispatcherContext->introspectOp.data(),
            SIGNAL(finished(Tp::PendingOperation*)),
            self->parent,
            SLOT(onDispatcherIntrospected(Tp::PendingOperation*)));
}

void Account::Private::introspectAvatar(Account::Private *self)
{
    debug() << "Calling GetAvatar(Account)";
    // we already checked if avatar interface exists, so bypass avatar interface
    // checking
    Client::AccountInterfaceAvatarInterface *iface =
        self->parent->interface<Client::AccountInterfaceAvatarInterface>();

    // If we are here it means the user cares about avatar, so
    // connect to avatar changed signal, so we update the avatar
    // when it changes.
    self->parent->connect(iface,
            SIGNAL(AvatarChanged()),
            SLOT(onAvatarChanged()));

    self->retrieveAvatar();
}

void Account::Private::introspectProtocolInfo(Account::Private *self)
{
    Q_ASSERT(!self->cm);

    self->cm = ConnectionManager::create(
            self->parent->dbusConnection(), self->cmName,
            self->connFactory, self->chanFactory, self->contactFactory);
    self->parent->connect(self->cm->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onConnectionManagerReady(Tp::PendingOperation*)));
}

void Account::Private::introspectCapabilities(Account::Private *self)
{
    if (!self->connection) {
        // there is no connection, just make capabilities ready
        self->readinessHelper->setIntrospectCompleted(FeatureCapabilities, true);
        return;
    }

    self->parent->connect(self->connection->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onConnectionReady(Tp::PendingOperation*)));
}

void Account::Private::updateProperties(const QVariantMap &props)
{
    debug() << "Account::updateProperties: changed:";

    if (props.contains(QLatin1String("Interfaces"))) {
        parent->setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
        debug() << " Interfaces:" << parent->interfaces();
    }

    QString oldIconName = parent->iconName();
    bool serviceNameChanged = false;
    bool profileChanged = false;
    if (props.contains(QLatin1String("Service")) &&
        serviceName != qdbus_cast<QString>(props[QLatin1String("Service")])) {
        serviceNameChanged = true;
        serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
        debug() << " Service Name:" << parent->serviceName();
        /* use parent->serviceName() here as if the service name is empty we are going to use the
         * protocol name */
        emit parent->serviceNameChanged(parent->serviceName());
        parent->notify("serviceName");

        /* if we had a profile and the service changed, it means the profile also changed */
        if (parent->isReady(Account::FeatureProfile)) {
            /* service name changed, let's recreate profile */
            profileChanged = true;
            profile.reset();
            emit parent->profileChanged(parent->profile());
            parent->notify("profile");
        }
    }

    if (props.contains(QLatin1String("DisplayName")) &&
        displayName != qdbus_cast<QString>(props[QLatin1String("DisplayName")])) {
        displayName = qdbus_cast<QString>(props[QLatin1String("DisplayName")]);
        debug() << " Display Name:" << displayName;
        emit parent->displayNameChanged(displayName);
        parent->notify("displayName");
    }

    if ((props.contains(QLatin1String("Icon")) &&
         oldIconName != qdbus_cast<QString>(props[QLatin1String("Icon")])) ||
        serviceNameChanged) {

        if (props.contains(QLatin1String("Icon"))) {
            iconName = qdbus_cast<QString>(props[QLatin1String("Icon")]);
        }

        QString newIconName = parent->iconName();
        if (oldIconName != newIconName) {
            debug() << " Icon:" << newIconName;
            emit parent->iconNameChanged(newIconName);
            parent->notify("iconName");
        }
    }

    if (props.contains(QLatin1String("Nickname")) &&
        nickname != qdbus_cast<QString>(props[QLatin1String("Nickname")])) {
        nickname = qdbus_cast<QString>(props[QLatin1String("Nickname")]);
        debug() << " Nickname:" << nickname;
        emit parent->nicknameChanged(nickname);
        parent->notify("nickname");
    }

    if (props.contains(QLatin1String("NormalizedName")) &&
        normalizedName != qdbus_cast<QString>(props[QLatin1String("NormalizedName")])) {
        normalizedName = qdbus_cast<QString>(props[QLatin1String("NormalizedName")]);
        debug() << " Normalized Name:" << normalizedName;
        emit parent->normalizedNameChanged(normalizedName);
        parent->notify("normalizedName");
    }

    if (props.contains(QLatin1String("Valid")) &&
        valid != qdbus_cast<bool>(props[QLatin1String("Valid")])) {
        valid = qdbus_cast<bool>(props[QLatin1String("Valid")]);
        debug() << " Valid:" << (valid ? "true" : "false");
        emit parent->validityChanged(valid);
        parent->notify("valid");
    }

    if (props.contains(QLatin1String("Enabled")) &&
        enabled != qdbus_cast<bool>(props[QLatin1String("Enabled")])) {
        enabled = qdbus_cast<bool>(props[QLatin1String("Enabled")]);
        debug() << " Enabled:" << (enabled ? "true" : "false");
        emit parent->stateChanged(enabled);
        parent->notify("enabled");
    }

    if (props.contains(QLatin1String("ConnectAutomatically")) &&
        connectsAutomatically !=
                qdbus_cast<bool>(props[QLatin1String("ConnectAutomatically")])) {
        connectsAutomatically =
                qdbus_cast<bool>(props[QLatin1String("ConnectAutomatically")]);
        debug() << " Connects Automatically:" << (connectsAutomatically ? "true" : "false");
        emit parent->connectsAutomaticallyPropertyChanged(connectsAutomatically);
        parent->notify("connectsAutomatically");
    }

    if (props.contains(QLatin1String("HasBeenOnline")) &&
        !hasBeenOnline &&
        qdbus_cast<bool>(props[QLatin1String("HasBeenOnline")])) {
        hasBeenOnline = true;
        debug() << " HasBeenOnline changed to true";
        // don't emit firstOnline unless we're already ready, that would be
        // misleading - we'd emit it just before any already-used account
        // became ready
        if (parent->isReady(Account::FeatureCore)) {
            emit parent->firstOnline();
        }
        parent->notify("hasBeenOnline");
    }

    if (props.contains(QLatin1String("Parameters")) &&
        parameters != qdbus_cast<QVariantMap>(props[QLatin1String("Parameters")])) {
        parameters = qdbus_cast<QVariantMap>(props[QLatin1String("Parameters")]);
        emit parent->parametersChanged(parameters);
        parent->notify("parameters");
    }

    if (props.contains(QLatin1String("AutomaticPresence")) &&
        automaticPresence.barePresence() != qdbus_cast<SimplePresence>(
                props[QLatin1String("AutomaticPresence")])) {
        automaticPresence = Presence(qdbus_cast<SimplePresence>(
                props[QLatin1String("AutomaticPresence")]));
        debug() << " Automatic Presence:" << automaticPresence.type() <<
            "-" << automaticPresence.status();
        emit parent->automaticPresenceChanged(automaticPresence);
        parent->notify("automaticPresence");
    }

    if (props.contains(QLatin1String("CurrentPresence")) &&
        currentPresence.barePresence() != qdbus_cast<SimplePresence>(
                props[QLatin1String("CurrentPresence")])) {
        currentPresence = Presence(qdbus_cast<SimplePresence>(
                props[QLatin1String("CurrentPresence")]));
        debug() << " Current Presence:" << currentPresence.type() <<
            "-" << currentPresence.status();
        emit parent->currentPresenceChanged(currentPresence);
        parent->notify("currentPresence");
        emit parent->onlinenessChanged(parent->isOnline());
        parent->notify("online");
    }

    if (props.contains(QLatin1String("RequestedPresence")) &&
        requestedPresence.barePresence() != qdbus_cast<SimplePresence>(
                props[QLatin1String("RequestedPresence")])) {
        requestedPresence = Presence(qdbus_cast<SimplePresence>(
                props[QLatin1String("RequestedPresence")]));
        debug() << " Requested Presence:" << requestedPresence.type() <<
            "-" << requestedPresence.status();
        emit parent->requestedPresenceChanged(requestedPresence);
        parent->notify("requestedPresence");
    }

    if (props.contains(QLatin1String("ChangingPresence")) &&
        changingPresence != qdbus_cast<bool>(
                props[QLatin1String("ChangingPresence")])) {
        changingPresence = qdbus_cast<bool>(
                props[QLatin1String("ChangingPresence")]);
        debug() << " Changing Presence:" << changingPresence;
        emit parent->changingPresence(changingPresence);
        parent->notify("changingPresence");
    }

    if (props.contains(QLatin1String("Connection"))) {
        QString path = qdbus_cast<QDBusObjectPath>(props[QLatin1String("Connection")]).path();
        if (path.isEmpty()) {
            debug() << " The map contains \"Connection\" but it's empty as a QDBusObjectPath!";
            debug() << " Trying QString (known bug in some MC/dbus-glib versions)";
            path = qdbus_cast<QString>(props[QLatin1String("Connection")]);
        }

        debug() << " Connection Object Path:" << path;
        if (path == QLatin1String("/")) {
            path = QString();
        }

        connObjPathQueue.enqueue(path);

        if (connObjPathQueue.size() == 1) {
            processConnQueue();
        }

        // onConnectionBuilt for a previous path will make sure the path we enqueued is processed if
        // the queue wasn't empty (so is now size() > 1)
    }

    bool connectionStatusChanged = false;
    if (props.contains(QLatin1String("ConnectionStatus")) ||
        props.contains(QLatin1String("ConnectionStatusReason")) ||
        props.contains(QLatin1String("ConnectionError")) ||
        props.contains(QLatin1String("ConnectionErrorDetails"))) {
        ConnectionStatus oldConnectionStatus = connectionStatus;

        if (props.contains(QLatin1String("ConnectionStatus")) &&
            connectionStatus != ConnectionStatus(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatus")]))) {
            connectionStatus = ConnectionStatus(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatus")]));
            debug() << " Connection Status:" << connectionStatus;
            connectionStatusChanged = true;
        }

        if (props.contains(QLatin1String("ConnectionStatusReason")) &&
            connectionStatusReason != ConnectionStatusReason(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatusReason")]))) {
            connectionStatusReason = ConnectionStatusReason(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatusReason")]));
            debug() << " Connection StatusReason:" << connectionStatusReason;
            connectionStatusChanged = true;
        }

        if (connectionStatusChanged) {
            parent->notify("connectionStatus");
            parent->notify("connectionStatusReason");
        }

        if (props.contains(QLatin1String("ConnectionError")) &&
            connectionError != qdbus_cast<QString>(
                props[QLatin1String("ConnectionError")])) {
            connectionError = qdbus_cast<QString>(
                    props[QLatin1String("ConnectionError")]);
            debug() << " Connection Error:" << connectionError;
            connectionStatusChanged = true;
        }

        if (props.contains(QLatin1String("ConnectionErrorDetails")) &&
            connectionErrorDetails.allDetails() != qdbus_cast<QVariantMap>(
                props[QLatin1String("ConnectionErrorDetails")])) {
            connectionErrorDetails = Connection::ErrorDetails(qdbus_cast<QVariantMap>(
                    props[QLatin1String("ConnectionErrorDetails")]));
            debug() << " Connection Error Details:" << connectionErrorDetails.allDetails();
            connectionStatusChanged = true;
        }

        if (connectionStatusChanged) {
            /* Something other than status changed, let's not emit connectionStatusChanged
             * and keep the error/errorDetails, for the next interaction.
             * It may happen if ConnectionError changes and in another property
             * change the status changes to Disconnected, so we use the error
             * previously signalled. If the status changes to something other
             * than Disconnected later, the error is cleared. */
            if (oldConnectionStatus != connectionStatus) {
                /* We don't signal error for status other than Disconnected */
                if (connectionStatus != ConnectionStatusDisconnected) {
                    connectionError = QString();
                    connectionErrorDetails = Connection::ErrorDetails();
                } else if (connectionError.isEmpty()) {
                    connectionError = ConnectionHelper::statusReasonToErrorName(
                            connectionStatusReason, oldConnectionStatus);
                }

                checkCapabilitiesChanged(profileChanged);

                emit parent->connectionStatusChanged(connectionStatus);
                parent->notify("connectionError");
                parent->notify("connectionErrorDetails");
            } else {
                connectionStatusChanged = false;
            }
        }
    }

    if (!connectionStatusChanged && profileChanged) {
        checkCapabilitiesChanged(profileChanged);
    }
}

void Account::Private::retrieveAvatar()
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            parent->mPriv->properties->Get(
                TP_QT_IFACE_ACCOUNT_INTERFACE_AVATAR,
                QLatin1String("Avatar")), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotAvatar(QDBusPendingCallWatcher*)));
}

bool Account::Private::processConnQueue()
{
    while (!connObjPathQueue.isEmpty()) {
        QString path = connObjPathQueue.head();
        if (path.isEmpty()) {
            if (!connection.isNull()) {
                debug() << "Dropping connection for account" << parent->objectPath();

                connection.reset();
                emit parent->connectionChanged(connection);
                parent->notify("connection");
                parent->notify("connectionObjectPath");
            }

            connObjPathQueue.dequeue();
        } else {
            debug() << "Building connection" << path << "for account" << parent->objectPath();

            if (connection && connection->objectPath() == path) {
                debug() << "  Connection already built";
                connObjPathQueue.dequeue();
                continue;
            }

            QString busName = path.mid(1).replace(QLatin1String("/"), QLatin1String("."));
            parent->connect(connFactory->proxy(busName, path, chanFactory, contactFactory),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onConnectionBuilt(Tp::PendingOperation*)));

            // No dequeue here, but only in onConnectionBuilt, so we will queue future changes
            return false; // Only move on to the next paths when that build finishes
        }
    }

    return true;
}

void Account::onDispatcherIntrospected(Tp::PendingOperation *op)
{
    if (!mPriv->dispatcherContext->introspected) {
        Tp::PendingVariant *pv = static_cast<Tp::PendingVariant *>(op);
        Q_ASSERT(pv != NULL);

        // Only the first Account for a given dispatcher will enter this branch, and will
        // immediately make further created accounts skip the whole waiting for CD to get
        // introspected part entirely
        mPriv->dispatcherContext->introspected = true;

        if (pv->isValid()) {
            mPriv->dispatcherContext->supportsHints = qdbus_cast<bool>(pv->result());
            debug() << "Discovered channel dispatcher support for request hints: "
                << mPriv->dispatcherContext->supportsHints;
        } else {
            if (pv->errorName() == TP_QT_ERROR_NOT_IMPLEMENTED) {
                debug() << "Channel Dispatcher does not implement support for request hints";
            } else {
                warning() << "(Too old?) Channel Dispatcher failed to tell us whether"
                    << "it supports request hints, assuming it doesn't:"
                    << pv->errorName() << ':' << pv->errorMessage();
            }
            mPriv->dispatcherContext->supportsHints = false;
        }
    }

    debug() << "Calling Properties::GetAll(Account) on " << objectPath();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            mPriv->properties->GetAll(
                TP_QT_IFACE_ACCOUNT), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void Account::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Account) for" << objectPath();
        mPriv->updateProperties(reply.value());

        mPriv->readinessHelper->setInterfaces(interfaces());
        mPriv->mayFinishCore = true;

        if (mPriv->connObjPathQueue.isEmpty()) {
            debug() << "Account basic functionality is ready";
            mPriv->coreFinished = true;
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        } else {
            debug() << "Deferring finishing Account::FeatureCore until the connection is built";
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());

        warning().nospace() <<
            "GetAll(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    watcher->deleteLater();
}

void Account::gotAvatar(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to GetAvatar(Account)";
        mPriv->avatar = qdbus_cast<Avatar>(reply);

        // It could be in either of actual or missing from the first time in corner cases like the
        // object going away, so let's be prepared for both (only checking for actualFeatures here
        // actually used to trigger a rare bug)
        //
        // Anyway, the idea is to not do setIntrospectCompleted twice
        if (!mPriv->readinessHelper->actualFeatures().contains(FeatureAvatar) &&
                !mPriv->readinessHelper->missingFeatures().contains(FeatureAvatar)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureAvatar, true);
        }

        emit avatarChanged(mPriv->avatar);
        notify("avatar");
    } else {
        // check if the feature is already there, and for some reason retrieveAvatar
        // failed when called the second time
        if (!mPriv->readinessHelper->actualFeatures().contains(FeatureAvatar) &&
                !mPriv->readinessHelper->missingFeatures().contains(FeatureAvatar)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureAvatar, false, reply.error());
        }

        warning().nospace() <<
            "GetAvatar(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    watcher->deleteLater();
}

void Account::onAvatarChanged()
{
    debug() << "Avatar changed, retrieving it";
    mPriv->retrieveAvatar();
}

void Account::onConnectionManagerReady(PendingOperation *operation)
{
    bool error = operation->isError();
    if (!error) {
        error = !mPriv->cm->hasProtocol(mPriv->protocolName);
    }

    if (!error) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureProtocolInfo, true);
    }
    else {
        warning() << "Failed to find the protocol in the CM protocols for account" << objectPath();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureProtocolInfo, false,
                operation->errorName(), operation->errorMessage());
    }
}

void Account::onConnectionReady(PendingOperation *op)
{
    mPriv->checkCapabilitiesChanged(false);

    /* let's not fail if connection can't become ready, the caps will still
     * work, but return the CM caps instead. Also no need to call
     * setIntrospectCompleted if the feature was already set to complete once,
     * since this method will be called whenever the account connection
     * changes */
    if (!isReady(FeatureCapabilities)) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCapabilities, true);
    }
}

void Account::onPropertyChanged(const QVariantMap &delta)
{
    mPriv->updateProperties(delta);
}

void Account::onRemoved()
{
    mPriv->valid = false;
    mPriv->enabled = false;
    invalidate(TP_QT_ERROR_OBJECT_REMOVED,
            QLatin1String("Account removed from AccountManager"));
    emit removed();
}

void Account::onConnectionBuilt(PendingOperation *op)
{
    PendingReady *readyOp = qobject_cast<PendingReady *>(op);
    Q_ASSERT(readyOp != NULL);

    if (op->isError()) {
        warning() << "Building connection" << mPriv->connObjPathQueue.head() << "failed with" <<
            op->errorName() << "-" << op->errorMessage();

        if (!mPriv->connection.isNull()) {
            mPriv->connection.reset();
            emit connectionChanged(mPriv->connection);
            notify("connection");
            notify("connectionObjectPath");
        }
    } else {
        ConnectionPtr prevConn = mPriv->connection;
        QString prevConnPath = mPriv->connectionObjectPath();

        mPriv->connection = ConnectionPtr::qObjectCast(readyOp->proxy());
        Q_ASSERT(mPriv->connection);

        debug() << "Connection" << mPriv->connectionObjectPath() << "built for" << objectPath();

        if (prevConn != mPriv->connection) {
            notify("connection");
            emit connectionChanged(mPriv->connection);
        }

        if (prevConnPath != mPriv->connectionObjectPath()) {
            notify("connectionObjectPath");
        }
    }

    mPriv->connObjPathQueue.dequeue();

    if (mPriv->processConnQueue() && !mPriv->coreFinished && mPriv->mayFinishCore) {
        debug() << "Account" << objectPath() << "basic functionality is ready (connections built)";
        mPriv->coreFinished = true;
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

/**
 * \fn void Account::removed()
 *
 * Emitted when this account is removed from the account manager it belonged.
 *
 * \sa remove().
 */

/**
 * \fn void Account::validityChanged(bool validity)
 *
 * Emitted when the value of isValidAccount() changes.
 *
 * \param validity The new validity of this account.
 * \sa isValidAccount()
 */

/**
 * \fn void Account::stateChanged(bool state)
 *
 * Emitted when the value of isEnabled() changes.
 *
 * \param state The new state of this account.
 * \sa isEnabled()
 */

/**
 * \fn void Account::serviceNameChanged(const QString &serviceName)
 *
 * Emitted when the value of serviceName() changes.
 *
 * \param serviceName The new service name of this account.
 * \sa serviceName(), setServiceName()
 */

/**
 * \fn void Account::profileChanged(const Tp::ProfilePtr &profile)
 *
 * Emitted when the value of profile() changes.
 *
 * \param profile The new profile of this account.
 * \sa profile()
 */

/**
 * \fn void Account::displayNameChanged(const QString &displayName)
 *
 * Emitted when the value of displayName() changes.
 *
 * \param displayName The new display name of this account.
 * \sa displayName(), setDisplayName()
 */

/**
 * \fn void Account::iconNameChanged(const QString &iconName)
 *
 * Emitted when the value of iconName() changes.
 *
 * \param iconName The new icon name of this account.
 * \sa iconName(), setIconName()
 */

/**
 * \fn void Account::nicknameChanged(const QString &nickname)
 *
 * Emitted when the value of nickname() changes.
 *
 * \param nickname The new nickname of this account.
 * \sa nickname(), setNickname()
 */

/**
 * \fn void Account::normalizedNameChanged(const QString &normalizedName)
 *
 * Emitted when the value of normalizedName() changes.
 *
 * \param normalizedName The new normalized name of this account.
 * \sa normalizedName()
 */

/**
 * \fn void Account::capabilitiesChanged(const Tp::ConnectionCapabilities &capabilities)
 *
 * Emitted when the value of capabilities() changes.
 *
 * \param capabilities The new capabilities of this account.
 * \sa capabilities()
 */

/**
 * \fn void Account::connectsAutomaticallyPropertyChanged(bool connectsAutomatically)
 *
 * Emitted when the value of connectsAutomatically() changes.
 *
 * \param connectsAutomatically The new value of connects automatically property
 *                              of this account.
 * \sa isEnabled()
 */

/**
 * \fn void Account::firstOnline()
 *
 * Emitted when this account is first put online.
 *
 * \sa hasBeenOnline()
 */

/**
 * \fn void Account::parametersChanged(const QVariantMap &parameters)
 *
 * Emitted when the value of parameters() changes.
 *
 * \param parameters The new parameters of this account.
 * \sa parameters()
 */

/**
 * \fn void Account::changingPresence(bool value)
 *
 * Emitted when the value of isChangingPresence() changes.
 *
 * \param value Whether this account's connection is changing presence.
 * \sa isChangingPresence()
 */

/**
 * \fn void Account::automaticPresenceChanged(const Tp::Presence &automaticPresence)
 *
 * Emitted when the value of automaticPresence() changes.
 *
 * \param automaticPresence The new value of automatic presence property of this
 *                          account.
 * \sa automaticPresence(), currentPresenceChanged()
 */

/**
 * \fn void Account::currentPresenceChanged(const Tp::Presence &currentPresence)
 *
 * Emitted when the value of currentPresence() changes.
 *
 * \param currentPresence The new value of the current presence property of this
 *                        account.
 * \sa currentPresence()
 */

/**
 * \fn void Account::requestedPresenceChanged(const Tp::Presence &requestedPresence)
 *
 * Emitted when the value of requestedPresence() changes.
 *
 * \param requestedPresence The new value of the requested presence property of this
 *                          account.
 * \sa requestedPresence(), currentPresenceChanged()
 */

/**
 * \fn void Account::onlinenessChanged(bool online)
 *
 * Emitted when the value of isOnline() changes.
 *
 * \param online Whether this account is online.
 * \sa isOnline(), currentPresence()
 */

/**
 * \fn void Account::avatarChanged(const Tp::Avatar &avatar)
 *
 * Emitted when the value of avatar() changes.
 *
 * \param avatar The new avatar of this account.
 * \sa avatar()
 */

/**
 * \fn void Account::connectionStatusChanged(Tp::ConnectionStatus status)
 *
 * Emitted when the connection status changes.
 *
 * \param status The new status of this account connection.
 * \sa connectionStatus(), connectionStatusReason(), connectionError(), connectionErrorDetails(),
 *     Connection::ErrorDetails
 */

/**
 * \fn void Account::connectionChanged(const Tp::ConnectionPtr &connection)
 *
 * Emitted when the value of connection() changes.
 *
 * The \a connection will have the features set in the ConnectionFactory used by this account ready
 * and the same channel and contact factories used by this account.
 *
 * \param connection A ConnectionPtr pointing to the new Connection object or a null ConnectionPtr
 *                   if there is no connection.
 * \sa connection()
 */

} // Tp
