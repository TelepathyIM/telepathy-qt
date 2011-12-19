/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt/PendingString>

#include "TelepathyQt/_gen/pending-string.moc.hpp"
#include "TelepathyQt/debug-internal.h"

#include <QDBusPendingReply>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingString::Private
{
    QString result;
};

/**
 * \class PendingString
 * \ingroup utils
 * \headerfile TelepathyQt/pending-string.h <TelepathyQt/PendingString>
 *
 * \brief The PendingString class is a generic subclass of PendingOperation
 * representing a pending D-Bus method call that returns a string.
 *
 * See \ref async_model
 */

PendingString::PendingString(QDBusPendingCall call, const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mPriv(new Private)
{
    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}

PendingString::PendingString(const QString &errorName, const QString &errorMessage)
    : PendingOperation(SharedPtr<RefCounted>()),
      mPriv(new Private)
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingString::~PendingString()
{
    delete mPriv;
}

QString PendingString::result() const
{
    return mPriv->result;
}

void PendingString::setResult(const QString &result)
{
    mPriv->result = result;
}

void PendingString::watcherFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to PendingString call";
        setResult(reply.value());
        setFinished();
    } else {
        debug().nospace() << "PendingString call failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
