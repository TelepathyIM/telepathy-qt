/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008, 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008, 2009 Nokia Corporation
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

#include <TelepathyQt4/Client/Connection>
#include "TelepathyQt4/Client/connection-internal.h"

#include "TelepathyQt4/_gen/cli-connection.moc.hpp"
#include "TelepathyQt4/_gen/cli-connection-body.hpp"
#include "TelepathyQt4/Client/_gen/connection.moc.hpp"
#include "TelepathyQt4/Client/_gen/connection-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingContactAttributes>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingFailure>
#include <TelepathyQt4/Client/PendingHandles>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/PendingVoidMethodCall>
#include <TelepathyQt4/Client/ReferencedHandles>

#include <QMap>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QQueue>
#include <QString>
#include <QTimer>
#include <QtGlobal>

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
 * Proxy objects representing remote Telepathy Connection objects.
 */

namespace Telepathy
{
namespace Client
{

struct Connection::Private
{
    Private(Connection *parent);
    ~Private();

    void init();

    static void introspectMain(Private *self);
    void introspectSelfHandle();
    void introspectContacts();
    static void introspectSelfContact(Private *self);
    static void introspectSimplePresence(Private *self);
    static void introspectRoster(Private *self);

    struct HandleContext;

    // Public object
    Connection *parent;

    // Instance of generated interface class
    ConnectionInterface *baseInterface;

    // Optional interface proxies
    DBus::PropertiesInterface *properties;
    ConnectionInterfaceSimplePresenceInterface *simplePresence;

    ReadinessHelper *readinessHelper;

    // Introspection
    QStringList interfaces;

    // Introspected properties
    // keep pendingStatus and pendingStatusReason until we emit statusChanged
    // so Connection::status() and Connection::statusReason() are consistent
    uint pendingStatus;
    uint pendingStatusReason;
    uint status;
    uint statusReason;

    SimpleStatusSpecMap simplePresenceStatuses;
    ContactPtr selfContact;
    QStringList contactAttributeInterfaces;
    uint selfHandle;
    QMap<uint, ContactManager::ContactListChannel> contactListsChannels;
    uint contactListsChannelsReady;

    // (Bus connection name, service name) -> HandleContext
    static QMap<QPair<QString, QString>, HandleContext *> handleContexts;
    static QMutex handleContextsLock;
    HandleContext *handleContext;

    ContactManager *contactManager;
};

// Handle tracking
struct Connection::Private::HandleContext
{
    struct Type
    {
        QMap<uint, uint> refcounts;
        QSet<uint> toRelease;
        uint requestsInFlight;
        bool releaseScheduled;

        Type()
            : requestsInFlight(0),
              releaseScheduled(false)
        {
        }
    };

    HandleContext()
        : refcount(0)
    {
    }

