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
#include <TelepathyQt4/PendingVoid>

#include <QHash>

#include "TelepathyQt4/future-internal.h"

using TpFuture::Client::CallContentInterface;
using TpFuture::Client::CallStreamInterface;
using TpFuture::Client::ChannelTypeCallInterface;

namespace Tp
{

enum IfaceType {
    IfaceTypeStreamedMedia,
    IfaceTypeCall,
};

/* ====== PendingMediaStreams ====== */
struct TELEPATHY_QT4_NO_EXPORT PendingMediaStreams::Private
{
    Private(PendingMediaStreams *parent, const StreamedMediaChannelPtr &channel)
        : parent(parent), channel(channel), contentsReady(0)
    {
    }

    PendingMediaStreams *parent;
    WeakPtr<StreamedMediaChannel> channel;
    MediaContents contents;
    uint contentsReady;
};

PendingMediaStreams::PendingMediaStreams(const StreamedMediaChannelPtr &channel,
        const ContactPtr &contact,
        const QList<MediaStreamType> &types)
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
            SLOT(gotSMStreams(QDBusPendingCallWatcher*)));
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

    MediaStreams ret;
    foreach (const MediaContentPtr &content, mPriv->contents) {
        ret << content->streams();
    }
    return ret;
}

void PendingMediaStreams::gotSMStreams(QDBusPendingCallWatcher *watcher)
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
    StreamedMediaChannelPtr channel(mPriv->channel);
    foreach (const MediaStreamInfo &streamInfo, list) {
        MediaContentPtr content = channel->lookupContentBySMStreamId(
                streamInfo.identifier);
        if (!content) {
            content = channel->addContentForSMStream(streamInfo);
        } else {
            channel->onSMStreamDirectionChanged(streamInfo.identifier,
                    streamInfo.direction, streamInfo.pendingSendFlags);
            channel->onSMStreamStateChanged(streamInfo.identifier,
                    streamInfo.state);
        }
        mPriv->contents.append(content);
        connect(channel.data(),
                SIGNAL(contentRemoved(Tp::MediaStreamPtr)),
                SLOT(onContentRemoved(Tp::MediaStreamPtr)));
        connect(content->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContentReady(Tp::PendingOperation*)));
    }

    watcher->deleteLater();
}

void PendingMediaStreams::onContentRemoved(const MediaContentPtr &content)
{
    if (isFinished()) {
        return;
    }

    if (mPriv->contents.contains(content)) {
        // the content was removed before becoming ready
        setFinishedWithError(TELEPATHY_ERROR_CANCELLED, "Content removed before ready");
    }
}

void PendingMediaStreams::onContentReady(PendingOperation *op)
{
    if (isFinished()) {
        return;
    }

    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    mPriv->contentsReady++;
    if (mPriv->contentsReady == (uint) mPriv->contents.size()) {
        setFinished();
    }
}

/* ====== MediaStream ====== */
struct TELEPATHY_QT4_NO_EXPORT MediaStream::Private
{
    Private(MediaStream *parent, const MediaContentPtr &content,
            const MediaStreamInfo &info);
    Private(MediaStream *parent, const MediaContentPtr &content,
            const QDBusObjectPath &objectPath);

    // SM specific methods
    static void introspectSMContact(Private *self);

    PendingOperation *updateSMDirection(bool send, bool receive);
    SendingState localSendingStateFromSMDirection();
    SendingState remoteSendingStateFromSMDirection();

    // Call specific methods
    static void introspectCallMainProperties(Private *self);

    void processCallSendersChanged();

    struct CallSendersChangedInfo;

    IfaceType ifaceType;
    MediaStream *parent;
    ReadinessHelper *readinessHelper;
    WeakPtr<MediaContent> content;

    // SM specific fields
    uint SMId;
    uint SMContactHandle;
    ContactPtr SMContact;
    uint SMDirection;
    uint SMPendingSend;
    uint SMState;

    // Call specific fields
    CallStreamInterface *callBaseInterface;
    Client::DBus::PropertiesInterface *callPropertiesInterface;
    QDBusObjectPath callObjectPath;
    TpFuture::ContactSendingStateMap senders;
    QHash<uint, ContactPtr> sendersContacts;
    bool buildingCallSenders;
    QQueue<CallSendersChangedInfo *> callSendersChangedQueue;
    CallSendersChangedInfo *currentCallSendersChangedInfo;
};

struct TELEPATHY_QT4_NO_EXPORT MediaStream::Private::CallSendersChangedInfo
{
    CallSendersChangedInfo(const TpFuture::ContactSendingStateMap &updates,
            const UIntList &removed)
        : updates(updates),
          removed(removed)
    {
    }

    TpFuture::ContactSendingStateMap updates;
    UIntList removed;
};

MediaStream::Private::Private(MediaStream *parent,
        const MediaContentPtr &content,
        const MediaStreamInfo &streamInfo)
    : ifaceType(IfaceTypeStreamedMedia),
      parent(parent),
      readinessHelper(parent->readinessHelper()),
      content(content),
      SMId(streamInfo.identifier),
      SMContactHandle(streamInfo.contact),
      SMDirection(MediaStreamDirectionNone),
      SMPendingSend(0),
      SMState(MediaStreamStateDisconnected)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectSMContact,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureCore);
}

MediaStream::Private::Private(MediaStream *parent,
        const MediaContentPtr &content,
        const QDBusObjectPath &objectPath)
    : ifaceType(IfaceTypeCall),
      parent(parent),
      readinessHelper(parent->readinessHelper()),
      content(content),
      callBaseInterface(0),
      callPropertiesInterface(0),
      callObjectPath(objectPath),
      buildingCallSenders(false),
      currentCallSendersChangedInfo(0)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCallMainProperties,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureCore);
}

void MediaStream::Private::introspectSMContact(MediaStream::Private *self)
{
    if (self->SMContactHandle == 0) {
        self->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        return;
    }

    ContactManager *contactManager =
        self->parent->channel()->connection()->contactManager();
    self->parent->connect(
            contactManager->contactsForHandles(
                UIntList() << self->SMContactHandle),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(gotSMContact(Tp::PendingOperation *)));
}

PendingOperation *MediaStream::Private::updateSMDirection(
        bool send, bool receive)
{
    uint newSMDirection = SMDirection;

    if (send) {
        newSMDirection |= MediaStreamDirectionSend;
    }
    else {
        newSMDirection &= ~MediaStreamDirectionSend;
    }

    if (receive) {
        newSMDirection |= MediaStreamDirectionReceive;
    }
    else {
        newSMDirection &= ~MediaStreamDirectionReceive;
    }

    StreamedMediaChannelPtr chan(MediaContentPtr(content)->channel());
    return new PendingVoid(
            chan->streamedMediaInterface()->RequestStreamDirection(
                SMId, newSMDirection),
            parent);
}

