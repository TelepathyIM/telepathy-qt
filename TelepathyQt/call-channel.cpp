/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/CallChannel>

#include "TelepathyQt/_gen/call-channel.moc.hpp"

#include <TelepathyQt/debug-internal.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/PendingVariant>

namespace Tp
{

struct TP_QT_NO_EXPORT CallChannel::Private
{
    Private(CallChannel *parent);
    ~Private();

    static void introspectCore(Private *self);
    static void introspectCallState(Private *self);
    static void introspectCallMembers(Private *self);
    static void introspectContents(Private *self);
    static void introspectLocalHoldState(Private *self);

    void processCallMembersChanged();

    struct CallMembersChangedInfo;

    // Public object
    CallChannel *parent;

    // Mandatory proxies
    Client::ChannelTypeCallInterface *callInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint state;
    uint flags;
    CallStateReason stateReason;
    QVariantMap stateDetails;

    CallMemberMap callMembers;
    QHash<uint, ContactPtr> callMembersContacts;
    QQueue< QSharedPointer<CallMembersChangedInfo> > callMembersChangedQueue;
    QSharedPointer<CallMembersChangedInfo> currentCallMembersChangedInfo;

    bool hardwareStreaming;
    uint initialTransportType;
    bool initialAudio;
    bool initialVideo;
    QString initialAudioName;
    QString initialVideoName;

    bool mutableContents;
    CallContents contents;
    CallContents incompleteContents;

    uint localHoldState;
    uint localHoldStateReason;
};

struct TP_QT_NO_EXPORT CallChannel::Private::CallMembersChangedInfo
{
    CallMembersChangedInfo(const CallMemberMap &updates,
            const HandleIdentifierMap &identifiers,
            const UIntList &removed,
            const CallStateReason &reason)
        : updates(updates),
          identifiers(identifiers),
          removed(removed),
          reason(reason)
    {
    }

    static QSharedPointer<CallMembersChangedInfo> create(
            const CallMemberMap &updates,
            const HandleIdentifierMap &identifiers,
            const UIntList &removed,
            const CallStateReason &reason)
    {
        CallMembersChangedInfo *info = new CallMembersChangedInfo(
                updates, identifiers, removed, reason);
        return QSharedPointer<CallMembersChangedInfo>(info);
    }

    CallMemberMap updates;
    HandleIdentifierMap identifiers;
    UIntList removed;
    CallStateReason reason;
};

CallChannel::Private::Private(CallChannel *parent)
    : parent(parent),
      callInterface(parent->interface<Client::ChannelTypeCallInterface>()),
      readinessHelper(parent->readinessHelper()),
      state(CallStateUnknown),
      flags((uint) -1),
      hardwareStreaming(false),
      initialTransportType(StreamTransportTypeUnknown),
      initialAudio(false),
      initialVideo(false),
      mutableContents(false),
      localHoldState(LocalHoldStateUnheld),
      localHoldStateReason(LocalHoldStateReasonNone)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                        // dependsOnFeatures (core)
        QStringList(),                                                             // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCore,
        this);
    introspectables[FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableCallState(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << CallChannel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList(),                                                             // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCallState,
        this);
    introspectables[FeatureCallState] = introspectableCallState;

    ReadinessHelper::Introspectable introspectableCallMembers(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << CallChannel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList(),                                                             // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCallMembers,
        this);
    introspectables[FeatureCallMembers] = introspectableCallMembers;

    ReadinessHelper::Introspectable introspectableContents(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << CallChannel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList(),                                                             // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectContents,
        this);
    introspectables[FeatureContents] = introspectableContents;

    ReadinessHelper::Introspectable introspectableLocalHoldState(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << CallChannel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_HOLD,                      // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectLocalHoldState,
        this);
    introspectables[FeatureLocalHoldState] = introspectableLocalHoldState;

    readinessHelper->addIntrospectables(introspectables);
}

CallChannel::Private::~Private()
{
}

