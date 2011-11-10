/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#include <TelepathyQt/PendingVariantMap>

#include "TelepathyQt/_gen/pending-variant-map.moc.hpp"
#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Global>

#include <QDBusPendingReply>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingVariantMap::Private
{
    QVariantMap result;
};

/**
 * \class PendingVariantMap
 * \ingroup utils
 * \headerfile TelepathyQt/pending-variant-map.h <TelepathyQt/PendingVariantMap>
 *
 * \brief The PendingVariantMap class is a generic subclass of PendingOperation
 * representing a pending D-Bus method call that returns a variant map.
 *
 * See \ref async_model
 */

PendingVariantMap::PendingVariantMap(QDBusPendingCall call, const SharedPtr<RefCounted> &object)
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
PendingVariantMap::~PendingVariantMap()
{
    delete mPriv;
}

QVariantMap PendingVariantMap::result() const
{
    return mPriv->result;
}

void PendingVariantMap::watcherFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to PendingVariantMap call";
        mPriv->result = reply.value();
        setFinished();
    } else {
        debug().nospace() << "PendingVariantMap call failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
