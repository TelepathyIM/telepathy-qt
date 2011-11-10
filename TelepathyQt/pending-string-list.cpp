/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt/PendingStringList>

#include "TelepathyQt/_gen/pending-string-list.moc.hpp"
#include "TelepathyQt/debug-internal.h"

#include <QDBusPendingReply>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingStringList::Private
{
    QStringList result;
};

/**
 * \class PendingStringList
 * \ingroup utils
 * \headerfile TelepathyQt/pending-string-list.h <TelepathyQt/PendingStringList>
 *
 * \brief The PendingStringList class is a generic subclass of PendingOperation
 * representing a pending D-Bus method call that returns a string list.
 *
 * See \ref async_model
 */

PendingStringList::PendingStringList(const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mPriv(new Private)
{
}

PendingStringList::PendingStringList(QDBusPendingCall call, const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mPriv(new Private)
{
    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}

/**
 * Class destructor.
 */
PendingStringList::~PendingStringList()
{
    delete mPriv;
}

QStringList PendingStringList::result() const
{
    return mPriv->result;
}

void PendingStringList::setResult(const QStringList &result)
{
    mPriv->result = result;
}

void PendingStringList::watcherFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to PendingStringList call";
        setResult(reply.value());
        setFinished();
    } else {
        debug().nospace() << "PendingStringList call failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
