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

#ifndef _TelepathyQt4_outgoing_stream_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_outgoing_stream_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/PendingOperation>

#include "TelepathyQt4/stream-tube-channel-internal.h"

namespace Tp {
class PendingVoid;

class TELEPATHY_QT4_NO_EXPORT PendingOpenTube : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingOpenTube)

    // private Q_SLOTS:
    Q_PRIVATE_SLOT(mPriv, void onOfferFinished(Tp::PendingOperation*))

public:
    PendingOpenTube(PendingVoid *offerOperation,
            const QVariantMap &parameters,
            const SharedPtr<RefCounted> &object);
    ~PendingOpenTube();

private Q_SLOTS:
    void onTubeStateChanged(Tp::TubeChannelState state);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_NO_EXPORT QueuedContactFactory : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QueuedContactFactory)

public:
    QueuedContactFactory(ContactManagerPtr contactManager, QObject* parent = 0);
    ~QueuedContactFactory();

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

struct TELEPATHY_QT4_NO_EXPORT PendingOpenTube::Private
{
    Private(const QVariantMap &parameters, PendingOpenTube *parent);
    ~Private();

    // Public object
    PendingOpenTube *parent;

    OutgoingStreamTubeChannelPtr tube;
    QVariantMap parameters;

    // Private slots
    void onOfferFinished(Tp::PendingOperation* op);
    void onTubeStateChanged(Tp::TubeChannelState state);
};

struct TELEPATHY_QT4_NO_EXPORT OutgoingStreamTubeChannel::Private
{
    Private(OutgoingStreamTubeChannel *parent);
    ~Private();

    OutgoingStreamTubeChannel *parent;

    QHash< uint, Tp::ContactPtr > contactsForConnections;
    QHash< QPair< QHostAddress, quint16 >, uint > connectionsForSourceAddresses;

    QHash< QUuid, QPair< uint, QDBusVariant > > pendingNewConnections;

    QueuedContactFactory *queuedContactFactory;

    // Private slots
    void onNewRemoteConnection(uint contactId, const QDBusVariant &parameter, uint connectionId);
    void onPendingOpenTubeFinished(Tp::PendingOperation* operation);
    void onContactsRetrieved(const QUuid &uuid, const QList< Tp::ContactPtr > &contacts);
    void onConnectionClosed(uint connectionId,const QString&,const QString&);
};

}

#endif