    int refcount;
    QMutex lock;
    QMap<uint, Type> types;
};

Connection::Private::Private(Connection *parent)
    : parent(parent),
      baseInterface(new ConnectionInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      properties(0),
      simplePresence(0),
      readinessHelper(parent->readinessHelper()),
      pendingStatus(Connection::StatusUnknown),
      pendingStatusReason(ConnectionStatusReasonNoneSpecified),
      status(Connection::StatusUnknown),
      statusReason(ConnectionStatusReasonNoneSpecified),
      selfHandle(0),
      contactListsChannelsReady(0),
      handleContext(0),
      contactManager(new ContactManager(parent))
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << Connection::StatusDisconnected << Connection::StatusConnected, // makesSenseForStatuses
        Features(),                                                                    // dependsOnFeatures (none)
        QStringList(),                                                                 // dependsOnInterfaces (none)
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableSelfContact(
        QSet<uint>() << Connection::StatusConnected,                                   // makesSenseForStatuses
        Features() << FeatureCore,                                                     // dependsOnFeatures (core)
        QStringList() << TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS,            // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectSelfContact,
        this);
    introspectables[FeatureSelfContact] = introspectableSelfContact;

    ReadinessHelper::Introspectable introspectableSimplePresence(
        QSet<uint>() << Connection::StatusConnected,                                   // makesSenseForStatuses
        Features() << FeatureCore,                                                     // dependsOnFeatures (core)
        QStringList() << TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,     // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectSimplePresence,
        this);
    introspectables[FeatureSimplePresence] = introspectableSimplePresence;

    ReadinessHelper::Introspectable introspectableRoster(
        QSet<uint>() << Connection::StatusConnected,                                   // makesSenseForStatuses
        Features() << FeatureCore,                                                     // dependsOnFeatures (core)
        QStringList() << TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS,            // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectRoster,
        this);
    introspectables[FeatureRoster] = introspectableRoster;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->setCurrentStatus(status);
    parent->connect(readinessHelper,
            SIGNAL(statusReady(uint)),
            SLOT(onStatusReady(uint)));
    readinessHelper->becomeReady(Features() << FeatureCore);

    init();
}

Connection::Private::~Private()
{
    // Clear selfContact so its handle will be released cleanly before the handleContext
    selfContact.clear();

    // FIXME: This doesn't look right! In fact, looks absolutely horrendous.
    if (!handleContext) {
        // initial introspection is not done
        return;
    }

    QMutexLocker locker(&handleContextsLock);

    // All handle contexts locked, so safe
    if (!--handleContext->refcount) {
        debug() << "Destroying HandleContext";

        foreach (uint handleType, handleContext->types.keys()) {
            HandleContext::Type type = handleContext->types[handleType];

            if (!type.refcounts.empty()) {
                debug() << " Still had references to" <<
                    type.refcounts.size() << "handles, releasing now";
                baseInterface->ReleaseHandles(handleType, type.refcounts.keys());
            }

            if (!type.toRelease.empty()) {
                debug() << " Was going to release" <<
                    type.toRelease.size() << "handles, doing that now";
                baseInterface->ReleaseHandles(handleType, type.toRelease.toList());
            }
        }

        handleContexts.remove(qMakePair(baseInterface->connection().name(),
                    baseInterface->service()));
        delete handleContext;
    }
    else {
        Q_ASSERT(handleContext->refcount > 0);
    }
}

void Connection::Private::init()
{
    debug() << "Connecting to StatusChanged()";
    parent->connect(baseInterface,
                    SIGNAL(StatusChanged(uint, uint)),
                    SLOT(onStatusChanged(uint, uint)));

    debug() << "Calling GetStatus()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetStatus(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(gotStatus(QDBusPendingCallWatcher *)));

    QMutexLocker locker(&handleContextsLock);
    QString busConnectionName = baseInterface->connection().name();
    QString busName = baseInterface->service();

    if (handleContexts.contains(qMakePair(busConnectionName, busName))) {
        debug() << "Reusing existing HandleContext";
        handleContext = handleContexts[qMakePair(busConnectionName, busName)];
    }
    else {
        debug() << "Creating new HandleContext";
        handleContext = new HandleContext;
        handleContexts[qMakePair(busConnectionName, busName)] = handleContext;
    }

    // All handle contexts locked, so safe
    ++handleContext->refcount;
}

void Connection::Private::introspectMain(Connection::Private *self)
{
    // Introspecting the main interface is currently just calling
    // GetInterfaces(), but it might include other stuff in the future if we
    // gain GetAll-able properties on the connection
    debug() << "Calling GetInterfaces()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(self->baseInterface->GetInterfaces(),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotInterfaces(QDBusPendingCallWatcher *)));
}

void Connection::Private::introspectContacts()
{
    if (!properties) {
        properties = parent->propertiesInterface();
        Q_ASSERT(properties != 0);
    }

    debug() << "Getting available interfaces for GetContactAttributes";
    QDBusPendingCall call =
        properties->Get(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS,
                        "ContactAttributeInterfaces");
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(call, parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(gotContactAttributeInterfaces(QDBusPendingCallWatcher *)));
}

void Connection::Private::introspectSimplePresence(Connection::Private *self)
{
    if (!self->properties) {
        self->properties = self->parent->propertiesInterface();
        Q_ASSERT(self->properties != 0);
    }

    debug() << "Getting available SimplePresence statuses";
    QDBusPendingCall call =
        self->properties->Get(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
                "Statuses");
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(call, self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotSimpleStatuses(QDBusPendingCallWatcher *)));
}

void Connection::Private::introspectSelfContact(Connection::Private *self)
{
    debug() << "Building self contact";
    // FIXME: these should be features when Connection is sanitized
    PendingContacts *contacts = self->contactManager->contactsForHandles(
            UIntList() << self->selfHandle,
            QSet<Contact::Feature>() << Contact::FeatureAlias
                                     << Contact::FeatureAvatarToken
                                     << Contact::FeatureSimplePresence);
    self->parent->connect(contacts,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(gotSelfContact(Telepathy::Client::PendingOperation *)));
}

void Connection::Private::introspectSelfHandle()
{
    parent->connect(baseInterface,
                    SIGNAL(SelfHandleChanged(uint)),
                    SLOT(onSelfHandleChanged(uint)));

    debug() << "Getting self handle";
    QDBusPendingCall call = baseInterface->GetSelfHandle();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(call, parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(gotSelfHandle(QDBusPendingCallWatcher *)));
}

void Connection::Private::introspectRoster(Connection::Private *self)
{
    debug() << "Requesting handles for contact lists";

    for (uint i = 0; i < ContactManager::ContactListChannel::LastType; ++i) {
        self->contactListsChannels.insert(i,
                ContactManager::ContactListChannel(
                    (ContactManager::ContactListChannel::Type) i));

        PendingHandles *pending = self->parent->requestHandles(
                Telepathy::HandleTypeList,
                QStringList() << ContactManager::ContactListChannel::identifierForType(
                    (ContactManager::ContactListChannel::Type) i));
        self->parent->connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(gotContactListsHandles(Telepathy::Client::PendingOperation*)));
    }
}

Connection::PendingConnect::PendingConnect(Connection *parent, const Features &requestedFeatures)
    : PendingReady(requestedFeatures, parent)
{
    QDBusPendingCall call = parent->baseInterface()->Connect();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, parent);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(onConnectReply(QDBusPendingCallWatcher*)));
}

