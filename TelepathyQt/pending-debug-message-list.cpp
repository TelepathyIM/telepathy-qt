/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/PendingDebugMessageList>

#include "TelepathyQt/_gen/pending-debug-message-list.moc.hpp"

#include <QtDBus/QDBusPendingCallWatcher>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingDebugMessageList::Private
{
    DebugMessageList result;
};

PendingDebugMessageList::PendingDebugMessageList(const QDBusPendingCall &call,
        const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mPriv(new Private)
{
    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}

PendingDebugMessageList::~PendingDebugMessageList()
{
    delete mPriv;
}

DebugMessageList PendingDebugMessageList::result() const
{
    return mPriv->result;
}

void PendingDebugMessageList::watcherFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<DebugMessageList> reply = *watcher;
    if (reply.isError()) {
        setFinishedWithError(reply.error());
    } else {
        mPriv->result = reply.value();
        setFinished();
    }
    watcher->deleteLater();
}

} // Tp
