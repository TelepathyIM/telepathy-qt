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

#ifndef _TelepathyQt4_Client_Channel_HEADER_GUARD_
#define _TelepathyQt4_Client_Channel_HEADER_GUARD_

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
 * \defgroup clientchannel Channel proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Channels and their optional
 * interfaces.
 */

#include <TelepathyQt4/_gen/cli-channel.h>

namespace Telepathy
{
namespace Client
{

/**
 * \class Channel
 * \ingroup clientchannel
 *
 * High-level proxy object for accessing remote %Telepathy %Channel objects.
 */
class Channel : public ChannelInterface
{
    Q_OBJECT

public:
    /**
     * Creates a Channel associated with the given object on the session bus.
     *
     * \param serviceName Name of the service the object is on.
     * \param objectPath  Path to the object on the service.
     * \param parent      Passed to the parent class constructor.
     */
    Channel(const QString& serviceName,
            const QString& objectPath,
            QObject* parent = 0);

    /**
     * Creates a Channel associated with the given object on the given bus.
     *
     * \param connection  The bus via which the object can be reached.
     * \param serviceName Name of the service the object is on.
     * \param objectPath  Path to the object on the service.
     * \param parent      Passed to the parent class constructor.
     */
    Channel(const QDBusConnection& connection,
            const QString &serviceName,
            const QString &objectPath,
            QObject* parent = 0);

    /**
     * Class destructor.
     */
    ~Channel();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif
