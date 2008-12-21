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

#ifndef _TelepathyQt4_cli_pending_contact_attributes_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_contact_attributes_h_HEADER_GUARD_

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
class PendingContactAttributes;
}
}

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/PendingOperation>

namespace Telepathy
{
namespace Client
{
class Connection;
class ReferencedHandles;

/**
 * \class PendingContactAttributes
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/cli-pending-contact-attributes.h> <TelepathyQt4/Client/PendingContactAttributes>
 *
 * Class containing the parameters of and the reply to an asynchronous request for raw contact
 * attributes, as used in the Connection::getContactAttributes() low-level convenience method
 * wrapping the ConnectionInterfaceContactsInterface::GetContactAttributes() D-Bus method.
 */
class PendingContactAttributes : public PendingOperation
{
    Q_OBJECT

public:
    /**
     * Class destructor.
     */
    ~PendingContactAttributes();

    /**
     * Returns the Connection object through which the request was made.
     *
     * \return Pointer to the Connection.
     */
    Connection *connection() const;

    /**
     * Returns the contacts for which attributes were requested.
     *
     * \return Reference to a list with the handles of the contacts.
     */
    const UIntList &contactsRequested() const;

    /**
     * Returns the interfaces the corresponding attributes of which were requested.
     *
     * \return Reference to a list of D-Bus interface names.
     */
    const QStringList &interfacesRequested() const;

    /**
     * Returns whether it was requested that the contact handles should be referenced in addition to
     * fetching their attributes. This corresponds to the <code>reference</code> argument to
     * Connection::getContactAttributes().
     *
     * \return Whether the handles should be referenced or not.
     */
    bool shouldReference() const;

    /**
     * If referencing the handles was requested (as indicated by shouldReference()), returns the
     * now-referenced handles resulting from the operation. If the operation has not (yet) finished
     * successfully (isFinished() returns <code>false</code>), or referencing was not requested, the
     * return value is undefined.
     *
     * Even if referencing was requested, the list will not always contain all of the handles in
     * contactsRequested(), only the ones which were valid. The valid handles will be in the same
     * order as in contactsRequested(), though.
     *
     * \return ReferencedHandles instance containing the handles.
     */
    ReferencedHandles validHandles() const;

    /**
     * Returns the handles which were found to be invalid while processing the operation. If the
     * operation has not (yet) finished successfully (isFinished() returns <code>false</code>), the
     * return value is undefined.
     *
     * \return A list with the invalid handles.
     */
    UIntList invalidHandles() const;

    /**
     * Returns a dictionary mapping the valid contact handles in contactsRequested() (the contents
     * of handles()) to contact attributes. If the operation has not (yet) finished successfully
     * (isFinished() returns <code>false</code>), the return value is undefined.
     *
     * \return Mapping from handles to variant maps containing the attributes.
     */
    ContactAttributesMap attributes() const;

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *watcher);

private:
    friend class Connection;

    PendingContactAttributes(Connection *connection, const UIntList &handles,
            const QStringList &interfaces, bool reference);
    void setUnsupported();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