void Connection::PendingConnect::onConnectReply(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError()) {
        setFinishedWithError(watcher->error());
    }
    else {
        connect(qobject_cast<Connection*>(parent())->becomeReady(requestedFeatures()),
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(onBecomeReadyReply(Telepathy::Client::PendingOperation*)));
    }
}

void Connection::PendingConnect::onBecomeReadyReply(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
    }
    else {
        setFinished();
    }
}

QMap<QPair<QString, QString>, Connection::Private::HandleContext*> Connection::Private::handleContexts;
QMutex Connection::Private::handleContextsLock;

/**
 * \class Connection
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/Client/connection.h> <TelepathyQt4/Client/Connection>
 *
 * Object representing a Telepathy connection.
 *
 * It adds the following features compared to using ConnectionInterface
 * directly:
 * <ul>
 *  <li>%Connection status tracking</li>
 *  <li>Getting the list of supported interfaces automatically</li>
 *  <li>Getting the valid presence statuses automatically</li>
 *  <li>Shared optional interface proxy instances</li>
 * </ul>
 *
 * The remote object state accessor functions on this object (status(),
 * statusReason(), and so on) don't make any DBus calls; instead,
 * they return values cached from a previous introspection run. The
 * introspection process populates their values in the most efficient way
 * possible based on what the service implements. Their return value is mostly
 * undefined until the introspection process is completed; a status change to
 * StatusConnected indicates that the introspection process is finished. See the
 * individual accessor descriptions for details on which functions can be used
 * in the different states.
 */

const Feature Connection::FeatureCore = Feature(Connection::staticMetaObject.className(), 0, true);
const Feature Connection::FeatureSelfContact = Feature(Connection::staticMetaObject.className(), 1);
const Feature Connection::FeatureSimplePresence = Feature(Connection::staticMetaObject.className(), 2);
const Feature Connection::FeatureRoster = Feature(Connection::staticMetaObject.className(), 3);

/**
 * Construct a new Connection object.
 *
 * \param busName The connection's well-known or unique bus name
 *                (sometimes called a "service name")
 * \param objectPath The connection's object path
 * \param parent Object parent
 */
Connection::Connection(const QString &busName,
                       const QString &objectPath,
                       QObject *parent)
    : StatefulDBusProxy(QDBusConnection::sessionBus(),
            busName, objectPath, parent),
      OptionalInterfaceFactory<Connection>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
}

/**
 * Construct a new Connection object.
 *
 * \param bus QDBusConnection to use
 * \param busName The connection's well-known or unique bus name
 *                (sometimes called a "service name")
 * \param objectPath The connection's object path
 * \param parent Object parent
 */
Connection::Connection(const QDBusConnection &bus,
                       const QString &busName,
                       const QString &objectPath,
                       QObject *parent)
    : StatefulDBusProxy(bus, busName, objectPath, parent),
      OptionalInterfaceFactory<Connection>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
Connection::~Connection()
{
    delete mPriv;
}

/**
 * Return the connection's status.
 *
 * The returned value may have changed whenever statusChanged() is
 * emitted.
 *
 * \return The status, as defined in Status.
 */
uint Connection::status() const
{
    return mPriv->status;
}

/**
 * Return the reason for the connection's status (which is returned by
 * status()). The validity and change rules are the same as for status().
 *
 * \return The reason, as defined in Status.
 */
uint Connection::statusReason() const
{
    return mPriv->statusReason;
}

/**
 * Return a list of optional interfaces supported by this object. The
 * contents of the list is undefined unless the Connection has status
 * StatusConnecting or StatusConnected. The returned value stays
 * constant for the entire time the connection spends in each of these
 * states; however interfaces might have been added to the supported set by
 * the time StatusConnected is reached.
 *
 * \return Names of the supported interfaces.
 */
QStringList Connection::interfaces() const
{
    if (!isReady()) {
        warning() << "Connection::interfaces() used while connection is not ready";
    }

    return mPriv->interfaces;
}

/**
 * Return the handle which represents the user on this connection, which will remain
 * valid for the lifetime of this connection, or until a change in the user's
 * identifier is signalled by the selfHandleChanged signal. If the connection is
 * not yet in the StatusConnected state, the value of this property MAY be zero.
 *
 * \return Self handle.
 */
uint Connection::selfHandle() const
{
    return mPriv->selfHandle;
}

/**
 * Return a dictionary of presence statuses valid for use with the new(er)
 * Telepathy SimplePresence interface on the remote object.
 *
 * The value is undefined if the list returned by interfaces() doesn't
 * contain %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE.
 *
 * The value may have changed arbitrarily during the time the
 * Connection spends in status StatusConnecting,
 * again staying fixed for the entire time in StatusConnected.
 *
 * \return Dictionary from string identifiers to structs for each valid
 * status.
 */
SimpleStatusSpecMap Connection::allowedPresenceStatuses() const
{
    if (!isReady(Features() << FeatureSimplePresence)) {
        warning() << "Trying to retrieve simple presence from connection, but "
                     "simple presence is not supported or was not requested. "
                     "Use becomeReady(FeatureSimplePresence)";
    }

    return mPriv->simplePresenceStatuses;
}