MediaStream::SendingState MediaStream::Private::localSendingStateFromSMDirection()
{
    if (SMPendingSend & MediaStreamPendingLocalSend) {
        return SendingStatePendingSend;
    }
    if (SMDirection & MediaStreamDirectionSend) {
        return SendingStateSending;
    }
    return SendingStateNone;
}

MediaStream::SendingState MediaStream::Private::remoteSendingStateFromSMDirection()
{
    if (SMPendingSend & MediaStreamPendingRemoteSend) {
        return SendingStatePendingSend;
    }
    if (SMDirection & MediaStreamDirectionReceive) {
        return SendingStateSending;
    }
    return SendingStateNone;
}

void MediaStream::Private::introspectCallMainProperties(
        MediaStream::Private *self)
{
    MediaStream *parent = self->parent;
    StreamedMediaChannelPtr channel = parent->channel();

    self->callBaseInterface = new CallStreamInterface(
            channel->dbusConnection(), channel->busName(),
            self->callObjectPath.path(), parent);
    // parent->connect(self->callBaseInterface,
    //         SIGNAL(SendersChanged(const TpFuture::ContactSendingStateMap &,
    //                 const TpFuture::UIntList &)),
    //         SLOT(onCallSendersChanged(const TpFuture::ContactSendingStateMap &,
    //                 const TpFuture::UIntList &)));

    self->callPropertiesInterface = new Client::DBus::PropertiesInterface(
            channel->dbusConnection(), channel->busName(),
            self->callObjectPath.path(), parent);
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->callPropertiesInterface->GetAll(
                    TP_FUTURE_INTERFACE_CALL_STREAM),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotCallMainProperties(QDBusPendingCallWatcher *)));
}

void MediaStream::Private::processCallSendersChanged()
{
    if (buildingCallSenders) {
        return;
    }

    if (callSendersChangedQueue.isEmpty()) {
        if (!parent->isReady(FeatureCore)) {
            readinessHelper->setIntrospectCompleted(FeatureCore, true);
        }
        return;
    }

    currentCallSendersChangedInfo = callSendersChangedQueue.dequeue();

    TpFuture::ContactSendingStateMap::const_iterator i =
        currentCallSendersChangedInfo->updates.constBegin();
    TpFuture::ContactSendingStateMap::const_iterator end =
        currentCallSendersChangedInfo->updates.constEnd();
    QSet<uint> pendingSenders;
    while (i != end) {
        pendingSenders.insert(i.key());
        ++i;
    }

    if (!pendingSenders.isEmpty()) {
        buildingCallSenders = true;

        ContactManager *contactManager =
            parent->channel()->connection()->contactManager();
        PendingContacts *contacts = contactManager->contactsForHandles(
                pendingSenders.toList());
        parent->connect(contacts,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(gotCallSendersContacts(Tp::PendingOperation *)));
    } else {
        if (callSendersChangedQueue.isEmpty()) {
            if (!parent->isReady(FeatureCore)) {
                readinessHelper->setIntrospectCompleted(FeatureCore, true);
            }
            return;
        } else {
            processCallSendersChanged();
        }
    }
}

const Feature MediaStream::FeatureCore = Feature(MediaStream::staticMetaObject.className(), 0);

MediaStream::MediaStream(const MediaContentPtr &content,
        const MediaStreamInfo &streamInfo)
    : QObject(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, content, streamInfo))
{
    gotSMDirection(streamInfo.direction, streamInfo.pendingSendFlags);
    gotSMStreamState(streamInfo.state);
}

MediaStream::MediaStream(const MediaContentPtr &content,
        const QDBusObjectPath &objectPath)
    : QObject(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, content, objectPath))
{
}

MediaStream::~MediaStream()
{
    delete mPriv;
}

StreamedMediaChannelPtr MediaStream::channel() const
{
    return content()->channel();
}

/**
 * Return the stream id.
 *
 * \return An integer representing the stream id.
 */
uint MediaStream::id() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->SMId;
    } else {
        return 0;
    }
}

/**
 * Return the contact who the stream is with.
 *
 * \return The contact who the stream is with.
 */
ContactPtr MediaStream::contact() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->SMContact;
    } else {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();

            if (handle != chanSelfHandle && handle != connSelfHandle) {
                Q_ASSERT(mPriv->sendersContacts.contains(handle));
                return mPriv->sendersContacts[handle];
            }
        }
    }
    return ContactPtr();
}

/**
 * Return the stream state.
 *
 * \return The stream state.
 */
MediaStreamState MediaStream::state() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return (MediaStreamState) mPriv->SMState;
    } else {
        return MediaStreamStateConnected;
    }
}

/**
 * Return the stream type.
 *
 * \return The stream type.
 */
MediaStreamType MediaStream::type() const
{
    return content()->type();
}

/**
 * Return whether media is being sent on this stream.
 *
 * \return A boolean indicating whether media is being sent on this stream.
 */
bool MediaStream::sending() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->SMDirection & MediaStreamDirectionSend;
    } else {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle == chanSelfHandle || handle == connSelfHandle) {
                return sendingState & SendingStateSending;
            }
        }
        return false;
    }
}

/**
 * Return whether media is being received on this stream.
 *
 * \return A boolean indicating whether media is being received on this stream.
 */
bool MediaStream::receiving() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->SMDirection & MediaStreamDirectionReceive;
    } else {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle != chanSelfHandle && handle != connSelfHandle &&
                sendingState & SendingStateSending) {
                return true;
            }
        }
        return false;
    }
}

/**
 * Return whether the local user has been asked to send media by the remote user.
 *
 * \return A boolean indicating whether the local user has been asked to
 *         send media by the remote user.
 */
bool MediaStream::localSendingRequested() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->SMPendingSend & MediaStreamPendingLocalSend;
    } else {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle == chanSelfHandle || handle == connSelfHandle) {
                return sendingState & SendingStatePendingSend;
            }
        }
        return false;
    }
}

/**
 * Return whether the remote user has been asked to send media by the local user.
 *
 * \return A boolean indicating whether the remote user has been asked to
 *         send media by the local user.
 */
bool MediaStream::remoteSendingRequested() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->SMPendingSend & MediaStreamPendingRemoteSend;
    } else {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle != chanSelfHandle && handle != connSelfHandle &&
                sendingState & SendingStatePendingSend) {
                return true;
            }
        }
        return false;
    }
}

/**
 * Return the stream direction.
 *
 * \return The stream direction.
 */
MediaStreamDirection MediaStream::direction() const
{
   if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
       return (MediaStreamDirection) mPriv->SMDirection;
   } else {
       uint dir = MediaStreamDirectionNone;
       if (sending()) {
           dir |= MediaStreamDirectionSend;
       }
       if (receiving()) {
           dir |= MediaStreamDirectionReceive;
       }
       return (MediaStreamDirection) dir;
   }
}

/**
 * Return the stream pending send flags.
 *
 * \return The stream pending send flags.
 */
