/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/StreamedMediaChannel>

#include "TelepathyQt/_gen/streamed-media-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>

#include <QHash>

namespace Tp
{

/* ====== PendingStreamedMediaStreams ====== */
struct TP_QT_NO_EXPORT PendingStreamedMediaStreams::Private
{
    StreamedMediaStreams streams;
    uint numStreams;
    uint streamsReady;
};

/**
 * \class PendingStreamedMediaStreams
 * \ingroup clientchannel
 * \headerfile TelepathyQt/streamed-media-channel.h <TelepathyQt/PendingStreamedMediaStreams>
 *
 * \brief Class containing the result of an asynchronous streamed media stream creation
 * request.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via StreamedMediaChannel.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingStreamedMediaStreams object.
 *
 * \param channel StreamedMediaChannel to use.
 * \param contact The contact who the media stream is with.
 * \param types A list of stream types to request.
 */
PendingStreamedMediaStreams::PendingStreamedMediaStreams(const StreamedMediaChannelPtr &channel,
        const ContactPtr &contact,
        const QList<MediaStreamType> &types)
    : PendingOperation(channel),
      mPriv(new Private)
{
    mPriv->numStreams = types.size();
    mPriv->streamsReady = 0;

    UIntList l;
    foreach (MediaStreamType type, types) {
        l << type;
    }

    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        channel->interface<Client::ChannelTypeStreamedMediaInterface>();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
            streamedMediaInterface->RequestStreams(
                contact->handle()[0], l), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotStreams(QDBusPendingCallWatcher*)));
}

/**
 * Class destructor.
 */
PendingStreamedMediaStreams::~PendingStreamedMediaStreams()
{
    delete mPriv;
}

/**
 * Return the channel through which the request was made.
 *
 * \return A pointer to the StreamedMediaChannel object.
 */
StreamedMediaChannelPtr PendingStreamedMediaStreams::channel() const
{
    return StreamedMediaChannelPtr(qobject_cast<StreamedMediaChannel*>(
                (StreamedMediaChannel*) object().data()));
}

/**
 * Return a list of the newly created StreamedMediaStreamPtr objects.
 *
 * \return A list of pointers to StreamedMediaStream objects, or an empty list if an error occurred.
 */
StreamedMediaStreams PendingStreamedMediaStreams::streams() const
{
    if (!isFinished()) {
        warning() << "PendingStreamedMediaStreams::streams called before finished, "
            "returning empty list";
        return StreamedMediaStreams();
    } else if (!isValid()) {
        warning() << "PendingStreamedMediaStreams::streams called when not valid, "
            "returning empty list";
        return StreamedMediaStreams();
    }

    return mPriv->streams;
}

void PendingStreamedMediaStreams::gotStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia::RequestStreams()"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to StreamedMedia::RequestStreams()";

    MediaStreamInfoList list = reply.value();
    foreach (const MediaStreamInfo &streamInfo, list) {
        StreamedMediaStreamPtr stream = channel()->lookupStreamById(
                streamInfo.identifier);
        if (!stream) {
            stream = channel()->addStream(streamInfo);
        } else {
            channel()->onStreamDirectionChanged(streamInfo.identifier,
                    streamInfo.direction, streamInfo.pendingSendFlags);
            channel()->onStreamStateChanged(streamInfo.identifier,
                    streamInfo.state);
        }
        mPriv->streams.append(stream);
        connect(channel().data(),
                SIGNAL(streamRemoved(Tp::StreamedMediaStreamPtr)),
                SLOT(onStreamRemoved(Tp::StreamedMediaStreamPtr)));
        connect(stream->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onStreamReady(Tp::PendingOperation*)));
    }

    watcher->deleteLater();
}

void PendingStreamedMediaStreams::onStreamRemoved(const StreamedMediaStreamPtr &stream)
{
    if (isFinished()) {
        return;
    }

    if (mPriv->streams.contains(stream)) {
        // the stream was removed before becoming ready
        setFinishedWithError(TP_QT_ERROR_CANCELLED,
                QLatin1String("Stream removed before ready"));
    }
}

void PendingStreamedMediaStreams::onStreamReady(PendingOperation *op)
{
    if (isFinished()) {
        return;
    }

    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    mPriv->streamsReady++;
    debug() << "PendingStreamedMediaStreams:";
    debug() << "  Streams count:" << mPriv->numStreams;
    debug() << "  Streams ready:" << mPriv->streamsReady;
    if (mPriv->streamsReady == mPriv->numStreams) {
        debug() << "All streams are ready";
        setFinished();
    }
}

/* ====== StreamedMediaStream ====== */
struct TP_QT_NO_EXPORT StreamedMediaStream::Private
{
    Private(StreamedMediaStream *parent, const StreamedMediaChannelPtr &channel,
            const MediaStreamInfo &info);

    static void introspectContact(Private *self);

