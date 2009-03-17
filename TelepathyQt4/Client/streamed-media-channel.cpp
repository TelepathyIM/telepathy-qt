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
#include <TelepathyQt4/Client/PendingVoidMethodCall>

#include <QHash>

namespace Telepathy
{
namespace Client
{

struct PendingMediaStreams::Private
{
    Private(PendingMediaStreams *parent, StreamedMediaChannel *channel)
        : parent(parent), channel(channel)
    {
    }

    Private(PendingMediaStreams *parent, StreamedMediaChannel *channel,
            const MediaStreams &streams)
        : parent(parent), channel(channel), streams(streams)
    {
        getContacts();
    }

    void getContacts();

    PendingMediaStreams *parent;
    StreamedMediaChannel *channel;
    MediaStreams streams;
};

void PendingMediaStreams::Private::getContacts()
{
    QSet<uint> contactsRequired;
    foreach (const QSharedPointer<MediaStream> &stream, streams) {
        contactsRequired |= stream->contactHandle();
    }

    ContactManager *contactManager = channel->connection()->contactManager();
    parent->connect(contactManager->contactsForHandles(contactsRequired.toList()),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(gotContacts(Telepathy::Client::PendingOperation *)));
}

PendingMediaStreams::PendingMediaStreams(StreamedMediaChannel *channel,
        QSharedPointer<Telepathy::Client::Contact> contact,
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

PendingMediaStreams::PendingMediaStreams(StreamedMediaChannel *channel,
        const MediaStreams &streams,
        QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(this, channel, streams))
{
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
    QSharedPointer<MediaStream> stream;
    foreach (const Telepathy::MediaStreamInfo &streamInfo, list) {
        stream = QSharedPointer<MediaStream>(
                new MediaStream(mPriv->channel,
                    streamInfo.identifier,
                    streamInfo.contact,
                    (Telepathy::MediaStreamType) streamInfo.type,
                    (Telepathy::MediaStreamState) streamInfo.state,
                    (Telepathy::MediaStreamDirection) streamInfo.direction,
                    (Telepathy::MediaStreamPendingSend) streamInfo.pendingSendFlags));
        mPriv->streams.append(stream);
    }

    mPriv->getContacts();

    watcher->deleteLater();
}

void PendingMediaStreams::gotContacts(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    Q_ASSERT(pc->isForHandles());

    if (pc->isError()) {
        warning().nospace() << "Gathering contacts failed: "
            << pc->errorName() << ": " << pc->errorMessage();
    }

    QHash<uint, QSharedPointer<Contact> > contactsForHandles;
    foreach (const QSharedPointer<Contact> &contact, pc->contacts()) {
        contactsForHandles.insert(contact->handle()[0], contact);
    }

    foreach (uint handle, pc->invalidHandles()) {
        contactsForHandles.insert(handle, QSharedPointer<Contact>());
    }

    foreach (const QSharedPointer<MediaStream> &stream, mPriv->streams) {
        stream->setContact(contactsForHandles[stream->contactHandle()]);
        // make sure the channel has all streams even if StreamAdded was not
        // emitted
        mPriv->channel->addStream(stream);
    }

    setFinished();
}

struct MediaStream::Private
{
    Private(StreamedMediaChannel *channel, uint id,
            uint contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend)
        : channel(channel), id(id), contactHandle(contactHandle), type(type),
          state(state), direction(direction), pendingSend(pendingSend)
    {
    }

    StreamedMediaChannel *channel;
    uint id;
    uint contactHandle;
    QSharedPointer<Contact> contact;
    MediaStreamType type;
    MediaStreamState state;
    MediaStreamDirection direction;
    MediaStreamPendingSend pendingSend;
};

MediaStream::MediaStream(StreamedMediaChannel *channel, uint id,
        uint contactHandle, MediaStreamType type,
        MediaStreamState state, MediaStreamDirection direction,
        MediaStreamPendingSend pendingSend)
    : QObject(),
      mPriv(new Private(channel, id, contactHandle, type,
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
QSharedPointer<Contact> MediaStream::contact() const
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
PendingOperation *MediaStream::requestStreamDirection(
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
 * \sa requestStreamDirection(Telepathy::MediaStreamDirection direction)
 */
PendingOperation *MediaStream::requestStreamDirection(bool send, bool receive)
{
    uint dir = Telepathy::MediaStreamDirectionNone;
    if (send) {
        dir |= Telepathy::MediaStreamDirectionSend;
    }
    if (receive) {
        dir |= Telepathy::MediaStreamDirectionReceive;
    }
    return requestStreamDirection((Telepathy::MediaStreamDirection) dir);
}

uint MediaStream::contactHandle() const
{
    return mPriv->contactHandle;
}

void MediaStream::setContact(const QSharedPointer<Contact> &contact)
{
    Q_ASSERT(!mPriv->contact || mPriv->contact == contact);
    mPriv->contact = contact;
}

void MediaStream::setDirection(Telepathy::MediaStreamDirection direction,
        Telepathy::MediaStreamPendingSend pendingSend)
{
    mPriv->direction = direction;
    mPriv->pendingSend = pendingSend;
    emit directionChanged(this, direction, pendingSend);
}

void MediaStream::setState(Telepathy::MediaStreamState state)
{
    mPriv->state = state;
    emit stateChanged(this, state);
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
    bool initialStreamsReceived;

    QHash<uint, QSharedPointer<MediaStream> > streams;
};

StreamedMediaChannel::Private::Private(StreamedMediaChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      initialStreamsReceived(false)
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
    foreach (const QSharedPointer<MediaStream> &stream, streams) {
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
        QSharedPointer<Telepathy::Client::Contact> contact,
        Telepathy::MediaStreamType type)
{
    return new PendingMediaStreams(this, contact,
            QList<Telepathy::MediaStreamType>() << type, this);
}

PendingMediaStreams *StreamedMediaChannel::requestStreams(
        QSharedPointer<Telepathy::Client::Contact> contact,
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
    foreach (const Telepathy::MediaStreamInfo &streamInfo, list) {
        mPriv->streams.insert(streamInfo.identifier,
                QSharedPointer<MediaStream>(
                    new MediaStream(this,
                        streamInfo.identifier,
                        streamInfo.contact,
                        (Telepathy::MediaStreamType) streamInfo.type,
                        (Telepathy::MediaStreamState) streamInfo.state,
                        (Telepathy::MediaStreamDirection) streamInfo.direction,
                        (Telepathy::MediaStreamPendingSend) streamInfo.pendingSendFlags)));
    }
    PendingMediaStreams *pms = new PendingMediaStreams(this,
            mPriv->streams.values(), this);
    connect(pms,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onStreamsReady(Telepathy::Client::PendingOperation *)));

    watcher->deleteLater();
}

void StreamedMediaChannel::onStreamsReady(PendingOperation *op)
{
    if (op->isError()) {
        mPriv->streams.clear();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, false,
                op->errorName(), op->errorMessage());
        return;
    }

    mPriv->initialStreamsReceived = true;
    mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
}

void StreamedMediaChannel::onNewStreamReady(PendingOperation *op)
{
    PendingMediaStreams *pms = qobject_cast<PendingMediaStreams *>(op);

    if (op->isError()) {
        return;
    }

    Q_ASSERT(pms->streams().size() == 1);

    if (mPriv->initialStreamsReceived) {
        QSharedPointer<MediaStream> stream = pms->streams().first();
        emit streamAdded(stream);
    }
}

void StreamedMediaChannel::onStreamAdded(uint streamId,
        uint contactHandle, uint streamType)
{
    if (mPriv->streams.contains(streamId)) {
        debug() << "Received StreamedMediaChannel.StreamAdded for an existing "
            "stream, ignoring";
        return;
    }

    QSharedPointer<MediaStream> stream = QSharedPointer<MediaStream>(
            new MediaStream(this, streamId,
                contactHandle,
                (Telepathy::MediaStreamType) streamType,
                // TODO where to get this info from?
                Telepathy::MediaStreamStateDisconnected,
                Telepathy::MediaStreamDirectionNone,
                (Telepathy::MediaStreamPendingSend) 0));
    mPriv->streams.insert(streamId, stream);
    PendingMediaStreams *pms = new PendingMediaStreams(this,
            MediaStreams() << stream, this);
    connect(pms,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onNewStreamReady(Telepathy::Client::PendingOperation *)));
}

void StreamedMediaChannel::onStreamRemoved(uint streamId)
{
    debug() << "StreamedMediaChannel::onStreamRemoved: stream" <<
        streamId << "removed";

    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(mPriv->streams.contains(streamId));
    }

    if (mPriv->streams.contains(streamId)) {
        QSharedPointer<MediaStream> stream = mPriv->streams[streamId];
        emit stream->removed(stream.data());
        mPriv->streams.remove(streamId);
    }
}

void StreamedMediaChannel::onStreamDirectionChanged(uint streamId,
        uint streamDirection, uint pendingFlags)
{
    debug() << "StreamedMediaChannel::onStreamDirectionChanged: stream" <<
        streamId << "direction changed to" << streamDirection;

    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(mPriv->streams.contains(streamId));
    }

    if (mPriv->streams.contains(streamId)) {
        QSharedPointer<MediaStream> stream = mPriv->streams[streamId];
        stream->setDirection(
                (Telepathy::MediaStreamDirection) streamDirection,
                (Telepathy::MediaStreamPendingSend) pendingFlags);
    }
}

void StreamedMediaChannel::onStreamStateChanged(uint streamId,
        uint streamState)
{
    debug() << "StreamedMediaChannel::onStreamStateChanged: stream" <<
        streamId << "state changed to" << streamState;

    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(mPriv->streams.contains(streamId));
    }

    if (mPriv->streams.contains(streamId)) {
        QSharedPointer<MediaStream> stream = mPriv->streams[streamId];
        stream->setState((Telepathy::MediaStreamState) streamState);
    }
}

void StreamedMediaChannel::onStreamError(uint streamId,
        uint errorCode, const QString &errorMessage)
{
    debug() << "StreamedMediaChannel::onStreamError: stream" <<
        streamId << "error:" << errorCode << "-" << errorMessage;

    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(mPriv->streams.contains(streamId));
    }

    if (mPriv->streams.contains(streamId)) {
        QSharedPointer<MediaStream> stream = mPriv->streams[streamId];
        emit stream->error(stream.data(),
                (Telepathy::MediaStreamError) errorCode,
                errorMessage);
    }
}

void StreamedMediaChannel::addStream(const QSharedPointer<MediaStream> &stream)
{
    if (mPriv->streams.contains(stream->id())) {
        return;
    }

    mPriv->streams.insert(stream->id(), stream);

    if (mPriv->initialStreamsReceived) {
        emit streamAdded(stream);
    }
}

} // Telepathy::Client
} // Telepathy
