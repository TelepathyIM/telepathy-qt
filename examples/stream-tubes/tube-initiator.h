/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_examples_stream_tubes_tube_initiator_h_HEADER_GUARD_
#define _TelepathyQt4_examples_stream_tubes_tube_initiator_h_HEADER_GUARD_

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Types>

class QTcpServer;
using namespace Tp;

namespace Tp
{
class PendingOperation;
}

class TubeInitiator : public QObject
{
    Q_OBJECT

public:
   TubeInitiator(const QString &username, const QString &password,
          const QString &receiver);
   ~TubeInitiator();

private Q_SLOTS:
    void onCMReady(Tp::PendingOperation *op);
    void onConnectionCreated(Tp::PendingOperation *op);
    void onConnectionConnected(Tp::PendingOperation *op);
    void onContactRetrieved(Tp::PendingOperation *op);
    void onContactPresenceChanged();
    /*
    void onCapabilitiesChanged(const Tp::CapabilityChangeList &caps);
    void gotContactCapabilities(QDBusPendingCallWatcher *watcher);
    */
    void onStreamTubeChannelCreated(Tp::PendingOperation *op);
    void onStreamTubeChannelReady(Tp::PendingOperation *op);
    void onStreamTubeChannelNewRemoteConnection(const Tp::ContactPtr&,const QVariant&,uint);
    void onOfferTubeFinished(Tp::PendingOperation*);
    void onTcpServerNewConnection();
    void onDataFromSocket();
    void onInvalidated();

private:
    void createStreamTubeChannel();

    QString mUsername;
    QString mPassword;
    QString mReceiver;
    QTcpServer *mServer;

    ConnectionManagerPtr mCM;
    ConnectionPtr mConn;
    OutgoingStreamTubeChannelPtr mChan;
    ContactPtr mContact;

    bool mTubeOffered;
};

#endif