    PendingOperation *updateDirection(bool send, bool receive);
    SendingState localSendingStateFromDirection();
    SendingState remoteSendingStateFromDirection();

    StreamedMediaStream *parent;
    WeakPtr<StreamedMediaChannel> channel;
    ReadinessHelper *readinessHelper;

    uint id;
    uint type;
    uint contactHandle;
    ContactPtr contact;
    uint direction;
    uint pendingSend;
    uint state;
};

StreamedMediaStream::Private::Private(StreamedMediaStream *parent,
        const StreamedMediaChannelPtr &channel,
        const MediaStreamInfo &streamInfo)
    : parent(parent),
      channel(channel),
      readinessHelper(parent->readinessHelper()),
      id(streamInfo.identifier),
      type(streamInfo.type),
      contactHandle(streamInfo.contact),
      direction(MediaStreamDirectionNone),
      pendingSend(0),
      state(MediaStreamStateDisconnected)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectContact,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
}

void StreamedMediaStream::Private::introspectContact(StreamedMediaStream::Private *self)
{
    debug() << "Introspecting stream";
    if (self->contactHandle == 0) {
        debug() << "Stream ready";
        self->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        return;
    }

    debug() << "Introspecting stream contact";
    ContactManagerPtr contactManager =
        self->parent->channel()->connection()->contactManager();
    debug() << "contact manager" << contactManager;
    // TODO: pass id hints to ContactManager if we ever gain support to retrieve contact ids
    //       from MediaStreamInfo or something similar.
    self->parent->connect(contactManager->contactsForHandles(
                UIntList() << self->contactHandle),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotContact(Tp::PendingOperation*)));
}

PendingOperation *StreamedMediaStream::Private::updateDirection(
        bool send, bool receive)
{
    uint newDirection = 0;

    if (send) {
        newDirection |= MediaStreamDirectionSend;
    }

    if (receive) {
        newDirection |= MediaStreamDirectionReceive;
    }

    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        parent->channel()->interface<Client::ChannelTypeStreamedMediaInterface>();
    return new PendingVoid(
            streamedMediaInterface->RequestStreamDirection(
                id, newDirection),
            StreamedMediaStreamPtr(parent));
}

StreamedMediaStream::SendingState StreamedMediaStream::Private::localSendingStateFromDirection()
{
    if (pendingSend & MediaStreamPendingLocalSend) {
        return SendingStatePendingSend;
    }
    if (direction & MediaStreamDirectionSend) {
        return SendingStateSending;
    }
    return SendingStateNone;
}

StreamedMediaStream::SendingState StreamedMediaStream::Private::remoteSendingStateFromDirection()
{
    if (pendingSend & MediaStreamPendingRemoteSend) {
        return SendingStatePendingSend;
    }
    if (direction & MediaStreamDirectionReceive) {
        return SendingStateSending;
    }
    return SendingStateNone;
}

/**
 * \class StreamedMediaStream
 * \ingroup clientchannel
 * \headerfile TelepathyQt/streamed-media-channel.h <TelepathyQt/StreamedMediaStream>
 *
 * \brief The StreamedMediaStream class represents a Telepathy streamed media
 * stream.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via StreamedMediaChannel.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * StreamedMediaStream object usable.
 *
 * Note that this feature must be enabled in order to use most StreamedMediaStream
 * methods. See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature StreamedMediaStream::FeatureCore = Feature(QLatin1String(StreamedMediaStream::staticMetaObject.className()), 0);

/**
 * Construct a new StreamedMediaStream object.
 *
 * \param channel The channel ownding this media stream.
 * \param streamInfo The stream info of this media stream.
 */
StreamedMediaStream::StreamedMediaStream(const StreamedMediaChannelPtr &channel,
        const MediaStreamInfo &streamInfo)
    : Object(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, channel, streamInfo))
{
    gotDirection(streamInfo.direction, streamInfo.pendingSendFlags);
    gotStreamState(streamInfo.state);
}

/**
 * Class destructor.
 */
StreamedMediaStream::~StreamedMediaStream()
{
    delete mPriv;
}

/**
 * Return the channel owning this media stream.
 *
 * \return A pointer to the StreamedMediaChannel object.
 */
StreamedMediaChannelPtr StreamedMediaStream::channel() const
{
    return StreamedMediaChannelPtr(mPriv->channel);
}

/**
 * Return the id of this media stream.
 *
 * \return An integer representing the media stream id.
 */
uint StreamedMediaStream::id() const
{
    return mPriv->id;
}

/**
 * Return the contact who this media stream is with.
 *
 * \return A pointer to the Contact object.
 */
ContactPtr StreamedMediaStream::contact() const
{
    return mPriv->contact;
}

/**
 * Return the state of this media stream.
 *
 * \return The state as #MediaStreamState.
 */
MediaStreamState StreamedMediaStream::state() const
{
    return (MediaStreamState) mPriv->state;
}

/**
 * Return the type of this media stream.
 *
 * \return The type as #MediaStreamType.
 */
