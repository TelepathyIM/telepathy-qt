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
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>

namespace Tp
{

/* ====== PendingCallContent ====== */
struct TP_QT_NO_EXPORT PendingCallContent::Private
{
    Private(PendingCallContent *parent, const CallChannelPtr &channel)
        : parent(parent),
          channel(channel)
    {
    }

    PendingCallContent *parent;
    CallChannelPtr channel;
    CallContentPtr content;
};

PendingCallContent::PendingCallContent(const CallChannelPtr &channel,
        const QString &name, MediaStreamType type, MediaStreamDirection direction)
    : PendingOperation(channel),
      mPriv(new Private(this, channel))
{
    Client::ChannelTypeCallInterface *callInterface =
        channel->interface<Client::ChannelTypeCallInterface>();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                callInterface->AddContent(name, type, direction), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotContent(QDBusPendingCallWatcher*)));
}

PendingCallContent::~PendingCallContent()
{
    delete mPriv;
}

CallContentPtr PendingCallContent::content() const
{
    if (!isFinished() || !isValid()) {
        return CallContentPtr();
    }

    return mPriv->content;
}

void PendingCallContent::gotContent(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Call::AddContent failed with " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    QDBusObjectPath contentPath = reply.value();
    CallChannelPtr channel(mPriv->channel);
    CallContentPtr content = channel->lookupContent(contentPath);
    if (!content) {
        content = channel->addContent(contentPath);
    }

    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContentReady(Tp::PendingOperation*)));
    connect(channel.data(),
            SIGNAL(contentRemoved(Tp::CallContentPtr)),
            SLOT(onContentRemoved(Tp::CallContentPtr)));

    mPriv->content = content;

    watcher->deleteLater();
}

void PendingCallContent::onContentReady(PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    setFinished();
}

void PendingCallContent::onContentRemoved(const CallContentPtr &content)
{
    if (isFinished()) {
        return;
    }

    if (mPriv->content == content) {
        // the content was removed before becoming ready
        setFinishedWithError(TP_QT_ERROR_CANCELLED,
                QLatin1String("Content removed before ready"));
    }
}


/* ====== CallChannel ====== */
struct TP_QT_NO_EXPORT CallChannel::Private
{
    Private(CallChannel *parent);
    ~Private();

    static void introspectContents(Private *self);
    static void introspectLocalHoldState(Private *self);

    CallContentPtr addContent(const QDBusObjectPath &contentPath);
    CallContentPtr lookupContent(const QDBusObjectPath &contentPath) const;

    // Public object
    CallChannel *parent;

    // Mandatory proxies
    Client::ChannelTypeCallInterface *callInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint state;
    uint flags;
    CallStateReason stateReason;
    QVariantMap stateDetails;
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

CallChannel::Private::Private(CallChannel *parent)
    : parent(parent),
      callInterface(parent->interface<Client::ChannelTypeCallInterface>()),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
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

    ReadinessHelper::Introspectable introspectableContents(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList(),                                                             // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectContents,
        this);
    introspectables[FeatureContents] = introspectableContents;

    ReadinessHelper::Introspectable introspectableLocalHoldState(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_HOLD,                      // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectLocalHoldState,
        this);
    introspectables[FeatureLocalHoldState] = introspectableLocalHoldState;

    readinessHelper->addIntrospectables(introspectables);
}

