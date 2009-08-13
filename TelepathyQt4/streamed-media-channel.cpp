/* StreamedMedia channel client-side proxy
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/StreamedMediaChannel>

#include "TelepathyQt4/_gen/streamed-media-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingVoidMethodCall>

#include <QHash>

namespace Tp
{

struct PendingMediaStreams::Private
{
    Private(PendingMediaStreams *parent, const StreamedMediaChannelPtr &channel)
        : parent(parent), channel(channel), streamsReady(0)
    {
    }

    PendingMediaStreams *parent;
    WeakPtr<StreamedMediaChannel> channel;
    MediaStreams streams;
    uint streamsReady;
};

PendingMediaStreams::PendingMediaStreams(const StreamedMediaChannelPtr &channel,
        ContactPtr contact,
        QList<MediaStreamType> types)
    : PendingOperation(channel.data()),
      mPriv(new Private(this, channel))
{
    UIntList l;
    foreach (MediaStreamType type, types) {
        l << type;
    }
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
            channel->streamedMediaInterface()->RequestStreams(
                contact->handle()[0], l), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotStreams(QDBusPendingCallWatcher*)));
}

PendingMediaStreams::~PendingMediaStreams()
{
    delete mPriv;
}

MediaStreams PendingMediaStreams::streams() const
{
    if (!isFinished()) {
        warning() << "PendingMediaStreams::streams called before finished, "
            "returning empty list";
        return MediaStreams();
    } else if (!isValid()) {
        warning() << "PendingMediaStreams::streams called when not valid, "
            "returning empty list";
        return MediaStreams();
    }

    return mPriv->streams;
}

void PendingMediaStreams::gotStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::RequestStreams()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();
        setFinishedWithError(reply.error());
        return;
    }

    debug() << "Got reply to StreamedMedia::RequestStreams()";

    MediaStreamInfoList list = reply.value();
    MediaStreamPtr stream;
    StreamedMediaChannelPtr channel(mPriv->channel);
    foreach (const MediaStreamInfo &streamInfo, list) {
        stream = channel->lookupStreamById(streamInfo.identifier);
        if (!stream) {
            stream = MediaStreamPtr(
                    new MediaStream(channel,
                        streamInfo.identifier,
                        streamInfo.contact,
                        (MediaStreamType) streamInfo.type,
                        (MediaStreamState) streamInfo.state,
                        (MediaStreamDirection) streamInfo.direction,
                        (MediaStreamPendingSend) streamInfo.pendingSendFlags));
            channel->addStream(stream);
        } else {
            channel->onStreamDirectionChanged(streamInfo.identifier,
                    streamInfo.direction, streamInfo.pendingSendFlags);
            channel->onStreamStateChanged(streamInfo.identifier,
                    streamInfo.state);
        }
        mPriv->streams.append(stream);
        connect(channel.data(),
                SIGNAL(streamRemoved(Tp::MediaStreamPtr)),
                SLOT(onStreamRemoved(Tp::MediaStreamPtr)));
        connect(stream->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onStreamReady(Tp::PendingOperation*)));
    }

    watcher->deleteLater();
}

void PendingMediaStreams::onStreamRemoved(MediaStreamPtr stream)
{
    if (isFinished()) {
        return;
    }

    if (mPriv->streams.contains(stream)) {
        // the stream was removed before becoming ready
        setFinishedWithError(TELEPATHY_ERROR_CANCELLED, "Stream removed before ready");
    }
}

void PendingMediaStreams::onStreamReady(PendingOperation *op)
{
    if (isFinished()) {
        return;
    }

    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    mPriv->streamsReady++;
    if (mPriv->streamsReady == (uint) mPriv->streams.size()) {
        setFinished();
    }
}

struct MediaStream::Private
{
    Private(MediaStream *parent, const StreamedMediaChannelPtr &channel, uint id,
            uint contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend);

    static void introspectContact(Private *self);

    MediaStream *parent;
    ReadinessHelper *readinessHelper;
    WeakPtr<StreamedMediaChannel> channel;
    uint id;
    uint contactHandle;
    ContactPtr contact;
    MediaStreamType type;
    MediaStreamState state;
    MediaStreamDirection direction;
    MediaStreamPendingSend pendingSend;
};

MediaStream::Private::Private(MediaStream *parent,
        const StreamedMediaChannelPtr &channel, uint id,
        uint contactHandle, MediaStreamType type,
        MediaStreamState state, MediaStreamDirection direction,
        MediaStreamPendingSend pendingSend)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      channel(channel),
      id(id),
      contactHandle(contactHandle),
      type(type),
      state(state),
      direction(direction),
      pendingSend(pendingSend)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableContact(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectContact,
        this);
    introspectables[FeatureContact] = introspectableContact;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureContact);
}

void MediaStream::Private::introspectContact(MediaStream::Private *self)
{
    if (self->contactHandle == 0) {
        self->readinessHelper->setIntrospectCompleted(FeatureContact, true);
        return;
    }

    StreamedMediaChannelPtr chan(self->channel);
    ContactManager *contactManager = chan->connection()->contactManager();
    self->parent->connect(
            contactManager->contactsForHandles(UIntList() << self->contactHandle),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(gotContact(Tp::PendingOperation *)));
}

const Feature MediaStream::FeatureContact = Feature(MediaStream::staticMetaObject.className(), 0);

MediaStream::MediaStream(const StreamedMediaChannelPtr &channel, uint id,
        uint contactHandle, MediaStreamType type,
        MediaStreamState state, MediaStreamDirection direction,
        MediaStreamPendingSend pendingSend)
    : QObject(),
      ReadyObject(this, FeatureContact),
      mPriv(new Private(this, channel, id, contactHandle, type,
                  state, direction, pendingSend))
{
}

MediaStream::~MediaStream()
{
    delete mPriv;
}

StreamedMediaChannelPtr MediaStream::channel() const
{
    return StreamedMediaChannelPtr(mPriv->channel);
}

/**
 * Return the stream id.
 *
 * \return An integer representing the stream id.
 */
