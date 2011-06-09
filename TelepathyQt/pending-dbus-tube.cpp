/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/PendingDBusTube>

#include "TelepathyQt/_gen/pending-dbus-tube.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/IncomingDBusTubeChannel>
#include <TelepathyQt/OutgoingDBusTubeChannel>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingDBusTube::Private
{
    Private(PendingDBusTube *parent);
    ~Private();

    // Public object
    PendingDBusTube *parent;

    DBusTubeChannelPtr tube;
};

PendingDBusTube::Private::Private(PendingDBusTube *parent)
    : parent(parent)
{
}

PendingDBusTube::Private::~Private()
{
}

/**
 * \class PendingDBusTube
 * \headerfile TelepathyQt/pending-dbus-tube.h <TelepathyQt/PendingDBusTube>
 *
 * A pending operation for accepting or offering a DBus tube
 *
 * This class represents an asynchronous operation for accepting or offering a DBus tube.
 * Upon completion, the address of the opened tube is returned as a QString.
 */

PendingDBusTube::PendingDBusTube(
        PendingString *string,
        const IncomingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = object;

    if (string->isFinished()) {
        onConnectionFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onConnectionFinished(Tp::PendingOperation*)));
    }
}

PendingDBusTube::PendingDBusTube(
        PendingString *string,
        const OutgoingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = object;

    if (string->isFinished()) {
        onConnectionFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onConnectionFinished(Tp::PendingOperation*)));
    }
}

PendingDBusTube::PendingDBusTube(
        const QString &errorName,
        const QString &errorMessage,
        const DBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTube::Private(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor
 */
PendingDBusTube::~PendingDBusTube()
{
    delete mPriv;
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
QString PendingDBusTube::address() const
{
    return mPriv->tube->address();
}

void PendingDBusTube::onConnectionFinished(PendingOperation *op)
{
    if (op->isError()) {
        // Fail
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Accept/Offer tube finished successfully";

    // Now get the address and set it
    PendingString *ps = qobject_cast<PendingString*>(op);
    mPriv->tube->setAddress(ps->result());

    // It might have been already opened - check
    if (mPriv->tube->state() == TubeChannelStateOpen) {
        onStateChanged(mPriv->tube->state());
    } else {
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(), SIGNAL(stateChanged(Tp::TubeChannelState)),
                this, SLOT(onStateChanged(Tp::TubeChannelState)));
    }
}

void PendingDBusTube::onStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready: mark the operation as finished
        setFinished();
    } else if (state != TubeChannelStateLocalPending) {
        // Something happened
        setFinishedWithError(QLatin1String("Connection refused"),
                QLatin1String("The connection to this tube was refused"));
    }
}

}