/**
 * Set the self presence status.
 *
 * \a status must be one of the allowed statuses returned by
 * allowedPresenceStatuses().
 *
 * Note that clients SHOULD set the status message for the local user to the empty string,
 * unless the user has actually provided a specific message (i.e. one that
 * conveys more information than the Status).
 *
 * \param status The desired status.
 * \param statusMessage The desired status message.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 *
 * \sa allowedPresenceStatuses()
 */
PendingOperation *Connection::setSelfPresence(const QString &status,
        const QString &statusMessage)
{
    if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Connection does not support SimplePresence");
    }
    return new PendingVoidMethodCall(this,
            simplePresenceInterface()->SetPresence(status, statusMessage));
}

ContactPtr Connection::selfContact() const
{
    if (!isReady()) {
        warning() << "Connection::selfContact() used before the connection is ready!";
    }
    // FIXME: add checks for the SelfContact feature having been requested when Connection features
    // actually work sensibly and selfContact is made a feature

    return mPriv->selfContact;
}

/**
 * \fn Connection::optionalInterface(InterfaceSupportedChecking check) const
 *
 * Get a pointer to a valid instance of a given %Connection optional
 * interface class, associated with the same remote object the Connection is
 * associated with, and destroyed at the same time the Connection is
 * destroyed.
 *
 * If the list returned by interfaces() doesn't contain the name of the
 * interface requested <code>0</code> is returned. This check can be
 * bypassed by specifying #BypassInterfaceCheck for <code>check</code>, in
 * which case a valid instance is always returned.
 *
 * If the object is not ready, the list returned by interfaces() isn't
 * guaranteed to yet represent the full set of interfaces supported by the
 * remote object.
 * Hence the check might fail even if the remote object actually supports
 * the requested interface; using #BypassInterfaceCheck is suggested when
 * the Connection is not suitably ready.
 *
 * \sa OptionalInterfaceFactory::interface
 *
 * \tparam Interface Class of the optional interface to get.
 * \param check Should an instance be returned even if it can't be
 *              determined that the remote object supports the
 *              requested interface.
 * \return Pointer to an instance of the interface class, or <code>0</code>.
 */

/**
 * \fn ConnectionInterfaceAliasingInterface *Connection::aliasingInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting an Aliasing interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ConnectionInterfaceAliasingInterface>(check)</code>
 */

/**
 * \fn ConnectionInterfaceAvatarsInterface *Connection::avatarsInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting an Avatars interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ConnectionInterfaceAvatarsInterface>(check)</code>
 */

/**
 * \fn ConnectionInterfaceCapabilitiesInterface *Connection::capabilitiesInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a Capabilities interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ConnectionInterfaceCapabilitiesInterface>(check)</code>
 */

/**
 * \fn ConnectionInterfacePresenceInterface *Connection::presenceInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a Presence interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ConnectionInterfacePresenceInterface>(check)</code>
 */

/**
 * \fn ConnectionInterfaceSimplePresenceInterface *Connection::simplePresenceInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a SimplePresence interface proxy.
 *
 * \param check Passed to optionalInterface()
 * \return <code>optionalInterface<ConnectionInterfaceSimplePresenceInterface>(check)</code>
 */

/**
 * \fn DBus::PropertiesInterface *Connection::propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * Properties interface is not necessarily reported by the services, so a
 * <code>check</code> parameter is not provided, and the interface is
 * always assumed to be present.
 *
 * \sa optionalInterface()
 *
 * \return <code>optionalInterface<DBus::PropertiesInterface>(BypassInterfaceCheck)</code>
 */

void Connection::onStatusReady(uint status)
{
    Q_ASSERT(status == mPriv->pendingStatus);

    mPriv->status = status;
    mPriv->statusReason = mPriv->pendingStatusReason;
    emit statusChanged(mPriv->status, mPriv->statusReason);
}

void Connection::onStatusChanged(uint status, uint reason)
{
    debug() << "StatusChanged from" << mPriv->pendingStatus
            << "to" << status << "with reason" << reason;

    if (mPriv->pendingStatus == status) {
        warning() << "New status was the same as the old status! Ignoring redundant StatusChanged";
        return;
    }

    mPriv->pendingStatus = status;
    mPriv->pendingStatusReason = reason;

    switch (status) {
        case ConnectionStatusConnected:
            debug() << "Performing introspection for the Connected status";
            mPriv->readinessHelper->setCurrentStatus(status);
            break;

        case ConnectionStatusConnecting:
            mPriv->readinessHelper->setCurrentStatus(status);
            break;

        case ConnectionStatusDisconnected:
            const char *errorName;

            // This is the best we can do right now: in an imminent
            // spec version we should define a different D-Bus error name
            // for each ConnectionStatusReason

            switch (reason) {
                case ConnectionStatusReasonNoneSpecified:
                case ConnectionStatusReasonRequested:
                    errorName = TELEPATHY_ERROR_DISCONNECTED;
                    break;

                case ConnectionStatusReasonNetworkError:
                case ConnectionStatusReasonAuthenticationFailed:
                case ConnectionStatusReasonEncryptionError:
                    errorName = TELEPATHY_ERROR_NETWORK_ERROR;
                    break;

                case ConnectionStatusReasonNameInUse:
                    errorName = TELEPATHY_ERROR_NOT_YOURS;
                    break;

                case ConnectionStatusReasonCertNotProvided:
                case ConnectionStatusReasonCertUntrusted:
                case ConnectionStatusReasonCertExpired:
                case ConnectionStatusReasonCertNotActivated:
                case ConnectionStatusReasonCertHostnameMismatch:
                case ConnectionStatusReasonCertFingerprintMismatch:
                case ConnectionStatusReasonCertSelfSigned:
                case ConnectionStatusReasonCertOtherError:
                    errorName = TELEPATHY_ERROR_NETWORK_ERROR;

                default:
                    errorName = TELEPATHY_ERROR_DISCONNECTED;
            }

            // TODO should we signal statusChanged to Disconnected here or just
            //      invalidate?
            //      Also none of the pendingOperations will finish. The
            //      user should just consider them to fail as the connection
            //      is invalid
            onStatusReady(StatusDisconnected);
            invalidate(QLatin1String(errorName),
                    QString("ConnectionStatusReason = %1").arg(uint(reason)));
            break;

        default:
            warning() << "Unknown connection status" << status;
            break;
    }
}

