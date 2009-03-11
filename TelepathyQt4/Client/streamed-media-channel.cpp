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
#include <TelepathyQt4/Client/PendingVoidMethodCall>

#include <QHash>

namespace Telepathy
{
namespace Client
{

struct MediaStream::Private
{
    Private(StreamedMediaChannel *channel, uint id,
            uint contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend)
        : id(id), contactHandle(contactHandle),
          type(type), state(state), direction(direction),
          pendingSend(pendingSend)
    {
    }

    StreamedMediaChannel *channel;
    uint id;
    uint contactHandle;
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

uint MediaStream::id() const
{
    return mPriv->id;
}

QSharedPointer<Contact> MediaStream::contact() const
{
    ContactManager *contactManager = mPriv->channel->connection()->contactManager();
    return contactManager->lookupContactByHandle(mPriv->contactHandle);
}

Telepathy::MediaStreamState MediaStream::state() const
{
    return mPriv->state;
}

Telepathy::MediaStreamType MediaStream::type() const
{
    return mPriv->type;
}

bool MediaStream::sending() const
{
    return (mPriv->direction & Telepathy::MediaStreamDirectionSend ||
            mPriv->direction & Telepathy::MediaStreamDirectionBidirectional);
}

bool MediaStream::receiving() const
{
    return (mPriv->direction & Telepathy::MediaStreamDirectionReceive ||
            mPriv->direction & Telepathy::MediaStreamDirectionBidirectional);
}

bool MediaStream::localSendingRequested() const
{
    return mPriv->pendingSend & MediaStreamPendingLocalSend;
}

bool MediaStream::remoteSendingRequested() const
{
    return mPriv->pendingSend & MediaStreamPendingRemoteSend;
}

Telepathy::MediaStreamDirection MediaStream::direction() const
{
    return mPriv->direction;
}

Telepathy::MediaStreamPendingSend MediaStream::pendingSend() const
{
    return mPriv->pendingSend;
}

PendingOperation *MediaStream::remove()
{
    return mPriv->channel->removeStreams(Telepathy::UIntList() << mPriv->id);
}

PendingOperation *MediaStream::requestStreamDirection(
        Telepathy::MediaStreamDirection direction)
{
    return new PendingVoidMethodCall(this,
            mPriv->channel->streamedMediaInterface()->RequestStreamDirection(mPriv->id, direction));
}

void MediaStream::setDirection(Telepathy::MediaStreamDirection direction,
        Telepathy::MediaStreamPendingSend pendingSend)
{
    mPriv->direction = direction;
    mPriv->pendingSend = pendingSend;
    emit directionChanged(direction, pendingSend);
}

void MediaStream::setState(Telepathy::MediaStreamState state)
{
    mPriv->state = state;
    emit stateChanged(state);
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
 * \headerfile TelepathyQt4/Client/streamed-media-channel.h <TelepathyQt4/Client/StreamedMediaChannel>
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

MediaStreams StreamedMediaChannel::streams() const
{
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

PendingOperation *StreamedMediaChannel::removeStreams(MediaStreams streams)
{
    Telepathy::UIntList ids;
    foreach (const QSharedPointer<MediaStream> &stream, streams) {
        ids << stream->id();
    }
    return removeStreams(ids);
}

PendingOperation *StreamedMediaChannel::removeStreams(const Telepathy::UIntList &ids)
{
    return new PendingVoidMethodCall(this,
            streamedMediaInterface()->RemoveStreams(ids));
}

PendingOperation *StreamedMediaChannel::requestStreams(
        QSharedPointer<Telepathy::Client::Contact> contact,
        QList<Telepathy::MediaStreamType> types)
{
    Telepathy::UIntList l;
    foreach (Telepathy::MediaStreamType type, types) {
        l << type;
    }
    return new PendingVoidMethodCall(this,
            streamedMediaInterface()->RequestStreams(
                contact->handle()[0], l));
}

void StreamedMediaChannel::gotStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::ListStreams()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams,
                false, reply.error());
        return;
    }

    debug() << "Got reply to StreamedMedia::ListStreams()";
    mPriv->initialStreamsReceived = true;

    MediaStreamInfoList list = qdbus_cast<MediaStreamInfoList>(reply.value());
    foreach (const MediaStreamInfo &streamInfo, list) {
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

    mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);

    watcher->deleteLater();
}

void StreamedMediaChannel::onStreamAdded(uint streamId,
        uint contactHandle, uint streamType)
{
    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(!mPriv->streams.contains(streamId));
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

    if (mPriv->initialStreamsReceived) {
        emit streamAdded(stream);
    }
}

void StreamedMediaChannel::onStreamRemoved(uint streamId)
{
    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(mPriv->streams.contains(streamId));
    }

    if (mPriv->streams.contains(streamId)) {
        QSharedPointer<MediaStream> stream = mPriv->streams[streamId];
        emit stream->removed();
        stream.clear();
    }
}

void StreamedMediaChannel::onStreamDirectionChanged(uint streamId,
        uint streamDirection, uint pendingFlags)
{
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
    if (mPriv->initialStreamsReceived) {
        Q_ASSERT(mPriv->streams.contains(streamId));
    }

    if (mPriv->streams.contains(streamId)) {
        QSharedPointer<MediaStream> stream = mPriv->streams[streamId];
        emit stream->error((Telepathy::MediaStreamError) errorCode,
                errorMessage);
    }
}

} // Telepathy::Client
} // Telepathy
