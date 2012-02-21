/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2011 Nokia Corporation
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

#include <TelepathyQt/PendingContacts>
#include "TelepathyQt/pending-contacts-internal.h"

#include "TelepathyQt/_gen/pending-contacts.moc.hpp"
#include "TelepathyQt/_gen/pending-contacts-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingContactAttributes>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/ReferencedHandles>

// FIXME: Refactor PendingContacts code to make it more readable/maintainable and reuse common code
//        when appropriate.

namespace Tp
{

struct TP_QT_NO_EXPORT PendingContacts::Private
{
    Private(PendingContacts *parent, const ContactManagerPtr &manager, const UIntList &handles,
            const Features &features, const Features &missingFeatures,
            const QMap<uint, ContactPtr> &satisfyingContacts)
        : parent(parent),
          manager(manager),
          features(features),
          missingFeatures(missingFeatures),
          satisfyingContacts(satisfyingContacts),
          requestType(PendingContacts::ForHandles),
          handles(handles),
          nested(0)
    {
    }

    Private(PendingContacts *parent, const ContactManagerPtr &manager, const QStringList &list,
            PendingContacts::RequestType type, const Features &features)
        : parent(parent),
          manager(manager),
          features(features),
          missingFeatures(features),
          requestType(type),
          addresses(list),
          nested(0)
    {
        if (type != PendingContacts::ForIdentifiers &&
            type != PendingContacts::ForUris) {
            Q_ASSERT(false);
        }
    }

    Private(PendingContacts *parent, const ContactManagerPtr &manager, const QString &vcardField,
            const QStringList &vcardAddresses, const Features &features)
        : parent(parent),
          manager(manager),
          features(features),
          missingFeatures(features),
          requestType(PendingContacts::ForVCardAddresses),
          addresses(vcardAddresses),
          vcardField(vcardField),
          nested(0)
    {
    }

    Private(PendingContacts *parent,
            const ContactManagerPtr &manager, const QList<ContactPtr> &contactsToUpgrade,
            const Features &features)
        : parent(parent),
          manager(manager),
          features(features),
          requestType(PendingContacts::Upgrade),
          contactsToUpgrade(contactsToUpgrade),
          nested(0)
    {
    }

    void setFinished();

    bool checkRequestTypeAndState(const char *methodName, const char *debug, RequestType type);

    // Public object
    PendingContacts *parent;

    // Generic parameters
    ContactManagerPtr manager;
    Features features;
    Features missingFeatures;
    QMap<uint, ContactPtr> satisfyingContacts;

    // Request type specific parameters
    RequestType requestType;
    UIntList handles;
    QStringList addresses;
    QString vcardField;
    QList<ContactPtr> contactsToUpgrade;
    PendingContacts *nested;

    // Results
    QList<ContactPtr> contacts;
    UIntList invalidHandles;
    QStringList validIds;
    QHash<QString, QPair<QString, QString> > invalidIds;
    QStringList validAddresses;
    QStringList invalidAddresses;

    ReferencedHandles handlesToInspect;
};

void PendingContacts::Private::setFinished()
{
    ConnectionLowlevelPtr connLowlevel = manager->connection()->lowlevel();
    UIntList handles = invalidHandles;
    foreach (uint handle, handles) {
        if (connLowlevel->hasContactId(handle)) {
            satisfyingContacts.insert(handle, manager->ensureContact(handle,
                        connLowlevel->contactId(handle), missingFeatures));
            invalidHandles.removeOne(handle);
        }
    }

    parent->setFinished();
}

bool PendingContacts::Private::checkRequestTypeAndState(const char *methodName,
        const char *debug,
        RequestType type)
{
    if (!parent->isFinished()) {
        warning().nospace() << "PendingContacts::" << methodName << "() called before finished";
        return false;
    } else if (parent->isError()) {
        warning().nospace() << "PendingContacts::" << methodName << "() called when errored";
        return false;
    } else if (requestType != type) {
        warning().nospace() << "PendingContacts::" << methodName << "() called for" <<
            this << "which is not for " << debug;
        return false;
    }

    return true;
}

/**
 * \class PendingContacts
 * \ingroup clientconn
 * \headerfile TelepathyQt/pending-contacts.h <TelepathyQt/PendingContacts>
 *
 * \brief The PendingContacts class is used by ContactManager when
 * creating/updating Contact objects.
 *
 * See \ref async_model
 */

PendingContacts::PendingContacts(const ContactManagerPtr &manager,
        const UIntList &handles,
        const Features &features,
        const Features &missingFeatures,
        const QStringList &interfaces,
        const QMap<uint, ContactPtr> &satisfyingContacts,
        const QSet<uint> &otherContacts,
        const QString &errorName,
        const QString &errorMessage)
    : PendingOperation(manager->connection()),
      mPriv(new Private(this, manager, handles, features, missingFeatures, satisfyingContacts))
{
    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }

