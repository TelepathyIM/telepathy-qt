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

#include "TelepathyQt/_gen/incoming-dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingDBusTube>
#include <TelepathyQt/PendingString>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT IncomingDBusTubeChannel::Private
{
public:
    Private(IncomingDBusTubeChannel *parent);
    virtual ~Private();

    // Public object
    IncomingDBusTubeChannel *parent;
};

IncomingDBusTubeChannel::Private::Private(IncomingDBusTubeChannel *parent)
        : parent(parent)
{
}

IncomingDBusTubeChannel::Private::~Private()
{
}


/**
 * \class IncomingDBusTubeChannel
 * \headerfile TelepathyQt/incoming-dbus-tube-channel.h <TelepathyQt/IncomingDBusTubeChannel>
 *
 * An high level wrapper for managing an incoming DBus tube
 *
 * \c IncomingDBusTubeChannel is an high level wrapper for managing Telepathy interface
 * #TELEPATHY_INTERFACE_CHANNEL_TYPE_DBUS_TUBE.
 * In particular, this class is meant to be used as a comfortable way for accepting incoming requests.
 *
 * \section incoming_dbus_tube_usage_sec Usage
 *
 * \subsection incoming_dbus_tube_create_sec Obtaining an incoming DBus tube
 *
 * Whenever a contact invites you to open an incoming DBus tube, if you are registered
 * as a channel handler for the channel type #TELEPATHY_INTERFACE_CHANNEL_TYPE_DBUS_TUBE,
 * you will be notified of the offer and you will be able to handle the channel.
 * Please refer to the documentation of #AbstractClientHandler for more
 * details on channel handling.
 *
 * Supposing your channel handler has been created correctly, you would do:
 *
 * \code
 * void MyChannelManager::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
 *                               const Tp::AccountPtr &account,
 *                               const Tp::ConnectionPtr &connection,
 *                               const QList<Tp::ChannelPtr> &channels,
 *                               const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
 *                               const QDateTime &userActionTime,
 *                               const QVariantMap &handlerInfo)
 * {
 *     foreach(const Tp::ChannelPtr &channel, channels) {
 *         QVariantMap properties = channel->immutableProperties();
 *
 *         if (properties[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] ==
 *                        TELEPATHY_INTERFACE_CHANNEL_TYPE_DBUS_TUBE) {
 *
 *             // Handle the channel
 *             Tp::IncomingDBusTubeChannelPtr myTube =
 *                      Tp::IncomingDBusTubeChannelPtr::qObjectCast(channel);
 *
 *          }
 *     }
 *
 *     context->setFinished();
 * }
 * \endcode
 *
 * \subsection incoming_dbus_tube_accept_sec Accepting the tube
 *
 * Before being ready to accept the tube, we must be sure the required features on our object
 * are ready. In this case, we need to enable TubeChannel::FeatureTube and
 * DBusTubeChannel::FeatureDBusTube.
 *
 * DBusTubeChannel features can be enabled by constructing a ChannelFactory and enabling the desired features,
 * and passing it to ChannelRequest or ClientRegistrar when creating them as appropriate. However,
 * if a particular feature is only ever used in a specific circumstance, such as an user opening
 * some settings dialog separate from the general view of the application, features can be later
 * enabled as needed by calling becomeReady().
 *
 * Once your object is ready, you can use #acceptTube to accept the tube and create a brand
 * new private DBus connection.
 *
 * The returned PendingDBusTube serves both for monitoring the state of the tube and for
 * obtaining, upon success, the address of the new connection.
 * When the operation finishes, you can do:
 *
 * \code
 * void MyTubeReceiver::onDBusTubeAccepted(PendingDBusTube *op)
 * {
 *     if (op->isError()) {
 *        return;
 *     }
 *
 *     QString address = op->address();
 *     // Connect to the address
 * }
 * \endcode
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
 * retrieve the address either from \c PendingDBusTube or from %address().
 *
 * This method requires DBusTubeChannel::FeatureDBusTube to be enabled.
 *
 * \param requireCredentials Whether the server should require an SCM_CREDENTIALS message
 *                           upon connection.
 *
 * \return A %PendingDBusTube which will finish as soon as the tube is ready to be used
 *         (hence in the Open state)
 */
PendingDBusTube *IncomingDBusTubeChannel::acceptTube(bool requireCredentials)
{
    SocketAccessControl accessControl = requireCredentials ?
                                        SocketAccessControlCredentials :
                                        SocketAccessControlLocalhost;

    if (!isReady(DBusTubeChannel::FeatureDBusTube)) {
        warning() << "DBusTubeChannel::FeatureDBusTube must be ready before "
            "calling offerTube";
        return new PendingDBusTube(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel not ready"), IncomingDBusTubeChannelPtr(this));
    }

    // The tube must be in local pending state
    if (state() != TubeChannelStateLocalPending) {
        warning() << "You can accept tubes only when they are in LocalPending state";
        return new PendingDBusTube(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
                QLatin1String("Channel busy"), IncomingDBusTubeChannelPtr(this));
    }

    // Let's offer the tube
    if (requireCredentials && !supportsCredentials()) {
        warning() << "You requested an access control "
            "not supported by this channel";
        return new PendingDBusTube(QLatin1String(TP_QT_ERROR_NOT_IMPLEMENTED),
                QLatin1String("The requested access control is not supported"),
                IncomingDBusTubeChannelPtr(this));
    }

    PendingString *ps = new PendingString(
        interface<Client::ChannelTypeDBusTubeInterface>()->Accept(
            accessControl),
        IncomingDBusTubeChannelPtr(this));

    PendingDBusTube *op = new PendingDBusTube(ps, IncomingDBusTubeChannelPtr(this));
    return op;
}

}
