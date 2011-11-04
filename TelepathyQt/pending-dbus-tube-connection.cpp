/*
 * This file is part of TelepathyQt
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

#include <TelepathyQt/PendingDBusTubeConnection>

#include "TelepathyQt/_gen/pending-dbus-tube-connection.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/IncomingDBusTubeChannel>
#include <TelepathyQt/OutgoingDBusTubeChannel>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingDBusTubeConnection::Private
{
    Private(PendingDBusTubeConnection *parent);
    ~Private();

    // Public object
    PendingDBusTubeConnection *parent;

    DBusTubeChannelPtr tube;

    bool requiresCredentials;
    uchar credentialByte;
    QVariantMap parameters;
};

PendingDBusTubeConnection::Private::Private(PendingDBusTubeConnection *parent)
    : parent(parent),
      requiresCredentials(false),
      credentialByte(0)
{
}

PendingDBusTubeConnection::Private::~Private()
{
}

/**
 * \class PendingDBusTubeConnection
 * \headerfile TelepathyQt4/pending-dbus-tube-connection.h <TelepathyQt4/PendingDBusTubeConnection>
 *
 * A pending operation for accepting or offering a DBus tube
 *
 * This class represents an asynchronous operation for accepting or offering a DBus tube.
 * Upon completion, the address of the opened tube is returned as a QString.
 */

PendingDBusTubeConnection::PendingDBusTubeConnection(
        PendingString *string,
        bool requiresCredentials,
        uchar credentialByte,
        const DBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = object;

    mPriv->requiresCredentials = requiresCredentials;
    mPriv->credentialByte = credentialByte;

    connect(mPriv->tube.data(), SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            this, SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));

    if (string->isFinished()) {
        onConnectionFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onConnectionFinished(Tp::PendingOperation*)));
    }
}

PendingDBusTubeConnection::PendingDBusTubeConnection(
        PendingString* string,
        bool requiresCredentials,
        const QVariantMap& parameters,
        const DBusTubeChannelPtr& object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = object;

    mPriv->requiresCredentials = requiresCredentials;
    mPriv->parameters = parameters;

    connect(mPriv->tube.data(), SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            this, SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));

    if (string->isFinished()) {
        onConnectionFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onConnectionFinished(Tp::PendingOperation*)));
    }
}


PendingDBusTubeConnection::PendingDBusTubeConnection(
        const QString &errorName,
        const QString &errorMessage,
        const DBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTubeConnection::Private(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor
 */
PendingDBusTubeConnection::~PendingDBusTubeConnection()
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
QString PendingDBusTubeConnection::address() const
{
    return mPriv->tube->address();
}

/**
 * Return whether sending a credential byte once connecting to the socket is required.
 *
 * Note that if this method returns \c true, one should send a SCM_CREDS or SCM_CREDENTIALS
 * and the credentialByte() once connected. If SCM_CREDS or SCM_CREDENTIALS cannot be sent,
 * the credentialByte() should still be sent.
 *
 * \return \c true if sending credentials is required, \c false otherwise.
 * \sa credentialByte()
 */
bool PendingDBusTubeConnection::requiresCredentials() const
{
    return mPriv->requiresCredentials;
}

/**
 * Return the credential byte to send once connecting to the socket if requiresCredentials() is \c
 * true.
 *
 * \return The credential byte.
 * \sa requiresCredentials()
 */
uchar PendingDBusTubeConnection::credentialByte() const
{
    return mPriv->credentialByte;
}

void PendingDBusTubeConnection::onConnectionFinished(PendingOperation *op)
{
    if (isFinished()) {
        // The operation has already failed
        return;
    }

    if (op->isError()) {
        // Fail
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Accept/Offer tube finished successfully";

    // Now get the address and set it
    PendingString *ps = qobject_cast<PendingString*>(op);
    debug() << "Got address " << ps->result();
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

void PendingDBusTubeConnection::onStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        if (!mPriv->parameters.isEmpty()) {
            // Inject the parameters into the tube
            mPriv->tube->setParameters(mPriv->parameters);
        }
        // The tube is ready: mark the operation as finished
        setFinished();
    }
}

void PendingDBusTubeConnection::onChannelInvalidated(DBusProxy* proxy,
        const QString& errorName, const QString& errorMessage)
{
    Q_UNUSED(proxy);

    if (isFinished()) {
        // The operation has already finished
        return;
    }

    setFinishedWithError(errorName, errorMessage);
}

}