void CallChannel::Private::introspectCore(CallChannel::Private *self)
{
    const static QString qualifiedNames[] = {
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".HardwareStreaming"),
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialTransport"),
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"),
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"),
        TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".MutableContents")
    };

    CallChannel *parent = self->parent;
    QVariantMap immutableProperties = parent->immutableProperties();
    bool needIntrospectMainProps = false;

    for (unsigned i = 0; i < sizeof(qualifiedNames)/sizeof(QString); ++i) {
        const QString &qualified = qualifiedNames[i];
        if (!immutableProperties.contains(qualified)) {
            needIntrospectMainProps = true;
            break;
        }
    }

    if (needIntrospectMainProps) {
        debug() << "Introspecting immutable properties of CallChannel";

        parent->connect(self->callInterface->requestAllProperties(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotMainProperties(Tp::PendingOperation*)));
    } else {
        self->hardwareStreaming = qdbus_cast<bool>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".HardwareStreaming")]);
        self->initialTransportType = qdbus_cast<uint>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialTransport")]);
        self->initialAudio = qdbus_cast<bool>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio")]);
        self->initialVideo = qdbus_cast<bool>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo")]);
        self->initialAudioName = qdbus_cast<QString>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName")]);
        self->initialVideoName = qdbus_cast<QString>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName")]);
        self->mutableContents = qdbus_cast<bool>(immutableProperties[
                TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".MutableContents")]);

        self->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

void CallChannel::Private::introspectCallState(CallChannel::Private *self)
{
    CallChannel *parent = self->parent;

    parent->connect(self->callInterface,
            SIGNAL(CallStateChanged(uint,uint,Tp::CallStateReason,QVariantMap)),
            SLOT(onCallStateChanged(uint,uint,Tp::CallStateReason,QVariantMap)));

    parent->connect(self->callInterface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotCallState(Tp::PendingOperation*)));
}

void CallChannel::Private::introspectCallMembers(CallChannel::Private *self)
{
    CallChannel *parent = self->parent;

    parent->connect(self->callInterface,
            SIGNAL(CallMembersChanged(Tp::CallMemberMap,Tp::HandleIdentifierMap,Tp::UIntList,Tp::CallStateReason)),
            SLOT(onCallMembersChanged(Tp::CallMemberMap,Tp::HandleIdentifierMap,Tp::UIntList,Tp::CallStateReason)));

    parent->connect(self->callInterface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotCallMembers(Tp::PendingOperation*)));
}

void CallChannel::Private::introspectContents(CallChannel::Private *self)
{
    CallChannel *parent = self->parent;

    parent->connect(self->callInterface,
            SIGNAL(ContentAdded(QDBusObjectPath)),
            SLOT(onContentAdded(QDBusObjectPath)));
    parent->connect(self->callInterface,
            SIGNAL(ContentRemoved(QDBusObjectPath,Tp::CallStateReason)),
            SLOT(onContentRemoved(QDBusObjectPath,Tp::CallStateReason)));

    parent->connect(self->callInterface->requestPropertyContents(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContents(Tp::PendingOperation*)));
}

void CallChannel::Private::introspectLocalHoldState(CallChannel::Private *self)
{
    CallChannel *parent = self->parent;
    Client::ChannelInterfaceHoldInterface *holdInterface =
        parent->interface<Client::ChannelInterfaceHoldInterface>();

    parent->connect(holdInterface,
            SIGNAL(HoldStateChanged(uint,uint)),
            SLOT(onLocalHoldStateChanged(uint,uint)));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            holdInterface->GetHoldState(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotLocalHoldState(QDBusPendingCallWatcher*)));
}

void CallChannel::Private::processCallMembersChanged()
{
    if (currentCallMembersChangedInfo) { // currently building contacts
        return;
    }

    if (callMembersChangedQueue.isEmpty()) {
        if (!parent->isReady(FeatureCallMembers)) {
            readinessHelper->setIntrospectCompleted(FeatureCallMembers, true);
        }
        return;
    }

    currentCallMembersChangedInfo = callMembersChangedQueue.dequeue();

    QSet<uint> pendingCallMembers;
    for (ContactSendingStateMap::const_iterator i = currentCallMembersChangedInfo->updates.constBegin();
            i != currentCallMembersChangedInfo->updates.constEnd(); ++i) {
        pendingCallMembers.insert(i.key());
    }

    foreach(uint i, currentCallMembersChangedInfo->removed) {
        pendingCallMembers.insert(i);
    }

    if (!pendingCallMembers.isEmpty()) {
        ConnectionPtr connection = parent->connection();
        connection->lowlevel()->injectContactIds(currentCallMembersChangedInfo->identifiers);

        ContactManagerPtr contactManager = connection->contactManager();
        PendingContacts *contacts = contactManager->contactsForHandles(
                pendingCallMembers.toList());
        parent->connect(contacts,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotCallMembersContacts(Tp::PendingOperation*)));
    } else {
        currentCallMembersChangedInfo.clear();
        processCallMembersChanged();
    }
}

