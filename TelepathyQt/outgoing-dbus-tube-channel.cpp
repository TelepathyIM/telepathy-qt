/*
 * This file is part of TelepathyQt
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

#include <TelepathyQt/OutgoingDBusTubeChannel>
#include "TelepathyQt/outgoing-dbus-tube-channel-internal.h"

#include "TelepathyQt/_gen/outgoing-dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingDBusTubeOffer>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

/**
 * \class OutgoingDBusTubeChannel
 * \headerfile TelepathyQt/stream-tube.h <TelepathyQt/OutgoingDBusTubeChannel>
 *
 * \brief A class representing a Stream Tube
 *
 * \c OutgoingDBusTubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Type.StreamTubeChannel.
 * In particular, this class is meant to be used as a comfortable way for exposing new tubes.
 * It provides a set of overloads for exporting a variety of sockets over a stream tube.
 *
 * For more details, please refer to Telepathy spec.
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
}


/**
 * \brief Offer a Unix socket over the tube
 *
 * This method offers a Unix socket over this tube. The socket is represented through
 * a QByteArray, which should contain the path to the socket. You can also expose an
 * abstract Unix socket, by including the leading null byte in the address
 *
 * If you are already handling a local socket logic in your application, you can also
 * use an overload which accepts a QLocalServer.
 *
 * The %PendingOperation returned by this method will be completed as soon as the tube is
 * open and ready to be used.
 *
 * \param address A valid path to an existing Unix socket or abstract Unix socket
 * \param parameters A dictionary of arbitrary Parameters to send with the tube offer.
 *                   Please read the specification for more details.
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \returns A %PendingOperation which will finish as soon as the tube is ready to be used
 *          (hence in the Open state)
 */
PendingDBusTubeOffer *OutgoingDBusTubeChannel::offerTube(
        const QVariantMap &parameters,
        bool requireCredentials)
{
    SocketAccessControl accessControl = requireCredentials ?
                                        SocketAccessControlCredentials :
                                        SocketAccessControlLocalhost;

    if (!isReady(DBusTubeChannel::FeatureDBusTube)) {
        warning() << "DBusTubeChannel::FeatureDBusTube must be ready before "
            "calling offerTube";
        return new PendingDBusTubeOffer(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"), OutgoingDBusTubeChannelPtr(this));
    }

    // The tube must be not offered
    if (state() != TubeChannelStateNotOffered) {
        warning() << "You can not expose more than a bus for each DBus Tube";
        return new PendingDBusTubeOffer(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"), OutgoingDBusTubeChannelPtr(this));
    }

    // Let's offer the tube
    if (!accessControls().contains(accessControl)) {
        warning() << "You requested an access control "
            "not supported by this channel";
        return new PendingDBusTubeOffer(QLatin1String(TP_QT_ERROR_NOT_IMPLEMENTED),
                QLatin1String("The requested access control is not supported"),
                OutgoingDBusTubeChannelPtr(this));
    }

    PendingString *ps = new PendingString(
        interface<Client::ChannelTypeDBusTubeInterface>()->Offer(
            parameters,
            accessControl),
        OutgoingDBusTubeChannelPtr(this));

    PendingDBusTubeOffer *op = new PendingDBusTubeOffer(ps, OutgoingDBusTubeChannelPtr(this));
    return op;
}

QString OutgoingDBusTubeChannel::address() const
{
    if (state() != TubeChannelStateOpen) {
        warning() << "OutgoingDBusTubeChannel::address() can be called only if "
            "the tube has already been opened";
        return QString();
    }

    return mPriv->address;
}

}