uint MediaStream::id() const
{
    return mPriv->id;
}

/**
 * Return the contact who the stream is with.
 *
 * \return The contact who the stream is with.
 */
ContactPtr MediaStream::contact() const
{
    return mPriv->contact;
}

/**
 * Return the stream state.
 *
 * \return The stream state.
 */
MediaStreamState MediaStream::state() const
{
    return mPriv->state;
}

/**
 * Return the stream type.
 *
 * \return The stream type.
 */
MediaStreamType MediaStream::type() const
{
    return mPriv->type;
}

/**
 * Return whether media is being sent on this stream.
 *
 * \return A boolean indicating whether media is being sent on this stream.
 */
bool MediaStream::sending() const
{
    return mPriv->direction & MediaStreamDirectionSend;
}

/**
 * Return whether media is being received on this stream.
 *
 * \return A boolean indicating whether media is being received on this stream.
 */
bool MediaStream::receiving() const
{
    return mPriv->direction & MediaStreamDirectionReceive;
}

/**
 * Return whether the local user has been asked to send media by the remote user.
 *
 * \return A boolean indicating whether the local user has been asked to
 *         send media by the remote user.
 */
bool MediaStream::localSendingRequested() const
{
    return mPriv->pendingSend & MediaStreamPendingLocalSend;
}

/**
 * Return whether the remote user has been asked to send media by the local user.
 *
 * \return A boolean indicating whether the remote user has been asked to
 *         send media by the local user.
 */
bool MediaStream::remoteSendingRequested() const
{
    return mPriv->pendingSend & MediaStreamPendingRemoteSend;
}

/**
 * Return the stream direction.
 *
 * \return The stream direction.
 */
MediaStreamDirection MediaStream::direction() const
{
    return mPriv->direction;
}

/**
 * Return the stream pending send flags.
 *
 * \return The stream pending send flags.
 */
MediaStreamPendingSend MediaStream::pendingSend() const
{
    return mPriv->pendingSend;
}

/**
 * Request a change in the direction of this stream. In particular, this
 * might be useful to stop sending media of a particular type, or inform the
 * peer that you are no longer using media that is being sent to you.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *MediaStream::requestDirection(
        MediaStreamDirection direction)
{
    StreamedMediaChannelPtr chan(mPriv->channel);
    return new PendingVoidMethodCall(this,
            chan->streamedMediaInterface()->RequestStreamDirection(mPriv->id, direction));
}

/**
 * Request a change in the direction of this stream. In particular, this
 * might be useful to stop sending media of a particular type, or inform the
 * peer that you are no longer using media that is being sent to you.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa requestDirection(Tp::MediaStreamDirection direction)
 */