/**
 * \class CallChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/call-channel.h <TelepathyQt/CallChannel>
 *
 * \brief The CallChannel class provides an object representing a
 * Telepathy channel of type Call.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * CallChannel object usable.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature CallChannel::FeatureCore = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 0, true);

/**
 * Feature used in order to access call state specific methods.
 *
 * See call state specific methods' documentation for more details.
 */
const Feature CallChannel::FeatureCallState = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 1);

/**
 * Feature used in order to access members specific methods.
 *
 * See local members specific methods' documentation for more details.
 */
const Feature CallChannel::FeatureCallMembers = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 2);

/**
 * Feature used in order to access content specific methods.
 *
 * See media content specific methods' documentation for more details.
 */
const Feature CallChannel::FeatureContents = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 3);

/**
 * Feature used in order to access local hold state info.
 *
 * See local hold state specific methods' documentation for more details.
 */
const Feature CallChannel::FeatureLocalHoldState = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 4);

/**
 * Create a new CallChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A CallChannelPtr object pointing to the newly created
 *         CallChannel object.
 */
CallChannelPtr CallChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return CallChannelPtr(new CallChannel(connection, objectPath, immutableProperties));
}

/**
 * Construct a new CallChannel associated with the given object on the same
 * service as the given connection.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \param coreFeature The core feature of the channel type. The corresponding introspectable
 *                     should depend on Channel::FeatureCore.
 */
CallChannel::CallChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
CallChannel::~CallChannel()
{
    delete mPriv;
}

/**
 * Return the current high-level state of this call.
 *
 * This function requires CallChannel::FeatureCallState to be enabled.
 *
 * \return The current high-level state of this call.
 * \sa callStateChanged()
 */
CallState CallChannel::callState() const
{
    if (!isReady(FeatureCallState)) {
        warning() << "CallChannel::callState() used with FeatureCallState not ready";
    }

    return (CallState) mPriv->state;
}

/**
 * Return the flags representing the status of this call as a whole, providing more specific
 * information than callState().
 *
 * This function requires CallChannel::FeatureCallState to be enabled.
 *
 * \return The flags representing the status of this call.
 * \sa callFlagsChanged()
 */
CallFlags CallChannel::callFlags() const
{
    if (!isReady(FeatureCallState)) {
        warning() << "CallChannel::callFlags() used with FeatureCallState not ready";
    }

    return (CallFlags) mPriv->flags;
}

/**
 * Return the reason for the last change to the callState() and/or callFlags().
 *
 * This function requires CallChannel::FeatureCallState to be enabled.
 *
 * \return The reason for the last change to the callState() and/or callFlags().
 * \sa callStateChanged(), callFlagsChanged()
 */
CallStateReason CallChannel::callStateReason() const
{
    if (!isReady(FeatureCallState)) {
        warning() << "CallChannel::callStateReason() used with FeatureCallState not ready";
    }

    return mPriv->stateReason;
}

/**
 * Return optional extensible details for the callState(),
 * callFlags() and/or callStateReason().
 *
 * This function requires CallChannel::FeatureCallState to be enabled.
 *
 * \return The optional extensible details for the callState(),
 * callFlags() and/or callStateReason().
 * \sa callStateChanged(), callFlagsChanged()
 */
QVariantMap CallChannel::callStateDetails() const
{
    if (!isReady(FeatureCallState)) {
        warning() << "CallChannel::callStateDetails() used with FeatureCallState not ready";
    }

    return mPriv->stateDetails;
}

/**
 * Return the remote members of this call.
 *
 * This function requires CallChannel::FeatureCallMembers to be enabled.
 *
 * \return The remote members of this call.
 * \sa remoteMemberFlags(), remoteMemberFlagsChanged(), remoteMembersRemoved()
 */
Contacts CallChannel::remoteMembers() const
{
    if (!isReady(FeatureCallMembers)) {
        warning() << "CallChannel::remoteMembers() used with FeatureCallMembers not ready";
        return Contacts();
    }

    return mPriv->callMembersContacts.values().toSet();
}

/**
 * Return the flags that describe the status of a given \a member of this call.
 *
 * This function requires CallChannel::FeatureCallMembers to be enabled.
 *
 * \param member The member of interest.
 * \return The flags that describe the status of the requested member.
 * \sa remoteMemberFlagsChanged(), remoteMembers(), remoteMembersRemoved()
 */
