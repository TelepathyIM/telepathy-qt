/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2009-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009,2011 Nokia Corporation
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

#ifndef _TelepathyQt_examples_stream_tubes_tube_initiator_h_HEADER_GUARD_
#define _TelepathyQt_examples_stream_tubes_tube_initiator_h_HEADER_GUARD_

#include <TelepathyQt/Constants>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Types>

#include <QHash>
#include <QHostAddress>
#include <QPair>

class QTcpServer;
using namespace Tp;

namespace Tp
{
class ChannelRequestHints;
class PendingOperation;
}

class TubeInitiator : public QObject
{
    Q_OBJECT

public:
   TubeInitiator(const QString &accountName, const QString &receiver, QObject *parent);
   ~TubeInitiator();

private Q_SLOTS:
    void onAccountReady(Tp::PendingOperation *op);
    void onAccountConnectionChanged(const Tp::ConnectionPtr &);
    void onContactRetrieved(Tp::PendingOperation *op);
    void onContactCapabilitiesChanged();
    void onTubeRequestFinished(Tp::PendingOperation *op);
    void onTubeNewConnection(const QHostAddress &sourceAddress, quint16 sourcePort,
            const Tp::AccountPtr &account, const Tp::ContactPtr &contact);
    void onTubeConnectionClosed(const QHostAddress &sourceAddress, quint16 sourcePort,
            const Tp::AccountPtr &account, const Tp::ContactPtr &contact,
            const QString &error, const QString &errorMessage);
    void onTcpServerNewConnection();
    void onDataFromSocket();

private:
    void createStreamTubeChannel();

    QString mReceiver;
    QTcpServer *mServer;

    StreamTubeServerPtr mTubeServer;
    AccountPtr mAccount;
    ConnectionPtr mConn;
    ContactPtr mContact;

    bool mTubeRequested;
    QHash<QPair<QHostAddress, quint16>, QString> mConnAliases;
};

#endif
