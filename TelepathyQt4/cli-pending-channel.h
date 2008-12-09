/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_pending_channel_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_channel_h_HEADER_GUARD_

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

/**
 * \defgroup clientconn Connection proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Connections and their optional
 * interfaces.
 */

namespace Telepathy
{
namespace Client
{
class PendingChannel;
}
}

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingOperation>

namespace Telepathy
{
namespace Client
{

/**
 * \class PendingChannel
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/cli-pending-channel.h> <TelepathyQt4/Client/Connection>
 *
 * Class containing the parameters of and the reply to an asynchronous channel
 * request. Instances of this class cannot be constructed directly; the only way
 * to get one is to use Connection::requestChannel().
 */
class PendingChannel : public PendingOperation
{
    Q_OBJECT

public:
    /**
     * Class destructor.
     */
    ~PendingChannel();

    /**
     * Returns the Connection object through which the channel request was made.
     *
     * \return Pointer to the Connection.
     */
    Connection* connection() const;

    /**
     * Returns the channel type specified in the channel request.
     *
     * \return The D-Bus interface name of the interface specific to the
     *         requested channel type.
     */
    const QString& channelType() const;

    /**
     * Returns the handle type specified in the channel request.
     *
     * \return The handle type, as specified in #HandleType.
     */
    uint handleType() const;

    /**
     * Returns the handle specified in the channel request.
     *
     * \return The handle.
     */
    uint handle() const;

    /**
     * Returns a newly constructed Channel high-level proxy object associated
     * with the remote channel resulting from the channel request. If isValid()
     * returns <code>false</code>, the request has not (at least yet) completed
     * successfully, and 0 will be returned.
     *
     * \param parent Passed to the Channel constructor.
     * \return Pointer to the new Channel object.
     */
    Channel* channel(QObject* parent = 0) const;

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);

private:
    friend class Connection;

    PendingChannel(Connection* connection, const QString& type, uint handleType, uint handle);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