CallMemberFlags CallChannel::remoteMemberFlags(const ContactPtr &member) const
{
    if (!isReady(FeatureCallMembers)) {
        warning() << "CallChannel::remoteMemberFlags() used with FeatureCallMembers not ready";
        return (CallMemberFlags) 0;
    }

    if (!member) {
        return (CallMemberFlags) 0;
    }

    for (CallMemberMap::const_iterator i = mPriv->callMembers.constBegin();
            i != mPriv->callMembers.constEnd(); ++i) {
        uint handle = i.key();
        CallMemberFlags sendingState = (CallMemberFlags) i.value();

        if (handle == member->handle()[0]) {
            return sendingState;
        }
    }

    return (CallMemberFlags) 0;
}

/**
 * Check whether media streaming by the handler is required for this channel.
 *
 * If \c false, all of the media streaming is done by some mechanism outside the scope
 * of Telepathy, otherwise the handler is responsible for doing the actual media streaming.
 *
 * \return \c true if required, \c false otherwise.
 */
bool CallChannel::handlerStreamingRequired() const
{
    return !mPriv->hardwareStreaming;
}

/**
 * Return the initial transport type used for this call if set on a requested channel.
 *
 * Where not applicable, this property is defined to be #StreamTransportTypeUnknown, in
 * particular, on CMs with hardware streaming.
 *
 * \return The initial transport type used for this call.
 */
StreamTransportType CallChannel::initialTransportType() const
{
    return (StreamTransportType) mPriv->initialTransportType;
}

/**
 * Return whether an audio content was requested at the channel's creation time.
 *
 * \return \c true if an audio content was requested, \c false otherwise.
 */
bool CallChannel::hasInitialAudio() const
{
    return mPriv->initialAudio;
}

/**
 * Return whether a video content was requested at the channel's creation time.
 *
 * \return \c true if an video content was requested, \c false otherwise.
 */
bool CallChannel::hasInitialVideo() const
{
    return mPriv->initialVideo;
}

/**
 * Return the name of the initial audio content if hasInitialAudio() returns \c true.
 *
 * \return The name of the initial audio content.
 */
QString CallChannel::initialAudioName() const
{
    return mPriv->initialAudioName;
}

/**
 * Return the name of the initial video content if hasInitialVideo() returns \c true.
 *
 * \return The name of the initial video content.
 */
QString CallChannel::initialVideoName() const
{
    return mPriv->initialVideoName;
}

/**
 * Return whether new contents can be added on the call after the Channel has
 * been requested.
 *
 * \return \c true if a new content can be added after the Channel has been
 *         requested, \c false otherwise.
 * \sa requestContent()
 */
bool CallChannel::hasMutableContents() const
{
    return mPriv->mutableContents;
}

/**
 * Indicate that the local user has been alerted about the incoming call.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *CallChannel::setRinging()
{
    return new PendingVoid(mPriv->callInterface->SetRinging(), CallChannelPtr(this));
}

/**
 * Notify the CM that the local user is already in a call, so this
 * call has been put in a call-waiting style queue.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *CallChannel::setQueued()
{
    return new PendingVoid(mPriv->callInterface->SetQueued(), CallChannelPtr(this));
}

/**
 * Accept an incoming call, or begin calling the remote contact on an outgoing call.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *CallChannel::accept()
{
    return new PendingVoid(mPriv->callInterface->Accept(), CallChannelPtr(this));
}

/**
 * Request that the call is ended.
 *
 * \param reason A generic hangup reason.
 * \param detailedReason A more specific reason for the call hangup, if one is
 *                       available, or an empty string otherwise.
 * \param message A human-readable message to be sent to the remote contact(s).
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *CallChannel::hangup(CallStateChangeReason reason,
        const QString &detailedReason, const QString &message)
{
    return new PendingVoid(mPriv->callInterface->Hangup(reason, detailedReason, message),
            CallChannelPtr(this));
}

/**
 * Return a list of media contents in this channel.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \return The contents in this channel.
 * \sa contentAdded(), contentRemoved(), contentsForType(), contentByName(), requestContent()
 */
CallContents CallChannel::contents() const
{
    if (!isReady(FeatureContents)) {
        warning() << "CallChannel::contents() used with FeatureContents not ready";
        return CallContents();
    }

    return mPriv->contents;
}

/**
 * Return a list of media contents in this channel for the given type \a type.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \param type The interested type.
 * \return A list of media contents in this channel for the given type \a type.
 * \sa contentAdded(), contentRemoved(), contents(), contentByName(), requestContent()
 */
