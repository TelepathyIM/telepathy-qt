/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/PendingVariant>

#include "TelepathyQt/_gen/pending-variant.moc.hpp"
#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Global>

#include <QDBusPendingReply>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingVariant::Private
{
    QVariant result;
};

/**
 * \class PendingVariant
 * \ingroup utils
 * \headerfile TelepathyQt/pending-variant.h <TelepathyQt/PendingVariant>
 *
 * \brief The PendingVariant class is a generic subclass of PendingOperation
 * representing a pending D-Bus method call that returns a variant.
 *
 * See \ref async_model
 */

PendingVariant::PendingVariant(QDBusPendingCall call, const SharedPtr<RefCounted> &object)
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
PendingVariant::~PendingVariant()
{
    delete mPriv;
}

QVariant PendingVariant::result() const
{
    return mPriv->result;
}

void PendingVariant::watcherFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to PendingVariant call";
        mPriv->result = reply.value().variant();
        setFinished();
    } else {
        debug().nospace() << "PendingVariant call failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