    if (!otherContacts.isEmpty()) {
        ConnectionPtr conn = manager->connection();
        if (conn->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS)) {
            PendingContactAttributes *attributes =
                conn->lowlevel()->contactAttributes(otherContacts.toList(),
                        interfaces, true);

            connect(attributes,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onAttributesFinished(Tp::PendingOperation*)));
        } else {
            // fallback to just create the contacts
            PendingHandles *handles = conn->lowlevel()->referenceHandles(HandleTypeContact,
                    otherContacts.toList());
            connect(handles,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onReferenceHandlesFinished(Tp::PendingOperation*)));
        }
    } else {
        allAttributesFetched();
    }
}

PendingContacts::PendingContacts(const ContactManagerPtr &manager,
        const QStringList &list, RequestType type,
        const Features &features, const QStringList &interfaces,
        const QString &errorName, const QString &errorMessage)
    : PendingOperation(manager->connection()),
      mPriv(new Private(this, manager, list, type, features))
{
    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }

    ConnectionPtr conn = manager->connection();

    if (type == ForIdentifiers) {
        Q_ASSERT(interfaces.isEmpty());
        PendingHandles *handles = conn->lowlevel()->requestHandles(HandleTypeContact, list);
        connect(handles,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onRequestHandlesFinished(Tp::PendingOperation*)));
    } else if (type == ForUris) {
        Client::ConnectionInterfaceAddressingInterface *connAddressingIface =
            conn->optionalInterface<Client::ConnectionInterfaceAddressingInterface>(
                    OptionalInterfaceFactory<Connection>::CheckInterfaceSupported);
        if (!connAddressingIface) {
            setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                    QLatin1String("Connection does not support Addressing interface"));
            return;
        }

        PendingAddressingGetContacts *pa = new PendingAddressingGetContacts(conn, list, interfaces);
        connect(pa,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAddressingGetContactsFinished(Tp::PendingOperation*)));
    }
}

PendingContacts::PendingContacts(const ContactManagerPtr &manager,
        const QString &vcardField, const QStringList &vcardAddresses,
        const Features &features, const QStringList &interfaces,
        const QString &errorName, const QString &errorMessage)
    : PendingOperation(manager->connection()),
      mPriv(new Private(this, manager, vcardField, vcardAddresses, features))
{
    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }

    ConnectionPtr conn = manager->connection();

    Client::ConnectionInterfaceAddressingInterface *connAddressingIface =
        conn->optionalInterface<Client::ConnectionInterfaceAddressingInterface>(
                OptionalInterfaceFactory<Connection>::CheckInterfaceSupported);
    if (!connAddressingIface) {
        setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Connection does not support Addressing interface"));
        return;
    }

    PendingAddressingGetContacts *pa = new PendingAddressingGetContacts(conn,
            vcardField, vcardAddresses, interfaces);
    connect(pa,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAddressingGetContactsFinished(Tp::PendingOperation*)));
}

PendingContacts::PendingContacts(const ContactManagerPtr &manager,
        const QList<ContactPtr> &contacts, const Features &features,
        const QString &errorName, const QString &errorMessage)
    : PendingOperation(manager->connection()),
      mPriv(new Private(this, manager, contacts, features))
{
    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }

    UIntList handles;
    foreach (const ContactPtr &contact, contacts) {
        handles.push_back(contact->handle()[0]);
    }

    mPriv->nested = manager->contactsForHandles(handles, features);
    connect(mPriv->nested,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onNestedFinished(Tp::PendingOperation*)));
}