CallChannel::Private::~Private()
{
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
    parent->connect(self->callInterface,
            SIGNAL(CallStateChanged(uint,uint,Tp::CallStateReason,QVariantMap)),
            SLOT(onCallStateChanged(uint,uint,Tp::CallStateReason,QVariantMap)));
    // TODO handle CallMembersChanged

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(
                    QLatin1String(TP_QT_IFACE_CHANNEL_TYPE_CALL)),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
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

/**
 * \class CallChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/call-channel.h <TelepathyQt/CallChannel>
 *
 * \brief The CallChannel class provides an object representing a
 * Telepathy channel of type Call.
 */

/**
 * Feature used in order to access content specific methods.
 *
 * See media content specific methods' documentation for more details.
 */
const Feature CallChannel::FeatureContents = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to access local hold state info.
 *
 * See local hold state specific methods' documentation for more details.
 */
const Feature CallChannel::FeatureLocalHoldState = Feature(QLatin1String(CallChannel::staticMetaObject.className()), 1);

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
 * Return the current state of this call.
 *
 * \return The current state of this call.
 * \sa stateChanged()
 */
CallState CallChannel::state() const
{
    return (CallState) mPriv->state;
}

/**
 * Return the flags representing the status of this call as a whole, providing more specific
 * information than state().
 *
 * \return The flags representing the status of this call.
 * \sa stateChanged()
 */
CallFlags CallChannel::flags() const
{
    return (CallFlags) mPriv->flags;
}

/**
 * Return the reason for the last change to the state() and/or flags().
 *
 * \return The reason for the last change to the state() and/or flags().
 * \sa stateChanged()
 */
CallStateReason CallChannel::stateReason() const
{
    return mPriv->stateReason;
}

/**
 * Return optional extensible details for the state(), flags() and/or stateReason().
 *
 * \return The optional extensible details for the state(), flags() and/or stateReason().
 * \sa stateChanged()
 */
QVariantMap CallChannel::stateDetails() const
{
    return mPriv->stateDetails;
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
 * Return whether an audio content has already been requested.
 *
 * \return \c true if an audio content has already been requested, \c false otherwise.
 */
bool CallChannel::hasInitialAudio() const
{
    return mPriv->initialAudio;
}

/**
 * Return whether a video content has already been requested.
 *
 * \return \c true if an video content has already been requested, \c false otherwise.
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
 * Return whether a stream of a different content type can be added after the Channel has
 * been requested.
 *
 * \return \c true if a stream of different content type can be added after the Channel has been
 *         requested, \c false otherwise.
 */
bool CallChannel::hasMutableContents() const
{
    return mPriv->mutableContents;
}

/**
 * Indicate that the local user has been alerted about the incoming call.
 */
PendingOperation* CallChannel::setRinging()
{
    return new PendingVoid(mPriv->callInterface->SetRinging(), CallChannelPtr(this));
}

/**
 * Notifies the CM that the local user is already in a call, so this
 * call has been put in a call-waiting style queue.
 */
PendingOperation* CallChannel::setQueued()
{
    return new PendingVoid(mPriv->callInterface->SetQueued(), CallChannelPtr(this));
}

/**
 * Accept an incoming call.
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
 * \deprecated Use hangupCall() instead.
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
 * \sa contentAdded(), contentRemoved(), contentsForType(), requestContent()
 */
CallContents CallChannel::contents() const
{
    return mPriv->contents;
}

/**
 * Return a list of media contents in this channel for the given type \a type.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \param type The interested type.
 * \return A list of media contents in this channel for the given type \a type.
 * \sa contentAdded(), contentRemoved(), contents(), requestContent()
 */
CallContents CallChannel::contentsForType(MediaStreamType type) const
{
    CallContents contents;
    foreach (const CallContentPtr &content, mPriv->contents) {
        if (content->type() == type) {
            contents.append(content);
        }
    }
    return contents;
}

/**
 * Request that media content be established to exchange the given type \a type
 * of media.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \return A PendingCallContent which will emit PendingCallContent::finished
 *         when the call has finished.
 * \sa contentAdded(), contents(), contentsForType()
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
 * \return The channel local hold state.
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
 * \return A %PendingOperation, which will emit PendingOperation::finished
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

void CallChannel::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Properties::GetAll(Call) failed with " <<
            reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to Properties::GetAll(Call)";

    QVariantMap props = reply.value();

    // TODO bind CallMembers
    mPriv->state = qdbus_cast<uint>(props[QLatin1String("CallState")]);
    mPriv->flags = qdbus_cast<uint>(props[QLatin1String("CallFlags")]);
    // TODO Add high-level classes for CallStateReason/Details
    mPriv->stateReason = qdbus_cast<CallStateReason>(props[QLatin1String("CallStateReason")]);
    mPriv->stateDetails = qdbus_cast<QVariantMap>(props[QLatin1String("CallStateDetails")]);
    mPriv->hardwareStreaming = qdbus_cast<bool>(props[QLatin1String("HardwareStreaming")]);
    mPriv->initialTransportType = qdbus_cast<uint>(props[QLatin1String("InitialTransport")]);
    mPriv->initialAudio = qdbus_cast<bool>(props[QLatin1String("InitialAudio")]);
    mPriv->initialVideo = qdbus_cast<bool>(props[QLatin1String("InitialVideo")]);
    mPriv->initialAudioName = qdbus_cast<QString>(props[QLatin1String("InitialAudioName")]);
    mPriv->initialVideoName = qdbus_cast<QString>(props[QLatin1String("InitialVideoName")]);
    mPriv->mutableContents = qdbus_cast<bool>(props[QLatin1String("MutableContents")]);

    ObjectPathList contentsPaths = qdbus_cast<ObjectPathList>(props[QLatin1String("Contents")]);
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

    watcher->deleteLater();
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
        emit contentRemoved(content);
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

void CallChannel::onCallStateChanged(uint state, uint flags,
        const CallStateReason &stateReason, const QVariantMap &stateDetails)
{
    if (mPriv->state == state && mPriv->flags == flags && mPriv->stateReason == stateReason &&
        mPriv->stateDetails == stateDetails) {
        // nothing changed
        return;
    }

    mPriv->state = state;
    mPriv->flags = flags;
    mPriv->stateReason = stateReason;
    mPriv->stateDetails = stateDetails;
    emit stateChanged((CallState) state);
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
 * \fn void CallChannel::contentAdded(const Tp::CallContentPtr &content);
 *
 * This signal is emitted when a media content is added to this channel.
 *
 * \param content The media content that was added.
 * \sa contents(), contentsForType()
 */

/**
 * \fn void CallChannel::contentRemoved(const Tp::CallContentPtr &content);
 *
 * This signal is emitted when a media content is removed from this channel.
 *
 * \param content The media content that was removed.
 * \sa contents(), contentsForType()
 */

/**
 * \fn void CallChannel::stateChanged(Tp::CallState &state);
 *
 * This signal is emitted when the value of state(), flags(), stateReason() or stateDetails()
 * changes.
 *
 * \param state The new state.
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
