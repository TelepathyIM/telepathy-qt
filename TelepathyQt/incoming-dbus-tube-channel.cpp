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

#include <TelepathyQt/IncomingDBusTubeChannel>
#include "TelepathyQt/incoming-dbus-tube-channel-internal.h"

#include "TelepathyQt/_gen/incoming-dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingDBusTubeAccept>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

IncomingDBusTubeChannel::Private::Private(IncomingDBusTubeChannel *parent)
        : parent(parent)
{
}

IncomingDBusTubeChannel::Private::~Private()
{
}


/**
 * \class IncomingDBusTubeChannel
 * \headerfile TelepathyQt/stream-tube.h <TelepathyQt/IncomingDBusTubeChannel>
 *
 * \brief A class representing a Stream Tube
 *
 * \c IncomingDBusTubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Type.StreamTubeChannel.
 * In particular, this class is meant to be used as a comfortable way for exposing new tubes.
 * It provides a set of overloads for exporting a variety of sockets over a stream tube.
 *
 * For more details, please refer to Telepathy spec.
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
PendingDBusTubeAccept *IncomingDBusTubeChannel::acceptTube(
        bool requireCredentials)
{
    SocketAccessControl accessControl = requireCredentials ?
                                        SocketAccessControlCredentials :
                                        SocketAccessControlLocalhost;

    if (!isReady(DBusTubeChannel::FeatureDBusTube)) {
        warning() << "DBusTubeChannel::FeatureDBusTube must be ready before "
            "calling offerTube";
        return new PendingDBusTubeAccept(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"), IncomingDBusTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingDBusTubeAccept(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"), IncomingDBusTubeChannelPtr(this));
    }

    // Let's offer the tube
    if (!accessControls().contains(accessControl)) {
        warning() << "You requested an access control "
            "not supported by this channel";
        return new PendingDBusTubeAccept(QLatin1String(TP_QT_ERROR_NOT_IMPLEMENTED),
                QLatin1String("The requested access control is not supported"),
                IncomingDBusTubeChannelPtr(this));
    }

    PendingString *ps = new PendingString(
        interface<Client::ChannelTypeDBusTubeInterface>()->Accept(
            accessControl),
        IncomingDBusTubeChannelPtr(this));

    PendingDBusTubeAccept *op = new PendingDBusTubeAccept(ps, IncomingDBusTubeChannelPtr(this));
    return op;
}

QString IncomingDBusTubeChannel::address() const
{
    if (state() != TubeChannelStateOpen) {
        warning() << "IncomingDBusTubeChannel::address() can be called only if "
            "the tube has already been opened";
        return QString();
    }

    return mPriv->address;
}

}
