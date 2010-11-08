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
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

namespace Tp
{

// TODO: (API/ABI break) split StreamedMediaChannel in CallChannel and StreamedMediaChannel

class StreamedMediaChannel;

typedef QList<MediaContentPtr> MediaContents;
typedef QList<MediaStreamPtr> MediaStreams;

// FIXME: (API/ABI break) Rename PendingMediaStreams to PendingStreamedMediaStreams
class TELEPATHY_QT4_EXPORT PendingMediaStreams : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingMediaStreams)

public:
    ~PendingMediaStreams();

    MediaStreams streams() const;

private Q_SLOTS:
    void gotSMStreams(QDBusPendingCallWatcher *);

    void gotCallContent(QDBusPendingCallWatcher *watcher);

    void onContentRemoved(const Tp::MediaContentPtr &);
    void onContentReady(Tp::PendingOperation *);

private:
    friend class StreamedMediaChannel;

    PendingMediaStreams(const StreamedMediaChannelPtr &channel,
            const ContactPtr &contact,
            const QList<MediaStreamType> &types);
    PendingMediaStreams(const StreamedMediaChannelPtr &channel,
            const QList<MediaStreamType> &types);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

// FIXME: (API/ABI break) Rename MediaStream to StreamedMediaStream
class TELEPATHY_QT4_EXPORT MediaStream : public QObject,
                    private ReadyObject,
                    public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(MediaStream)

public:
    enum SendingState {
        SendingStateNone = 0,
        SendingStatePendingSend = 1,
        SendingStateSending = 2
    };

    ~MediaStream();

    TELEPATHY_QT4_DEPRECATED MediaContentPtr content() const;

    StreamedMediaChannelPtr channel() const;

    uint id() const;

    TELEPATHY_QT4_DEPRECATED Contacts members() const;

    ContactPtr contact() const;

    MediaStreamState state() const;
    MediaStreamType type() const;

    SendingState localSendingState() const;
    SendingState remoteSendingState() const;
    TELEPATHY_QT4_DEPRECATED SendingState remoteSendingState(const ContactPtr &contact) const;

    bool sending() const;
    bool receiving() const;

    bool localSendingRequested() const;
    bool remoteSendingRequested() const;

    MediaStreamDirection direction() const;
    MediaStreamPendingSend pendingSend() const;

    PendingOperation *requestSending(bool send);
    PendingOperation *requestReceiving(bool receive);
    TELEPATHY_QT4_DEPRECATED PendingOperation *requestReceiving(const ContactPtr &contact, bool receive);

    PendingOperation *requestDirection(
            MediaStreamDirection direction);
    PendingOperation *requestDirection(
            bool send, bool receive);

    PendingOperation *startDTMFTone(DTMFEvent event);
    PendingOperation *stopDTMFTone();

Q_SIGNALS:
    void localSendingStateChanged(
            Tp::MediaStream::SendingState localSendingState);
    // FIXME: (API/ABI break) Remove remoteSendingStateChanged taking a QHash<Contact,SendingState>
    void remoteSendingStateChanged(
            const QHash<Tp::ContactPtr, Tp::MediaStream::SendingState> &remoteSendingStates);
    void remoteSendingStateChanged(Tp::MediaStream::SendingState remoteSendingState);

    // FIXME: (API/ABI break) Remove membersRemoved
    void membersRemoved(const Tp::Contacts &members);

protected:
    // FIXME: (API/ABI break) Remove connectNotify
    void connectNotify(const char *);

private Q_SLOTS:
    void gotSMContact(Tp::PendingOperation *op);

    void gotCallMainProperties(QDBusPendingCallWatcher *);
    void gotCallSendersContacts(Tp::PendingOperation *);

private:
    friend class MediaContent;
    friend class PendingMediaStreams;
    friend class StreamedMediaChannel;

    static const Feature FeatureCore;

    MediaStream(const MediaContentPtr &content, const MediaStreamInfo &info);
    MediaStream(const MediaContentPtr &content,
            const QDBusObjectPath &streamPath);

    void gotSMDirection(uint direction, uint pendingSend);
    void gotSMStreamState(uint state);

    QDBusObjectPath callObjectPath() const;

    MediaContentPtr _deprecated_content() const;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

// FIXME: (API/ABI break) Remove PendingMediaContent
class TELEPATHY_QT4_EXPORT PendingMediaContent : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingMediaContent)

public:
    ~PendingMediaContent();

    MediaContentPtr content() const;

private Q_SLOTS:
    void gotSMStream(QDBusPendingCallWatcher *watcher);

    void gotCallContent(QDBusPendingCallWatcher *watcher);

    void onContentReady(Tp::PendingOperation *op);
    void onContentRemoved(const Tp::MediaContentPtr &content);

private:
    friend class StreamedMediaChannel;

    PendingMediaContent(const StreamedMediaChannelPtr &channel,
            const ContactPtr &contact,
            const QString &contentName,
            MediaStreamType type);
    PendingMediaContent(const StreamedMediaChannelPtr &channel,
            const QString &contentName,
            MediaStreamType type);
    PendingMediaContent(const StreamedMediaChannelPtr &channel,
            const QString &errorName,
            const QString &errorMessage);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