void Connection::gotStatus(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (mPriv->pendingStatus != StatusUnknown) {
        // we already got the status from StatusChanged, ignoring here
        return;
    }

    if (reply.isError()) {
        warning().nospace() << "GetStatus() failed with " <<
            reply.error().name() << ":" << reply.error().message();

        invalidate(reply.error());
        return;
    }

    uint status = reply.value();
    debug() << "Got connection status" << status;

    mPriv->pendingStatus = status;
    mPriv->readinessHelper->setCurrentStatus(status);

    watcher->deleteLater();
}

void Connection::gotInterfaces(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        mPriv->interfaces = reply.value();
        debug() << "Got reply to GetInterfaces():" << mPriv->interfaces;
        mPriv->readinessHelper->setInterfaces(mPriv->interfaces);
    }
    else {
        warning().nospace() << "GetInterfaces() failed with " <<
            reply.error().name() << ":" << reply.error().message() <<
            " - assuming no new interfaces";
        // do not fail if GetInterfaces fail
    }

    if (mPriv->pendingStatus == StatusConnected) {
        mPriv->introspectSelfHandle();
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }

    watcher->deleteLater();
}

void Connection::gotContactAttributeInterfaces(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        mPriv->contactAttributeInterfaces = qdbus_cast<QStringList>(reply.value().variant());
        debug() << "Got" << mPriv->contactAttributeInterfaces.size() << "contact attribute interfaces";
    } else {
        warning().nospace() << "Getting contact attribute interfaces failed with " <<
            reply.error().name() << ":" << reply.error().message();

        // TODO should we remove Contacts interface from interfaces?
        warning() << "Connection supports Contacts interface but contactAttributeInterfaces "
            "can't be retrieved";
    }

    mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);

    watcher->deleteLater();
}

void Connection::gotSelfContact(PendingOperation *op)
{
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);

    if (pending->isValid()) {
        Q_ASSERT(pending->contacts().size() == 1);
        ContactPtr contact = pending->contacts()[0];

        if (mPriv->selfContact != contact) {
            mPriv->selfContact = contact;

            // first time
            if (!mPriv->readinessHelper->actualFeatures().contains(FeatureSelfContact)) {
                mPriv->readinessHelper->setIntrospectCompleted(FeatureSelfContact, true);
            }

            emit selfContactChanged();
        }
    } else {
        warning().nospace() << "Getting self contact failed with " <<
            pending->errorName() << ":" << pending->errorMessage();

        // check if the feature is already there, and for some reason introspectSelfContact
        // failed when called the second time
        if (!mPriv->readinessHelper->missingFeatures().contains(FeatureSelfContact)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureSelfContact, false,
                    op->errorName(), op->errorMessage());
        }
    }
}

void Connection::gotSimpleStatuses(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        mPriv->simplePresenceStatuses = qdbus_cast<SimpleStatusSpecMap>(reply.value().variant());
        debug() << "Got" << mPriv->simplePresenceStatuses.size() << "simple presence statuses";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureSimplePresence, true);
    }
    else {
        warning().nospace() << "Getting simple presence statuses failed with " <<
            reply.error().name() << ":" << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureSimplePresence, false, reply.error());
    }

    watcher->deleteLater();
}

void Connection::gotSelfHandle(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (!reply.isError()) {
        mPriv->selfHandle = reply.value();
        debug() << "Got self handle" << mPriv->selfHandle;

        if (mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS)) {
            mPriv->introspectContacts();
        } else {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        }
    } else {
        warning().nospace() << "Getting self handle failed with " <<
            reply.error().name() << ":" << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());
    }

    watcher->deleteLater();
}