/**
 * Class destructor.
 */
PendingContacts::~PendingContacts()
{
    delete mPriv;
}

ContactManagerPtr PendingContacts::manager() const
{
    return mPriv->manager;
}

Features PendingContacts::features() const
{
    return mPriv->features;
}

bool PendingContacts::isForHandles() const
{
    return mPriv->requestType == ForHandles;
}

UIntList PendingContacts::handles() const
{
    if (!isForHandles()) {
        warning() << "Tried to get handles from" << this << "which is not for handles!";
    }

    return mPriv->handles;
}

bool PendingContacts::isForIdentifiers() const
{
    return mPriv->requestType == ForIdentifiers;
}

QStringList PendingContacts::identifiers() const
{
    if (!isForIdentifiers()) {
        warning() << "Tried to get identifiers from" << this << "which is not for identifiers!";
        return QStringList();
    }

    return mPriv->addresses;
}

bool PendingContacts::isForVCardAddresses() const
{
    return mPriv->requestType == ForVCardAddresses;
}

QString PendingContacts::vcardField() const
{
    if (!isForVCardAddresses()) {
        warning() << "Tried to get vcard field from" << this << "which is not for vcard addresses!";
    }

    return mPriv->vcardField;
}

QStringList PendingContacts::vcardAddresses() const
{
    if (!isForVCardAddresses()) {
        warning() << "Tried to get vcard addresses from" << this << "which is not for vcard addresses!";
        return QStringList();
    }

    return mPriv->addresses;
}

bool PendingContacts::isForUris() const
{
    return mPriv->requestType == ForUris;
}

QStringList PendingContacts::uris() const
{
    if (!isForUris()) {
        warning() << "Tried to get uris from" << this << "which is not for uris!";
        return QStringList();
    }

    return mPriv->addresses;
}

bool PendingContacts::isUpgrade() const
{
    return mPriv->requestType == Upgrade;
}

QList<ContactPtr> PendingContacts::contactsToUpgrade() const
{
    if (!isUpgrade()) {
        warning() << "Tried to get contacts to upgrade from" << this << "which is not an upgrade!";
    }

    return mPriv->contactsToUpgrade;
}

QList<ContactPtr> PendingContacts::contacts() const
{
    if (!isFinished()) {
        warning() << "PendingContacts::contacts() called before finished";
    } else if (isError()) {
        warning() << "PendingContacts::contacts() called when errored";
    }

    return mPriv->contacts;
}

UIntList PendingContacts::invalidHandles() const
{
    if (!mPriv->checkRequestTypeAndState("invalidHandles", "handles", ForHandles)) {
        return UIntList();
    }

    return mPriv->invalidHandles;
}

QStringList PendingContacts::validIdentifiers() const
{
    if (!mPriv->checkRequestTypeAndState("validIdentifiers", "IDs", ForIdentifiers)) {
        return QStringList();
    }

    return mPriv->validIds;
}

QHash<QString, QPair<QString, QString> > PendingContacts::invalidIdentifiers() const
{
    if (!mPriv->checkRequestTypeAndState("invalidIdentifiers", "IDs", ForIdentifiers)) {
        return QHash<QString, QPair<QString, QString> >();
    }

    return mPriv->invalidIds;
}

QStringList PendingContacts::validVCardAddresses() const
{
    if (!mPriv->checkRequestTypeAndState("validVCardAddresses", "vcard addresses", ForVCardAddresses)) {
        return QStringList();
    }

    return mPriv->validAddresses;
}

QStringList PendingContacts::invalidVCardAddresses() const
{
    if (!mPriv->checkRequestTypeAndState("invalidVCardAddresses", "vcard addresses", ForVCardAddresses)) {
        return QStringList();
    }

    return mPriv->invalidAddresses;
}

QStringList PendingContacts::validUris() const
{
    if (!mPriv->checkRequestTypeAndState("validUris", "URIS", ForUris)) {
        return QStringList();
    }

    return mPriv->validAddresses;
}

QStringList PendingContacts::invalidUris() const
{
    if (!mPriv->checkRequestTypeAndState("invalidUris", "URIS", ForUris)) {
        return QStringList();
    }

    return mPriv->invalidAddresses;
}

