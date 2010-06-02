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
public:
    PendingOpenTube(PendingVoid *offerOperation, const SharedPtr<RefCounted> &object);
    virtual ~PendingOpenTube();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;

// private Q_SLOTS:
    Q_PRIVATE_SLOT(mPriv, void __k__onOfferFinished(Tp::PendingOperation*))
    Q_PRIVATE_SLOT(mPriv, void __k__onTubeStateChanged(Tp::TubeChannelState))
};

struct TELEPATHY_QT4_NO_EXPORT PendingOpenTube::Private
{
    Private(PendingOpenTube *parent);
    ~Private();

    // Public object
    PendingOpenTube *parent;

    OutgoingStreamTubeChannelPtr tube;

    // Private slots
    void __k__onOfferFinished(Tp::PendingOperation* op);
    void __k__onTubeStateChanged(Tp::TubeChannelState state);
};

class TELEPATHY_QT4_NO_EXPORT OutgoingStreamTubeChannelPrivate : public StreamTubeChannelPrivate
{
    Q_DECLARE_PUBLIC(OutgoingStreamTubeChannel)
public:
    OutgoingStreamTubeChannelPrivate(OutgoingStreamTubeChannel *parent);
    virtual ~OutgoingStreamTubeChannelPrivate();

    QHash< uint, Tp::ContactPtr > contactsForConnections;
    QHash< QPair< QHostAddress, quint16 >, uint > connectionsForSourceAddresses;

    QHash< QUuid, QPair< uint, QDBusVariant > > pendingNewConnections;

    // Private slots
    void __k__onNewRemoteConnection(uint contactId, const QDBusVariant &parameter, uint connectionId);
    void __k__onPendingOpenTubeFinished(Tp::PendingOperation* operation);
    void __k__onContactsRetrieved(const QUuid &uuid, const QList< Tp::ContactPtr > &contacts);
    void __k__onConnectionClosed(uint connectionId,const QString&,const QString&);
};

}

#endif