MediaStreamPendingSend MediaStream::pendingSend() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return (MediaStreamPendingSend) mPriv->SMPendingSend;
    }
    else {
        uint pending = 0;
        if (localSendingRequested()) {
            pending |= MediaStreamPendingLocalSend;
        }
        if (remoteSendingRequested()) {
            pending |= MediaStreamPendingRemoteSend;
        }
        return (MediaStreamPendingSend) pending;
    }
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
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return new PendingVoid(
                channel()->streamedMediaInterface()->RequestStreamDirection(
                    mPriv->SMId, direction),
                this);
    } else {
        // TODO add Call iface support
        return NULL;
    }
}

/**
 * Start sending a DTMF tone on this stream. Where possible, the tone will
 * continue until stopDTMFTone() is called. On certain protocols, it may only be
 * possible to send events with a predetermined length. In this case, the
 * implementation may emit a fixed-length tone, and the stopDTMFTone() method
 * call should return %TELEPATHY_ERROR_NOT_AVAILABLE.
 *
 * If the channel() does not support the %TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF
 * interface, the resulting PendingOperation will fail with error code
 * %TELEPATHY_ERROR_NOT_IMPLEMENTED.

 * \param event A numeric event code from the %DTMFEvent enum.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa stopDTMFTone()
 */
PendingOperation *MediaStream::startDTMFTone(DTMFEvent event)
{
    if (mPriv->ifaceType != IfaceTypeStreamedMedia) {
        // TODO add Call iface support
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "MediaStream does not have DTMF support", this);
    }

    StreamedMediaChannelPtr chan(channel());
    if (!chan->interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF)) {
        warning() << "MediaStream::startDTMFTone() used with no dtmf interface";
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "StreamedMediaChannel does not support dtmf interface",
                this);
    }
    return new PendingVoid(
            chan->DTMFInterface()->StartTone(mPriv->SMId, event),
            this);
}

/**
 * Stop sending any DTMF tone which has been started using the startDTMFTone()
 * method. If there is no current tone, the resulting PendingOperation will
 * finish successfully.
 *
 * If continuous tones are not supported by this stream, the resulting
 * PendingOperation will fail with error code %TELEPATHY_ERROR_NOT_AVAILABLE.
 *
 * If the channel() does not support the %TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF
 * interface, the resulting PendingOperation will fail with error code
 * %TELEPATHY_ERROR_NOT_IMPLEMENTED.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa startDTMFTone()
 */
PendingOperation *MediaStream::stopDTMFTone()
{
    if (mPriv->ifaceType != IfaceTypeStreamedMedia) {
        // TODO add Call iface support
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "MediaStream does not have DTMF support", this);
    }

    StreamedMediaChannelPtr chan(channel());
    if (!chan->interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF)) {
        warning() << "MediaStream::stopDTMFTone() used with no dtmf interface";
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "StreamedMediaChannel does not support dtmf interface",
                this);
    }
    return new PendingVoid(
            chan->DTMFInterface()->StopTone(mPriv->SMId),
            this);
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

MediaContentPtr MediaStream::content() const
{
    return MediaContentPtr(mPriv->content);
}

/**
 * Return the contacts whose the stream is with.
 *
 * \return The contacts whose the stream is with.
 */
Contacts MediaStream::members() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return Contacts() << mPriv->SMContact;
    } else {
        return mPriv->sendersContacts.values().toSet();
    }
}

/**
 * Return the stream local sending state.
 *
 * \return The stream local sending state.
 */
MediaStream::SendingState MediaStream::localSendingState() const
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->localSendingStateFromSMDirection();
    } else {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle == chanSelfHandle || handle == connSelfHandle) {
                return sendingState;
            }
        }
    }
    return SendingStateNone;
}

/**
 * Return the stream remote sending state for a given \a contact.
 *
 * \return The stream remote sending state for a contact.
 */
MediaStream::SendingState MediaStream::remoteSendingState(
        const ContactPtr &contact) const
{
    if (!contact) {
        return SendingStateNone;
    }

    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        if (mPriv->SMContact == contact) {
            return mPriv->remoteSendingStateFromSMDirection();
        }
    }
    else {
        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->senders.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->senders.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle == contact->handle()[0]) {
                return sendingState;
            }
        }
    }

    return SendingStateNone;
}