CallContents CallChannel::contentsForType(MediaStreamType type) const
{
    if (!isReady(FeatureContents)) {
        warning() << "CallChannel::contents() used with FeatureContents not ready";
        return CallContents();
    }

    CallContents contents;
    foreach (const CallContentPtr &content, mPriv->contents) {
        if (content->type() == type) {
            contents.append(content);
        }
    }
    return contents;
}

/**
 * Return the media content in this channel that has the specified \a name.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \param name The interested name.
 * \return The media content in this channel that has the specified \a name.
 * \sa contentAdded(), contentRemoved(), contents(), contentsForType(), requestContent()
 */
CallContentPtr CallChannel::contentByName(const QString &contentName) const
{
    if (!isReady(FeatureContents)) {
        warning() << "CallChannel::contentByName() used with FeatureContents not ready";
        return CallContentPtr();
    }

    foreach (const CallContentPtr &content, mPriv->contents) {
        if (content->name() == contentName) {
            return content;
        }
    }
    return CallContentPtr();
}

/**
 * Request a new media content to be created to exchange the given type \a type
 * of media.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \return A PendingCallContent which will emit PendingCallContent::finished
 *         when the call has finished.
 * \sa contentAdded(), contents(), contentsForType(), contentByName()
 */
PendingCallContent *CallChannel::requestContent(const QString &name,
        MediaStreamType type, MediaStreamDirection direction)
{
    return new PendingCallContent(CallChannelPtr(this), name, type, direction);
}

/**
 * Return whether the local user has placed this channel on hold.
 *
 * This method requires CallChannel::FeatureHoldState to be enabled.
 *
 * \return The channel's local hold state.
 * \sa requestHold(), localHoldStateChanged()
 */
LocalHoldState CallChannel::localHoldState() const
{
    if (!isReady(FeatureLocalHoldState)) {
        warning() << "CallChannel::localHoldState() used with FeatureLocalHoldState not ready";
    } else if (!hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "CallChannel::localHoldStateReason() used with no hold interface";
    }

    return (LocalHoldState) mPriv->localHoldState;
}

/**
 * Return the reason why localHoldState() changed to its current value.
 *
 * This method requires CallChannel::FeatureLocalHoldState to be enabled.
 *
 * \return The channel local hold state reason.
 * \sa requestHold(), localHoldStateChanged()
 */
LocalHoldStateReason CallChannel::localHoldStateReason() const
{
    if (!isReady(FeatureLocalHoldState)) {
        warning() << "CallChannel::localHoldStateReason() used with FeatureLocalHoldState not ready";
    } else if (!hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "CallChannel::localHoldStateReason() used with no hold interface";
    }

    return (LocalHoldStateReason) mPriv->localHoldStateReason;
}

/**
 * Request that the channel be put on hold (be instructed not to send any media
 * streams to you) or be taken off hold.
 *
 * If the connection manager can immediately tell that the requested state
 * change could not possibly succeed, the resulting PendingOperation will fail
 * with error code #TP_QT_ERROR_NOT_AVAILABLE.
 * If the requested state is the same as the current state, the resulting
 * PendingOperation will finish successfully.
 *
 * Otherwise, the channel's local hold state will change to
 * Tp::LocalHoldStatePendingHold or Tp::LocalHoldStatePendingUnhold (as
 * appropriate), then the resulting PendingOperation will finish successfully.
 *
 * The eventual success or failure of the request is indicated by a subsequent
 * localHoldStateChanged() signal, changing the local hold state to
 * Tp::LocalHoldStateHeld or Tp::LocalHoldStateUnheld.
 *
 * If the channel has multiple streams, and the connection manager succeeds in
 * changing the hold state of one stream but fails to change the hold state of
 * another, it will attempt to revert all streams to their previous hold
 * states.
 *
 * If the channel does not support the #TP_QT_IFACE_CHANNEL_INTERFACE_HOLD
 * interface, the PendingOperation will fail with error code
 * #TP_QT_ERROR_NOT_IMPLEMENTED.
 *
 * \param hold A boolean indicating whether or not the channel should be on hold
 * \return A PendingOperation, which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa localHoldState(), localHoldStateReason(), localHoldStateChanged()
 */
PendingOperation *CallChannel::requestHold(bool hold)
{
    if (!hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "CallChannel::requestHold() used with no hold interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("CallChannel does not support hold interface"),
                CallChannelPtr(this));
    }

    Client::ChannelInterfaceHoldInterface *holdInterface =
        interface<Client::ChannelInterfaceHoldInterface>();
    return new PendingVoid(holdInterface->RequestHold(hold), CallChannelPtr(this));
}