PendingOperation *MediaStream::requestDirection(bool send, bool receive)
{
    uint dir = MediaStreamDirectionNone;
    if (send) {
        dir |= MediaStreamDirectionSend;
    }
    if (receive) {
        dir |= MediaStreamDirectionReceive;
    }
    return requestDirection((MediaStreamDirection) dir);
}

uint MediaStream::contactHandle() const
{
    return mPriv->contactHandle;
}

void MediaStream::setDirection(MediaStreamDirection direction,
        MediaStreamPendingSend pendingSend)
{
    mPriv->direction = direction;
    mPriv->pendingSend = pendingSend;
}

void MediaStream::setState(MediaStreamState state)
{
    mPriv->state = state;
}

void MediaStream::gotContact(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    Q_ASSERT(pc->isForHandles());

    if (pc->isError()) {
        warning().nospace() << "Gathering media stream contact failed: "
            << pc->errorName() << ": " << pc->errorMessage();
    }

    QList<ContactPtr> contacts = pc->contacts();
    UIntList invalidHandles = pc->invalidHandles();
    if (contacts.size()) {
        Q_ASSERT(contacts.size() == 1);
        Q_ASSERT(invalidHandles.size() == 0);
        mPriv->contact = contacts.first();
    } else {
        Q_ASSERT(invalidHandles.size() == 1);
        warning().nospace() << "Error retrieving media stream contact (invalid handle)";
    }

    mPriv->readinessHelper->setIntrospectCompleted(FeatureContact, true);
}

struct StreamedMediaChannel::Private
{
    Private(StreamedMediaChannel *parent);
    ~Private();

    static void introspectStreams(Private *self);
    static void introspectLocalHoldState(Private *self);

    // Public object
    StreamedMediaChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection

    QHash<uint, MediaStreamPtr> incompleteStreams;
    QHash<uint, MediaStreamPtr> streams;

    LocalHoldState localHoldState;
    LocalHoldStateReason localHoldStateReason;
};

StreamedMediaChannel::Private::Private(StreamedMediaChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      localHoldState(LocalHoldStateUnheld),
      localHoldStateReason(LocalHoldStateReasonNone)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableStreams(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectStreams,
        this);
    introspectables[FeatureStreams] = introspectableStreams;

    ReadinessHelper::Introspectable introspectableLocalHoldState(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_HOLD),                // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectLocalHoldState,
        this);
    introspectables[FeatureLocalHoldState] = introspectableLocalHoldState;

    readinessHelper->addIntrospectables(introspectables);
}

StreamedMediaChannel::Private::~Private()
{
}