/**
 * Request that media starts or stops being sent on this stream.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *MediaStream::requestSending(bool send)
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return mPriv->updateSMDirection(
                send,
                mPriv->SMDirection & MediaStreamDirectionReceive);
    }
    else {
        return new PendingVoid(
                mPriv->callBaseInterface->SetSending(send),
                this);
    }
}

/**
 * Request that a remote \a contact stops or starts sending on this stream.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *MediaStream::requestReceiving(const ContactPtr &contact,
        bool receive)
{
    if (!contact) {
        return new PendingFailure(TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Invalid contact", this);
    }

    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        if (mPriv->SMContact != contact) {
            return new PendingFailure(TELEPATHY_ERROR_INVALID_ARGUMENT,
                    "Contact is not a member of the stream", this);
        }

        return mPriv->updateSMDirection(
                mPriv->SMDirection & MediaStreamDirectionSend,
                receive);
    }
    else {
        return new PendingVoid(mPriv->callBaseInterface->RequestReceiving(
                    contact->handle()[0], receive), this);
    }
}

void MediaStream::gotSMContact(PendingOperation *op)
{
    Q_ASSERT(mPriv->ifaceType == IfaceTypeStreamedMedia);

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
        mPriv->SMContact = contacts.first();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        Q_ASSERT(invalidHandles.size() == 1);
        warning().nospace() << "Error retrieving media stream contact (invalid handle)";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Invalid contact handle");
    }
}

void MediaStream::gotSMDirection(uint direction, uint pendingSend)
{
    Q_ASSERT(mPriv->ifaceType == IfaceTypeStreamedMedia);

    if (direction == mPriv->SMDirection &&
        pendingSend == mPriv->SMPendingSend) {
        return;
    }

    mPriv->SMDirection = direction;
    mPriv->SMPendingSend = pendingSend;

    if (!isReady()) {
        return;
    }

    SendingState localSendingState =
        mPriv->localSendingStateFromSMDirection();
    emit localSendingStateChanged(localSendingState);

    SendingState remoteSendingState =
        mPriv->remoteSendingStateFromSMDirection();
    QHash<ContactPtr, SendingState> remoteSendingStates;
    remoteSendingStates.insert(mPriv->SMContact, remoteSendingState);
    emit remoteSendingStateChanged(remoteSendingStates);
}

void MediaStream::gotSMStreamState(uint state)
{
    Q_ASSERT(mPriv->ifaceType == IfaceTypeStreamedMedia);

    if (state == mPriv->SMState) {
        return;
    }

    mPriv->SMState = state;
}

void MediaStream::gotCallMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Properties.GetAll(Call.Stream) failed with" <<
            reply.error().name() << ": " << reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to Properties.GetAll(Call.Stream)";

    QVariantMap props = reply.value();
    TpFuture::ContactSendingStateMap senders =
        qdbus_cast<TpFuture::ContactSendingStateMap>(props["Senders"]);

    mPriv->callSendersChangedQueue.enqueue(
            new Private::CallSendersChangedInfo(
                senders, UIntList()));
    mPriv->processCallSendersChanged();

    watcher->deleteLater();
}

void MediaStream::gotCallSendersContacts(PendingOperation *op)
{
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);

    mPriv->buildingCallSenders = false;

    if (pending->isValid()) {
        warning().nospace() << "Getting contacts failed with " <<
            pending->errorName() << ":" << pending->errorMessage() <<
            ", ignoring";
        mPriv->processCallSendersChanged();
        return;
    }

    QMap<uint, ContactPtr> removed;

    TpFuture::ContactSendingStateMap::const_iterator i =
        mPriv->currentCallSendersChangedInfo->updates.constBegin();
    TpFuture::ContactSendingStateMap::const_iterator end =
        mPriv->currentCallSendersChangedInfo->updates.constEnd();
    while (i != end) {
        mPriv->senders.insert(i.key(), i.value());
        ++i;
    }

    foreach (const ContactPtr &contact, pending->contacts()) {
        mPriv->sendersContacts.insert(contact->handle()[0], contact);
    }

    foreach (uint handle, mPriv->currentCallSendersChangedInfo->removed) {
        mPriv->senders.remove(handle);
        if (isReady(FeatureCore) && mPriv->sendersContacts.contains(handle)) {
            removed.insert(handle, mPriv->sendersContacts[handle]);

            // make sure we don't have updates for removed contacts
            mPriv->currentCallSendersChangedInfo->updates.remove(handle);
        }
        mPriv->sendersContacts.remove(handle);
    }

    foreach (uint handle, pending->invalidHandles()) {
        mPriv->senders.remove(handle);
        if (isReady(FeatureCore) && mPriv->sendersContacts.contains(handle)) {
            removed.insert(handle, mPriv->sendersContacts[handle]);

            // make sure we don't have updates for invalid handles
            mPriv->currentCallSendersChangedInfo->updates.remove(handle);
        }
        mPriv->sendersContacts.remove(handle);
    }

    if (isReady(FeatureCore)) {
        StreamedMediaChannelPtr chan(channel());
        uint chanSelfHandle = chan->groupSelfContact() ?
            chan->groupSelfContact()->handle()[0] : 0;
        uint connSelfHandle = chan->connection()->selfHandle();

        QHash<ContactPtr, SendingState> remoteSendingStates;

        TpFuture::ContactSendingStateMap::const_iterator i =
            mPriv->currentCallSendersChangedInfo->updates.constBegin();
        TpFuture::ContactSendingStateMap::const_iterator end =
            mPriv->currentCallSendersChangedInfo->updates.constEnd();
        while (i != end) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            if (handle == chanSelfHandle || handle == connSelfHandle) {
                emit localSendingStateChanged(sendingState);
            } else {
                Q_ASSERT(mPriv->sendersContacts.contains(handle));
                remoteSendingStates.insert(mPriv->sendersContacts[handle],
                        sendingState);
            }

            mPriv->senders.insert(i.key(), i.value());
            ++i;
        }

        if (!remoteSendingStates.isEmpty()) {
            emit remoteSendingStateChanged(remoteSendingStates);
        }

        if (!removed.isEmpty()) {
            emit membersRemoved(removed.values().toSet());
        }
    }

    mPriv->processCallSendersChanged();
}

/*
void MediaStream::onCallSendersChanged(
        const TpFuture::ContactSendingStateMap &updates,
        const TpFuture::UIntList &removed)
{
    if (updates.isEmpty() && removed.isEmpty()) {
        debug() << "Received Call.Content.SendersChanged with 0 changes and "
            "updates, skipping it";
        return;
    }

    mPriv->callSendersChangedQueue.enqueue(
            new Private::CallSendersChangedInfo(
                updates, removed));
    mPriv->processCallSendersChanged();
}
*/

QDBusObjectPath MediaStream::callObjectPath() const
{
    return mPriv->callObjectPath;
}

/* ====== PendingMediaContent ====== */
struct TELEPATHY_QT4_NO_EXPORT PendingMediaContent::Private
{
    Private(PendingMediaContent *parent, const StreamedMediaChannelPtr &channel)
        : parent(parent), channel(channel)
    {
    }

    PendingMediaContent *parent;
    WeakPtr<StreamedMediaChannel> channel;
    MediaContentPtr content;
};

PendingMediaContent::PendingMediaContent(const StreamedMediaChannelPtr &channel,
        const ContactPtr &contact,
        const QString &name,
        MediaStreamType type)
    : PendingOperation(channel.data()),
      mPriv(new Private(this, channel))
{
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
            channel->streamedMediaInterface()->RequestStreams(
                contact->handle()[0], UIntList() << type), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotSMStream(QDBusPendingCallWatcher*)));
}

PendingMediaContent::PendingMediaContent(const StreamedMediaChannelPtr &channel,
        const QString &errorName, const QString &errorMessage)
    : PendingOperation(channel.data()),
      mPriv(0)
{
    setFinishedWithError(errorName, errorMessage);
}

PendingMediaContent::~PendingMediaContent()
{
    delete mPriv;
}

MediaContentPtr PendingMediaContent::content() const
{
    if (!isFinished() || !isValid()) {
        return MediaContentPtr();
    }

    return mPriv->content;
}

void PendingMediaContent::gotSMStream(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia.RequestStreams failed with" <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    MediaStreamInfoList streamInfoList = reply.value();
    Q_ASSERT(streamInfoList.size() == 1);
    MediaStreamInfo streamInfo = streamInfoList.first();
    StreamedMediaChannelPtr channel(mPriv->channel);
    MediaContentPtr content = channel->lookupContentBySMStreamId(
            streamInfo.identifier);
    if (!content) {
        content = channel->addContentForSMStream(streamInfo);
    } else {
        channel->onSMStreamDirectionChanged(streamInfo.identifier,
                streamInfo.direction, streamInfo.pendingSendFlags);
        channel->onSMStreamStateChanged(streamInfo.identifier,
                streamInfo.state);
    }

    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContentReady(Tp::PendingOperation *)));
    connect(channel.data(),
            SIGNAL(contentRemoved(const Tp::MediaContentPtr &)),
            SLOT(onContentRemoved(const Tp::MediaContentPtr &)));

    watcher->deleteLater();
}

void PendingMediaContent::onContentReady(PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    setFinished();
}

void PendingMediaContent::onContentRemoved(const MediaContentPtr &content)
{
    if (isFinished()) {
        return;
    }

    if (mPriv->content == content) {
        // the content was removed before becoming ready
        setFinishedWithError(TELEPATHY_ERROR_CANCELLED,
                "Content removed before ready");
    }
}