void Connection::gotContactListsHandles(PendingOperation *op)
{
    if (op->isError()) {
        // let's not fail, because the contact lists are not supported
        debug() << "Unable to retrieve contact list handle, ignoring";
        contactListChannelReady();
        return;
    }

    debug() << "Got handles for contact lists";
    PendingHandles *pending = qobject_cast<PendingHandles*>(op);

    // FIXME check for handles in pending->invalidHandles() when
    //       invalidHandles is implemented
    // if (pending->invalidHandles().size() == 1) {
    //     contactListChannelReady();
    //     return;
    // }

    debug() << "Requesting channels for contact lists";
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_LIST);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Telepathy::HandleTypeList);

    Q_ASSERT(pending->handles().size() == 1);
    Q_ASSERT(pending->namesRequested().size() == 1);
    ReferencedHandles handle = pending->handles();
    uint type = ContactManager::ContactListChannel::typeForIdentifier(
            pending->namesRequested().first());
    Q_ASSERT(type != (uint) -1 && type < ContactManager::ContactListChannel::LastType);
    mPriv->contactListsChannels[type].handle = handle;
    request[QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle")] = handle[0];
    connect(ensureChannel(request),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(gotContactListChannel(Telepathy::Client::PendingOperation*)));
}

void Connection::gotContactListChannel(PendingOperation *op)
{
    if (op->isError()) {
        contactListChannelReady();
        return;
    }

    PendingChannel *pending = qobject_cast<PendingChannel*>(op);
    ChannelPtr channel = pending->channel();
    uint handle = pending->targetHandle();
    Q_ASSERT(channel);
    Q_ASSERT(handle);
    for (int i = 0; i < ContactManager::ContactListChannel::LastType; ++i) {
        if (mPriv->contactListsChannels[i].handle.size() > 0 &&
            mPriv->contactListsChannels[i].handle[0] == handle) {
            Q_ASSERT(!mPriv->contactListsChannels[i].channel);
            mPriv->contactListsChannels[i].channel = channel;
            connect(channel->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(contactListChannelReady()));
        }
    }
}

void Connection::contactListChannelReady()
{
    if (++mPriv->contactListsChannelsReady ==
            ContactManager::ContactListChannel::LastType) {
        debug() << "FeatureRoster ready";
        mPriv->contactManager->setContactListChannels(mPriv->contactListsChannels);
        mPriv->readinessHelper->setIntrospectCompleted(FeatureRoster, true);
    }
}

/**
 * Get the ConnectionInterface for this Connection. This
 * method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing ConnectionInterface for this
 *         Connection.
 */
ConnectionInterface *Connection::baseInterface() const
{
    return mPriv->baseInterface;
}

/**
 * Asynchronously creates a channel satisfying the given request.
 *
 * The request MUST contain the following keys:
 *   org.freedesktop.Telepathy.Account.ChannelType
 *   org.freedesktop.Telepathy.Account.TargetHandleType
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingChannel object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to get
 * notification of the request finishing processing. See the documentation
 * for that class for more info.
 *
 * The returned PendingChannel object should be freed using
 * its QObject::deleteLater() method after it is no longer used. However,
 * all PendingChannel objects resulting from requests to a particular
 * Connection will be freed when the Connection itself is freed. Conversely,
 * this means that the PendingChannel object should not be used after the
 * Connection is destroyed.
 *
 * \sa PendingChannel
 *
 * \param request A dictionary containing the desirable properties.
 * \return Pointer to a newly constructed PendingChannel object, tracking
 *         the progress of the request.
 */