void PendingContacts::onAttributesFinished(PendingOperation *operation)
{
    PendingContactAttributes *pendingAttributes =
        qobject_cast<PendingContactAttributes *>(operation);

    if (pendingAttributes->isError()) {
        debug() << "PendingAttrs error" << pendingAttributes->errorName()
                << "message" << pendingAttributes->errorMessage();
        setFinishedWithError(pendingAttributes->errorName(), pendingAttributes->errorMessage());
        return;
    }

    ReferencedHandles validHandles = pendingAttributes->validHandles();
    ContactAttributesMap attributes = pendingAttributes->attributes();

    foreach (uint handle, mPriv->handles) {
        if (!mPriv->satisfyingContacts.contains(handle)) {
            int indexInValid = validHandles.indexOf(handle);
            if (indexInValid >= 0) {
                ReferencedHandles referencedHandle = validHandles.mid(indexInValid, 1);
                QVariantMap handleAttributes = attributes[handle];
                mPriv->satisfyingContacts.insert(handle, manager()->ensureContact(referencedHandle,
                            mPriv->missingFeatures, handleAttributes));
            } else {
                mPriv->invalidHandles.push_back(handle);
            }
        }
    }

    allAttributesFetched();
}

void PendingContacts::onRequestHandlesFinished(PendingOperation *operation)
{
    PendingHandles *pendingHandles = qobject_cast<PendingHandles *>(operation);

    mPriv->validIds = pendingHandles->validNames();
    mPriv->invalidIds = pendingHandles->invalidNames();

    if (pendingHandles->isError()) {
        debug() << "RequestHandles error" << operation->errorName()
                << "message" << operation->errorMessage();
        setFinishedWithError(operation->errorName(), operation->errorMessage());
        return;
    }

    mPriv->nested = manager()->contactsForHandles(pendingHandles->handles(), features());
    connect(mPriv->nested,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onNestedFinished(Tp::PendingOperation*)));
}

void PendingContacts::onAddressingGetContactsFinished(PendingOperation *operation)
{
    PendingAddressingGetContacts *pa = qobject_cast<PendingAddressingGetContacts *>(operation);

    Q_ASSERT(pa->isForUris() || pa->isForVCardAddresses());
    mPriv->validAddresses = pa->validAddresses();
    mPriv->invalidAddresses = pa->invalidAddresses();

    if (pa->isError()) {
        setFinishedWithError(operation->errorName(), operation->errorMessage());
        return;
    }

    ConnectionPtr conn = mPriv->manager->connection();
    ContactAttributesMap attributes = pa->attributes();
    UIntList handles = attributes.keys();
    ReferencedHandles referencedHandles(conn, HandleTypeContact, handles);

    foreach (uint handle, handles) {
        int indexInValid = referencedHandles.indexOf(handle);
        Q_ASSERT(indexInValid >= 0);
        ReferencedHandles referencedHandle = referencedHandles.mid(indexInValid, 1);
        QVariantMap handleAttributes = attributes[handle];
        ContactPtr contact = mPriv->manager->ensureContact(referencedHandle,
                    mPriv->missingFeatures, handleAttributes);
        mPriv->contacts.push_back(contact);
    }

    setFinished();
}

void PendingContacts::onReferenceHandlesFinished(PendingOperation *operation)
{
    PendingHandles *pendingHandles = qobject_cast<PendingHandles *>(operation);

    if (pendingHandles->isError()) {
        debug() << "ReferenceHandles error" << operation->errorName()
                << "message" << operation->errorMessage();
        setFinishedWithError(operation->errorName(), operation->errorMessage());
        return;
    }

    ReferencedHandles validHandles = pendingHandles->handles();
    UIntList invalidHandles = pendingHandles->invalidHandles();
    ConnectionPtr conn = mPriv->manager->connection();
    mPriv->handlesToInspect = ReferencedHandles(conn, HandleTypeContact, UIntList());
    foreach (uint handle, mPriv->handles) {
        if (!mPriv->satisfyingContacts.contains(handle)) {
            int indexInValid = validHandles.indexOf(handle);
            if (indexInValid >= 0) {
                ReferencedHandles referencedHandle = validHandles.mid(indexInValid, 1);
                mPriv->handlesToInspect.append(referencedHandle);
            } else {
                mPriv->invalidHandles.push_back(handle);
            }
        }
    }

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                conn->baseInterface()->InspectHandles(HandleTypeContact,
                    mPriv->handlesToInspect.toList()),
                this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onInspectHandlesFinished(QDBusPendingCallWatcher*)));
}