/* ====== MediaContent ====== */
struct TELEPATHY_QT4_NO_EXPORT MediaContent::Private
{
    Private(MediaContent *parent,
            const StreamedMediaChannelPtr &channel,
            const QString &name,
            const MediaStreamInfo &streamInfo);
    Private(MediaContent *parent,
            const StreamedMediaChannelPtr &channel,
            const QDBusObjectPath &objectPath);

    // SM specific methods
    static void introspectSMStream(Private *self);

    // Call specific methods
    static void introspectCallMainProperties(Private *self);

    MediaStreamPtr lookupStreamByCallObjectPath(
            const QDBusObjectPath &streamPath);

    // general methods
    void checkIntrospectionCompleted();

    void addStream(const MediaStreamPtr &stream);

    IfaceType ifaceType;
    MediaContent *parent;
    ReadinessHelper *readinessHelper;
    WeakPtr<StreamedMediaChannel> channel;
    QString name;
    uint type;
    uint creatorHandle;
    ContactPtr creator;

    MediaStreams incompleteStreams;
    MediaStreams streams;

    // SM specific fields
    MediaStreamInfo SMStreamInfo;

    // Call specific fields
    CallContentInterface *callBaseInterface;
    Client::DBus::PropertiesInterface *callPropertiesInterface;
    QDBusObjectPath callObjectPath;
};

MediaContent::Private::Private(MediaContent *parent,
        const StreamedMediaChannelPtr &channel,
        const QString &name,
        const MediaStreamInfo &streamInfo)
    : ifaceType(IfaceTypeStreamedMedia),
      parent(parent),
      readinessHelper(parent->readinessHelper()),
      channel(channel),
      name(name),
      type(streamInfo.type),
      creatorHandle(0),
      SMStreamInfo(streamInfo)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectSMStream,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureCore);
}

MediaContent::Private::Private(MediaContent *parent,
        const StreamedMediaChannelPtr &channel,
        const QDBusObjectPath &objectPath)
    : ifaceType(IfaceTypeCall),
      parent(parent),
      readinessHelper(parent->readinessHelper()),
      channel(channel),
      callBaseInterface(0),
      callPropertiesInterface(0),
      callObjectPath(objectPath)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCallMainProperties,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureCore);
}

void MediaContent::Private::introspectSMStream(MediaContent::Private *self)
{
    MediaStreamPtr stream = MediaStreamPtr(
            new MediaStream(MediaContentPtr(self->parent), self->SMStreamInfo));
    self->addStream(stream);
}

void MediaContent::Private::introspectCallMainProperties(
        MediaContent::Private *self)
{
    MediaContent *parent = self->parent;
    StreamedMediaChannelPtr channel = parent->channel();

    self->callBaseInterface = new CallContentInterface(
            channel->dbusConnection(), channel->busName(),
            self->callObjectPath.path(), parent);
    parent->connect(self->callBaseInterface,
            SIGNAL(StreamAdded(const QDBusObjectPath &)),
            SLOT(onCallStreamAdded(const QDBusObjectPath &)));
    parent->connect(self->callBaseInterface,
            SIGNAL(StreamRemoved(const QDBusObjectPath &)),
            SLOT(onCallStreamRemoved(const QDBusObjectPath &)));

    self->callPropertiesInterface = new Client::DBus::PropertiesInterface(
            channel->dbusConnection(), channel->busName(),
            self->callObjectPath.path(), parent);
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->callPropertiesInterface->GetAll(
                    TP_FUTURE_INTERFACE_CALL_CONTENT),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotCallMainProperties(QDBusPendingCallWatcher *)));
}

MediaStreamPtr MediaContent::Private::lookupStreamByCallObjectPath(
        const QDBusObjectPath &streamPath)
{
    foreach (const MediaStreamPtr &stream, streams) {
        if (stream->callObjectPath() == streamPath) {
            return stream;
        }
    }
    foreach (const MediaStreamPtr &stream, incompleteStreams) {
        if (stream->callObjectPath() == streamPath) {
            return stream;
        }
    }
    return MediaStreamPtr();
}

void MediaContent::Private::checkIntrospectionCompleted()
{
    if (!parent->isReady(FeatureCore) &&
        incompleteStreams.size() == 0 &&
        ((creatorHandle != 0 && creator) || (creatorHandle == 0))) {
        readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

void MediaContent::Private::addStream(const MediaStreamPtr &stream)
{
    incompleteStreams.append(stream);
    parent->connect(stream->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onStreamReady(Tp::PendingOperation*)));
}

const Feature MediaContent::FeatureCore = Feature(MediaStream::staticMetaObject.className(), 0);

MediaContent::MediaContent(const StreamedMediaChannelPtr &channel,
        const QString &name,
        const MediaStreamInfo &streamInfo)
    : QObject(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, channel, name, streamInfo))
{
}

MediaContent::MediaContent(const StreamedMediaChannelPtr &channel,
        const QDBusObjectPath &objectPath)
    : QObject(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, channel, objectPath))
{
}

MediaContent::~MediaContent()
{
    delete mPriv;
}

StreamedMediaChannelPtr MediaContent::channel() const
{
    return StreamedMediaChannelPtr(mPriv->channel);
}

/**
 * Return the content name.
 *
 * \return The content name.
 */
QString MediaContent::name() const
{
    return mPriv->name;
}

/**
 * Return the content type.
 *
 * \return The content type.
 */
MediaStreamType MediaContent::type() const
{
    return (MediaStreamType) mPriv->type;
}

/**
 * Return the content creator.
 *
 * \return The content creator.
 */
ContactPtr MediaContent::creator() const
{
    // For SM Streams creator will always return ContactPtr(0)
    return mPriv->creator;
}

/**
 * Return the content streams.
 *
 * \return A list of content streams.
 */
MediaStreams MediaContent::streams() const
{
    return mPriv->streams;
}

void MediaContent::onStreamReady(Tp::PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    MediaStreamPtr stream = MediaStreamPtr(
            qobject_cast<MediaStream*>(pr->object()));

    if (op->isError() || !mPriv->incompleteStreams.contains(stream)) {
        mPriv->incompleteStreams.removeOne(stream);
        mPriv->checkIntrospectionCompleted();
        return;
    }

    mPriv->incompleteStreams.removeOne(stream);
    mPriv->streams.append(stream);

    if (isReady(FeatureCore)) {
        emit streamAdded(stream);
    }

    mPriv->checkIntrospectionCompleted();
}

void MediaContent::gotCreator(PendingOperation *op)
{
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);

    if (pending->isValid()) {
        Q_ASSERT(pending->contacts().size() == 1);
        mPriv->creator = pending->contacts()[0];

    } else {
        warning().nospace() << "Getting creator failed with " <<
            pending->errorName() << ":" << pending->errorMessage() <<
            ", ignoring";
        mPriv->creatorHandle = 0;
    }

    mPriv->checkIntrospectionCompleted();
}

