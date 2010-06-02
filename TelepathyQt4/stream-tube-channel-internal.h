/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_stream_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_stream_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/StreamTubeChannel>
#include "TelepathyQt4/tube-channel-internal.h"

#include <QtNetwork/QHostAddress>

namespace Tp {

class TELEPATHY_QT4_NO_EXPORT QueuedContactFactory : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QueuedContactFactory)

public:
    QueuedContactFactory(ContactManagerPtr contactManager, QObject* parent = 0);
    virtual ~QueuedContactFactory();

    QUuid appendNewRequest(const UIntList &handles);

Q_SIGNALS:
    void contactsRetrieved(QUuid, QList< Tp::ContactPtr >);

private slots:
    void onPendingContactsFinished(Tp::PendingOperation*);

private:
    struct Entry {
        QUuid uuid;
        UIntList handles;
    };

    void processNextRequest();

    bool m_isProcessing;
    ContactManagerPtr m_manager;
    QQueue< Entry > m_queue;
};

class TELEPATHY_QT4_NO_EXPORT StreamTubeChannelPrivate : public TubeChannelPrivate
{
    Q_DECLARE_PUBLIC(StreamTubeChannel)

public:
    enum BaseTubeType {
        NoKnownType = 0,
        OutgoingTubeType,
        IncomingTubeType
    };

    StreamTubeChannelPrivate(StreamTubeChannel *parent);
    virtual ~StreamTubeChannelPrivate();

    virtual void init();

    void extractStreamTubeProperties(const QVariantMap &props);

    static void introspectConnectionMonitoring(StreamTubeChannelPrivate *self);
    static void introspectStreamTube(StreamTubeChannelPrivate *self);

    UIntList connections;

    // Properties
    SupportedSocketMap socketTypes;
    QString serviceName;

    BaseTubeType baseType;

    QPair< QHostAddress, quint16 > ipAddress;
    QString unixAddress;
    SocketAddressType addressType;
    QueuedContactFactory *queuedContactFactory;

    // Private slots
    void __k__gotStreamTubeProperties(QDBusPendingCallWatcher *watcher);
    void __k__onConnectionClosed(uint connectionId, const QString &error, const QString &message);

};

}

#endif
