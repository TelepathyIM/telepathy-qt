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

#include <TelepathyQt4/Client/StreamedMediaChannel>

#include "TelepathyQt4/Client/_gen/streamed-media-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/PendingVoidMethodCall>

#include <QHash>

namespace Telepathy
{
namespace Client
{

struct PendingMediaStreams::Private
{
    Private(PendingMediaStreams *parent, StreamedMediaChannel *channel)
        : parent(parent), channel(channel), streamsReady(0)
    {
    }

    PendingMediaStreams *parent;
    StreamedMediaChannel *channel;
    MediaStreams streams;
    uint streamsReady;
};

PendingMediaStreams::PendingMediaStreams(StreamedMediaChannel *channel,
        ContactPtr contact,
        QList<Telepathy::MediaStreamType> types,
        QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(this, channel))
{
    Telepathy::UIntList l;
    foreach (Telepathy::MediaStreamType type, types) {
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
    QDBusPendingReply<Telepathy::MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::RequestStreams()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();
        setFinishedWithError(reply.error());
        return;
    }

    debug() << "Got reply to StreamedMedia::RequestStreams()";

    Telepathy::MediaStreamInfoList list = reply.value();
    MediaStreamPtr stream;
    foreach (const Telepathy::MediaStreamInfo &streamInfo, list) {
        stream = mPriv->channel->lookupStreamById(streamInfo.identifier);
        if (!stream) {
            stream = MediaStreamPtr(
                    new MediaStream(mPriv->channel,
                        streamInfo.identifier,
                        streamInfo.contact,
                        (Telepathy::MediaStreamType) streamInfo.type,
                        (Telepathy::MediaStreamState) streamInfo.state,
                        (Telepathy::MediaStreamDirection) streamInfo.direction,
                        (Telepathy::MediaStreamPendingSend) streamInfo.pendingSendFlags));
            mPriv->channel->addStream(stream);
        }
        mPriv->streams.append(stream);
        connect(mPriv->channel,
                SIGNAL(streamRemoved(Telepathy::Client::MediaStreamPtr)),
                SLOT(onStreamRemoved(Telepathy::Client::MediaStreamPtr)));
        connect(stream->becomeReady(),
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(onStreamReady(Telepathy::Client::PendingOperation*)));
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
    Private(MediaStream *parent, StreamedMediaChannel *channel, uint id,
            uint contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend);

    static void introspectContact(Private *self);

    MediaStream *parent;
    ReadinessHelper *readinessHelper;
    StreamedMediaChannel *channel;
    uint id;
    uint contactHandle;
    ContactPtr contact;
    MediaStreamType type;
    MediaStreamState state;
    MediaStreamDirection direction;
    MediaStreamPendingSend pendingSend;
};

MediaStream::Private::Private(MediaStream *parent,
        StreamedMediaChannel *channel, uint id,
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
    ContactManager *contactManager = self->channel->connection()->contactManager();
    self->parent->connect(
            contactManager->contactsForHandles(UIntList() << self->contactHandle),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(gotContact(Telepathy::Client::PendingOperation *)));
}

const Feature MediaStream::FeatureContact = Feature(MediaStream::staticMetaObject.className(), 0);

MediaStream::MediaStream(StreamedMediaChannel *channel, uint id,
        uint contactHandle, MediaStreamType type,
        MediaStreamState state, MediaStreamDirection direction,
        MediaStreamPendingSend pendingSend)
    : QObject(),
      ReadyObject(this, 0, FeatureContact),
      mPriv(new Private(this, channel, id, contactHandle, type,
                  state, direction, pendingSend))
{
}

MediaStream::~MediaStream()
{
    delete mPriv;
}

StreamedMediaChannel *MediaStream::channel() const
{
    return mPriv->channel;
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
Telepathy::MediaStreamState MediaStream::state() const
{
    return mPriv->state;
}

/**
 * Return the stream type.
 *
 * \return The stream type.
 */
Telepathy::MediaStreamType MediaStream::type() const
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
    return mPriv->direction & Telepathy::MediaStreamDirectionSend;
}

/**
 * Return whether media is being received on this stream.
 *
 * \return A boolean indicating whether media is being received on this stream.
 */
bool MediaStream::receiving() const
{
    return mPriv->direction & Telepathy::MediaStreamDirectionReceive;
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
Telepathy::MediaStreamDirection MediaStream::direction() const
{
    return mPriv->direction;
}

/**
 * Return the stream pending send flags.
 *
 * \return The stream pending send flags.
 */
Telepathy::MediaStreamPendingSend MediaStream::pendingSend() const
{
    return mPriv->pendingSend;
}

/**
 * Request this stream to be removed.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *MediaStream::remove()
{
    return mPriv->channel->removeStreams(Telepathy::UIntList() << mPriv->id);
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
        Telepathy::MediaStreamDirection direction)
{
    return new PendingVoidMethodCall(this,
            mPriv->channel->streamedMediaInterface()->RequestStreamDirection(mPriv->id, direction));
}

/**
 * Request a change in the direction of this stream. In particular, this
 * might be useful to stop sending media of a particular type, or inform the
 * peer that you are no longer using media that is being sent to you.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa requestDirection(Telepathy::MediaStreamDirection direction)
 */
PendingOperation *MediaStream::requestDirection(bool send, bool receive)
{
    uint dir = Telepathy::MediaStreamDirectionNone;
    if (send) {
        dir |= Telepathy::MediaStreamDirectionSend;
    }
    if (receive) {
        dir |= Telepathy::MediaStreamDirectionReceive;
    }
    return requestDirection((Telepathy::MediaStreamDirection) dir);
}

uint MediaStream::contactHandle() const
{
    return mPriv->contactHandle;
}

void MediaStream::setDirection(Telepathy::MediaStreamDirection direction,
        Telepathy::MediaStreamPendingSend pendingSend)
{
    mPriv->direction = direction;
    mPriv->pendingSend = pendingSend;
}

void MediaStream::setState(Telepathy::MediaStreamState state)
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

    // Public object
    StreamedMediaChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection

    QHash<uint, MediaStreamPtr> incompleteStreams;
    QHash<uint, MediaStreamPtr> streams;
};

StreamedMediaChannel::Private::Private(StreamedMediaChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper())
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableStreams(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectStreams,
        this);
    introspectables[FeatureStreams] = introspectableStreams;

    readinessHelper->addIntrospectables(introspectables);
}

StreamedMediaChannel::Private::~Private()
{
}

void StreamedMediaChannel::Private::introspectStreams(StreamedMediaChannel::Private *self)
{
    StreamedMediaChannel *parent = self->parent;
    ChannelTypeStreamedMediaInterface *streamedMediaInterface =
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

/**
 * \class StreamedMediaChannel
 * \ingroup clientchannel
 * \headerfile <TelepathyQt4/Client/streamed-media-channel.h> <TelepathyQt4/Client/StreamedMediaChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the
 * StreamedMedia channel type.
 *
 * This subclass of Channel will eventually provide a high-level API for the
 * StreamedMedia interface. Until then, it's just a Channel.
 */

const Feature StreamedMediaChannel::FeatureStreams = Feature(StreamedMediaChannel::staticMetaObject.className(), 0);

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
 * \param parent      Passed to the parent class constructor.
 */
StreamedMediaChannel::StreamedMediaChannel(Connection *connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        QObject *parent)
    : Channel(connection, objectPath, immutableProperties, parent),
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

MediaStreams StreamedMediaChannel::streamsForType(Telepathy::MediaStreamType type) const
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
 * Remove the specified streams from this channel.
 *
 * \param streams List of streams to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *StreamedMediaChannel::removeStreams(MediaStreams streams)
{
    Telepathy::UIntList ids;
    foreach (const MediaStreamPtr &stream, streams) {
        ids << stream->id();
    }
    return removeStreams(ids);
}

/**
 * Remove the specified streams from this channel.
 *
 * \param streams List of ids corresponding to the streams to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *StreamedMediaChannel::removeStreams(const Telepathy::UIntList &ids)
{
    return new PendingVoidMethodCall(this,
            streamedMediaInterface()->RemoveStreams(ids));
}

PendingMediaStreams *StreamedMediaChannel::requestStream(
        const ContactPtr &contact,
        Telepathy::MediaStreamType type)
{
    return new PendingMediaStreams(this, contact,
            QList<Telepathy::MediaStreamType>() << type, this);
}

PendingMediaStreams *StreamedMediaChannel::requestStreams(
        const ContactPtr &contact,
        QList<Telepathy::MediaStreamType> types)
{
    return new PendingMediaStreams(this, contact, types, this);
}

void StreamedMediaChannel::gotStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<Telepathy::MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::ListStreams()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams,
                false, reply.error());
        return;
    }

    debug() << "Got reply to StreamedMedia::ListStreams()";

    Telepathy::MediaStreamInfoList list = reply.value();
    if (list.size() > 0) {
        foreach (const Telepathy::MediaStreamInfo &streamInfo, list) {
            if (!lookupStreamById(streamInfo.identifier)) {
                addStream(MediaStreamPtr(
                            new MediaStream(this,
                                streamInfo.identifier,
                                streamInfo.contact,
                                (Telepathy::MediaStreamType) streamInfo.type,
                                (Telepathy::MediaStreamState) streamInfo.state,
                                (Telepathy::MediaStreamDirection) streamInfo.direction,
                                (Telepathy::MediaStreamPendingSend) streamInfo.pendingSendFlags)));
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
            new MediaStream(this, streamId,
                contactHandle,
                (Telepathy::MediaStreamType) streamType,
                // TODO where to get this info from?
                Telepathy::MediaStreamStateDisconnected,
                Telepathy::MediaStreamDirectionNone,
                (Telepathy::MediaStreamPendingSend) 0));
    addStream(stream);
}

void StreamedMediaChannel::onStreamRemoved(uint streamId)
{
    debug() << "StreamedMediaChannel::onStreamRemoved: stream" <<
        streamId << "removed";

    MediaStreamPtr stream = lookupStreamById(streamId);
    Q_ASSERT(stream);
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
    Q_ASSERT(stream);
    stream->setDirection(
            (Telepathy::MediaStreamDirection) streamDirection,
            (Telepathy::MediaStreamPendingSend) streamPendingFlags);
    if (isReady(FeatureStreams) &&
        !mPriv->incompleteStreams.contains(stream->id())) {
        emit streamDirectionChanged(stream,
                (Telepathy::MediaStreamDirection) streamDirection,
                (Telepathy::MediaStreamPendingSend) streamPendingFlags);
    }
}

void StreamedMediaChannel::onStreamStateChanged(uint streamId,
        uint streamState)
{
    debug() << "StreamedMediaChannel::onStreamStateChanged: stream" <<
        streamId << "state changed to" << streamState;

    MediaStreamPtr stream = lookupStreamById(streamId);
    Q_ASSERT(stream);
    stream->setState((Telepathy::MediaStreamState) streamState);
    if (isReady(FeatureStreams) &&
        !mPriv->incompleteStreams.contains(stream->id())) {
        emit streamStateChanged(stream,
                (Telepathy::MediaStreamState) streamState);
    }
}

void StreamedMediaChannel::onStreamError(uint streamId,
        uint errorCode, const QString &errorMessage)
{
    debug() << "StreamedMediaChannel::onStreamError: stream" <<
        streamId << "error:" << errorCode << "-" << errorMessage;

    MediaStreamPtr stream = lookupStreamById(streamId);
    Q_ASSERT(stream);
    if (isReady(FeatureStreams) &&
        !mPriv->incompleteStreams.contains(stream->id())) {
        emit streamError(stream,
            (Telepathy::MediaStreamError) errorCode,
            errorMessage);
    }
}

void StreamedMediaChannel::addStream(const MediaStreamPtr &stream)
{
    mPriv->incompleteStreams.insert(stream->id(), stream);
    connect(stream->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onStreamReady(Telepathy::Client::PendingOperation*)));
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

} // Telepathy::Client
} // Telepathy
