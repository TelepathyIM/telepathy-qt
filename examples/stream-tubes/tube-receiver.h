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

#ifndef _TelepathyQt_examples_stream_tubes_tube_receiver_h_HEADER_GUARD_
#define _TelepathyQt_examples_stream_tubes_tube_receiver_h_HEADER_GUARD_

#include <TelepathyQt/Types>

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
   TubeReceiver(QObject *parent);
   ~TubeReceiver();

private Q_SLOTS:
    void onTubeAccepted(const QString &listenAddress);
    void onStateChanged(QLocalSocket::LocalSocketState newState);
    void onTimerTimeout();
    void onDataFromSocket();

private:
    StreamTubeClientPtr mTubeClient;
    QLocalSocket *mDevice;
};

#endif
