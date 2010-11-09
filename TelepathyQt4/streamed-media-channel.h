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

#ifndef _TelepathyQt4_streamed_media_channel_h_HEADER_GUARD_
#define _TelepathyQt4_streamed_media_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Object>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

namespace Tp
{

class StreamedMediaChannel;

typedef QList<StreamedMediaStreamPtr> StreamedMediaStreams;

class TELEPATHY_QT4_EXPORT PendingStreamedMediaStreams : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingStreamedMediaStreams)

public:
    ~PendingStreamedMediaStreams();

    StreamedMediaStreams streams() const;

private Q_SLOTS:
    void gotStreams(QDBusPendingCallWatcher *op);

    void onStreamRemoved(const Tp::StreamedMediaStreamPtr &stream);
    void onStreamReady(Tp::PendingOperation *op);

private:
    friend class StreamedMediaChannel;

    PendingStreamedMediaStreams(const StreamedMediaChannelPtr &channel,
            const ContactPtr &contact,
            const QList<MediaStreamType> &types);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT StreamedMediaStream : public Object, private ReadyObject
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamedMediaStream)

public:
    enum SendingState {
        SendingStateNone = 0,
        SendingStatePendingSend = 1,
        SendingStateSending = 2
    };

    ~StreamedMediaStream();

    StreamedMediaChannelPtr channel() const;

    uint id() const;

    ContactPtr contact() const;

    MediaStreamState state() const;
    MediaStreamType type() const;

    SendingState localSendingState() const;
    SendingState remoteSendingState() const;

    bool sending() const;
    bool receiving() const;

    bool localSendingRequested() const;
    bool remoteSendingRequested() const;

    MediaStreamDirection direction() const;
    MediaStreamPendingSend pendingSend() const;

    PendingOperation *requestSending(bool send);
    PendingOperation *requestReceiving(bool receive);

    PendingOperation *requestDirection(MediaStreamDirection direction);
    PendingOperation *requestDirection(bool send, bool receive);

    PendingOperation *startDTMFTone(DTMFEvent event);
    PendingOperation *stopDTMFTone();

Q_SIGNALS:
    void localSendingStateChanged(Tp::StreamedMediaStream::SendingState localSendingState);
    void remoteSendingStateChanged(Tp::StreamedMediaStream::SendingState remoteSendingState);

private Q_SLOTS:
    void gotContact(Tp::PendingOperation *op);

private:
    friend class PendingStreamedMediaStreams;
    friend class StreamedMediaChannel;

    static const Feature FeatureCore;

    StreamedMediaStream(const StreamedMediaChannelPtr &channel, const MediaStreamInfo &info);

    void gotDirection(uint direction, uint pendingSend);
    void gotStreamState(uint state);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT StreamedMediaChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamedMediaChannel)
    Q_ENUMS(StateChangeReason)

public:
    static const Feature FeatureStreams;
    static const Feature FeatureLocalHoldState;

    enum StateChangeReason {
        StateChangeReasonUnknown = 0,
        StateChangeReasonUserRequested = 1
    };

    static StreamedMediaChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~StreamedMediaChannel();

    bool awaitingLocalAnswer() const;
    bool awaitingRemoteAnswer() const;

    PendingOperation *acceptCall();
    PendingOperation *hangupCall();

    StreamedMediaStreams streams() const;
    StreamedMediaStreams streamsForType(MediaStreamType type) const;

    PendingStreamedMediaStreams *requestStream(const ContactPtr &contact, MediaStreamType type);
    PendingStreamedMediaStreams *requestStreams(const ContactPtr &contact, QList<MediaStreamType> types);

    PendingOperation *removeStream(const StreamedMediaStreamPtr &stream);
    PendingOperation *removeStreams(const StreamedMediaStreams &streams);

    bool handlerStreamingRequired() const;

    LocalHoldState localHoldState() const;
    LocalHoldStateReason localHoldStateReason() const;
    PendingOperation *requestHold(bool hold);

Q_SIGNALS:
    void streamAdded(const Tp::StreamedMediaStreamPtr &stream);
    void streamRemoved(const Tp::StreamedMediaStreamPtr &stream);
    void streamDirectionChanged(const Tp::StreamedMediaStreamPtr &stream,
            Tp::MediaStreamDirection direction,
            Tp::MediaStreamPendingSend pendingSend);
    void streamStateChanged(const Tp::StreamedMediaStreamPtr &stream,
            Tp::MediaStreamState state);
    void streamError(const Tp::StreamedMediaStreamPtr &stream,
            Tp::MediaStreamError errorCode,
            const QString &errorMessage);

    void localHoldStateChanged(Tp::LocalHoldState state,
            Tp::LocalHoldStateReason reason);

protected:
    StreamedMediaChannel(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

private Q_SLOTS:
    void onStreamReady(Tp::PendingOperation *op);

    void gotStreams(QDBusPendingCallWatcher *);
    void onStreamAdded(uint, uint, uint);
    void onStreamRemoved(uint);
    void onStreamDirectionChanged(uint, uint, uint);
    void onStreamStateChanged(uint streamId, uint streamState);
    void onStreamError(uint, uint, const QString &);

    void gotLocalHoldState(QDBusPendingCallWatcher *);
    void onLocalHoldStateChanged(uint, uint);

private:
    friend class PendingStreamedMediaStreams;

    StreamedMediaStreamPtr addStream(const MediaStreamInfo &streamInfo);
    StreamedMediaStreamPtr lookupStreamById(uint streamId);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