void MediaContent::gotCallMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Properties.GetAll(Call.Content) failed with" <<
            reply.error().name() << ": " << reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to Properties.GetAll(Call.Content)";

    QVariantMap props = reply.value();
    mPriv->name = qdbus_cast<QString>(props["Name"]);
    mPriv->type = qdbus_cast<uint>(props["Type"]);
    mPriv->creatorHandle = qdbus_cast<uint>(props["Creator"]);
    ObjectPathList streamsPaths = qdbus_cast<ObjectPathList>(props["Streams"]);

    if (streamsPaths.size() == 0 && mPriv->creatorHandle == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }

    foreach (const QDBusObjectPath &streamPath, streamsPaths) {
        MediaStreamPtr stream = mPriv->lookupStreamByCallObjectPath(streamPath);
        if (!stream) {
            MediaStreamPtr stream = MediaStreamPtr(
                    new MediaStream(MediaContentPtr(this), streamPath));
            mPriv->addStream(stream);
        }
    }

    if (mPriv->creatorHandle != 0) {
        ContactManager *contactManager =
            channel()->connection()->contactManager();
        PendingContacts *contacts = contactManager->contactsForHandles(
                UIntList() << mPriv->creatorHandle);
        connect(contacts,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(gotCreator(Tp::PendingOperation *)));
    }

    watcher->deleteLater();
}

void MediaContent::onCallStreamAdded(const QDBusObjectPath &streamPath)
{
    if (mPriv->lookupStreamByCallObjectPath(streamPath)) {
        debug() << "Received Call.Content.StreamAdded for an existing "
            "stream, ignoring";
        return;
    }

    MediaStreamPtr stream = MediaStreamPtr(
        new MediaStream(MediaContentPtr(this), streamPath));
    mPriv->addStream(stream);
}

void MediaContent::onCallStreamRemoved(const QDBusObjectPath &streamPath)
{
    debug() << "Received Call.Content.StreamRemoved for stream" <<
        streamPath.path();

    MediaStreamPtr stream = mPriv->lookupStreamByCallObjectPath(streamPath);
    if (!stream) {
        return;
    }
    bool incomplete = mPriv->incompleteStreams.contains(stream);
    if (incomplete) {
        mPriv->incompleteStreams.removeOne(stream);
    } else {
        mPriv->streams.removeOne(stream);
    }

    if (isReady(FeatureCore) && !incomplete) {
        emit streamRemoved(stream);
    }

    mPriv->checkIntrospectionCompleted();
}

MediaStreamPtr MediaContent::SMStream() const
{
    Q_ASSERT(mPriv->ifaceType == IfaceTypeStreamedMedia);
    if (mPriv->streams.size() == 1) {
        return mPriv->streams.first();
    }
    return MediaStreamPtr();
}

void MediaContent::removeSMStream()
{
    Q_ASSERT(mPriv->ifaceType == IfaceTypeStreamedMedia);
    Q_ASSERT(mPriv->streams.size() == 1);
    MediaStreamPtr stream = mPriv->streams.first();
    mPriv->streams.removeOne(stream);
    emit streamRemoved(stream);
}

QDBusObjectPath MediaContent::callObjectPath() const
{
    return mPriv->callObjectPath;
}

/* ====== StreamedMediaChannel ====== */
struct TELEPATHY_QT4_NO_EXPORT StreamedMediaChannel::Private
{
    Private(StreamedMediaChannel *parent);
    ~Private();

    static void introspectContents(Private *self);
    void introspectSMStreams();
    void introspectCallContents();

    static void introspectLocalHoldState(Private *self);

    inline ChannelTypeCallInterface *callInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return parent->typeInterface<ChannelTypeCallInterface>(check);
    }

    // Public object
    StreamedMediaChannel *parent;

    ReadinessHelper *readinessHelper;

    IfaceType ifaceType;

    // Introspection

    MediaContents incompleteContents;
    MediaContents contents;

    LocalHoldState localHoldState;
    LocalHoldStateReason localHoldStateReason;
};

StreamedMediaChannel::Private::Private(StreamedMediaChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      localHoldState(LocalHoldStateUnheld),
      localHoldStateReason(LocalHoldStateReasonNone)
{
    QString channelType = parent->immutableProperties().value(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
    if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) {
        ifaceType = IfaceTypeStreamedMedia;
    } else {
        ifaceType = IfaceTypeCall;
    }

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableContents(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectContents,
        this);
    introspectables[FeatureContents] = introspectableContents;

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

void StreamedMediaChannel::Private::introspectContents(StreamedMediaChannel::Private *self)
{
    if (self->ifaceType == IfaceTypeStreamedMedia) {
        self->introspectSMStreams();
    } else {
        self->introspectCallContents();
        return;
    }
}

void StreamedMediaChannel::Private::introspectSMStreams()
{
    Client::ChannelTypeStreamedMediaInterface *streamedMediaInterface =
        parent->streamedMediaInterface();

    parent->connect(streamedMediaInterface,
            SIGNAL(StreamAdded(uint, uint, uint)),
            SLOT(onSMStreamAdded(uint, uint, uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamRemoved(uint)),
            SLOT(onSMStreamRemoved(uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamDirectionChanged(uint, uint, uint)),
            SLOT(onSMStreamDirectionChanged(uint, uint, uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamStateChanged(uint, uint)),
            SLOT(onSMStreamStateChanged(uint, uint)));
    parent->connect(streamedMediaInterface,
            SIGNAL(StreamError(uint, uint, const QString &)),
            SLOT(onSMStreamError(uint, uint, const QString &)));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            streamedMediaInterface->ListStreams(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotSMStreams(QDBusPendingCallWatcher *)));
}

void StreamedMediaChannel::Private::introspectCallContents()
{
    parent->connect(callInterface(),
            SIGNAL(ContentAdded(const QDBusObjectPath &, uint)),
            SLOT(onCallContentAdded(const QDBusObjectPath &, uint)));
    parent->connect(callInterface(),
            SIGNAL(ContentRemoved(const QDBusObjectPath &)),
            SLOT(onCallContentRemoved(const QDBusObjectPath &)));

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                parent->propertiesInterface()->GetAll(
                    TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotCallMainProperties(QDBusPendingCallWatcher *)));
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

const Feature StreamedMediaChannel::FeatureContents = Feature(StreamedMediaChannel::staticMetaObject.className(), 0);
const Feature StreamedMediaChannel::FeatureLocalHoldState = Feature(StreamedMediaChannel::staticMetaObject.className(), 1);
const Feature StreamedMediaChannel::FeatureStreams = FeatureContents;

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
    MediaStreams ret;
    foreach (const MediaContentPtr &content, mPriv->contents) {
        ret << content->streams();
    }
    return ret;
}

MediaStreams StreamedMediaChannel::streamsForType(MediaStreamType type) const
{
    MediaStreams ret;
    foreach (const MediaContentPtr &content, mPriv->contents) {
        if (content->type() == type) {
            ret << content->streams();
        }
    }
    return ret;
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
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return groupAddSelfHandle();
    } else {
        // TODO add Call iface support
        return NULL;
    }
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
    if (!stream) {
        return new PendingFailure(TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Unable to remove a null stream", this);
    }

    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        // StreamedMedia.RemoveStreams will trigger StreamedMedia.StreamRemoved
        // that will proper remove the content
        return new PendingVoid(
                streamedMediaInterface()->RemoveStreams(
                    UIntList() << stream->id()),
                this);
    } else {
        // TODO add Call iface support once RemoveContent is implement
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Removing streams is not supported", this);
    }
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
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        UIntList ids;
        foreach (const MediaStreamPtr &stream, streams) {
            if (!stream) {
                continue;
            }
            ids << stream->id();
        }

        if (ids.isEmpty()) {
            return new PendingFailure(TELEPATHY_ERROR_INVALID_ARGUMENT,
                    "Unable to remove invalid streams", this);
        }

        return new PendingVoid(
                streamedMediaInterface()->RemoveStreams(ids),
                this);
    } else {
        // TODO add Call iface support once RemoveContent is implement
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Removing streams is not supported", this);
    }
}

PendingMediaStreams *StreamedMediaChannel::requestStream(
        const ContactPtr &contact,
        MediaStreamType type)
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return new PendingMediaStreams(StreamedMediaChannelPtr(this),
                contact,
                QList<MediaStreamType>() << type);
    } else {
        // TODO add Call iface support
        return NULL;
    }
}