MediaStreamType StreamedMediaStream::type() const
{
    return (MediaStreamType) mPriv->type;
}

/**
 * Return whether media is being sent on this media stream.
 *
 * \return \c true if media is being sent, \c false otherwise.
 * \sa localSendingStateChanged()
 */
bool StreamedMediaStream::sending() const
{
    return mPriv->direction & MediaStreamDirectionSend;
}

/**
 * Return whether media is being received on this media stream.
 *
 * \return \c true if media is being received, \c false otherwise.
 * \sa remoteSendingStateChanged()
 */
bool StreamedMediaStream::receiving() const
{
    return mPriv->direction & MediaStreamDirectionReceive;
}

/**
 * Return whether the local user has been asked to send media by the
 * remote user on this media stream.
 *
 * \return \c true if the local user has been asked to send media by the
 *         remote user, \c false otherwise.
 * \sa localSendingStateChanged()
 */
bool StreamedMediaStream::localSendingRequested() const
{
    return mPriv->pendingSend & MediaStreamPendingLocalSend;
}

/**
 * Return whether the remote user has been asked to send media by the local
 * user on this media stream.
 *
 * \return \c true if the remote user has been asked to send media by the
 *         local user, \c false otherwise.
 * \sa remoteSendingStateChanged()
 */
bool StreamedMediaStream::remoteSendingRequested() const
{
    return mPriv->pendingSend & MediaStreamPendingRemoteSend;
}

/**
 * Return the direction of this media stream.
 *
 * \return The direction as #MediaStreamDirection.
 * \sa localSendingState(), remoteSendingState(),
 *     localSendingStateChanged(), remoteSendingStateChanged(),
 *     sending(), receiving()
 */
MediaStreamDirection StreamedMediaStream::direction() const
{
    return (MediaStreamDirection) mPriv->direction;
}

/**
 * Return the pending send flags of this media stream.
 *
 * \return The pending send flags as #MediaStreamPendingSend.
 * \sa localSendingStateChanged()
 */
MediaStreamPendingSend StreamedMediaStream::pendingSend() const
{
    return (MediaStreamPendingSend) mPriv->pendingSend;
}

/**
 * Request a change in the direction of this media stream. In particular, this
 * might be useful to stop sending media of a particular type, or inform the
 * peer that you are no longer using media that is being sent to you.
 *
 * \param direction The new direction.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa localSendingStateChanged(), remoteSendingStateChanged()
 */
PendingOperation *StreamedMediaStream::requestDirection(
        MediaStreamDirection direction)
{
    StreamedMediaChannelPtr chan(channel());
    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        chan->interface<Client::ChannelTypeStreamedMediaInterface>();
    return new PendingVoid(
            streamedMediaInterface->RequestStreamDirection(
                mPriv->id, direction),
            StreamedMediaStreamPtr(this));
}

/**
 * Start sending a DTMF tone on this media stream.
 *
 * Where possible, the tone will continue until stopDTMFTone() is called.
 * On certain protocols, it may only be possible to send events with a predetermined
 * length. In this case, the implementation may emit a fixed-length tone,
 * and the stopDTMFTone() method call should return #TP_QT_ERROR_NOT_AVAILABLE.
 *
 * If the channel() does not support the #TP_QT_IFACE_CHANNEL_INTERFACE_DTMF
 * interface, the resulting PendingOperation will fail with error code
 * #TP_QT_ERROR_NOT_IMPLEMENTED.

 * \param event A numeric event code from the #DTMFEvent enum.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa stopDTMFTone()
 */
PendingOperation *StreamedMediaStream::startDTMFTone(DTMFEvent event)
{
    StreamedMediaChannelPtr chan(channel());
    if (!chan->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_DTMF)) {
        warning() << "StreamedMediaStream::startDTMFTone() used with no dtmf interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("StreamedMediaChannel does not support dtmf interface"),
                StreamedMediaStreamPtr(this));
    }

    Client::ChannelInterfaceDTMFInterface *dtmfInterface =
        chan->interface<Client::ChannelInterfaceDTMFInterface>();
    return new PendingVoid(
            dtmfInterface->StartTone(mPriv->id, event),
            StreamedMediaStreamPtr(this));
}

/**
 * Stop sending any DTMF tone which has been started using the startDTMFTone()
 * method.
 *
 * If there is no current tone, the resulting PendingOperation will
 * finish successfully.
 *
 * If continuous tones are not supported by this media stream, the resulting
 * PendingOperation will fail with error code #TP_QT_ERROR_NOT_AVAILABLE.
 *
 * If the channel() does not support the #TP_QT_IFACE_CHANNEL_INTERFACE_DTMF
 * interface, the resulting PendingOperation will fail with error code
 * #TP_QT_ERROR_NOT_IMPLEMENTED.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa startDTMFTone()
 */