void CallChannel::gotMainProperties(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "CallInterface::requestAllProperties() failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
            op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Got reply to CallInterface::requestAllProperties()";

    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);
    Q_ASSERT(pvm);

    QVariantMap props = pvm->result();

    mPriv->hardwareStreaming = qdbus_cast<bool>(props[QLatin1String("HardwareStreaming")]);
    mPriv->initialTransportType = qdbus_cast<uint>(props[QLatin1String("InitialTransport")]);
    mPriv->initialAudio = qdbus_cast<bool>(props[QLatin1String("InitialAudio")]);
    mPriv->initialVideo = qdbus_cast<bool>(props[QLatin1String("InitialVideo")]);
    mPriv->initialAudioName = qdbus_cast<QString>(props[QLatin1String("InitialAudioName")]);
    mPriv->initialVideoName = qdbus_cast<QString>(props[QLatin1String("InitialVideoName")]);
    mPriv->mutableContents = qdbus_cast<bool>(props[QLatin1String("MutableContents")]);

    mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

void CallChannel::gotCallState(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "CallInterface::requestAllProperties() failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCallState, false,
            op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Got reply to CallInterface::requestAllProperties()";

    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);
    Q_ASSERT(pvm);

    QVariantMap props = pvm->result();

    mPriv->state = qdbus_cast<uint>(props[QLatin1String("CallState")]);
    mPriv->flags = qdbus_cast<uint>(props[QLatin1String("CallFlags")]);
    mPriv->stateReason = qdbus_cast<CallStateReason>(props[QLatin1String("CallStateReason")]);
    mPriv->stateDetails = qdbus_cast<QVariantMap>(props[QLatin1String("CallStateDetails")]);

    mPriv->readinessHelper->setIntrospectCompleted(FeatureCallState, true);
}

void CallChannel::onCallStateChanged(uint state, uint flags,
        const CallStateReason &stateReason, const QVariantMap &stateDetails)
{
    if (mPriv->state == state && mPriv->flags == flags && mPriv->stateReason == stateReason &&
        mPriv->stateDetails == stateDetails) {
        // nothing changed
        return;
    }

    uint oldState = mPriv->state;
    uint oldFlags = mPriv->flags;

    mPriv->state = state;
    mPriv->flags = flags;
    mPriv->stateReason = stateReason;
    mPriv->stateDetails = stateDetails;

    if (oldState != state) {
        emit callStateChanged((CallState) state);
    }

    if (oldFlags != flags) {
        emit callFlagsChanged((CallFlags) flags);
    }
}

void CallChannel::gotCallMembers(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "CallInterface::requestAllProperties() failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCallMembers, false,
            op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Got reply to CallInterface::requestAllProperties()";

    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);
    Q_ASSERT(pvm);

    QVariantMap props = pvm->result();

    HandleIdentifierMap ids = qdbus_cast<HandleIdentifierMap>(props[QLatin1String("MemberIdentifiers")]);
    CallMemberMap callMembers = qdbus_cast<CallMemberMap>(props[QLatin1String("CallMembers")]);

    mPriv->callMembersChangedQueue.enqueue(Private::CallMembersChangedInfo::create(
                callMembers, ids, UIntList(), CallStateReason()));
    mPriv->processCallMembersChanged();
}