void PendingContacts::onNestedFinished(PendingOperation *operation)
{
    Q_ASSERT(operation == mPriv->nested);

    if (operation->isError()) {
        debug() << " error" << operation->errorName()
                << "message" << operation->errorMessage();
        setFinishedWithError(operation->errorName(), operation->errorMessage());
        return;
    }

    mPriv->contacts = mPriv->nested->contacts();
    mPriv->nested = 0;
    mPriv->setFinished();
}

void PendingContacts::onInspectHandlesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (reply.isError()) {
        debug().nospace() << "InspectHandles: error " << reply.error().name() << ": "
            << reply.error().message();
        setFinishedWithError(reply.error());
        return;
    }

    QStringList names = reply.value();
    int i = 0;
    ConnectionPtr conn = mPriv->manager->connection();
    foreach (uint handle, mPriv->handlesToInspect) {
        QVariantMap handleAttributes;
        handleAttributes.insert(TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id"),
                names[i++]);
        ReferencedHandles referencedHandle(conn, HandleTypeContact,
                UIntList() << handle);
        mPriv->satisfyingContacts.insert(handle, manager()->ensureContact(referencedHandle,
                    mPriv->missingFeatures, handleAttributes));
    }

    allAttributesFetched();

    watcher->deleteLater();
}

void PendingContacts::allAttributesFetched()
{
    foreach (uint handle, mPriv->handles) {
        if (mPriv->satisfyingContacts.contains(handle)) {
            mPriv->contacts.push_back(mPriv->satisfyingContacts[handle]);
        }
    }

    mPriv->setFinished();
}

PendingAddressingGetContacts::PendingAddressingGetContacts(const ConnectionPtr &connection,
        const QString &vcardField, const QStringList &vcardAddresses,
        const QStringList &interfaces)
    : PendingOperation(connection),
      mConnection(connection),
      mRequestType(ForVCardAddresses),
      mVCardField(vcardField),
      mAddresses(vcardAddresses)
{
    // no check for the interface here again, we expect this interface to be used only when
    // Conn.I.Addressing is available
    Client::ConnectionInterfaceAddressingInterface *connAddressingIface =
        connection->optionalInterface<Client::ConnectionInterfaceAddressingInterface>(
                OptionalInterfaceFactory<Connection>::BypassInterfaceCheck);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            connAddressingIface->GetContactsByVCardField(vcardField, vcardAddresses, interfaces));
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onGetContactsFinished(QDBusPendingCallWatcher*)));
}

PendingAddressingGetContacts::PendingAddressingGetContacts(const ConnectionPtr &connection,
        const QStringList &uris, const QStringList &interfaces)
    : PendingOperation(connection),
      mConnection(connection),
      mRequestType(ForUris),
      mAddresses(uris)
{
    // no check for the interface here again, we expect this interface to be used only when
    // Conn.I.Addressing is available
    Client::ConnectionInterfaceAddressingInterface *connAddressingIface =
        connection->optionalInterface<Client::ConnectionInterfaceAddressingInterface>(
                OptionalInterfaceFactory<Connection>::BypassInterfaceCheck);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            connAddressingIface->GetContactsByURI(uris, interfaces));
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onGetContactsFinished(QDBusPendingCallWatcher*)));
}

PendingAddressingGetContacts::~PendingAddressingGetContacts()
{
}

void PendingAddressingGetContacts::onGetContactsFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<AddressingNormalizationMap, ContactAttributesMap> reply = *watcher;

    if (!reply.isError()) {
        AddressingNormalizationMap requested = reply.argumentAt<0>();

        mValidHandles = requested.values();
        mValidAddresses = requested.keys();
        mInvalidAddresses = mAddresses.toSet().subtract(mValidAddresses.toSet()).toList();
        mAttributes = reply.argumentAt<1>();
        setFinished();
    } else {
        debug().nospace() << "GetContactsBy* failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