// FIXME: (API/ABI break) Remove MediaContent
class TELEPATHY_QT4_EXPORT MediaContent : public QObject,
                    private ReadyObject,
                    public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(MediaContent)

public:
    ~MediaContent();

    StreamedMediaChannelPtr channel() const;

    QString name() const;
    MediaStreamType type() const;
    ContactPtr creator() const;

    MediaStreams streams() const;

Q_SIGNALS:
    void streamAdded(const Tp::MediaStreamPtr &stream);
    void streamRemoved(const Tp::MediaStreamPtr &stream);

private Q_SLOTS:
    void onStreamReady(Tp::PendingOperation *op);
    void gotCreator(Tp::PendingOperation *op);

    void gotCallMainProperties(QDBusPendingCallWatcher *);
    void onCallStreamAdded(const QDBusObjectPath &);
    void onCallStreamRemoved(const QDBusObjectPath &);

private:
    friend class StreamedMediaChannel;
    friend class PendingMediaContent;
    friend class PendingMediaStreams;

    static const Feature FeatureCore;

    MediaContent(const StreamedMediaChannelPtr &channel,
            const QString &name,
            const MediaStreamInfo &streamInfo);
    MediaContent(const StreamedMediaChannelPtr &channel,
            const QDBusObjectPath &contentPath);

    void setSMStream(const MediaStreamPtr &stream);
    MediaStreamPtr SMStream() const;
    void removeSMStream();

    QDBusObjectPath callObjectPath() const;
    PendingOperation *callRemove();

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
    static const Feature FeatureContents;
    static const Feature FeatureLocalHoldState;
    static const Feature FeatureStreams;

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
    TELEPATHY_QT4_DEPRECATED PendingOperation *hangupCall(StateChangeReason reason,
            const QString &detailedReason, const QString &message);

    TELEPATHY_QT4_DEPRECATED MediaContents contents() const;
    TELEPATHY_QT4_DEPRECATED MediaContents contentsForType(MediaStreamType type) const;

    MediaStreams streams() const;
    MediaStreams streamsForType(
            MediaStreamType type) const;

    TELEPATHY_QT4_DEPRECATED PendingMediaContent *requestContent(const QString &name,
            MediaStreamType type);

    PendingMediaStreams *requestStream(
            const ContactPtr &contact,
            MediaStreamType type);
    PendingMediaStreams *requestStreams(
            const ContactPtr &contact,
            QList<MediaStreamType> types);

    TELEPATHY_QT4_DEPRECATED PendingOperation *removeContent(const MediaContentPtr &content);

    PendingOperation *removeStream(
            const MediaStreamPtr &stream);
    PendingOperation *removeStreams(
            const MediaStreams &streams);

    bool handlerStreamingRequired() const;

    LocalHoldState localHoldState() const;
    LocalHoldStateReason localHoldStateReason() const;
    PendingOperation *requestHold(bool hold);

Q_SIGNALS:
    // FIXME: (API/ABI break) Remove contentAdded/Removed signals
    void contentAdded(const Tp::MediaContentPtr &content);
    void contentRemoved(const Tp::MediaContentPtr &content);

    void streamAdded(const Tp::MediaStreamPtr &stream);
    void streamRemoved(const Tp::MediaStreamPtr &stream);
    void streamDirectionChanged(const Tp::MediaStreamPtr &stream,
            Tp::MediaStreamDirection direction,
            Tp::MediaStreamPendingSend pendingSend);
    void streamStateChanged(const Tp::MediaStreamPtr &stream,
            Tp::MediaStreamState state);
    void streamError(const Tp::MediaStreamPtr &stream,
            Tp::MediaStreamError errorCode,
            const QString &errorMessage);

    void localHoldStateChanged(Tp::LocalHoldState state,
            Tp::LocalHoldStateReason reason);

protected:
    StreamedMediaChannel(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    // FIXME: (API/ABI break) Remove connectNotify
    void connectNotify(const char *);

private Q_SLOTS:
    void onContentReady(Tp::PendingOperation *op);

    void gotSMStreams(QDBusPendingCallWatcher *);
    void onSMStreamAdded(uint, uint, uint);
    void onSMStreamRemoved(uint);
    void onSMStreamDirectionChanged(uint, uint, uint);
    void onSMStreamStateChanged(uint streamId, uint streamState);
    void onSMStreamError(uint, uint, const QString &);

    void gotCallMainProperties(QDBusPendingCallWatcher *);
    void onCallContentAdded(const QDBusObjectPath &, uint);
    void onCallContentRemoved(const QDBusObjectPath &);

    void gotLocalHoldState(QDBusPendingCallWatcher *);
    void onLocalHoldStateChanged(uint, uint);

private:
    friend class PendingMediaContent;
    friend class PendingMediaStreams;

    MediaContentPtr addContentForSMStream(const MediaStreamInfo &streamInfo);
    MediaContentPtr lookupContentBySMStreamId(uint streamId);

    MediaContentPtr addContentForCallObjectPath(
            const QDBusObjectPath &contentPath);
    MediaContentPtr lookupContentByCallObjectPath(
            const QDBusObjectPath &contentPath);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