PendingOperation *StreamedMediaStream::stopDTMFTone()
{
    StreamedMediaChannelPtr chan(channel());
    if (!chan->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_DTMF)) {
        warning() << "StreamedMediaStream::stopDTMFTone() used with no dtmf interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("StreamedMediaChannel does not support dtmf interface"),
                StreamedMediaStreamPtr(this));
    }

    Client::ChannelInterfaceDTMFInterface *dtmfInterface =
        chan->interface<Client::ChannelInterfaceDTMFInterface>();
    return new PendingVoid(
            dtmfInterface->StopTone(mPriv->id),
            StreamedMediaStreamPtr(this));
}

/**
 * Request a change in the direction of this media stream.
 *
 * In particular, this might be useful to stop sending media of a particular type,
 * or inform the peer that you are no longer using media that is being sent to you.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa requestDirection(Tp::MediaStreamDirection direction),
 *     localSendingStateChanged(), remoteSendingStateChanged()
 */
PendingOperation *StreamedMediaStream::requestDirection(bool send, bool receive)
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

/**
 * Return the media stream local sending state.
 *
 * \return The local sending state as StreamedMediaStream::SendingState.
 * \sa localSendingStateChanged()
 */
StreamedMediaStream::SendingState StreamedMediaStream::localSendingState() const
{
    return mPriv->localSendingStateFromDirection();
}

/**
 * Return the media stream remote sending state.
 *
 * \return The remote sending state as StreamedMediaStream::SendingState.
 * \sa remoteSendingStateChanged()
 */
StreamedMediaStream::SendingState StreamedMediaStream::remoteSendingState() const
{
    return mPriv->remoteSendingStateFromDirection();
}

/**
 * Request that media starts or stops being sent on this media stream.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa localSendingStateChanged(), requestDirection()
 */
PendingOperation *StreamedMediaStream::requestSending(bool send)
{
    return mPriv->updateDirection(
            send,
            mPriv->direction & MediaStreamDirectionReceive);
}

/**
 * Request that the remote contact stops or starts sending on this media stream.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa remoteSendingStateChanged(), requestDirection()
 */
PendingOperation *StreamedMediaStream::requestReceiving(bool receive)
{
    return mPriv->updateDirection(
            mPriv->direction & MediaStreamDirectionSend,
            receive);
}

/**
 * \fn void StreamedMediaStream::localSendingStateChanged(
 *          Tp::StreamedMediaStream::SendingState localSendingState)
 *
 * Emitted when the local sending state of this media stream changes.
 *
 * \param localSendingState The new local sending state of this media stream.
 * \sa localSendingState()
 */

/**
 * \fn void MediaStream::remoteSendingStateChanged(
 *          Tp::MediaStream::SendingState &remoteSendingState)
 *
 * Emitted when the remote sending state of this media stream changes.
 *
 * \param remoteSendingState The new remote sending state of this media stream.
 * \sa remoteSendingState()
 */

void StreamedMediaStream::gotContact(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    Q_ASSERT(pc->isForHandles());

    if (op->isError()) {
        warning().nospace() << "Gathering media stream contact failed: "
            << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                op->errorName(), op->errorMessage());
        return;
    }

    QList<ContactPtr> contacts = pc->contacts();
    UIntList invalidHandles = pc->invalidHandles();
    if (contacts.size()) {
        Q_ASSERT(contacts.size() == 1);
        Q_ASSERT(invalidHandles.size() == 0);
        mPriv->contact = contacts.first();

        debug() << "Got stream contact";
        debug() << "Stream ready";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        Q_ASSERT(invalidHandles.size() == 1);
        warning().nospace() << "Error retrieving media stream contact (invalid handle)";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Invalid contact handle"));
    }
}

void StreamedMediaStream::gotDirection(uint direction, uint pendingSend)
{
    if (direction == mPriv->direction &&
        pendingSend == mPriv->pendingSend) {
        return;
    }

    SendingState oldLocalState = mPriv->localSendingStateFromDirection();
    SendingState oldRemoteState = mPriv->remoteSendingStateFromDirection();

    mPriv->direction = direction;
    mPriv->pendingSend = pendingSend;

    if (!isReady()) {
        return;
    }

    SendingState localSendingState =
        mPriv->localSendingStateFromDirection();
    if (localSendingState != oldLocalState) {
        emit localSendingStateChanged(localSendingState);
    }

    SendingState remoteSendingState =
        mPriv->remoteSendingStateFromDirection();
    if (remoteSendingState != oldRemoteState) {
        emit remoteSendingStateChanged(remoteSendingState);
    }
}

void StreamedMediaStream::gotStreamState(uint state)
{
    if (state == mPriv->state) {
        return;
    }

    mPriv->state = state;
}

/* ====== StreamedMediaChannel ====== */
struct TP_QT_NO_EXPORT StreamedMediaChannel::Private
{
    Private(StreamedMediaChannel *parent);
    ~Private();

    static void introspectStreams(Private *self);
    static void introspectLocalHoldState(Private *self);

