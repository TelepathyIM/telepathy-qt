/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/PendingDBusTubeAccept>

#include "TelepathyQt/_gen/pending-dbus-tube-accept.moc.hpp"

#include "TelepathyQt/dbus-tube-channel-internal.h"
#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingDBusTubeAccept::Private
{
    Private(PendingDBusTubeAccept *parent);
    ~Private();

    // Public object
    PendingDBusTubeAccept *parent;

    IncomingDBusTubeChannelPtr tube;
};

PendingDBusTubeAccept::Private::Private(PendingDBusTubeAccept *parent)
    : parent(parent)
{
}

PendingDBusTubeAccept::Private::~Private()
{
}

/**
 * \class PendingDBusTubeAccept
 * \headerfile TelepathyQt4/pending-dbus-tube-accept.h <TelepathyQt4/PendingDBusTubeAccept>
 *
 * A pending operation for accepting a DBus tube
 *
 * This class represents an asynchronous operation for accepting a DBus tube.
 * Upon completion, the address of the opened tube is returned as a QString.
 */

PendingDBusTubeAccept::PendingDBusTubeAccept(
        PendingString *string,
        const IncomingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTubeAccept::Private(this))
{
    mPriv->tube = object;

    if (string->isFinished()) {
        onAcceptFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onAcceptFinished(Tp::PendingOperation*)));
    }
}

PendingDBusTubeAccept::PendingDBusTubeAccept(
        const QString &errorName,
        const QString &errorMessage,
        const IncomingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTubeAccept::Private(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor
 */
PendingDBusTubeAccept::~PendingDBusTubeAccept()
{
    delete mPriv;
}

void PendingDBusTubeAccept::onAcceptFinished(PendingOperation *op)
{
    if (op->isError()) {
        // Fail
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Accept tube finished successfully";

    // Now get the address and set it
    PendingString *ps = qobject_cast<PendingString*>(op);
    DBusTubeChannelPtr::qObjectCast<DBusTubeChannel>(mPriv->tube)->mPriv->address = ps->result();

    // It might have been already opened - check
    if (mPriv->tube->state() == TubeChannelStateOpen) {
        onTubeStateChanged(mPriv->tube->state());
    } else {
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                this, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingDBusTubeAccept::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready: let's inject the address into the tube itself
        setFinished();
    } else if (state != TubeChannelStateLocalPending) {
        // Something happened
        setFinishedWithError(QLatin1String("Connection refused"),
                QLatin1String("The connection to this tube was refused"));
    }
}

/**
 * When the operation has been completed successfully, returns the address of the opened DBus connection.
 *
 * Please note this function will return a meaningful value only if the operation has already
 * been completed successfully: in case of failure or non-completion, an empty QString will be
 * returned.
 *
 * \returns The address of the opened DBus connection.
 */
QString PendingDBusTubeAccept::address() const
{
    return mPriv->tube->address();
}

}
