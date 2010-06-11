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
#include "TelepathyQt/dbus-tube-channel-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

#include "TelepathyQt/debug-internal.h"

namespace Tp
{

class TP_QT_NO_EXPORT OutgoingDBusTubeChannelPrivate : public DBusTubeChannelPrivate
{
    Q_DECLARE_PUBLIC(OutgoingDBusTubeChannel)
public:
    OutgoingDBusTubeChannelPrivate(OutgoingDBusTubeChannel* parent);
    virtual ~OutgoingDBusTubeChannelPrivate();

    QDBusServer *server;
};

OutgoingDBusTubeChannelPrivate::OutgoingDBusTubeChannelPrivate(OutgoingDBusTubeChannel* parent)
        : DBusTubeChannelPrivate(parent)
        , server(0)
{
}

OutgoingDBusTubeChannelPrivate::~OutgoingDBusTubeChannelPrivate()
{
}



struct TP_QT_NO_EXPORT PendingDBusTubeOfferPrivate
{
    PendingDBusTubeOfferPrivate(PendingDBusTubeOffer *parent);
    ~PendingDBusTubeOfferPrivate();

    // Public object
    PendingDBusTubeOffer *parent;

    OutgoingDBusTubeChannelPtr tube;

    // Private slots
    void onOfferFinished(Tp::PendingOperation* op);
    void onStateChanged(Tp::TubeChannelState state);
};

PendingDBusTubeOfferPrivate::PendingDBusTubeOfferPrivate(PendingDBusTubeOffer* parent)
    : parent(parent)
{
}

PendingDBusTubeOfferPrivate::~PendingDBusTubeOfferPrivate()
{
}

void PendingDBusTubeOfferPrivate::onOfferFinished(PendingOperation* op)
{
    if (op->isError()) {
        // Fail
        parent->setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Offer tube finished successfully";

    // Now get the address and set it
    PendingString *ps = qobject_cast< PendingString* >(op);
    tube->d_func()->address = ps->result();

    // It might have been already opened - check
    if (tube->state() == TubeChannelStateOpen) {
        onStateChanged(tube->state());
    } else {
        // Wait until the tube gets opened on the other side
        parent->connect(tube.data(), SIGNAL(stateChanged(Tp::TubeChannelState)),
                        parent, SLOT(onStateChanged(Tp::TubeChannelState)));
    }
}

void PendingDBusTubeOfferPrivate::onStateChanged(TubeChannelState state)
{
    debug() << "Tube state changed to " << state;
    if (state == TubeChannelStateOpen) {
        // The tube is ready: let's create the QDBusServer
        QDBusServer *server = new QDBusServer(tube->d_func()->address, tube.data());
        if (!server->isConnected()) {
            // Something went wrong
            warning() << "Could not create a QDBusServer";
            parent->setFinishedWithError(QLatin1String("Connection refused"),
                      QLatin1String("Could not create a valid QDBusServer from the tube"));
        } else {
            // Inject the server
            tube->d_func()->server = server;
            parent->setFinished();
        }
    } else if (state != TubeChannelStateRemotePending) {
        // Something happened
        parent->setFinishedWithError(QLatin1String("Connection refused"),
                      QLatin1String("The connection to this tube was refused"));
    }
}

PendingDBusTubeOffer::PendingDBusTubeOffer(
        PendingString* string,
        const OutgoingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTubeOfferPrivate(this))
{
    mPriv->tube = object;

    if (string->isFinished()) {
        mPriv->onOfferFinished(string);
    } else {
        // Connect the pending void
        connect(string, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onOfferFinished(Tp::PendingOperation*)));
    }
}

PendingDBusTubeOffer::PendingDBusTubeOffer(
        const QString& errorName,
        const QString& errorMessage,
        const OutgoingDBusTubeChannelPtr &object)
    : PendingOperation(object)
    , mPriv(new PendingDBusTubeOfferPrivate(this))
{
    setFinishedWithError(errorName, errorMessage);
}

PendingDBusTubeOffer::~PendingDBusTubeOffer()
{
    delete mPriv;
}

QDBusServer* PendingDBusTubeOffer::server()
{
    return mPriv->tube->server();
}



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
    : DBusTubeChannel(connection, objectPath, immutableProperties, *new OutgoingDBusTubeChannelPrivate(this))
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
PendingDBusTubeOffer* OutgoingDBusTubeChannel::offerTube(
        const QVariantMap& parameters,
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

    Q_D(OutgoingDBusTubeChannel);

    // Let's offer the tube
    if (!d->accessControls.contains(accessControl)) {
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

QDBusServer* OutgoingDBusTubeChannel::server()
{
    if (state() != TubeChannelStateOpen) {
        warning() << "OutgoingDBusTubeChannel::server() can be called only if "
            "the tube has already been opened";
        return 0;
    }

    Q_D(OutgoingDBusTubeChannel);

    return d->server;
}

}

#include "TelepathyQt/_gen/outgoing-dbus-tube-channel.moc.hpp"
