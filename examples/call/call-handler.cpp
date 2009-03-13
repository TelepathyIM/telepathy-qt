/*
 * This file is part of TelepathyQt4
 *
 * Copyright (call) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "call-handler.h"
#include "_gen/call-handler.moc.hpp"

#include "call-widget.h"

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/Contact>

#include <QDebug>

using namespace Telepathy::Client;

CallHandler::CallHandler(QObject *parent)
    : QObject(parent)
{
}

CallHandler::~CallHandler()
{
    foreach (CallWidget *call, mCalls) {
        call->close();
    }
}

void CallHandler::addOutgoingCall(const QSharedPointer<Contact> &contact)
{
    CallWidget *call = new CallWidget(contact);
    mCalls.append(call);
    connect(call,
            SIGNAL(destroyed(QObject *)),
            SLOT(onCallTerminated(QObject *)));
    call->show();
}

void CallHandler::addIncomingCall(const ChannelPtr &chan)
{
    CallWidget *call = new CallWidget(chan);
    mCalls.append(call);
    connect(call,
            SIGNAL(destroyed(QObject *)),
            SLOT(onCallTerminated(QObject *)));
    call->show();
}

void CallHandler::onCallTerminated(QObject *obj)
{
    // Why does obj comes 0x0?
    mCalls.removeOne((CallWidget *) sender());
}
