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

#include <TelepathyQt/OutgoingDBusTubeChannel>

#include "TelepathyQt/_gen/outgoing-dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingDBusTubeConnection>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT OutgoingDBusTubeChannel::Private
{
    Private(OutgoingDBusTubeChannel *parent);

    // Public object
    OutgoingDBusTubeChannel *parent;
};

OutgoingDBusTubeChannel::Private::Private(OutgoingDBusTubeChannel *parent)
        : parent(parent)
{
}

/**
 * \class OutgoingDBusTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/outgoing-dbus-tube-channel.h <TelepathyQt/OutgoingDBusTubeChannel>
 *
 * \brief The OutgoingDBusTubeChannel class represents an outgoing Telepathy channel
 * of type DBusTube.
 *
 * Outgoing (locally initiated/requested) tubes are initially in the #TubeChannelStateNotOffered state.
 * When offerTube is called, the connection manager takes care of instantiating a new DBus server,
 * at which point the tube state becomes #TubeChannelStateRemotePending.
 *
 * If the target accepts the connection request, the state goes #TubeChannelStateOpen and both sides
 * can start using the new private bus, the address of which can be retrieved from the completed
 * PendingDBusTubeConnection or from this class.
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
 * Create a new OutgoingDBusTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A OutgoingDBusTubeChannelPtr object pointing to the newly created
 *         OutgoingDBusTubeChannel object.
 */
OutgoingDBusTubeChannelPtr OutgoingDBusTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return OutgoingDBusTubeChannelPtr(new OutgoingDBusTubeChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new OutgoingDBusTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
OutgoingDBusTubeChannel::OutgoingDBusTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : DBusTubeChannel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
OutgoingDBusTubeChannel::~OutgoingDBusTubeChannel()
{
    delete mPriv;
}

/**
 * Offer the tube
 *
 * This method sets up a private DBus connection to the channel target(s), and offers it through the tube.
 *
 * The %PendingDBusTubeConnection returned by this method will be completed as soon as the tube is
 * opened and ready to be used.
 *
 * This method requires DBusTubeChannel::FeatureCore to be enabled.
 *
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   The other end will receive this QVariantMap in the parameters() method
 *                   of the corresponding IncomingDBusTubeChannel.
 * \param allowOtherUsers Whether the server should allow other users to connect to this tube more
 *                        than just the current one. If your application has no specific needs, it is
 *                        advisable not to modify the default value of this argument.
 *
 * \note If allowOtherUsers == false, but one of the ends does not support current user restriction,
 *       the tube will be offered regardless, falling back to allowing any connection. If your
 *       application requires strictly this condition to be enforced, you should check
 *       DBusTubeChannel::supportsRestrictingToCurrentUser <b>before</b> offering the tube,
 *       and take action from there.
 *       The tube is guaranteed either to be offered with the desired
 *       restriction or to fail the accept phase if supportsRestrictingToCurrentUser is true
 *       and allowOtherUsers is false.
 *
 * \returns A %PendingDBusTubeConnection which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 */
PendingDBusTubeConnection *OutgoingDBusTubeChannel::offerTube(const QVariantMap &parameters,
        bool allowOtherUsers)
{
    SocketAccessControl accessControl = allowOtherUsers ?
                                        SocketAccessControlLocalhost :
                                        SocketAccessControlCredentials;

    if (!isReady(DBusTubeChannel::FeatureCore)) {
        warning() << "DBusTubeChannel::FeatureCore must be ready before "
            "calling offerTube";
        return new PendingDBusTubeConnection(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"), OutgoingDBusTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (state() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a bus for each DBus Tube";
        return new PendingDBusTubeConnection(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"), OutgoingDBusTubeChannelPtr(this));
    }

    // Let's offer the tube
    if (!allowOtherUsers && !supportsRestrictingToCurrentUser()) {
        warning() << "Current user restriction is not available for this tube, "
            "falling back to allowing any connection";
        accessControl = SocketAccessControlLocalhost;
    }

    PendingString *ps = new PendingString(
        interface<Client::ChannelTypeDBusTubeInterface>()->Offer(
            parameters,
            accessControl),
        OutgoingDBusTubeChannelPtr(this));

    PendingDBusTubeConnection *op = new PendingDBusTubeConnection(ps,
            accessControl == SocketAccessControlLocalhost,
            parameters, OutgoingDBusTubeChannelPtr(this));
    return op;
}

}
