/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/Client/PendingReadyConnectionManager>

#include "TelepathyQt4/Client/_gen/pending-ready-connection-manager.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

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

namespace Telepathy
{
namespace Client
{

struct PendingReadyConnectionManager::Private
{
    Private(ConnectionManager::Features requestedFeatures, ConnectionManager *connectionManager) :
        requestedFeatures(requestedFeatures),
        connectionManager(connectionManager)
    {
    }

    ConnectionManager::Features requestedFeatures;
    ConnectionManager *connectionManager;
};

/**
 * \class PendingReadyConnectionManager
 * \ingroup clientcm
 * \headerfile <TelepathyQt4/Client/pending-ready-connection-manager.h> <TelepathyQt4/Client/PendingReadyConnectionManager>
 *
 * Class containing the features requested and the reply to a request
 * for a connection manager to become ready. Instances of this class cannot be
 * constructed directly; the only way to get one is via ConnectionManager::becomeReady().
 */

/**
 * Construct a PendingReadyConnectionManager object.
 *
 * \param connectionManager The ConnectionManager that will become ready.
 */
PendingReadyConnectionManager::PendingReadyConnectionManager(ConnectionManager::Features requestedFeatures, ConnectionManager *connectionManager)
    : PendingOperation(connectionManager),
      mPriv(new Private(requestedFeatures, connectionManager))
{
}

/**
 * Class destructor.
 */
PendingReadyConnectionManager::~PendingReadyConnectionManager()
{
    delete mPriv;
}

/**
 * Return the ConnectionManager object through which the request was made.
 *
 * \return ConnectionManager object.
 */
ConnectionManager *PendingReadyConnectionManager::connectionManager() const
{
    return mPriv->connectionManager;
}

/**
 * Return the Features that were requested to become ready on the
 * connection manager.
 *
 * \return Features.
 */
ConnectionManager::Features PendingReadyConnectionManager::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

} // Telepathy::Client
} // Telepathy
