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

#include <TelepathyQt/PendingDBusTubeOffer>

#include "TelepathyQt/_gen/pending-dbus-tube-offer.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/outgoing-dbus-tube-channel-internal.h"

#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingDBusTubeOffer::Private
{
    Private(PendingDBusTubeOffer *parent);
    ~Private();

    // Public object
    PendingDBusTubeOffer *parent;

    OutgoingDBusTubeChannelPtr tube;
};

PendingDBusTubeOffer::Private::Private(PendingDBusTubeOffer *parent)
    : parent(parent)
{
}

PendingDBusTubeOffer::Private::~Private()
{
}

/**
 * \class PendingDBusTubeOffer
 * \headerfile TelepathyQt4/pending-dbus-tube-offer.h <TelepathyQt4/PendingDBusTubeOffer>
 *
 * A pending operation for offering a DBus tube
 *
 * This class represents an asynchronous operation for offering a DBus tube.
 * Upon completion, the address of the opened tube is returned as a QString.
 */

PendingDBusTubeOffer::PendingDBusTubeOffer(
        PendingString *string,
        const OutgoingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = object;

    if (string->isFinished()) {
        onOfferFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onOfferFinished(Tp::PendingOperation*)));
    }
}

PendingDBusTubeOffer::PendingDBusTubeOffer(
        const QString &errorName,
        const QString &errorMessage,
        const OutgoingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTubeOffer::Private(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor
 */
PendingDBusTubeOffer::~PendingDBusTubeOffer()
{
    delete mPriv;
}

void PendingDBusTubeOffer::onOfferFinished(PendingOperation *op)
{
    if (op->isError()) {
        // Fail
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Offer tube finished successfully";

    // Now get the address and set it
    PendingString *ps = qobject_cast<PendingString*>(op);
    mPriv->tube->mPriv->address = ps->result();

    // It might have been already opened - check
    if (mPriv->tube->state() == TubeChannelStateOpen) {
        onTubeStateChanged(mPriv->tube->state());
    } else {
        // Wait until the tube gets opened on the other side
        connect(mPriv->tube.data(), SIGNAL(tubeStateChanged(Tp::TubeChannelState)),
                this, SLOT(onTubeStateChanged(Tp::TubeChannelState)));
    }
}

void PendingDBusTubeOffer::onTubeStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready: let's finish the operation
        setFinished();
    } else if (state != TubeChannelStateRemotePending) {
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
QString PendingDBusTubeOffer::address() const
{
    return mPriv->tube->address();
}

}