PendingChannel *Connection::createChannel(const QVariantMap &request)
{
    if (mPriv->pendingStatus != StatusConnected) {
        warning() << "Calling createChannel with connection not yet connected";
        return new PendingChannel(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Connection not yet connected");
    }

    if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_REQUESTS)) {
        warning() << "Requests interface is not support by this connection";
        return new PendingChannel(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Connection does not support Requests Interface");
    }

    if (!request.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"))) {
        return new PendingChannel(this, TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Invalid 'request' argument");
    }

    debug() << "Creating a Channel";
    PendingChannel *channel =
        new PendingChannel(this, request, true);
    return channel;
}

/**
 * Asynchronously ensures a channel exists satisfying the given request.
 *
 * The request MUST contain the following keys:
 *   org.freedesktop.Telepathy.Account.ChannelType
 *   org.freedesktop.Telepathy.Account.TargetHandleType
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingChannel object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to get
 * notification of the request finishing processing. See the documentation
 * for that class for more info.
 *
 * The returned PendingChannel object should be freed using
 * its QObject::deleteLater() method after it is no longer used. However,
 * all PendingChannel objects resulting from requests to a particular
 * Connection will be freed when the Connection itself is freed. Conversely,
 * this means that the PendingChannel object should not be used after the
 * Connection is destroyed.
 *
 * \sa PendingChannel
 *
 * \param request A dictionary containing the desirable properties.
 * \return Pointer to a newly constructed PendingChannel object, tracking
 *         the progress of the request.
 */
PendingChannel *Connection::ensureChannel(const QVariantMap &request)
{
    if (mPriv->pendingStatus != StatusConnected) {
        warning() << "Calling ensureChannel with connection not yet connected";
        return new PendingChannel(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "Connection not yet connected");
    }

    if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_REQUESTS)) {
        warning() << "Requests interface is not support by this connection";
        return new PendingChannel(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Connection does not support Requests Interface");
    }

    if (!request.contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"))) {
        return new PendingChannel(this, TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Invalid 'request' argument");
    }

    debug() << "Creating a Channel";
    PendingChannel *channel =
        new PendingChannel(this, request, false);
    return channel;
}

/**
 * Request handles of the given type for the given entities (contacts,
 * rooms, lists, etc.).
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingHandles object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to to get
 * notification of the request finishing processing. See the documentation
 * for that class for more info.
 *
 * The returned PendingHandles object should be freed using
 * its QObject::deleteLater() method after it is no longer used. However,
 * all PendingHandles objects resulting from requests to a particular
 * Connection will be freed when the Connection itself is freed. Conversely,
 * this means that the PendingHandles object should not be used after the
 * Connection is destroyed.
 *
 * \sa PendingHandles
 *
 * \param handleType Type for the handles to request, as specified in
 *                   #HandleType.
 * \param names Names of the entities to request handles for.
 * \return Pointer to a newly constructed PendingHandles object, tracking
 *         the progress of the request.
 */
PendingHandles *Connection::requestHandles(uint handleType, const QStringList &names)
{
    debug() << "Request for" << names.length() << "handles of type" << handleType;

    {
        Private::HandleContext *handleContext = mPriv->handleContext;
        QMutexLocker locker(&handleContext->lock);
        handleContext->types[handleType].requestsInFlight++;
    }

    PendingHandles *pending =
        new PendingHandles(this, handleType, names);
    return pending;
}

/**
 * Request a reference to the given handles. Handles not explicitly
 * requested (via requestHandles()) but eg. observed in a signal need to be
 * referenced to guarantee them staying valid.
 *
 * Upon completion, the reply to the operation can be retrieved through the
 * returned PendingHandles object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to to get
 * notification of the request finishing processing. See the documentation
 * for that class for more info.
 *
 * The returned PendingHandles object should be freed using
 * its QObject::deleteLater() method after it is no longer used. However,
 * all PendingHandles objects resulting from requests to a particular
 * Connection will be freed when the Connection itself is freed. Conversely,
 * this means that the PendingHandles object should not be used after the
 * Connection is destroyed.
 *
 * \sa PendingHandles
 *
 * \param handleType Type of the handles given, as specified in #HandleType.
 * \param handles Handles to request a reference to.
 * \return Pointer to a newly constructed PendingHandles object, tracking
 *         the progress of the request.
 */
PendingHandles *Connection::referenceHandles(uint handleType, const UIntList &handles)
{
    debug() << "Reference of" << handles.length() << "handles of type" << handleType;

    UIntList alreadyHeld;
    UIntList notYetHeld;
    {
        Private::HandleContext *handleContext = mPriv->handleContext;
        QMutexLocker locker(&handleContext->lock);

        foreach (uint handle, handles) {
            if (handleContext->types[handleType].refcounts.contains(handle) ||
                handleContext->types[handleType].toRelease.contains(handle)) {
                alreadyHeld.push_back(handle);
            }
            else {
                notYetHeld.push_back(handle);
            }
        }
    }

    debug() << " Already holding" << alreadyHeld.size() <<
        "of the handles -" << notYetHeld.size() << "to go";

    PendingHandles *pending =
        new PendingHandles(this, handleType, handles,
                alreadyHeld, notYetHeld);
    return pending;
}

/**
 * Start an asynchronous request that the connection be connected.
 *
 * The returned PendingOperation will finish successfully when the connection
 * has reached StatusConnected and the requested \a features are all ready, or
 * finish with an error if a fatal error occurs during that process.
 *
 * \param requestedFeatures The features which should be enabled
 * \return A PendingReady object which will emit finished
 *         when the Connection has reached StatusConnected, and initial setup
 *         for basic functionality, plus the given features, has succeeded or
 *         failed
 */
PendingReady *Connection::requestConnect(const Features &requestedFeatures)
{
    return new PendingConnect(this, requestedFeatures);
}

/**
 * Start an asynchronous request that the connection be disconnected.
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * \return A %PendingOperation, which will emit finished when the
 *         request finishes.
 */
PendingOperation *Connection::requestDisconnect()
{
    return new PendingVoidMethodCall(this, baseInterface()->Disconnect());
}

/**
 * Requests attributes for contacts. Optionally, the handles of the contacts will be referenced
 * automatically. Essentially, this method wraps
 * ConnectionInterfaceContactsInterface::GetContactAttributes(), integrating it with the rest of the
 * handle-referencing machinery.
 *
 * Upon completion, the reply to the request can be retrieved through the returned
 * PendingContactAttributes object. The object also provides access to the parameters with which the
 * call was made and a signal to connect to to get notification of the request finishing processing.
 * See the documentation for that class for more info.
 *
 * If the remote object doesn't support the Contacts interface (as signified by the list returned by
 * interfaces() not containing TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS), the returned
 * PendingContactAttributes instance will fail instantly with the error
 * TELEPATHY_ERROR_NOT_IMPLEMENTED.
 *
 * Similarly, if the connection isn't both connected and ready (<code>status() == StatusConnected &&
 * isReady()</code>), the returned PendingContactAttributes instance will fail instantly with the
 * error TELEPATHY_ERROR_NOT_AVAILABLE.
 *
 * \sa PendingContactAttributes
 *
 * \param handles A list of handles of type HandleTypeContact
 * \param interfaces D-Bus interfaces for which the client requires information
 * \param reference Whether the handles should additionally be referenced.
 * \return Pointer to a newly constructed PendingContactAttributes, tracking the progress of the
 *         request.
 */
PendingContactAttributes *Connection::getContactAttributes(const UIntList &handles,
        const QStringList &interfaces, bool reference)
{
    debug() << "Request for attributes for" << handles.size() << "contacts";

    PendingContactAttributes *pending =
        new PendingContactAttributes(this, handles, interfaces, reference);
    if (!isReady()) {
        warning() << "Connection::getContactAttributes() used when not ready";
        pending->failImmediately(TELEPATHY_ERROR_NOT_AVAILABLE, "The connection isn't ready");
        return pending;
    } /* FIXME: readd this check when Connection isn't FSCKING broken anymore: else if (status() != StatusConnected) {
        warning() << "Connection::getContactAttributes() used with status" << status() << "!= StatusConnected";
        pending->failImmediately(TELEPATHY_ERROR_NOT_AVAILABLE,
                "The connection isn't Connected");
        return pending;
    } */else if (!this->interfaces().contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS)) {
        warning() << "Connection::getContactAttributes() used without the remote object supporting"
                  << "the Contacts interface";
        pending->failImmediately(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "The connection doesn't support the Contacts interface");
        return pending;
    }

    {
        Private::HandleContext *handleContext = mPriv->handleContext;
        QMutexLocker locker(&handleContext->lock);
        handleContext->types[HandleTypeContact].requestsInFlight++;
    }

    ConnectionInterfaceContactsInterface *contactsInterface =
        optionalInterface<ConnectionInterfaceContactsInterface>();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(contactsInterface->GetContactAttributes(handles, interfaces,
                    reference));
    pending->connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                              SLOT(onCallFinished(QDBusPendingCallWatcher*)));
    return pending;
}