    // Public object
    StreamedMediaChannel *parent;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    StreamedMediaStreams incompleteStreams;
    StreamedMediaStreams streams;

    LocalHoldState localHoldState;
    LocalHoldStateReason localHoldStateReason;
};

StreamedMediaChannel::Private::Private(StreamedMediaChannel *parent)
    : parent(parent),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
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
        QSet<uint>() << 0,                                                          // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                         // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_HOLD, // dependsOnInterfaces
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
        parent->interface<Client::ChannelTypeStreamedMediaInterface>();

    parent->connect(streamedMediaInterface,
            SIGNAL(StreamAdded(uint,uint,uint)),
            SLOT(onStreamAdded(uint,uint,uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamRemoved(uint)),
            SLOT(onStreamRemoved(uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamDirectionChanged(uint,uint,uint)),
            SLOT(onStreamDirectionChanged(uint,uint,uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamStateChanged(uint,uint)),
            SLOT(onStreamStateChanged(uint,uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamError(uint,uint,QString)),
            SLOT(onStreamError(uint,uint,QString)));

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
 * \class StreamedMediaChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/streamed-media-channel.h <TelepathyQt/StreamedMediaChannel>
 *
 * \brief The StreamedMediaChannel class represents a Telepathy channel of type StreamedMedia.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * StreamedMediaChannel object usable.
 *
 * This is currently the same as Channel::FeatureCore, but may change to include more.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 * \sa awaitingLocalAnswer(), awaitingRemoteAnswer(), acceptCall(), hangupCall(), handlerStreamingRequired()
 */
const Feature StreamedMediaChannel::FeatureCore = Feature(QLatin1String(Channel::staticMetaObject.className()), 0, true);

/**
 * Feature used in order to access media stream specific methods.
 *
 * See media stream specific methods' documentation for more details.
 * \sa streams(), streamsForType(),
 *     requestStream(), requestStreams(), streamAdded()
 *     removeStream(), removeStreams(), streamRemoved(),
 *     streamDirectionChanged(), streamStateChanged(), streamError()
 */
const Feature StreamedMediaChannel::FeatureStreams = Feature(QLatin1String(StreamedMediaChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to access local hold state info.
 *
 * See local hold state specific methods' documentation for more details.
 * \sa localHoldState(), localHoldStateReason(), requestHold(), localHoldStateChanged()
 */
const Feature StreamedMediaChannel::FeatureLocalHoldState = Feature(QLatin1String(StreamedMediaChannel::staticMetaObject.className()), 1);

/**
 * Create a new StreamedMediaChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A StreamedMediaChannelPtr object pointing to the newly created
 *         StreamedMediaChannel object.
 */
StreamedMediaChannelPtr StreamedMediaChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return StreamedMediaChannelPtr(new StreamedMediaChannel(connection,
                objectPath, immutableProperties, StreamedMediaChannel::FeatureCore));
}

/**
 * Construct a new StreamedMediaChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on StreamedMediaChannel::FeatureCore.
 */
StreamedMediaChannel::StreamedMediaChannel(const ConnectionPtr &connection,
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
StreamedMediaChannel::~StreamedMediaChannel()
{
    delete mPriv;
}

/**
 * Return a list of media streams in this channel.
 *
 * This methods requires StreamedMediaChannel::FeatureStreams to be ready.
 *
 * \return A list of pointers to StreamedMediaStream objects.
 * \sa streamAdded(), streamRemoved(), streamsForType(), requestStreams()
 */
StreamedMediaStreams StreamedMediaChannel::streams() const
{
    return mPriv->streams;
}

/**
 * Return a list of media streams in this channel for the given type \a type.
 *
 * This methods requires StreamedMediaChannel::FeatureStreams to be ready.
 *
 * \param type The interested type.
 * \return A list of pointers to StreamedMediaStream objects.
 * \sa streamAdded(), streamRemoved(), streams(), requestStreams()
 */
StreamedMediaStreams StreamedMediaChannel::streamsForType(MediaStreamType type) const
{
    StreamedMediaStreams ret;
    foreach (const StreamedMediaStreamPtr &stream, mPriv->streams) {
        if (stream->type() == type) {
            ret << stream;
        }
    }
    return ret;
}

/**
 * Return whether this channel is awaiting local answer.
 *
 * This method requires StreamedMediaChannel::FeatureCore to be ready.
 *
 * \return \c true if awaiting local answer, \c false otherwise.
 * \sa awaitingRemoteAnswer(), acceptCall()
 */
bool StreamedMediaChannel::awaitingLocalAnswer() const
{
    return groupSelfHandleIsLocalPending();
}

/**
 * Return whether this channel is awaiting remote answer.
 *
 * This method requires StreamedMediaChannel::FeatureCore to be ready.
 *
 * \return \c true if awaiting remote answer, \c false otherwise.
 * \sa awaitingLocalAnswer()
 */
bool StreamedMediaChannel::awaitingRemoteAnswer() const
{
    return !groupRemotePendingContacts().isEmpty();
}

/**
 * Accept an incoming call.
 *
 * This method requires StreamedMediaChannel::FeatureCore to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa awaitingLocalAnswer(), hangupCall()
 */
PendingOperation *StreamedMediaChannel::acceptCall()
{
    return groupAddSelfHandle();
}

/**
 * Remove the specified media stream from this channel.
 *
 * This methods requires StreamedMediaChannel::FeatureStreams to be ready.
 *
 * \param stream Media stream to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa streamRemoved(), streams(), streamsForType()
 */
PendingOperation *StreamedMediaChannel::removeStream(const StreamedMediaStreamPtr &stream)
{
    if (!stream) {
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Unable to remove a null stream"),
                StreamedMediaChannelPtr(this));
    }

    // StreamedMedia.RemoveStreams will trigger StreamedMedia.StreamRemoved
    // that will proper remove the stream
    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        interface<Client::ChannelTypeStreamedMediaInterface>();
    return new PendingVoid(
            streamedMediaInterface->RemoveStreams(
                UIntList() << stream->id()),
            StreamedMediaChannelPtr(this));
}

/**
 * Remove the specified media streams from this channel.
 *
 * This methods requires StreamedMediaChannel::FeatureStreams to be ready.
 *
 * \param streams List of media streams to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa streamRemoved(), streams(), streamsForType()
 */
PendingOperation *StreamedMediaChannel::removeStreams(const StreamedMediaStreams &streams)
{
    UIntList ids;
    foreach (const StreamedMediaStreamPtr &stream, streams) {
        if (!stream) {
            continue;
        }
        ids << stream->id();
    }

    if (ids.isEmpty()) {
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Unable to remove invalid streams"),
                StreamedMediaChannelPtr(this));
    }

    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        interface<Client::ChannelTypeStreamedMediaInterface>();
    return new PendingVoid(
            streamedMediaInterface->RemoveStreams(ids),
            StreamedMediaChannelPtr(this));
}

/**
 * Request that media streams be established to exchange the given type \a type
 * of media with the given contact \a contact.
 *
 * This methods requires StreamedMediaChannel::FeatureStreams to be ready.
 *
 * \return A PendingStreamedMediaStreams which will emit PendingStreamedMediaStreams::finished
 *         when the call has finished.
 * \sa streamAdded(), streams(), streamsForType()
 */
PendingStreamedMediaStreams *StreamedMediaChannel::requestStream(
        const ContactPtr &contact,
        MediaStreamType type)
{
    return new PendingStreamedMediaStreams(StreamedMediaChannelPtr(this),
            contact,
            QList<MediaStreamType>() << type);
}

/**
 * Request that media streams be established to exchange the given types \a
 * types of media with the given contact \a contact.
 *
 * This methods requires StreamedMediaChannel::FeatureStreams to be ready.
 *
 * \return A PendingStreamedMediaStreams which will emit PendingStreamedMediaStreams::finished
 *         when the call has finished.
 * \sa streamAdded(), streams(), streamsForType()
 */
PendingStreamedMediaStreams *StreamedMediaChannel::requestStreams(
        const ContactPtr &contact,
        QList<MediaStreamType> types)
{
    return new PendingStreamedMediaStreams(StreamedMediaChannelPtr(this),
            contact, types);
}

/**
 * Request that the call is ended.
 *
 * This method requires StreamedMediaChannel::FeatureCore to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *StreamedMediaChannel::hangupCall()
{
    return requestLeave();
}

/**
 * Check whether media streaming by the handler is required for this channel.
 *
 * For channels with the #TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING interface,
 * the main handler of the channel is responsible for doing the actual streaming, for instance by
 * calling createFarsightChannel(channel) from TelepathyQt-Farsight library
 * and using the telepathy-farsight API on the resulting TfChannel.
 *
 * This method requires StreamedMediaChannel::FeatureCore to be ready.
 *
 * \return \c true if required, \c false otherwise.
 */
bool StreamedMediaChannel::handlerStreamingRequired() const
{
    return interfaces().contains(
            TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING);
}

/**
 * Return the local hold state for this channel.
 *
 * Whether the local user has placed this channel on hold.
 *
 * This method requires StreamedMediaChannel::FeatureHoldState to be ready.
 *
 * \return The local hold state as #LocalHoldState.
 * \sa requestHold(), localHoldStateChanged()
 */
LocalHoldState StreamedMediaChannel::localHoldState() const
{
    if (!isReady(FeatureLocalHoldState)) {
        warning() << "StreamedMediaChannel::localHoldState() used with FeatureLocalHoldState not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::localHoldStateReason() used with no hold interface";
    }

    return mPriv->localHoldState;
}

/**
 * Return the reason why localHoldState() changed to its current value.
 *
 * This method requires StreamedMediaChannel::FeatureLocalHoldState to be ready.
 *
 * \return The local hold state reason as #LocalHoldStateReason.
 * \sa requestHold(), localHoldStateChanged()
 */
LocalHoldStateReason StreamedMediaChannel::localHoldStateReason() const
{
    if (!isReady(FeatureLocalHoldState)) {
        warning() << "StreamedMediaChannel::localHoldStateReason() used with FeatureLocalHoldState not ready";
    } else if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::localHoldStateReason() used with no hold interface";
    }

    return mPriv->localHoldStateReason;
}

/**
 * Request that the channel be put on hold (be instructed not to send any media
 * streams to you) or be taken off hold.
 *
 * If the CM can immediately tell that the requested state
 * change could not possibly succeed, the resulting PendingOperation will fail
 * with error code #TP_QT_ERROR_NOT_AVAILABLE.
 * If the requested state is the same as the current state, the resulting
 * PendingOperation will finish successfully.
 *
 * Otherwise, the channel's local hold state will change to
 * #LocalHoldStatePendingHold or #LocalHoldStatePendingUnhold (as
 * appropriate), then the resulting PendingOperation will finish successfully.
 *
 * The eventual success or failure of the request is indicated by a subsequent
 * localHoldStateChanged() signal, changing the local hold state to
 * #LocalHoldStateHeld or #LocalHoldStateUnheld.
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
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa localHoldState(), localHoldStateReason(), localHoldStateChanged()
 */
PendingOperation *StreamedMediaChannel::requestHold(bool hold)
{
    if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::requestHold() used with no hold interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("StreamedMediaChannel does not support hold interface"),
                StreamedMediaChannelPtr(this));
    }

    Client::ChannelInterfaceHoldInterface *holdInterface =
        interface<Client::ChannelInterfaceHoldInterface>();
    return new PendingVoid(holdInterface->RequestHold(hold),
            StreamedMediaChannelPtr(this));
}

/**
 * \fn void StreamedMediaChannel::streamAdded(const Tp::StreamedMediaStreamPtr &stream)
 *
 * Emitted when a media stream is added to this channel.
 *
 * \param stream The media stream that was added.
 * \sa streams(), streamsForType(), streamRemoved()
 */

/**
 * \fn void StreamedMediaChannel::streamRemoved(const Tp::StreamedMediaStreamPtr &stream)
 *
 * Emitted when a media stream is removed from this channel.
 *
 * \param stream The media stream that was removed.
 * \sa streams(), streamsForType(), streamAdded()
 */

/**
 * \fn void StreamedMediaChannel::streamDirectionChanged(
 *          const Tp::StreamedMediaStreamPtr &stream, Tp::MediaStreamDirection direction,
 *          Tp::MediaStreamPendingSend pendingSend)
 *
 * Emitted when a media stream direction changes.
 *
 * \param stream The media stream which the direction changed.
 * \param direction The new direction of the stream that changed.
 * \param pendingSend The new pending send flags of the stream that changed.
 * \sa StreamedMediaStream::direction()
 */

/**
 * \fn void StreamedMediaChannel::streamStateChanged(
 *          const Tp::StreamedMediaStreamPtr &stream, Tp::MediaStreamState state)
 *
 * Emitted when a media stream state changes.
 *
 * \param stream The media stream which the state changed.
 * \param state The new state of the stream that changed.
 * \sa StreamedMediaStream::state()
 */

/**
 * \fn void StreamedMediaChannel::streamError(
 *          const Tp::StreamedMediaStreamPtr &stream,
 *          Tp::StreamedMediaStreamError errorCode, const QString &errorMessage)
 *
 * Emitted when an error occurs on a media stream.
 *
 * \param stream The media stream which the error occurred.
 * \param errorCode The error code.
 * \param errorMessage The error message.
 */

/**
 * \fn void StreamedMediaChannel::localHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);
 *
 * Emitted when the local hold state of this channel changes.
 *
 * \param state The new local hold state of this channel.
 * \param reason The reason why the change occurred.
 * \sa localHoldState(), localHoldStateReason()
 */

void StreamedMediaChannel::onStreamReady(PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    StreamedMediaStreamPtr stream = StreamedMediaStreamPtr::qObjectCast(pr->proxy());

    if (op->isError()) {
        mPriv->incompleteStreams.removeOne(stream);
        if (!isReady(FeatureStreams) && mPriv->incompleteStreams.size() == 0) {
            // let's not fail because a stream could not become ready
            mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
        }
        return;
    }

    // the stream was removed before become ready
    if (!mPriv->incompleteStreams.contains(stream)) {
        if (!isReady(FeatureStreams) && mPriv->incompleteStreams.size() == 0) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
        }
        return;
    }

    mPriv->incompleteStreams.removeOne(stream);
    mPriv->streams.append(stream);

    if (isReady(FeatureStreams)) {
        emit streamAdded(stream);
    }

    if (!isReady(FeatureStreams) && mPriv->incompleteStreams.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
    }
}

