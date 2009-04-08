/*
 * This file is part of TelepathyQt4
 *
 * Copyright © 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_examples_call_call_handler_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_call_handler_h_HEADER_GUARD_

#include <QObject>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Contact>

namespace Tp {
class PendingOperation;
class StreamedMediaChannel;
}

class CallWidget;

class CallHandler : public QObject
{
    Q_OBJECT

public:
    CallHandler(QObject *parent = 0);
    virtual ~CallHandler();

    void addOutgoingCall(const Tp::ContactPtr &contact);
    void addIncomingCall(const Tp::StreamedMediaChannelPtr &chan);

private Q_SLOTS:
    void onOutgoingChannelCreated(Tp::PendingOperation *);
    void onOutgoingChannelReady(Tp::PendingOperation *);
    void onIncomingChannelReady(Tp::PendingOperation *);
    void onCallTerminated(QObject *);

private:
    QList<Tp::StreamedMediaChannelPtr> mChannels;
    QList<CallWidget *> mCalls;
};

#endif
