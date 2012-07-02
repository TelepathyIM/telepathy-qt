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

#include <TelepathyQt/IncomingDBusTubeChannel>

#include "TelepathyQt/_gen/incoming-dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingDBusTubeConnection>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT IncomingDBusTubeChannel::Private
{
public:
    Private(IncomingDBusTubeChannel *parent);

    // Public object
    IncomingDBusTubeChannel *parent;
};

IncomingDBusTubeChannel::Private::Private(IncomingDBusTubeChannel *parent)
    : parent(parent)
{
}


/**
 * \class IncomingDBusTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/incoming-dbus-tube-channel.h <TelepathyQt/IncomingDBusTubeChannel>
 *
 * \brief The IncomingStreamTubeChannel class represents an incoming Telepathy channel
 * of type StreamTube.
 *
 * In particular, this class is meant to be used as a comfortable way for
 * accepting incoming DBus tubes. Unless a different behavior is specified, tubes
 * will be always accepted allowing connections just from the current user, unless this
 * or one of the other ends do not support that. Unless your application has specific needs,
 * you usually want to keep this behavior.
 *
 * Once a tube is successfully accepted and open (the PendingDBusTubeConnection returned from the
 * accepting methods has finished), the application can connect to the DBus server, the address of which
 * can be retrieved from PendingDBusTubeConnection::address().
 *
 * \note If you plan to use QtDBus for the DBus connection, please note you should always use
 *       QDBusConnection::connectToPeer(), regardless of the fact this tube is a p2p or a group one.
 *       The above function has been introduced in Qt 4.8, previous versions of Qt do not allow the use
 *       of DBus Tubes through QtDBus.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Create a new IncomingDBusTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A IncomingDBusTubeChannelPtr object pointing to the newly created
 *         IncomingDBusTubeChannel object.
 */
IncomingDBusTubeChannelPtr IncomingDBusTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return IncomingDBusTubeChannelPtr(new IncomingDBusTubeChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new IncomingDBusTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
IncomingDBusTubeChannel::IncomingDBusTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : DBusTubeChannel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
IncomingDBusTubeChannel::~IncomingDBusTubeChannel()
{
    delete mPriv;
}

/**
 * Accepts an incoming DBus tube.
 *
 * This method accepts an incoming connection request for a DBus tube. It can be called
 * only if the tube is in the \c LocalPending state.
 *
 * Once called, this method will try opening the tube, and will create a new private DBus connection
 * which can be used to communicate with the other end. You can then
 * retrieve the address either from \c PendingDBusTubeConnection or from %address().
 *
 * This method requires DBusTubeChannel::FeatureCore to be enabled.
 *
 * \param allowOtherUsers Whether the server should allow other users to connect to this tube more
 *                        than just the current one. If your application has no specific needs, it is
 *                        advisable not to modify the default value of this argument.
 *
 * \note If allowOtherUsers == false, but one of the ends does not support current user restriction,
 *       the tube will be offered regardless, falling back to allowing any connection. If your
 *       application requires strictly this condition to be enforced, you should check
 *       DBusTubeChannel::supportsRestrictingToCurrentUser <b>before</b> accepting the tube,
 *       and take action from there.
 *       The tube is guaranteed either to be accepted with the desired
 *       restriction or to fail the accept phase if supportsRestrictingToCurrentUser is true
 *       and allowOtherUsers is false.
 *
 * \return A %PendingDBusTubeConnection which will finish as soon as the tube is ready to be used
 *         (hence in the Open state)
 */
PendingDBusTubeConnection *IncomingDBusTubeChannel::acceptTube(bool allowOtherUsers)
{
    SocketAccessControl accessControl = allowOtherUsers ?
                                        SocketAccessControlLocalhost :
                                        SocketAccessControlCredentials;

    if (!isReady(DBusTubeChannel::FeatureCore)) {
        warning() << "DBusTubeChannel::FeatureCore must be ready before "
            "calling acceptTube";
        return new PendingDBusTubeConnection(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"), IncomingDBusTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingDBusTubeConnection(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"), IncomingDBusTubeChannelPtr(this));
    }

    // Let's accept the tube
    if (!allowOtherUsers && !supportsRestrictingToCurrentUser()) {
        warning() << "Current user restriction is not available for this tube, "
            "falling back to allowing any connection";
        accessControl = SocketAccessControlLocalhost;
    }

    PendingString *ps = new PendingString(
        interface<Client::ChannelTypeDBusTubeInterface>()->Accept(
            accessControl),
        IncomingDBusTubeChannelPtr(this));

    PendingDBusTubeConnection *op = new PendingDBusTubeConnection(ps,
        accessControl == SocketAccessControlLocalhost,
        QVariantMap(), IncomingDBusTubeChannelPtr(this));
    return op;
}

}