void CallChannel::gotCallMembersContacts(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);

    if (!pc->isValid()) {
        warning().nospace() << "Getting contacts failed with " <<
            pc->errorName() << ":" << pc->errorMessage() << ", ignoring";
        mPriv->currentCallMembersChangedInfo.clear();
        mPriv->processCallMembersChanged();
        return;
    }

    QHash<uint, ContactPtr> removed;

    for (ContactSendingStateMap::const_iterator i =
                mPriv->currentCallMembersChangedInfo->updates.constBegin();
            i != mPriv->currentCallMembersChangedInfo->updates.constEnd(); ++i) {
        mPriv->callMembers.insert(i.key(), i.value());
    }

    foreach (const ContactPtr &contact, pc->contacts()) {
        mPriv->callMembersContacts.insert(contact->handle()[0], contact);
    }

    foreach (uint handle, mPriv->currentCallMembersChangedInfo->removed) {
        mPriv->callMembers.remove(handle);
        if (isReady(FeatureCallMembers) && mPriv->callMembersContacts.contains(handle)) {
            removed.insert(handle, mPriv->callMembersContacts[handle]);

            // make sure we don't have updates for removed contacts
            mPriv->currentCallMembersChangedInfo->updates.remove(handle);
        }
        mPriv->callMembersContacts.remove(handle);
    }

    foreach (uint handle, pc->invalidHandles()) {
        mPriv->callMembers.remove(handle);
        if (isReady(FeatureCallMembers) && mPriv->callMembersContacts.contains(handle)) {
            removed.insert(handle, mPriv->callMembersContacts[handle]);

            // make sure we don't have updates for invalid handles
            mPriv->currentCallMembersChangedInfo->updates.remove(handle);
        }
        mPriv->callMembersContacts.remove(handle);
    }

    if (isReady(FeatureCallMembers)) {
        QHash<ContactPtr, CallMemberFlags> remoteMemberFlags;
        for (CallMemberMap::const_iterator i =
                    mPriv->currentCallMembersChangedInfo->updates.constBegin();
                i != mPriv->currentCallMembersChangedInfo->updates.constEnd(); ++i) {
            uint handle = i.key();
            CallMemberFlags flags = (CallMemberFlags) i.value();

            Q_ASSERT(mPriv->callMembersContacts.contains(handle));
            remoteMemberFlags.insert(mPriv->callMembersContacts[handle], flags);

            mPriv->callMembers.insert(i.key(), i.value());
        }

        if (!remoteMemberFlags.isEmpty()) {
            emit remoteMemberFlagsChanged(remoteMemberFlags,
                    mPriv->currentCallMembersChangedInfo->reason);
        }

        if (!removed.isEmpty()) {
            emit remoteMembersRemoved(removed.values().toSet(),
                    mPriv->currentCallMembersChangedInfo->reason);
        }
    }

    mPriv->currentCallMembersChangedInfo.clear();
    mPriv->processCallMembersChanged();
}

void CallChannel::onCallMembersChanged(const CallMemberMap &updates,
        const HandleIdentifierMap &identifiers,
        const UIntList &removed,
        const CallStateReason &reason)
{
    if (updates.isEmpty() && removed.isEmpty()) {
        debug() << "Received Call::CallMembersChanged with 0 removals and updates, skipping it";
        return;
    }

    debug() << "Received Call::CallMembersChanged with" << updates.size() <<
        "updated and" << removed.size() << "removed";
    mPriv->callMembersChangedQueue.enqueue(
            Private::CallMembersChangedInfo::create(updates, identifiers, removed, reason));
    mPriv->processCallMembersChanged();
}

void CallChannel::gotContents(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "CallInterface::requestPropertyContents() failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, false,
            op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Got reply to CallInterface::requestPropertyContents()";

    PendingVariant *pv = qobject_cast<PendingVariant*>(op);
    Q_ASSERT(pv);

    ObjectPathList contentsPaths = qdbus_cast<ObjectPathList>(pv->result());
    if (contentsPaths.size() > 0) {
        foreach (const QDBusObjectPath &contentPath, contentsPaths) {
            CallContentPtr content = lookupContent(contentPath);
            if (!content) {
                addContent(contentPath);
            }
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }
}

void CallChannel::onContentAdded(const QDBusObjectPath &contentPath)
{
    debug() << "Received Call::ContentAdded for content" << contentPath.path();

    if (lookupContent(contentPath)) {
        debug() << "Content already exists, ignoring";
        return;
    }

    addContent(contentPath);
}

void CallChannel::onContentRemoved(const QDBusObjectPath &contentPath,
        const CallStateReason &reason)
{
    debug() << "Received Call::ContentRemoved for content" << contentPath.path();

    CallContentPtr content = lookupContent(contentPath);
    if (!content) {
        debug() << "Content does not exist, ignoring";
        return;
    }

    bool incomplete = mPriv->incompleteContents.contains(content);
    if (incomplete) {
        mPriv->incompleteContents.removeOne(content);
    } else {
        mPriv->contents.removeOne(content);
    }

    if (isReady(FeatureContents) && !incomplete) {
        emit contentRemoved(content, reason);
    }

    // the content was added/removed before become ready
    if (!isReady(FeatureContents) &&
        mPriv->contents.size() == 0 &&
        mPriv->incompleteContents.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }
}

void CallChannel::onContentReady(PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    CallContentPtr content = CallContentPtr::qObjectCast(pr->proxy());

    if (op->isError()) {
        mPriv->incompleteContents.removeOne(content);
        if (!isReady(FeatureContents) && mPriv->incompleteContents.size() == 0) {
            // let's not fail because a content could not become ready
            mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
        }
        return;
    }

    // the content was removed before become ready
    if (!mPriv->incompleteContents.contains(content)) {
        if (!isReady(FeatureContents) &&
            mPriv->incompleteContents.size() == 0) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
        }
        return;
    }

    mPriv->incompleteContents.removeOne(content);
    mPriv->contents.append(content);

    if (isReady(FeatureContents)) {
        emit contentAdded(content);
    }

    if (!isReady(FeatureContents) && mPriv->incompleteContents.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }
}

