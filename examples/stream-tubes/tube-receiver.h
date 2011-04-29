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

#ifndef _TelepathyQt4_examples_stream_tubes_tube_receiver_h_HEADER_GUARD_
#define _TelepathyQt4_examples_stream_tubes_tube_receiver_h_HEADER_GUARD_

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Types>

#include <QLocalSocket>

using namespace Tp;

namespace Tp
{
class PendingOperation;
}

class TubeReceiver : public QObject
{
    Q_OBJECT

public:
   TubeReceiver(const QString &accountName, QObject *parent);
   ~TubeReceiver();

private Q_SLOTS:
    void onAccountReady(Tp::PendingOperation *op);
    void onAccountConnectionChanged(const Tp::ConnectionPtr &);
    void onNewChannels(const Tp::ChannelDetailsList &channels);
    void onStreamTubeChannelReady(Tp::PendingOperation*);
    void onStreamTubeAccepted(Tp::PendingOperation*);
    void onStateChanged(QLocalSocket::LocalSocketState);
    void onTimerTimeout();
    void onDataFromSocket();
    void onInvalidated();

private:
    AccountPtr mAccount;
    QLocalSocket *mDevice;

    ConnectionManagerPtr mCM;
    ConnectionPtr mConn;
    IncomingStreamTubeChannelPtr mChan;
};

#endif