void StreamedMediaChannel::gotStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia.ListStreams failed with" <<
            reply.error().name() << ": " << reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to StreamedMedia::ListStreams()";

    MediaStreamInfoList streamInfoList = reply.value();
    if (streamInfoList.size() > 0) {
        foreach (const MediaStreamInfo &streamInfo, streamInfoList) {
            StreamedMediaStreamPtr stream = lookupStreamById(
                    streamInfo.identifier);
            if (!stream) {
                addStream(streamInfo);
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

void StreamedMediaChannel::onStreamAdded(uint streamId,
        uint contactHandle, uint streamType)
{
    if (lookupStreamById(streamId)) {
        debug() << "Received StreamedMedia.StreamAdded for an existing "
            "stream, ignoring";
        return;
    }

    MediaStreamInfo streamInfo = {
        streamId,
        contactHandle,
        streamType,
        MediaStreamStateDisconnected,
        MediaStreamDirectionReceive,
        MediaStreamPendingLocalSend
    };
    addStream(streamInfo);
}

void StreamedMediaChannel::onStreamRemoved(uint streamId)
{
    debug() << "Received StreamedMedia.StreamRemoved for stream" <<
        streamId;

    StreamedMediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }
    bool incomplete = mPriv->incompleteStreams.contains(stream);
    if (incomplete) {
        mPriv->incompleteStreams.removeOne(stream);
    } else {
        mPriv->streams.removeOne(stream);
    }

    if (isReady(FeatureStreams) && !incomplete) {
        emit streamRemoved(stream);
    }

    // the stream was added/removed before become ready
    if (!isReady(FeatureStreams) &&
        mPriv->streams.size() == 0 &&
        mPriv->incompleteStreams.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureStreams, true);
    }
}

void StreamedMediaChannel::onStreamDirectionChanged(uint streamId,
        uint streamDirection, uint streamPendingFlags)
{
    debug() << "Received StreamedMedia.StreamDirectionChanged for stream" <<
        streamId << "with direction changed to" << streamDirection;

    StreamedMediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }

    uint oldDirection = stream->direction();
    uint oldPendingFlags = stream->pendingSend();

    stream->gotDirection(streamDirection, streamPendingFlags);

    if (oldDirection != streamDirection ||
        oldPendingFlags != streamPendingFlags) {
        emit streamDirectionChanged(stream,
                (MediaStreamDirection) streamDirection,
                (MediaStreamPendingSend) streamPendingFlags);
    }
}