void CallChannel::gotLocalHoldState(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Call::Hold::GetHoldState() failed with " <<
            reply.error().name() << ": " << reply.error().message();
        debug() << "Ignoring error getting hold state and assuming we're not on hold";
        onLocalHoldStateChanged(mPriv->localHoldState, mPriv->localHoldStateReason);
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to Call::Hold::GetHoldState()";
    onLocalHoldStateChanged(reply.argumentAt<0>(), reply.argumentAt<1>());
    watcher->deleteLater();
}

void CallChannel::onLocalHoldStateChanged(uint localHoldState, uint localHoldStateReason)
{
    bool changed = false;
    if (mPriv->localHoldState != localHoldState ||
        mPriv->localHoldStateReason != localHoldStateReason) {
        changed = true;
    }

    mPriv->localHoldState = localHoldState;
    mPriv->localHoldStateReason = localHoldStateReason;

    if (!isReady(FeatureLocalHoldState)) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureLocalHoldState, true);
    } else {
        if (changed) {
            emit localHoldStateChanged((LocalHoldState) mPriv->localHoldState,
                    (LocalHoldStateReason) mPriv->localHoldStateReason);
        }
    }
}

CallContentPtr CallChannel::addContent(const QDBusObjectPath &contentPath)
{
    CallContentPtr content = CallContentPtr(
            new CallContent(CallChannelPtr(this), contentPath));
    mPriv->incompleteContents.append(content);
    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContentReady(Tp::PendingOperation*)));
    return content;
}

CallContentPtr CallChannel::lookupContent(const QDBusObjectPath &contentPath) const
{
    foreach (const CallContentPtr &content, mPriv->contents) {
        if (content->objectPath() == contentPath.path()) {
            return content;
        }
    }
    foreach (const CallContentPtr &content, mPriv->incompleteContents) {
        if (content->objectPath() == contentPath.path()) {
            return content;
        }
    }
    return CallContentPtr();
}

/**
 * \fn void CallChannel::callStateChanged(Tp::CallState state);
 *
 * This signal is emitted when the value of callState() changes.
 *
 * \param state The new state.
 */

/**
 * \fn void CallChannel::callFlagsChanged(Tp::CallFlags flags);
 *
 * This signal is emitted when the value of callFlags() changes.
 *
 * \param flags The new flags.
 */

/**
 * \fn void CallChannel::remoteMemberFlagsChanged(const QHash<Tp::ContactPtr, Tp::CallMemberFlags> &remoteMemberFlags, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when the flags of members of the call change,
 * or when new members are added in the call.
 *
 * \param remoteMemberFlags A maping of all the call members whose flags were
 * changed to their new flags, and of all the new members of the call to their initial flags.
 * \param reason The reason for this change.
 */

/**
 * \fn void CallChannel::remoteMembersRemoved(const Tp::Contacts &remoteMembers, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when remote members are removed from the call.
 *
 * \param remoteMembers The members that were removed.
 * \param reason The reason for this removal.
 */

/**
 * \fn void CallChannel::contentAdded(const Tp::CallContentPtr &content);
 *
 * This signal is emitted when a media content is added to this channel.
 *
 * \param content The media content that was added.
 * \sa contents(), contentsForType()
 */

/**
 * \fn void CallChannel::contentRemoved(const Tp::CallContentPtr &content, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when a media content is removed from this channel.
 *
 * \param content The media content that was removed.
 * \param reason The reason for this removal.
 * \sa contents(), contentsForType()
 */

/**
 * \fn void CallChannel::localHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);
 *
 * This signal is emitted when the local hold state of this channel changes.
 *
 * \param state The new local hold state of this channel.
 * \param reason The reason why the change occurred.
 * \sa localHoldState(), localHoldStateReason()
 */

} // Tp
