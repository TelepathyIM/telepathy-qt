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

namespace Telepathy
{
namespace Client
{

class StreamedMediaChannel;

class MediaStream : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MediaStream)

#if 0
public:
    StreamedMediaChannel *channel() const;
    uint streamId() const;

    QSharedPointer<Contact> contact() const;
    Telepathy::MediaStreamState state() const;
    Telepathy::MediaStreamType type() const;

    bool sending() const;
    bool receiving() const;
    bool localSendingRequested() const;
    bool remoteSendingRequested() const;

    Telepathy::MediaStreamDirection direction() const;
    Telepathy::MediaStreamPendingSend pendingSend() const;

public Q_SLOTS:
    PendingOperation *remove();
    PendingOperation *requestStreamDirection(bool send, bool receive);
    PendingOperation *requestStreamDirection(
            Telepathy::MediaStreamDirection direction);

Q_SIGNALS:
    void directionChanged(Telepathy::Client::MediaStream *stream,
            bool sending, bool receiving, bool localSendingRequested,
            bool remoteSendingRequested);
    void error(Telepathy::Client::MediaStream *stream,
            Telepathy::MediaStreamError code, const QString &debugMessage);
    void stateChanged(Telepathy::Client::MediaStream *stream);
    void removed(Telepathy::Client::MediaStream *stream);

private:
    friend class StreamedMediaChannel;

    MediaStream(StreamedMediaChannel *parent, uint streamId, uint
            contactHandle, MediaStreamType type,
            MediaStreamState state, MediaStreamDirection direction,
            MediaStreamPendingSend pendingSend) const;

    struct Private;
    friend struct Private;
    Private *mPriv;
#endif
};

typedef QSet<QSharedPointer<MediaStream> > MediaStreams;

class StreamedMediaChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamedMediaChannel)

public:
    StreamedMediaChannel(Connection *connection, const QString &objectPath,
            const QVariantMap &immutableProperties, QObject *parent = 0);
    ~StreamedMediaChannel();

#if 0
public:
    MediaStreams streams() const;
#endif

public:
    bool awaitingLocalAnswer() const;
    bool awaitingRemoteAnswer() const;

public Q_SLOTS:
    PendingOperation *acceptCall();

#if 0
public Q_SLOTS:
    PendingOperation *removeStreams(MediaStreams streams);
    PendingOperation *removeStreams(QSet<uint> streams);

    PendingOperation *requestStreams(
            QSharedPointer<Telepathy::Client::Contact> contact,
            QList<Telepathy::MediaStreamType> types);
#endif

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

typedef QExplicitlySharedDataPointer<StreamedMediaChannel> StreamedMediaChannelPtr;

} // Telepathy::Client
} // Telepathy

#endif
