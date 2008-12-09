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

#ifndef _TelepathyQt4_cli_pending_handles_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_handles_h_HEADER_GUARD_

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
class PendingHandles;
}
}

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/ReferencedHandles>

namespace Telepathy
{
namespace Client
{

/**
 * \class PendingHandles
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/cli-pending-handles.h> <TelepathyQt4/Client/Connection>
 *
 * Class containing the parameters of and the reply to an asynchronous handle
 * request/hold. Instances of this class cannot be constructed directly; the
 * only ways to get one are to use Connection::requestHandles() or
 * Connection::referenceHandles().
 */
class PendingHandles : public PendingOperation
{
    Q_OBJECT

public:
    /**
     * Class destructor.
     */
    ~PendingHandles();

    /**
     * Returns the Connection object through which the operation was made.
     *
     * \return Pointer to the Connection.
     */
    Connection* connection() const;

    /**
     * Returns the handle type specified in the operation.
     *
     * \return The handle type, as specified in #HandleType.
     */
    uint handleType() const;

    /**
     * Returns whether the operation was a handle request (as opposed to a
     * reference of existing handles).
     *
     * \sa isReference()
     *
     * \return Whether the operation was a request (== !isReference()).
     */
    bool isRequest() const;

    /**
     * Returns whether the operation was a handle reference (as opposed to a
     * request for new handles).
     *
     * \sa isRequest()
     *
     * \return Whether the operation was a reference (== !isRequest()).
     */
    bool isReference() const;

    /**
     * If the operation was a request (as returned by isRequest()), returns the
     * names of the entities for which handles were requested for. Otherwise,
     * returns an empty list.
     *
     * \return Reference to a list of the names of the entities.
     */
    const QStringList& namesRequested() const;

    /**
     * If the operation was a reference (as returned by isReference()), returns
     * the handles which were to be referenced. Otherwise, returns an empty
     * list.
     *
     * \return Reference to a list of the handles specified to be referenced.
     */
    const UIntList& handlesToReference() const;

    /**
     * Returns the now-referenced handles resulting from the operation. If the
     * operation has not (yet) finished successfully (isFinished() returns
     * <code>false</code>), the return value is undefined.
     *
     * For requests of new handles, <code>handles()[i]</code> will be the handle
     * corresponding to the entity name <code>namesToRequest()[i]</code>. For
     * references of existing handles, <code>handles()[i] ==
     * handlesToReference()[i]</code> will be true for any <code>i</code>.
     *
     * \return ReferencedHandles instance containing the handles.
     */
    ReferencedHandles handles() const;

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);

private:
    friend class Connection;

    PendingHandles(Connection* connection, uint handleType, const QStringList& names);
    PendingHandles(Connection* connection, uint handleType, const UIntList& handles, bool allHeld);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
