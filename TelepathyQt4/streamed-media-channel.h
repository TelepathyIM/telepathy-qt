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

#ifndef _TelepathyQt4_Client_streamed_media_channel_h_HEADER_GUARD_
#define _TelepathyQt4_Client_streamed_media_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

namespace Telepathy
{
namespace Client
{

class StreamedMediaChannel;

typedef QList<MediaStreamPtr> MediaStreams;

class PendingMediaStreams : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingMediaStreams)

public:
    ~PendingMediaStreams();

    MediaStreams streams() const;

private Q_SLOTS:
    void gotStreams(QDBusPendingCallWatcher *);
    void onStreamRemoved(Telepathy::Client::MediaStreamPtr);
    void onStreamReady(Telepathy::Client::PendingOperation *);

private:
    friend class StreamedMediaChannel;

    PendingMediaStreams(const StreamedMediaChannelPtr &channel,
            ContactPtr contact,
            QList<Telepathy::MediaStreamType> types);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class MediaStream : public QObject,
                    private ReadyObject,
                    public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(MediaStream)

public:
    ~MediaStream();

    StreamedMediaChannelPtr channel() const;
    uint id() const;

    ContactPtr contact() const;
    Telepathy::MediaStreamState state() const;
    Telepathy::MediaStreamType type() const;

    bool sending() const;
    bool receiving() const;
    bool localSendingRequested() const;
    bool remoteSendingRequested() const;

    Telepathy::MediaStreamDirection direction() const;
    Telepathy::MediaStreamPendingSend pendingSend() const;

    PendingOperation *requestDirection(
            Telepathy::MediaStreamDirection direction);
    PendingOperation *requestDirection(
            bool send, bool receive);

private Q_SLOTS:
    void gotContact(Telepathy::Client::PendingOperation *op);

private:
    friend class PendingMediaStreams;
    friend class StreamedMediaChannel;

    static const Feature FeatureContact;

    MediaStream(const StreamedMediaChannelPtr &channel, uint id,
            uint contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend);

    uint contactHandle() const;
    void setContact(const ContactPtr &contact);
    void setDirection(Telepathy::MediaStreamDirection direction,
            Telepathy::MediaStreamPendingSend pendingSend);
    void setState(Telepathy::MediaStreamState state);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class StreamedMediaChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamedMediaChannel)

public:
    static const Feature FeatureStreams;

    static StreamedMediaChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    ~StreamedMediaChannel();

    MediaStreams streams() const;
    MediaStreams streamsForType(Telepathy::MediaStreamType type) const;

    bool awaitingLocalAnswer() const;
    bool awaitingRemoteAnswer() const;

    PendingOperation *acceptCall();

    PendingOperation *removeStream(const MediaStreamPtr &stream);
    PendingOperation *removeStreams(const MediaStreams &streams);

    PendingMediaStreams *requestStream(
            const ContactPtr &contact,
            Telepathy::MediaStreamType type);
    PendingMediaStreams *requestStreams(
            const ContactPtr &contact,
            QList<Telepathy::MediaStreamType> types);

Q_SIGNALS:
    void streamAdded(const Telepathy::Client::MediaStreamPtr &stream);
    void streamRemoved(const Telepathy::Client::MediaStreamPtr &stream);
    void streamDirectionChanged(const Telepathy::Client::MediaStreamPtr &stream,
            Telepathy::MediaStreamDirection direction,
            Telepathy::MediaStreamPendingSend pendingSend);
    void streamStateChanged(const Telepathy::Client::MediaStreamPtr &stream,
            Telepathy::MediaStreamState);
    void streamError(const Telepathy::Client::MediaStreamPtr &stream,
            Telepathy::MediaStreamError errorCode,
            const QString &errorMessage);

protected:
    StreamedMediaChannel(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

private Q_SLOTS:
    void gotStreams(QDBusPendingCallWatcher *);
    void onStreamReady(Telepathy::Client::PendingOperation *);
    void onStreamAdded(uint, uint, uint);
    void onStreamRemoved(uint);
    void onStreamDirectionChanged(uint, uint, uint);
    void onStreamStateChanged(uint, uint);
    void onStreamError(uint, uint, const QString &);

private:
    friend class PendingMediaStreams;

    void addStream(const MediaStreamPtr &stream);
    MediaStreamPtr lookupStreamById(uint streamId);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
