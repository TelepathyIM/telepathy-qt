/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

    // Public object
    PendingDBusTubeConnection *parent;

    DBusTubeChannelPtr tube;

    bool allowOtherUsers;
    QVariantMap parameters;
};

PendingDBusTubeConnection::Private::Private(PendingDBusTubeConnection *parent)
    : parent(parent),
      allowOtherUsers(false)
{
}

/**
 * \class PendingDBusTubeConnection
 * \ingroup clientchannel
 * \headerfile TelepathyQt/pending-dbus-tube-connection.h <TelepathyQt/PendingDBusTubeConnection>
 *
 * A pending operation for accepting or offering a DBus tube
 *
 * This class represents an asynchronous operation for accepting or offering a DBus tube.
 * Upon completion, the address of the opened tube is returned as a QString.
 */

PendingDBusTubeConnection::PendingDBusTubeConnection(
        PendingString* string,
        bool allowOtherUsers,
        const QVariantMap& parameters,
        const DBusTubeChannelPtr& object)
    : PendingOperation(object)
    , mPriv(new Private(this))
{
    mPriv->tube = object;

    mPriv->allowOtherUsers = allowOtherUsers;
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
 * \note If you plan to use QtDBus for the DBus connection, please note you should always use
 *       QDBusConnection::connectToPeer(), regardless of the fact this tube is a p2p or a group one.
 *       The above function has been introduced in Qt 4.8, previous versions of Qt do not allow the use
 *       of DBus Tubes through QtDBus.
 *
 * \returns The address of the opened DBus connection.
 */
QString PendingDBusTubeConnection::address() const
{
    return mPriv->tube->address();
}

/**
 * Return whether this tube allows other users more than the current one to connect to the
 * private bus created by the tube.
 *
 * Note that even if the tube was accepted or offered specifying not to allow other users, this
 * method might still return true if one of the ends did not support such a restriction.
 *
 * In fact, if one of the ends does not support current user restriction,
 * the tube will be offered regardless, falling back to allowing any connection. If your
 * application requires strictly this condition to be enforced, you should check
 * DBusTubeChannel::supportsRestrictingToCurrentUser <b>before</b> offering the tube,
 * and take action from there.
 *
 * This function, however, is guaranteed to return the same value of the given allowOtherUsers
 * parameter when accepting or offering a tube if supportsRestrictingToCurrentUser is true.
 *
 * \return \c true if any user is allow to connect, \c false otherwise.
 */
bool PendingDBusTubeConnection::allowsOtherUsers() const
{
    return mPriv->allowOtherUsers;
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