PendingMediaStreams *StreamedMediaChannel::requestStreams(
        const ContactPtr &contact,
        QList<MediaStreamType> types)
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return new PendingMediaStreams(StreamedMediaChannelPtr(this),
                contact, types);
    } else {
        // TODO add Call iface support
        return NULL;
    }
}

PendingOperation *StreamedMediaChannel::hangupCall()
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return requestClose();
    } else {
        // TODO add Call iface support
        return NULL;
    }
}

/**
 * Return a list of contents in this channel. This list is empty unless
 * the FeatureContents Feature has been enabled.
 *
 * Contents are added to the list when they are received; the contentAdded
 * signal is emitted.
 *
 * \return The contents in this channel.
 */
MediaContents StreamedMediaChannel::contents() const
{
    return mPriv->contents;
}

MediaContents StreamedMediaChannel::contentsForType(MediaStreamType type) const
{
    MediaContents contents;
    foreach (const MediaContentPtr &content, mPriv->contents) {
        if (content->type() == type) {
            contents.append(content);
        }
    }
    return contents;
}

PendingMediaContent *StreamedMediaChannel::requestContent(
        const QString &name,
        MediaStreamType type)
{
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        // get the first contact whose this channel is with. The contact is
        // either in groupContacts or groupRemotePendingContacts
        ContactPtr otherContact;
        foreach (const ContactPtr &contact, groupContacts()) {
            if (contact != groupSelfContact()) {
                otherContact = contact;
                break;
            }
        }

        if (!otherContact) {
            otherContact = *(groupRemotePendingContacts().begin());
        }

        return new PendingMediaContent(StreamedMediaChannelPtr(this),
            otherContact, name, type);
    } else {
        // TODO add Call iface support
        return NULL;
    }
}