void StreamedMediaChannel::Private::introspectStreams(StreamedMediaChannel::Private *self)
{
    StreamedMediaChannel *parent = self->parent;
    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        parent->streamedMediaInterface();

    parent->connect(streamedMediaInterface,
            SIGNAL(StreamAdded(uint, uint, uint)),
            SLOT(onStreamAdded(uint, uint, uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamRemoved(uint)),
            SLOT(onStreamRemoved(uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamDirectionChanged(uint, uint, uint)),
            SLOT(onStreamDirectionChanged(uint, uint, uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamStateChanged(uint, uint)),
            SLOT(onStreamStateChanged(uint, uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamError(uint, uint, const QString &)),
            SLOT(onStreamError(uint, uint, const QString &)));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            streamedMediaInterface->ListStreams(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotStreams(QDBusPendingCallWatcher *)));
}

void StreamedMediaChannel::Private::introspectLocalHoldState(StreamedMediaChannel::Private *self)
{
    StreamedMediaChannel *parent = self->parent;
    Client::ChannelInterfaceHoldInterface *holdInterface =
        parent->holdInterface();

    parent->connect(holdInterface,
            SIGNAL(HoldStateChanged(uint, uint)),
            SLOT(onLocalHoldStateChanged(uint, uint)));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            holdInterface->GetHoldState(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotLocalHoldState(QDBusPendingCallWatcher *)));
}


/**
 * \class StreamedMediaChannel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/streamed-media-channel.h> <TelepathyQt4/StreamedMediaChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the
 * StreamedMedia channel type.
 */

const Feature StreamedMediaChannel::FeatureStreams = Feature(StreamedMediaChannel::staticMetaObject.className(), 0);
const Feature StreamedMediaChannel::FeatureLocalHoldState = Feature(StreamedMediaChannel::staticMetaObject.className(), 1);

StreamedMediaChannelPtr StreamedMediaChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return StreamedMediaChannelPtr(new StreamedMediaChannel(connection,
                objectPath, immutableProperties));
}

/**
 * Creates a StreamedMediaChannel associated with the given object on the same
 * service as the given connection.
 *
 * \param connection  Connection owning this StreamedMediaChannel, and
 *                    specifying the service.
 * \param objectPath  Path to the object on the service.
 * \param immutableProperties  The immutable properties of the channel, as
 *                             signalled by NewChannels or returned by
 *                             CreateChannel or EnsureChannel
 */
StreamedMediaChannel::StreamedMediaChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : Channel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
StreamedMediaChannel::~StreamedMediaChannel()
{
    delete mPriv;
}

/**
 * Return a list of streams in this channel. This list is empty unless
 * the FeatureStreams Feature has been enabled.
 *
 * Streams are added to the list when they are received; the streamAdded signal
 * is emitted.
 *
 * \return The streams in this channel.
 */
MediaStreams StreamedMediaChannel::streams() const
{
    if (!isReady(FeatureStreams)) {
        warning() << "Trying to retrieve streams from streamed media channel, but "
                     "streams was not requested or the request did not finish yet. "
                     "Use becomeReady(FeatureStreams)";
        return MediaStreams();
    }

    return mPriv->streams.values();
}

MediaStreams StreamedMediaChannel::streamsForType(MediaStreamType type) const
{
    if (!isReady(FeatureStreams)) {
        warning() << "Trying to retrieve streams from streamed media channel, but "
                     "streams was not requested or the request did not finish yet. "
                     "Use becomeReady(FeatureStreams)";
        return MediaStreams();
    }

    QHash<uint, MediaStreamPtr> allStreams = mPriv->streams;
    MediaStreams streams;
    foreach (const MediaStreamPtr &stream, allStreams) {
        if (stream->type() == type) {
            streams.append(stream);
        }
    }
    return streams;
}

bool StreamedMediaChannel::awaitingLocalAnswer() const
{
    return groupSelfHandleIsLocalPending();
}

bool StreamedMediaChannel::awaitingRemoteAnswer() const
{
    return !groupRemotePendingContacts().isEmpty();
}

PendingOperation *StreamedMediaChannel::acceptCall()
{
    return groupAddSelfHandle();
}

/**
 * Remove the specified stream from this channel.
 *
 * \param stream Stream to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *StreamedMediaChannel::removeStream(const MediaStreamPtr &stream)
{
    return new PendingVoidMethodCall(this,
            streamedMediaInterface()->RemoveStreams(UIntList() << stream->id()));
}

/**
 * Remove the specified streams from this channel.
 *
 * \param streams List of streams to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *StreamedMediaChannel::removeStreams(const MediaStreams &streams)
{
    UIntList ids;
    foreach (const MediaStreamPtr &stream, streams) {
        ids << stream->id();
    }
    return new PendingVoidMethodCall(this,
            streamedMediaInterface()->RemoveStreams(ids));
}

PendingMediaStreams *StreamedMediaChannel::requestStream(
        const ContactPtr &contact,
        MediaStreamType type)
{
    return new PendingMediaStreams(StreamedMediaChannelPtr(this),
            contact,
            QList<MediaStreamType>() << type);
}

PendingMediaStreams *StreamedMediaChannel::requestStreams(
        const ContactPtr &contact,
        QList<MediaStreamType> types)
{
    return new PendingMediaStreams(StreamedMediaChannelPtr(this),
            contact, types);
}

/**
 * Check whether media streaming by the handler is required for this channel.
 *
 * For channels with the MediaSignalling interface, the main handler of the
 * channel is responsible for doing the actual streaming, for instance by
 * calling Tp::createFarsightChannel(channel) from TelepathyQt4-Farsight library
 * and using the telepathy-farsight API on the resulting TfChannel.
 *
 * \return \c true if required, \c false otherwise.
 */
bool StreamedMediaChannel::handlerStreamingRequired() const
{
    if (!isReady()) {
        warning() << "Trying to check if handler streaming is required, "
                     "but channel is not ready. Use becomeReady()";
    }

    return interfaces().contains(
            TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING);
}

/**
 * Return the channel local hold state.
 *
 * This method requires StreamedMediaChannel::FeatureHoldState to be enabled.
 *
 * \return The channel local hold state.
 * \sa requestLocalHold()
 */
LocalHoldState StreamedMediaChannel::localHoldState() const
{
    if (!isReady(FeatureLocalHoldState)) {
        warning() << "StreamedMediaChannel::localHoldState() used with FeatureLocalHoldState not ready";
    } else if (!interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::localHoldStateReason() used with no hold interface";
    }

    return mPriv->localHoldState;
}

/**
 * Return the channel local hold state reason.
 *
 * This method requires StreamedMediaChannel::FeatureLocalHoldState to be enabled.
 *
 * \return The channel local hold state reason.
 * \sa requestLocalHold()
 */
LocalHoldStateReason StreamedMediaChannel::localHoldStateReason() const
{
    if (!isReady(FeatureLocalHoldState)) {
        warning() << "StreamedMediaChannel::localHoldStateReason() used with FeatureLocalHoldState not ready";
    } else if (!interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::localHoldStateReason() used with no hold interface";
    }

    return mPriv->localHoldStateReason;
}

/**
 * Request that the channel be put on hold (be instructed not to send any media
 * streams to you) or be taken off hold.
 *
 * If the connection manager can immediately tell that the requested state
 * change could not possibly succeed, the resulting PendingOperation will fail
 * with error code %TELEPATHY_ERROR_NOT_AVAILABLE.
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
 * If the channel does not support the %TELEPATHY_INTERFACE_CHANNEL_INTERFACE_HOLD
 * interface, the PendingOperation will fail with error code
 * %TELEPATHY_ERROR_NOT_IMPLEMENTED.
 *
 * \param hold A boolean indicating whether or not the channel should be on hold 
 * \return A %PendingOperation, which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa localHoldState(), localHoldStateReason()
 */
PendingOperation *StreamedMediaChannel::requestLocalHold(bool hold)
{
    if (!interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::requestLocalHold() used with no hold interface";
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "StreamedMediaChannel does not support hold interface");
    }
    return new PendingVoidMethodCall(this, holdInterface()->RequestHold(hold));
}