void StreamedMediaChannel::onStreamStateChanged(uint streamId,
        uint streamState)
{
    debug() << "Received StreamedMedia.StreamStateChanged for stream" <<
        streamId << "with state changed to" << streamState;

    StreamedMediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }

    uint oldState = stream->state();

    stream->gotStreamState(streamState);

    if (oldState != streamState) {
        emit streamStateChanged(stream, (MediaStreamState) streamState);
    }
}

void StreamedMediaChannel::onStreamError(uint streamId,
        uint errorCode, const QString &errorMessage)
{
    debug() << "Received StreamedMedia.StreamError for stream" <<
        streamId << "with error code" << errorCode <<
        "and message:" << errorMessage;

    StreamedMediaStreamPtr stream = lookupStreamById(streamId);
    if (!stream) {
        return;
    }

    emit streamError(stream, (MediaStreamError) errorCode, errorMessage);
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
    bool changed = false;
    if (mPriv->localHoldState != static_cast<LocalHoldState>(localHoldState) ||
        mPriv->localHoldStateReason != static_cast<LocalHoldStateReason>(localHoldStateReason)) {
        changed = true;
    }

    mPriv->localHoldState = static_cast<LocalHoldState>(localHoldState);
    mPriv->localHoldStateReason = static_cast<LocalHoldStateReason>(localHoldStateReason);

    if (!isReady(FeatureLocalHoldState)) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureLocalHoldState, true);
    } else {
        if (changed) {
            emit localHoldStateChanged(mPriv->localHoldState,
                    mPriv->localHoldStateReason);
        }
    }
}

StreamedMediaStreamPtr StreamedMediaChannel::addStream(const MediaStreamInfo &streamInfo)
{
    StreamedMediaStreamPtr stream = StreamedMediaStreamPtr(
            new StreamedMediaStream(StreamedMediaChannelPtr(this), streamInfo));

    mPriv->incompleteStreams.append(stream);
    connect(stream->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onStreamReady(Tp::PendingOperation*)));
    return stream;
}

StreamedMediaStreamPtr StreamedMediaChannel::lookupStreamById(uint streamId)
{
    foreach (const StreamedMediaStreamPtr &stream, mPriv->streams) {
        if (stream->id() == streamId) {
            return stream;
        }
    }

    foreach (const StreamedMediaStreamPtr &stream, mPriv->incompleteStreams) {
        if (stream->id() == streamId) {
            return stream;
        }
    }

    return StreamedMediaStreamPtr();
}

} // Tp
