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

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/PendingOperation>

namespace Telepathy
{
namespace Client
{

class StreamedMediaChannel;
class MediaStream;

typedef QList<QSharedPointer<MediaStream> > MediaStreams;

class PendingMediaStreams : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingMediaStreams)

public:
    ~PendingMediaStreams();

    MediaStreams streams() const;

private Q_SLOTS:
    void gotStreams(QDBusPendingCallWatcher *);
    void gotContacts(Telepathy::Client::PendingOperation *);

private:
    friend class StreamedMediaChannel;

    PendingMediaStreams(StreamedMediaChannel *channel,
            QSharedPointer<Telepathy::Client::Contact> contact,
            QList<Telepathy::MediaStreamType> types,
            QObject *parent = 0);
    PendingMediaStreams(StreamedMediaChannel *channel,
            const MediaStreams &streams,
            QObject *parent = 0);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class MediaStream : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MediaStream)

public:
    ~MediaStream();

    StreamedMediaChannel *channel() const;
    uint id() const;

    QSharedPointer<Contact> contact() const;
    Telepathy::MediaStreamState state() const;
    Telepathy::MediaStreamType type() const;

    bool sending() const;
    bool receiving() const;
    bool localSendingRequested() const;
    bool remoteSendingRequested() const;

    Telepathy::MediaStreamDirection direction() const;
    Telepathy::MediaStreamPendingSend pendingSend() const;

    PendingOperation *remove();
    PendingOperation *requestStreamDirection(
            Telepathy::MediaStreamDirection direction);
    PendingOperation *requestStreamDirection(
            bool send, bool receive);

Q_SIGNALS:
    void removed(Telepathy::Client::MediaStream *stream);
    void directionChanged(Telepathy::Client::MediaStream *stream,
            Telepathy::MediaStreamDirection direction,
            Telepathy::MediaStreamPendingSend pendingSend);
    void stateChanged(Telepathy::Client::MediaStream *stream,
            Telepathy::MediaStreamState);
    void error(Telepathy::Client::MediaStream *stream,
            Telepathy::MediaStreamError errorCode,
            const QString &errorMessage);

private:
    friend class PendingMediaStreams;
    friend class StreamedMediaChannel;

    MediaStream(StreamedMediaChannel *channel, uint id,
            uint contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend);

    uint contactHandle() const;
    void setContact(const QSharedPointer<Contact> &contact);
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

    StreamedMediaChannel(Connection *connection, const QString &objectPath,
            const QVariantMap &immutableProperties, QObject *parent = 0);
    ~StreamedMediaChannel();

    MediaStreams streams() const;

    bool awaitingLocalAnswer() const;
    bool awaitingRemoteAnswer() const;

    PendingOperation *acceptCall();

    PendingOperation *removeStreams(MediaStreams streams);
    PendingOperation *removeStreams(const Telepathy::UIntList &ids);

    PendingMediaStreams *requestStreams(
            QSharedPointer<Telepathy::Client::Contact> contact,
            QList<Telepathy::MediaStreamType> types);

Q_SIGNALS:
    void streamAdded(const QSharedPointer<Telepathy::Client::MediaStream> &stream);

private Q_SLOTS:
    void gotStreams(QDBusPendingCallWatcher *);
    void onStreamsReady(Telepathy::Client::PendingOperation *);
    void onNewStreamReady(Telepathy::Client::PendingOperation *);
    void onStreamAdded(uint, uint, uint);
    void onStreamRemoved(uint);
    void onStreamDirectionChanged(uint, uint, uint);
    void onStreamStateChanged(uint, uint);
    void onStreamError(uint, uint, const QString &);

private:
    friend class PendingMediaStreams;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

typedef QExplicitlySharedDataPointer<StreamedMediaChannel> StreamedMediaChannelPtr;

} // Telepathy::Client
} // Telepathy

#endif