void StreamedMediaChannel::gotStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::ListStreams()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams,
                false, reply.error());
        return;
    }

    debug() << "Got reply to StreamedMedia::ListStreams()";

    MediaStreamInfoList list = reply.value();
    if (list.size() > 0) {
        foreach (const MediaStreamInfo &streamInfo, list) {
            MediaStreamPtr stream = lookupStreamById(streamInfo.identifier);
            if (!stream) {
                addStream(MediaStreamPtr(
                            new MediaStream(StreamedMediaChannelPtr(this),
                                streamInfo.identifier,
                                streamInfo.contact,
                                (MediaStreamType) streamInfo.type,
                                (MediaStreamState) streamInfo.state,
                                (MediaStreamDirection) streamInfo.direction,
                                (MediaStreamPendingSend) streamInfo.pendingSendFlags)));
            } else {
                onStreamDirectionChanged(streamInfo.identifier,
                        streamInfo.direction, streamInfo.pendingSendFlags);
                onStreamStateChanged(streamInfo.identifier,
                        streamInfo.state);
            }
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
    }

    watcher->deleteLater();
}

void StreamedMediaChannel::onStreamReady(PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    MediaStreamPtr stream = MediaStreamPtr(qobject_cast<MediaStream*>(pr->object()));

    if (op->isError()) {
        mPriv->incompleteStreams.remove(stream->id());
        // let's not fail because a stream could not become ready
        if (!isReady(FeatureStreams) && mPriv->incompleteStreams.size() == 0) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
        }
        return;
    }

    // the stream was removed before become ready
    if (!mPriv->incompleteStreams.contains(stream->id())) {
        if (!isReady(FeatureStreams) && mPriv->incompleteStreams.size() == 0) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
        }
        return;
    }

    mPriv->incompleteStreams.remove(stream->id());
    mPriv->streams.insert(stream->id(), stream);

    if (!isReady(FeatureStreams) && mPriv->incompleteStreams.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
    }

    if (isReady(FeatureStreams)) {
        emit streamAdded(stream);
    }
}

void StreamedMediaChannel::onStreamAdded(uint streamId,
        uint contactHandle, uint streamType)
{
    if (lookupStreamById(streamId)) {
        debug() << "Received StreamedMediaChannel.StreamAdded for an existing "
            "stream, ignoring";
        return;
    }

    MediaStreamPtr stream = MediaStreamPtr(
            new MediaStream(StreamedMediaChannelPtr(this), streamId,
                contactHandle,
                (MediaStreamType) streamType,
                // TODO where to get this info from?
                MediaStreamStateDisconnected,
                MediaStreamDirectionNone,
                (MediaStreamPendingSend) 0));
    addStream(stream);
}

