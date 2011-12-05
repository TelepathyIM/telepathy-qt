/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt/PendingContactAttributes>

#include "TelepathyQt/_gen/pending-contact-attributes.moc.hpp"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ReferencedHandles>

#include "TelepathyQt/debug-internal.h"

namespace Tp
{

/**
 * \class PendingContactAttributes
 * \ingroup clientconn
 * \headerfile TelepathyQt/pending-contact-attributes.h <TelepathyQt/PendingContactAttributes>
 *
 * \brief The PendingContactAttributes class represents the parameters of and
 * the reply to an asynchronous request for raw contact attributes, as used in
 * the ConnectionLowlevel::contactAttributes() low-level convenience method wrapping the
 * Client::ConnectionInterfaceContactsInterface::GetContactAttributes() D-Bus
 * method.
 *
 * See \ref async_model
 */

struct TP_QT_NO_EXPORT PendingContactAttributes::Private
{
    UIntList contactsRequested;
    QStringList interfacesRequested;
    bool shouldReference;
    ReferencedHandles validHandles;
    UIntList invalidHandles;
    ContactAttributesMap attributes;
};

PendingContactAttributes::PendingContactAttributes(const ConnectionPtr &connection,
        const UIntList &handles, const QStringList &interfaces, bool reference)
    : PendingOperation(connection),
      mPriv(new Private)
{
    mPriv->contactsRequested = handles;
    mPriv->interfacesRequested = interfaces;
    mPriv->shouldReference = reference;
}

/**
 * Class destructor.
 */
PendingContactAttributes::~PendingContactAttributes()
{
    delete mPriv;
}

/**
 * Return the connection through which the request was made.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr PendingContactAttributes::connection() const
{
    return ConnectionPtr(qobject_cast<Connection*>((Connection*) object().data()));
}

/**
 * Return the contacts for which attributes were requested.
 *
 * \return Reference to a list with the handles of the contacts.
 */
const UIntList &PendingContactAttributes::contactsRequested() const
{
    return mPriv->contactsRequested;
}

/**
 * Return the interfaces the corresponding attributes of which were requested.
 *
 * \return Reference to a list of D-Bus interface names.
 */
const QStringList &PendingContactAttributes::interfacesRequested() const
{
    return mPriv->interfacesRequested;
}

/**
 * Return whether it was requested that the contact handles should be referenced in addition to
 * fetching their attributes. This corresponds to the <code>reference</code> argument to
 * Connection::contactAttributes().
 *
 * \return Whether the handles should be referenced or not.
 */
bool PendingContactAttributes::shouldReference() const
{
    return mPriv->shouldReference;
}

/**
 * If referencing the handles was requested (as indicated by shouldReference()), returns the
 * now-referenced handles resulting from the operation. If the operation has not (yet) finished
 * successfully (isFinished() returns <code>false</code>), or referencing was not requested, the
 * return value is undefined.
 *
 * Even if referencing was requested, the list will not always contain all of the handles in
 * contactsRequested(), only the ones which were valid. The valid handles will be in the same order
 * as in contactsRequested(), though.
 *
 * \return ReferencedHandles instance containing the handles.
 */
ReferencedHandles PendingContactAttributes::validHandles() const
{
    if (!isFinished()) {
        warning() << "PendingContactAttributes::validHandles() called before finished";
    } else if (isError()) {
        warning() << "PendingContactAttributes::validHandles() called when errored";
    } else if (!shouldReference()) {
        warning() << "PendingContactAttributes::validHandles() called but weren't asked to"
                  << "reference handles";
    }

    return mPriv->validHandles;
}

/**
 * Return the handles which were found to be invalid while processing the operation. If the
 * operation has not (yet) finished successfully (isFinished() returns <code>false</code>), the
 * return value is undefined.
 *
 * \return A list with the invalid handles.
 */
UIntList PendingContactAttributes::invalidHandles() const
{
    if (!isFinished()) {
        warning() << "PendingContactAttributes::validHandles() called before finished";
    } else if (isError()) {
        warning() << "PendingContactAttributes::validHandles() called when errored";
    }

    return mPriv->invalidHandles;
}

/**
 * Return a dictionary mapping the valid contact handles in contactsRequested() (when also
 * referencing, this means the contents of validHandles()) to contact attributes. If the operation
 * has not (yet) finished successfully (isFinished() returns <code>false</code>), the return value
 * is undefined.
 *
 * \return Mapping from handles to variant maps containing the attributes.
 */
ContactAttributesMap PendingContactAttributes::attributes() const
{
    if (!isFinished()) {
        warning() << "PendingContactAttributes::validHandles() called before finished";
    } else if (isError()) {
        warning() << "PendingContactAttributes::validHandles() called when errored";
    }

    return mPriv->attributes;
}

void PendingContactAttributes::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<ContactAttributesMap> reply = *watcher;

    if (reply.isError()) {
        debug().nospace() << "GetCAs: error " << reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    } else {
        mPriv->attributes = reply.value();

        UIntList validHandles;
        foreach (uint contact, mPriv->contactsRequested) {
            if (mPriv->attributes.contains(contact)) {
                validHandles << contact;
            } else {
                mPriv->invalidHandles << contact;
            }
        }

        if (shouldReference()) {
            mPriv->validHandles = ReferencedHandles(connection(), HandleTypeContact,
                    validHandles);
        }

        setFinished();
    }

    connection()->handleRequestLanded(HandleTypeContact);

    watcher->deleteLater();
}

void PendingContactAttributes::failImmediately(const QString &error, const QString &errorMessage)
{
    setFinishedWithError(error, errorMessage);
}

} // Tp
