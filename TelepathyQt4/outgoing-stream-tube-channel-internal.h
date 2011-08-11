/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_outgoing_stream_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_outgoing_stream_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/PendingOperation>

namespace Tp
{

class PendingVoid;

class TELEPATHY_QT4_NO_EXPORT PendingOpenTube : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingOpenTube)

public:
    PendingOpenTube(PendingVoid *offerOperation,
            const QVariantMap &parameters,
            const OutgoingStreamTubeChannelPtr &object);
    ~PendingOpenTube();

private Q_SLOTS:
    void onTubeStateChanged(Tp::TubeChannelState state);
    void onOfferFinished(Tp::PendingOperation *operation);

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
    void contactsRetrieved(QUuid uuid, QList<Tp::ContactPtr> contacts);

private Q_SLOTS:
    void onPendingContactsFinished(Tp::PendingOperation *operation);

private:
    struct Entry {
        QUuid uuid;
        UIntList handles;
    };

    void processNextRequest();

    bool m_isProcessing;
    ContactManagerPtr m_manager;
    QQueue<Entry> m_queue;
};

struct TELEPATHY_QT4_NO_EXPORT PendingOpenTube::Private
{
    Private(const QVariantMap &parameters, PendingOpenTube *parent);

    // Public object
    PendingOpenTube *parent;

    OutgoingStreamTubeChannelPtr tube;
    QVariantMap parameters;
};

struct TELEPATHY_QT4_NO_EXPORT OutgoingStreamTubeChannel::Private
{
    Private(OutgoingStreamTubeChannel *parent);

    OutgoingStreamTubeChannel *parent;

    QHash<uint, Tp::ContactPtr> contactsForConnections;
    QHash<QPair<QHostAddress, quint16>, uint> connectionsForSourceAddresses;
    QHash<uchar, uint> connectionsForCredentials;

    QHash<QUuid, QPair<uint, QDBusVariant> > pendingNewConnections;

    QueuedContactFactory *queuedContactFactory;
};

}

#endif