void StreamedMediaChannel::onStreamRemoved(uint streamId)
{
    debug() << "StreamedMediaChannel::onStreamRemoved: stream" <<
        streamId << "removed";

    MediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }
    bool incomplete = mPriv->incompleteStreams.contains(streamId);
    if (incomplete) {
        mPriv->incompleteStreams.remove(streamId);
    } else {
        mPriv->streams.remove(streamId);
    }

    // the stream was added/removed before become ready
    if (!isReady(FeatureStreams) &&
        mPriv->streams.size() == 0 &&
        mPriv->incompleteStreams.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
    }

    if (isReady(FeatureStreams) && !incomplete) {
        emit streamRemoved(stream);
    }
}

void StreamedMediaChannel::onStreamDirectionChanged(uint streamId,
        uint streamDirection, uint streamPendingFlags)
{
    debug() << "StreamedMediaChannel::onStreamDirectionChanged: stream" <<
        streamId << "direction changed to" << streamDirection;

    MediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }

    if ((uint) stream->direction() == streamDirection &&
        (uint) stream->pendingSend() == streamPendingFlags) {
        return;
    }

    stream->setDirection(
            (MediaStreamDirection) streamDirection,
            (MediaStreamPendingSend) streamPendingFlags);
    if (isReady(FeatureStreams) &&
        !mPriv->incompleteStreams.contains(stream->id())) {
        emit streamDirectionChanged(stream,
                (MediaStreamDirection) streamDirection,
                (MediaStreamPendingSend) streamPendingFlags);
    }
}

void StreamedMediaChannel::onStreamStateChanged(uint streamId,
        uint streamState)
{
    debug() << "StreamedMediaChannel::onStreamStateChanged: stream" <<
        streamId << "state changed to" << streamState;

    MediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }

    if ((uint) stream->state() == streamState) {
        return;
    }

    stream->setState((MediaStreamState) streamState);
    if (isReady(FeatureStreams) &&
        !mPriv->incompleteStreams.contains(stream->id())) {
        emit streamStateChanged(stream,
                (MediaStreamState) streamState);
    }
}

void StreamedMediaChannel::onStreamError(uint streamId,
        uint errorCode, const QString &errorMessage)
{
    debug() << "StreamedMediaChannel::onStreamError: stream" <<
        streamId << "error:" << errorCode << "-" << errorMessage;

    MediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }
    if (isReady(FeatureStreams) &&
        !mPriv->incompleteStreams.contains(stream->id())) {
        emit streamError(stream,
            (MediaStreamError) errorCode,
            errorMessage);
    }
}

void StreamedMediaChannel::gotLocalHoldState(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::Hold::GetHoldState()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();

        debug() << "Ignoring error getting hold state and assuming we're not "
            "on hold";
        onLocalHoldStateChanged(mPriv->localHoldState,
                mPriv->localHoldStateReason);
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to StreamedMedia::Hold::GetHoldState()";
    onLocalHoldStateChanged(reply.argumentAt<0>(), reply.argumentAt<1>());
    watcher->deleteLater();
}

void StreamedMediaChannel::onLocalHoldStateChanged(uint localHoldState,
        uint localHoldStateReason)
{
    mPriv->localHoldState = static_cast<LocalHoldState>(localHoldState);
    mPriv->localHoldStateReason = static_cast<LocalHoldStateReason>(localHoldStateReason);

    if (!isReady(FeatureLocalHoldState)) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureLocalHoldState, true);
    } else {
        emit localHoldStateChanged(mPriv->localHoldState,
                mPriv->localHoldStateReason);
    }
}

void StreamedMediaChannel::addStream(const MediaStreamPtr &stream)
{
    mPriv->incompleteStreams.insert(stream->id(), stream);
    connect(stream->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onStreamReady(Tp::PendingOperation*)));
}

MediaStreamPtr StreamedMediaChannel::lookupStreamById(uint streamId)
{
    if (mPriv->streams.contains(streamId)) {
        return mPriv->streams.value(streamId);
    } else if (mPriv->incompleteStreams.contains(streamId)) {
        return mPriv->incompleteStreams.value(streamId);
    }
    return MediaStreamPtr();
}

} // Tp