/**
 * Remove the specified content from this channel.
 *
 * \param content Content to remove.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *StreamedMediaChannel::removeContent(const MediaContentPtr &content)
{
    if (!content) {
        return new PendingFailure(TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Unable to remove a null content", this);
    }

    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        // StreamedMedia.RemoveStreams will trigger StreamedMedia.StreamRemoved
        // that will proper remove the content
        MediaStreamPtr stream = content->SMStream();
        return new PendingVoid(
                streamedMediaInterface()->RemoveStreams(
                    UIntList() << stream->id()),
            this);
    } else {
        // TODO add Call iface support
        return NULL;
    }
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
    if (mPriv->ifaceType == IfaceTypeStreamedMedia) {
        return interfaces().contains(
                TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING);
    } else {
        // TODO add Call iface support
        return false;
    }
}

/**
 * Return whether the local user has placed this channel on hold.
 *
 * This method requires StreamedMediaChannel::FeatureHoldState to be enabled.
 *
 * \return The channel local hold state.
 * \sa requestHold()
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
 * Return the reason why localHoldState() changed to its current value.
 *
 * This method requires StreamedMediaChannel::FeatureLocalHoldState to be enabled.
 *
 * \return The channel local hold state reason.
 * \sa requestHold()
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
PendingOperation *StreamedMediaChannel::requestHold(bool hold)
{
    if (!interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_HOLD)) {
        warning() << "StreamedMediaChannel::requestHold() used with no hold interface";
        return new PendingFailure(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "StreamedMediaChannel does not support hold interface",
                this);
    }
    return new PendingVoid(holdInterface()->RequestHold(hold), this);
}

void StreamedMediaChannel::onContentReady(PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    MediaContentPtr content = MediaContentPtr(
            qobject_cast<MediaContent*>(pr->object()));

    if (op->isError()) {
        mPriv->incompleteContents.removeOne(content);
        if (!isReady(FeatureContents) && mPriv->incompleteContents.size() == 0) {
            // let's not fail because a content could not become ready
            mPriv->readinessHelper->setIntrospectCompleted(
                    FeatureContents, true);
        }
        return;
    }

    // the content was removed before become ready
    if (!mPriv->incompleteContents.contains(content)) {
        if (!isReady(FeatureContents) &&
            mPriv->incompleteContents.size() == 0) {
            mPriv->readinessHelper->setIntrospectCompleted(
                    FeatureContents, true);
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

void StreamedMediaChannel::gotSMStreams(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<MediaStreamInfoList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "StreamedMedia.ListStreams failed with" <<
            reply.error().name() << ": " << reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to StreamedMedia::ListStreams()";

    MediaStreamInfoList streamInfoList = reply.value();
    if (streamInfoList.size() > 0) {
        foreach (const MediaStreamInfo &streamInfo, streamInfoList) {
            MediaContentPtr content = lookupContentBySMStreamId(
                    streamInfo.identifier);
            if (!content) {
                addContentForSMStream(streamInfo);
            } else {
                onSMStreamDirectionChanged(streamInfo.identifier,
                        streamInfo.direction, streamInfo.pendingSendFlags);
                onSMStreamStateChanged(streamInfo.identifier,
                        streamInfo.state);
            }
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }

    watcher->deleteLater();
}

void StreamedMediaChannel::onSMStreamAdded(uint streamId,
        uint contactHandle, uint streamType)
{
    if (lookupContentBySMStreamId(streamId)) {
        debug() << "Received StreamedMedia.StreamAdded for an existing "
            "stream, ignoring";
        return;
    }

    MediaStreamInfo streamInfo = {
        streamId,
        contactHandle,
        streamType,
        MediaStreamStateDisconnected,
        MediaStreamDirectionNone,
        0
    };
    addContentForSMStream(streamInfo);
}

void StreamedMediaChannel::onSMStreamRemoved(uint streamId)
{
    debug() << "Received StreamedMedia.StreamRemoved for stream" <<
        streamId;

    MediaContentPtr content = lookupContentBySMStreamId(streamId);
    if (!content) {
        return;
    }
    bool incomplete = mPriv->incompleteContents.contains(content);
    if (incomplete) {
        mPriv->incompleteContents.removeOne(content);
    } else {
        mPriv->contents.removeOne(content);
    }

    if (isReady(FeatureContents) && !incomplete) {
        // fake stream removal then content removal
        content->removeSMStream();
        emit contentRemoved(content);
    }

    // the content was added/removed before become ready
    if (!isReady(FeatureContents) &&
        mPriv->contents.size() == 0 &&
        mPriv->incompleteContents.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }
}

void StreamedMediaChannel::onSMStreamDirectionChanged(uint streamId,
        uint streamDirection, uint streamPendingFlags)
{
    debug() << "Received StreamedMedia.StreamDirectionChanged for stream" <<
        streamId << "with direction changed to" << streamDirection;

    MediaContentPtr content = lookupContentBySMStreamId(streamId);
    if (!content) {
        return;
    }

    MediaStreamPtr stream = content->SMStream();

    uint oldDirection = stream->direction();
    uint oldPendingFlags = stream->pendingSend();

    stream->gotSMDirection(streamDirection, streamPendingFlags);

    if (oldDirection != streamDirection ||
        oldPendingFlags != streamPendingFlags) {
        emit streamDirectionChanged(stream,
                (MediaStreamDirection) streamDirection,
                (MediaStreamPendingSend) streamPendingFlags);
    }
}

void StreamedMediaChannel::onSMStreamStateChanged(uint streamId,
        uint streamState)
{
    debug() << "Received StreamedMedia.StreamStateChanged for stream" <<
        streamId << "with state changed to" << streamState;

    MediaContentPtr content = lookupContentBySMStreamId(streamId);
    if (!content) {
        return;
    }

    MediaStreamPtr stream = content->SMStream();

    uint oldState = stream->state();

    stream->gotSMStreamState(streamState);

    if (oldState != streamState) {
        emit streamStateChanged(stream, (MediaStreamState) streamState);
    }
}

void StreamedMediaChannel::onSMStreamError(uint streamId,
        uint errorCode, const QString &errorMessage)
{
    debug() << "Received StreamedMedia.StreamError for stream" <<
        streamId << "with error code" << errorCode <<
        "and message:" << errorMessage;

    MediaContentPtr content = lookupContentBySMStreamId(streamId);
    if (!content) {
        return;
    }

    MediaStreamPtr stream = content->SMStream();
    emit streamError(stream, (MediaStreamError) errorCode, errorMessage);
}

void StreamedMediaChannel::gotCallMainProperties(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Properties.GetAll(Call) failed with" <<
            reply.error().name() << ": " << reply.error().message();

        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to Properties.GetAll(Call)";

    QVariantMap props = reply.value();
    ObjectPathList contentsPaths = qdbus_cast<ObjectPathList>(props["Contents"]);
    if (contentsPaths.size() > 0) {
        foreach (const QDBusObjectPath &contentPath, contentsPaths) {
            MediaContentPtr content = lookupContentByCallObjectPath(
                    contentPath);
            if (!content) {
                addContentForCallObjectPath(contentPath);
            }
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }

    watcher->deleteLater();
}

void StreamedMediaChannel::onCallContentAdded(
        const QDBusObjectPath &contentPath,
        uint contentType)
{
    if (lookupContentByCallObjectPath(contentPath)) {
        debug() << "Received Call.ContentAdded for an existing "
            "content, ignoring";
        return;
    }

    addContentForCallObjectPath(contentPath);
}

void StreamedMediaChannel::onCallContentRemoved(
        const QDBusObjectPath &contentPath)
{
    debug() << "Received Call.ContentRemoved for content" <<
        contentPath.path();

    MediaContentPtr content = lookupContentByCallObjectPath(contentPath);
    if (!content) {
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

MediaContentPtr StreamedMediaChannel::addContentForSMStream(
        const MediaStreamInfo &streamInfo)
{
    /* Simulate content creation. For SM channels each stream will have one
     * fake content */
    MediaContentPtr content = MediaContentPtr(
            new MediaContent(StreamedMediaChannelPtr(this),
                QString("%1 %2")
                    .arg(streamInfo.type == MediaStreamTypeAudio ? "audio" : "video")
                    .arg((qulonglong) this),
                streamInfo));

    /* Forward MediaContent::streamAdded/Removed signals */
    connect(content.data(),
            SIGNAL(streamAdded(const Tp::MediaStreamPtr &)),
            SIGNAL(streamAdded(const Tp::MediaStreamPtr &)));
    connect(content.data(),
            SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)),
            SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)));

    mPriv->incompleteContents.append(content);
    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContentReady(Tp::PendingOperation*)));
    return content;
}

MediaContentPtr StreamedMediaChannel::lookupContentBySMStreamId(uint streamId)
{
    foreach (const MediaContentPtr &content, mPriv->contents) {
        if (content->SMStream()->id() == streamId) {
            return content;
        }
    }
    foreach (const MediaContentPtr &content, mPriv->incompleteContents) {
        if (content->SMStream() && content->SMStream()->id() == streamId) {
            return content;
        }
    }
    return MediaContentPtr();
}

MediaContentPtr StreamedMediaChannel::addContentForCallObjectPath(
        const QDBusObjectPath &contentPath)
{
    /* Simulate content creation. For SM channels each stream will have one
     * fake content */
    MediaContentPtr content = MediaContentPtr(
            new MediaContent(StreamedMediaChannelPtr(this),
                contentPath));

    /* Forward MediaContent::streamAdded/Removed signals */
    connect(content.data(),
            SIGNAL(streamAdded(const Tp::MediaStreamPtr &)),
            SIGNAL(streamAdded(const Tp::MediaStreamPtr &)));
    connect(content.data(),
            SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)),
            SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)));

    mPriv->incompleteContents.append(content);
    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContentReady(Tp::PendingOperation*)));
    return content;
}

MediaContentPtr StreamedMediaChannel::lookupContentByCallObjectPath(
        const QDBusObjectPath &contentPath)
{
    foreach (const MediaContentPtr &content, mPriv->contents) {
        if (content->callObjectPath() == contentPath) {
            return content;
        }
    }
    foreach (const MediaContentPtr &content, mPriv->incompleteContents) {
        if (content->callObjectPath() == contentPath) {
            return content;
        }
    }
    return MediaContentPtr();
}

} // Tp