QStringList Connection::contactAttributeInterfaces() const
{
    if (mPriv->pendingStatus != StatusConnected) {
        warning() << "Connection::contactAttributeInterfaces() used with status"
            << status() << "!= StatusConnected";
    } else if (!this->interfaces().contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACTS)) {
        warning() << "Connection::contactAttributeInterfaces() used without the remote object supporting"
                  << "the Contacts interface";
    }

    return mPriv->contactAttributeInterfaces;
}

ContactManager *Connection::contactManager() const
{
    return mPriv->contactManager;
}

void Connection::refHandle(uint type, uint handle)
{
    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    if (handleContext->types[type].toRelease.contains(handle)) {
        handleContext->types[type].toRelease.remove(handle);
    }

    handleContext->types[type].refcounts[handle]++;
}

void Connection::unrefHandle(uint type, uint handle)
{
    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    Q_ASSERT(handleContext->types.contains(type));
    Q_ASSERT(handleContext->types[type].refcounts.contains(handle));

    if (!--handleContext->types[type].refcounts[handle]) {
        handleContext->types[type].refcounts.remove(handle);
        handleContext->types[type].toRelease.insert(handle);

        if (!handleContext->types[type].releaseScheduled) {
            if (!handleContext->types[type].requestsInFlight) {
                debug() << "Lost last reference to at least one handle of type" <<
                    type << "and no requests in flight for that type - scheduling a release sweep";
                QMetaObject::invokeMethod(this, "doReleaseSweep",
                        Qt::QueuedConnection, Q_ARG(uint, type));
                handleContext->types[type].releaseScheduled = true;
            }
        }
    }
}

void Connection::doReleaseSweep(uint type)
{
    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    Q_ASSERT(handleContext->types.contains(type));
    Q_ASSERT(handleContext->types[type].releaseScheduled);

    debug() << "Entering handle release sweep for type" << type;
    handleContext->types[type].releaseScheduled = false;

    if (handleContext->types[type].requestsInFlight > 0) {
        debug() << " There are requests in flight, deferring sweep to when they have been completed";
        return;
    }

    if (handleContext->types[type].toRelease.isEmpty()) {
        debug() << " No handles to release - every one has been resurrected";
        return;
    }

    debug() << " Releasing" << handleContext->types[type].toRelease.size() << "handles";

    mPriv->baseInterface->ReleaseHandles(type, handleContext->types[type].toRelease.toList());
    handleContext->types[type].toRelease.clear();
}

void Connection::handleRequestLanded(uint type)
{
    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    Q_ASSERT(handleContext->types.contains(type));
    Q_ASSERT(handleContext->types[type].requestsInFlight > 0);

    if (!--handleContext->types[type].requestsInFlight &&
        !handleContext->types[type].toRelease.isEmpty() &&
        !handleContext->types[type].releaseScheduled) {
        debug() << "All handle requests for type" << type <<
            "landed and there are handles of that type to release - scheduling a release sweep";
        QMetaObject::invokeMethod(this, "doReleaseSweep", Qt::QueuedConnection, Q_ARG(uint, type));
        handleContext->types[type].releaseScheduled = true;
    }
}

void Connection::onSelfHandleChanged(uint handle)
{
    mPriv->selfHandle = handle;
    emit selfHandleChanged(handle);

    if (mPriv->readinessHelper->actualFeatures().contains(FeatureSelfContact)) {
        Private::introspectSelfContact(mPriv);
    }
}

}
}
