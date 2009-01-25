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

#include <TelepathyQt4/Client/Connection>

#include "TelepathyQt4/Client/_gen/connection.moc.hpp"
#include "TelepathyQt4/_gen/cli-connection.moc.hpp"
#include "TelepathyQt4/_gen/cli-connection-body.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingHandles>
#include <TelepathyQt4/Client/PendingVoidMethodCall>

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
    /*
     * \enum Connection::Private::Readiness
     *
     * Describes readiness of the Connection for usage. The readiness depends
     * on the state of the remote object. In suitable states, an asynchronous
     * introspection process is started, and the Connection becomes more ready
     * when that process is completed.
     *
     * \value ReadinessJustCreated, The object has just been created and introspection
     *                              is still in progress. No functionality is available.
     *                              The readiness can change to any other state depending
     *                              on the result of the initial state query to the remote
     *                              object.
     * \value ReadinessNotYetConnected The remote object is in the Disconnected state and
     *                                 introspection relevant to that state has been completed.
     *                                 This state is useful for being able to set your presence status
     *                                 (through the SimplePresence interface) before connecting. Most other
     *                                 functionality is unavailable, though.
     *                                 The readiness can change to ReadinessConnecting and ReadinessDead.
     * \value ReadinessConnecting The remote object is in the Connecting state. Most functionality is
     *                            unavailable.
     *                            The readiness can change to ReadinessFull and ReadinessDead.
     * \value ReadinessFull The connection is in the Connected state and all introspection
     *                      has been completed. Most functionality is available.
     *                      The readiness can change to ReadinessDead.
     * \value ReadinessDead The remote object has gone into a state where it can no longer be
     *                      used. No functionality is available.
     *                      No further readiness changes are possible.
     * \value _ReadinessInvalid The remote object has gone into a invalid state.
     */
    enum Readiness {
        ReadinessJustCreated = 0,
        ReadinessNotYetConnected = 5,
        ReadinessConnecting = 10,
        ReadinessFull = 15,
        ReadinessDead = 20,
        _ReadinessInvalid = 0xffff
    };

    Private(Connection *parent);
    ~Private();

    void startIntrospection();
    void introspectAliasing();
    void introspectMain();
    void introspectPresence();
    void introspectSimplePresence();

    void changeReadiness(Readiness newReadiness);

    void updatePendingOperations();

    struct HandleContext;
    class PendingReady;

    // Public object
    Connection *parent;

    // Instance of generated interface class
    ConnectionInterface *baseInterface;

    // Optional interface proxies
    ConnectionInterfaceAliasingInterface *aliasing;
    ConnectionInterfacePresenceInterface *presence;
    DBus::PropertiesInterface *properties;

    bool ready;
    QList<PendingReady *> pendingOperations;

    // Introspection
    bool initialIntrospection;
    Readiness readiness;
    QStringList interfaces;
    QQueue<void (Private::*)()> introspectQueue;

    Connection::Features features;
    Connection::Features pendingFeatures;
    Connection::Features missingFeatures;

    // Introspected properties
    // keep pendingStatus and pendingStatusReason until we emit statusChanged
    // so Connection::status() and Connection::statusReason() are consistent
    uint pendingStatus;
    uint pendingStatusReason;
    uint status;
    uint statusReason;
    bool haveInitialStatus;
    uint aliasFlags;
    StatusSpecMap presenceStatuses;
    SimpleStatusSpecMap simplePresenceStatuses;

    // (Bus connection name, service name) -> HandleContext
    static QMap<QPair<QString, QString>, HandleContext *> handleContexts;
    static QMutex handleContextsLock;
    HandleContext *handleContext;
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

class Connection::Private::PendingReady : public PendingOperation
{
    // Connection is a friend so it can call finished() etc.
    friend class Connection;

public:
    PendingReady(Connection::Features features, QObject *parent);

    Connection::Features features;
};

Connection::Private::PendingReady::PendingReady(Connection::Features features, QObject *parent)
    : PendingOperation(parent),
      features(features)
{
}

Connection::Private::Private(Connection *parent)
    : parent(parent),
      baseInterface(new ConnectionInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      aliasing(0),
      presence(0),
      properties(0),
      initialIntrospection(false),
      readiness(ReadinessJustCreated),
      pendingStatus(ConnectionStatusDisconnected),
      pendingStatusReason(ConnectionStatusReasonNoneSpecified),
      status(ConnectionStatusDisconnected),
      statusReason(ConnectionStatusReasonNoneSpecified),
      haveInitialStatus(false),
      aliasFlags(0),
      handleContext(0)
{
}

Connection::Private::~Private()
{
    QMutexLocker locker(&handleContextsLock);

    if (!handleContext) {
        // initial introspection is not done
        return;
    }

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

void Connection::Private::startIntrospection()
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
    QString serviceName = baseInterface->service();

    if (handleContexts.contains(qMakePair(busConnectionName, serviceName))) {
        debug() << "Reusing existing HandleContext";
        handleContext = handleContexts[qMakePair(busConnectionName, serviceName)];
    }
    else {
        debug() << "Creating new HandleContext";
        handleContext = new HandleContext;
        handleContexts[qMakePair(busConnectionName, serviceName)] = handleContext;
    }

    // All handle contexts locked, so safe
    ++handleContext->refcount;
}

void Connection::Private::introspectAliasing()
{
    // The Aliasing interface is not usable before the connection is established
    if (initialIntrospection) {
        parent->continueIntrospection();
        return;
    }

    if (!aliasing) {
        aliasing = parent->aliasingInterface();
        Q_ASSERT(aliasing != 0);
    }

    debug() << "Calling GetAliasFlags()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(aliasing->GetAliasFlags(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotAliasFlags(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectMain()
{
    // Introspecting the main interface is currently just calling
    // GetInterfaces(), but it might include other stuff in the future if we
    // gain GetAll-able properties on the connection
    debug() << "Calling GetInterfaces()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetInterfaces(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(gotInterfaces(QDBusPendingCallWatcher *)));
}

void Connection::Private::introspectPresence()
{
    // The Presence interface is not usable before the connection is established
    if (initialIntrospection) {
        parent->continueIntrospection();
        return;
    }

    if (!presence) {
        presence = parent->presenceInterface();
        Q_ASSERT(presence != 0);
    }

    debug() << "Calling GetStatuses() (legacy)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(presence->GetStatuses(), parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(gotStatuses(QDBusPendingCallWatcher *)));
}

void Connection::Private::introspectSimplePresence()
{
    if (!properties) {
        properties = parent->propertiesInterface();
        Q_ASSERT(properties != 0);
    }

    debug() << "Getting available SimplePresence statuses";
    QDBusPendingCall call =
        properties->Get(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
                       "Statuses");
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(call, parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher *)),
                    SLOT(gotSimpleStatuses(QDBusPendingCallWatcher *)));
}

void Connection::Private::changeReadiness(Readiness newReadiness)
{
    debug() << "changing readiness from" << readiness <<
        "to" << newReadiness;
    Q_ASSERT(newReadiness != readiness);

    switch (readiness) {
        case ReadinessJustCreated:
            break;
        case ReadinessNotYetConnected:
            Q_ASSERT(newReadiness == ReadinessConnecting
                    || newReadiness == ReadinessDead);
            break;
        case ReadinessConnecting:
            Q_ASSERT(newReadiness == ReadinessFull
                    || newReadiness == ReadinessDead);
            break;
        case ReadinessFull:
            Q_ASSERT(newReadiness == ReadinessDead);
            break;
        case ReadinessDead:
        default:
            Q_ASSERT(false);
    }

    debug() << "Readiness changed from" << readiness << "to" << newReadiness;
    readiness = newReadiness;

    // emit statusChanged only here as we are now in the correct readiness
    // e.g: status was already Connected but readiness != ReadinessFull so the user was
    // not able to call Connection::aliasFlags() for example.
    if (status != pendingStatus ||
        statusReason != pendingStatusReason) {
        status = pendingStatus;
        statusReason = pendingStatusReason;
        emit parent->statusChanged(status, statusReason);
    }
}

void Connection::Private::updatePendingOperations()
{
    foreach (Private::PendingReady *operation, pendingOperations) {
        if (ready &&
            ((operation->features &
                (features | missingFeatures)) == operation->features)) {
            operation->setFinished();
        }
        if (operation->isFinished()) {
            pendingOperations.removeOne(operation);
        }
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
 *  <li>Getting the alias flags automatically</li>
 *  <li>Getting the valid presence statuses automatically</li>
 *  <li>Shared optional interface proxy instances</li>
 * </ul>
 *
 * The remote object state accessor functions on this object (status(),
 * statusReason(), aliasFlags(), and so on) don't make any DBus calls; instead,
 * they return values cached from a previous introspection run. The
 * introspection process populates their values in the most efficient way
 * possible based on what the service implements. Their return value is mostly
 * undefined until the introspection process is completed; a readiness change to
 * #ReadinessFull indicates that the introspection process is finished. See the
 * individual accessor descriptions for details on which functions can be used
 * in the different states.
 */

/**
 * \enum Connection::InterfaceSupportedChecking
 *
 * Specifies if the interface being supported by the remote object should be
 * checked by optionalInterface() and the convenience functions for it.
 *
 * \value CheckInterfaceSupported Don't return an interface instance unless it
 *                                can be guaranteed that the remote object
 *                                actually implements the interface.
 * \value BypassInterfaceCheck Return an interface instance even if it can't
 *                             be verified that the remote object supports the
 *                             interface.
 * \sa optionalInterface()
 */

/**
 * Construct a new Connection object.
 *
 * \param serviceName Connection service name.
 * \param objectPath Connection object path.
 * \param parent Object parent.
 */
Connection::Connection(const QString &serviceName,
                       const QString &objectPath,
                       QObject *parent)
    : StatefulDBusProxy(QDBusConnection::sessionBus(),
            serviceName, objectPath, parent),
      OptionalInterfaceFactory<Connection>(this),
      mPriv(new Private(this))
{
    mPriv->introspectQueue.enqueue(&Private::startIntrospection);
    QTimer::singleShot(0, this, SLOT(continueIntrospection()));
}

/**
 * Construct a new Connection object.
 *
 * \param bus QDBusConnection to use.
 * \param serviceName Connection service name.
 * \param objectPath Connection object path.
 * \param parent Object parent.
 */
Connection::Connection(const QDBusConnection &bus,
                       const QString &serviceName,
                       const QString &objectPath,
                       QObject *parent)
    : StatefulDBusProxy(bus, serviceName, objectPath, parent),
      OptionalInterfaceFactory<Connection>(this),
      mPriv(new Private(this))
{
    mPriv->introspectQueue.enqueue(&Private::startIntrospection);
    QTimer::singleShot(0, this, SLOT(continueIntrospection()));
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
 * The returned value may have changed whenever readinessChanged() is
 * emitted. The value is valid in all states except for
 * #ReadinessJustCreated.
 *
 * \return The status, as defined in #ConnectionStatus.
 */
uint Connection::status() const
{
    if (mPriv->readiness == Private::ReadinessJustCreated) {
        warning() << "Connection::status() used with readiness ReadinessJustCreated";
    }

    return mPriv->status;
}

/**
 * Return the reason for the connection's status (which is returned by
 * status()). The validity and change rules are the same as for status().
 *
 * \return The reason, as defined in #ConnectionStatusReason.
 */
uint Connection::statusReason() const
{
    if (mPriv->readiness == Private::ReadinessJustCreated) {
        warning() << "Connection::statusReason() used with readiness ReadinessJustCreated";
    }

    return mPriv->statusReason;
}

/**
 * Return a list of optional interfaces supported by this object. The
 * contents of the list is undefined unless the Connection has readiness
 * #ReadinessNotYetConnected or #ReadinessFull. The returned value stays
 * constant for the entire time the connection spends in each of these
 * states; however interfaces might have been added to the supported set by
 * the time #ReadinessFull is reached.
 *
 * \return Names of the supported interfaces.
 */
QStringList Connection::interfaces() const
{
    // Different check than the others, because the optional interface getters
    // may be used internally with the knowledge about getting the interfaces
    // list, so we don't want this to cause warnings.
    if (mPriv->readiness != Private::ReadinessNotYetConnected &&
        mPriv->readiness != Private::ReadinessFull &&
        mPriv->interfaces.empty()) {
        warning() << "Connection::interfaces() used possibly before the list of interfaces has been received";
    }
    else if (mPriv->readiness == Private::ReadinessDead) {
        warning() << "Connection::interfaces() used with readiness ReadinessDead";
    }

    return mPriv->interfaces;
}

/**
 * Return the bitwise OR of flags detailing the behavior of the Aliasing
 * interface on the remote object.
 *
 * The returned value is undefined unless the Connection has readiness
 * #ReadinessFull and the list returned by interfaces() contains
 * %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING.
 *
 * \return Bitfield of flags, as specified in #ConnectionAliasFlag.
 */
uint Connection::aliasFlags() const
{
    if (mPriv->missingFeatures & FeatureAliasing) {
        warning() << "Trying to retrieve aliasFlags from connection, but "
                     "aliasing is not supported";
    }
    else if (!(mPriv->features & FeatureAliasing)) {
        warning() << "Trying to retrieve aliasFlags from connection without "
                     "calling Connection::becomeReady(FeatureAliasing)";
    }
    else if (mPriv->pendingFeatures & FeatureAliasing) {
        warning() << "Trying to retrieve aliasFlags from connection, but "
                     "aliasing is still being retrieved";
    }

    return mPriv->aliasFlags;
}

/**
 * Return a dictionary of presence statuses valid for use with the legacy
 * Telepathy Presence interface on the remote object.
 *
 * The returned value is undefined unless the Connection has readiness
 * #ReadinessFull and the list returned by interfaces() contains
 * %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_PRESENCE.
 *
 * \return Dictionary from string identifiers to structs for each valid status.
 */
StatusSpecMap Connection::presenceStatuses() const
{
    if (mPriv->missingFeatures & FeaturePresence) {
        warning() << "Trying to retrieve presence from connection, but "
                     "presence is not supported";
    }
    else if (!(mPriv->features & FeaturePresence)) {
        warning() << "Trying to retrieve presence from connection without "
                     "calling Connection::becomeReady(FeaturePresence)";
    }
    else if (mPriv->pendingFeatures & FeaturePresence) {
        warning() << "Trying to retrieve presence from connection, but "
                     "presence is still being retrieved";
    }

    return mPriv->presenceStatuses;
}

/**
 * Return a dictionary of presence statuses valid for use with the new(er)
 * Telepathy SimplePresence interface on the remote object.
 *
 * The value is undefined if the list returned by interfaces() doesn't
 * contain %TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE.
 *
 * The value will stay fixed for the whole time the connection stays with
 * readiness #ReadinessNotYetConnected, but may have changed arbitrarily
 * during the time the Connection spends in readiness #ReadinessConnecting,
 * again staying fixed for the entire time in #ReadinessFull.
 *
 * \return Dictionary from string identifiers to structs for each valid
 * status.
 */
SimpleStatusSpecMap Connection::simplePresenceStatuses() const
{
    if (mPriv->missingFeatures & FeatureSimplePresence) {
        warning() << "Trying to retrieve simple presence from connection, but "
                     "simple presence is not supported";
    }
    else if (!(mPriv->features & FeatureSimplePresence)) {
        warning() << "Trying to retrieve simple presence from connection without "
                     "calling Connection::becomeReady(FeatureSimplePresence)";
    }
    else if (mPriv->pendingFeatures & FeatureSimplePresence) {
        warning() << "Trying to retrieve simple presence from connection, but "
                     "simple presence is still being retrieved";
    }

    return mPriv->simplePresenceStatuses;
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

void Connection::onStatusChanged(uint status, uint reason)
{
    debug() << "StatusChanged from" << mPriv->status
            << "to" << status << "with reason" << reason;

    if (!mPriv->haveInitialStatus) {
        debug() << "Still haven't got the GetStatus reply, ignoring StatusChanged until we have (but saving reason)";
        mPriv->pendingStatusReason = reason;
        return;
    }

    if (mPriv->pendingStatus == status) {
        warning() << "New status was the same as the old status! Ignoring redundant StatusChanged";
        return;
    }

    if (status == ConnectionStatusConnected &&
        mPriv->pendingStatus != ConnectionStatusConnecting) {
        // CMs aren't meant to go straight from Disconnected to
        // Connected; recover by faking Connecting
        warning() << " Non-compliant CM - went straight to Connected! Faking a transition through Connecting";
        onStatusChanged(ConnectionStatusConnecting, reason);
    }

    mPriv->pendingStatus = status;
    mPriv->pendingStatusReason = reason;

    switch (status) {
        case ConnectionStatusConnected:
            debug() << " Performing introspection for the Connected status";
            mPriv->introspectQueue.enqueue(&Private::introspectMain);
            continueIntrospection();
            break;

        case ConnectionStatusConnecting:
            if (mPriv->readiness < Private::ReadinessConnecting) {
                mPriv->changeReadiness(Private::ReadinessConnecting);
            }
            else {
                warning() << " Got unexpected status change to Connecting";
            }
            break;

        case ConnectionStatusDisconnected:
            if (mPriv->readiness != Private::ReadinessDead) {
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

                invalidate(QLatin1String(errorName),
                        QString("ConnectionStatusReason = %1").arg(uint(reason)));

                mPriv->changeReadiness(Private::ReadinessDead);
            }
            else {
                warning() << " Got unexpected status change to Disconnected";
            }
            break;

        default:
            warning() << "Unknown connection status" << status;
            break;
    }
}

void Connection::gotStatus(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "GetStatus() failed with" <<
            reply.error().name() << ":" << reply.error().message();
        invalidate(QLatin1String(TELEPATHY_ERROR_DISCONNECTED),
                QString("ConnectionStatusReason = %1").arg(uint(mPriv->pendingStatusReason)));
        mPriv->changeReadiness(Private::ReadinessDead);
        return;
    }

    uint status = reply.value();

    debug() << "Got connection status" << status;
    mPriv->pendingStatus = status;
    mPriv->haveInitialStatus = true;

    // Don't do any introspection yet if the connection is in the Connecting
    // state; the StatusChanged handler will take care of doing that, if the
    // connection ever gets to the Connected state.
    if (status == ConnectionStatusConnecting) {
        debug() << "Not introspecting yet because the connection is currently Connecting";
        mPriv->changeReadiness(Private::ReadinessConnecting);
        return;
    }

    if (status == ConnectionStatusDisconnected) {
        debug() << "Performing introspection for the Disconnected status";
        mPriv->initialIntrospection = true;
    }
    else {
        if (status != ConnectionStatusConnected) {
            warning() << "Not performing introspection for unknown status" << status;
            return;
        }
        else {
            debug() << "Performing introspection for the Connected status";
        }
    }

    mPriv->introspectQueue.enqueue(&Private::introspectMain);
    continueIntrospection();

    watcher->deleteLater();
}

void Connection::gotInterfaces(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Connection basic functionality is ready";
        mPriv->ready = true;
        debug() << "Got reply to GetInterfaces():" << mPriv->interfaces;
        mPriv->interfaces = reply.value();

        // queue introspection of all optional features and add the feature to
        // pendingFeatures so we don't queue up the introspect func for the feature
        // again on becomeReady.
        if (mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING)) {
            mPriv->introspectQueue.enqueue(&Private::introspectAliasing);
            mPriv->pendingFeatures |= FeatureAliasing;
        }
        if (mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_PRESENCE)) {
            mPriv->introspectQueue.enqueue(&Private::introspectPresence);
            mPriv->pendingFeatures |= FeaturePresence;
        }
        if (mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
            mPriv->introspectQueue.enqueue(&Private::introspectSimplePresence);
            mPriv->pendingFeatures |= FeatureSimplePresence;
        }
    }
    else {
        warning().nospace() << "GetInterfaces() failed with" <<
            reply.error().name() << ":" << reply.error().message() <<
            "- assuming no new interfaces";
    }

    continueIntrospection();

    watcher->deleteLater();
}

void Connection::gotAliasFlags(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    mPriv->pendingFeatures &= ~FeatureAliasing;

    if (!reply.isError()) {
        mPriv->features |= FeatureAliasing;
        debug() << "Adding FeatureAliasing to features";

        mPriv->aliasFlags = static_cast<ConnectionAliasFlag>(reply.value());
        debug().nospace() << "Got alias flags 0x" << hex << mPriv->aliasFlags;
    }
    else {
        mPriv->missingFeatures |= FeatureAliasing;
        debug() << "Adding FeatureAliasing to missing features";

        warning().nospace() << "GetAliasFlags() failed with" <<
            reply.error().name() << ":" << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
}

void Connection::gotStatuses(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<StatusSpecMap> reply = *watcher;

    mPriv->pendingFeatures &= ~FeaturePresence;

    if (!reply.isError()) {
        mPriv->features |= FeaturePresence;
        debug() << "Adding FeaturePresence to features";

        mPriv->presenceStatuses = reply.value();
        debug() << "Got" << mPriv->presenceStatuses.size() << "legacy presence statuses";
    }
    else {
        mPriv->missingFeatures |= FeaturePresence;
        debug() << "Adding FeaturePresence to missing features";


        warning().nospace() << "GetStatuses() failed with" <<
            reply.error().name() << ":" << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
}

void Connection::gotSimpleStatuses(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    mPriv->pendingFeatures &= ~FeatureSimplePresence;

    if (!reply.isError()) {
        mPriv->features |= FeatureSimplePresence;
        debug() << "Adding FeatureSimplePresence to features";

        mPriv->simplePresenceStatuses = qdbus_cast<SimpleStatusSpecMap>(reply.value().variant());
        debug() << "Got" << mPriv->simplePresenceStatuses.size() << "simple presence statuses";
    }
    else {
        mPriv->missingFeatures |= FeatureSimplePresence;
        debug() << "Adding FeatureSimplePresence to missing features";

        warning().nospace() << "Getting simple presence statuses failed with" <<
            reply.error().name() << ":" << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
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
 * Asynchronously requests a channel satisfying the given channel type and
 * communicating with the contact, room, list etc. given by the handle type
 * and handle.
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingChannel object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to to get
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
 * \param channelType D-Bus interface name of the channel type to request,
 *                    such as TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT.
 * \param handleType Type of the handle given, as specified in #HandleType.
 * \param handle Handle specifying the remote entity to communicate with.
 * \return Pointer to a newly constructed PendingChannel object, tracking
 *         the progress of the request.
 */
PendingChannel *Connection::requestChannel(const QString &channelType,
        uint handleType, uint handle)
{
    debug() << "Requesting a Channel with type" << channelType
            << "and handle" << handle << "of type" << handleType;

    PendingChannel *channel =
        new PendingChannel(this, channelType, handleType, handle);
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(mPriv->baseInterface->RequestChannel(
                    channelType, handleType, handle, true), channel);

    channel->connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                              SLOT(onCallFinished(QDBusPendingCallWatcher *)));

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
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                mPriv->baseInterface->RequestHandles(handleType, names),
                pending);

    pending->connect(watcher,
                     SIGNAL(finished(QDBusPendingCallWatcher *)),
                     SLOT(onCallFinished(QDBusPendingCallWatcher *)));

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
        new PendingHandles(this, handleType, handles, alreadyHeld);

    if (!notYetHeld.isEmpty()) {
        debug() << " Calling HoldHandles";

        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    mPriv->baseInterface->HoldHandles(handleType, notYetHeld),
                    pending);

        pending->connect(watcher,
                         SIGNAL(finished(QDBusPendingCallWatcher *)),
                         SLOT(onCallFinished(QDBusPendingCallWatcher *)));
    }
    else {
        debug() << " All handles already held, not calling HoldHandles";
    }

    return pending;
}

/**
 * Return whether this object has finished its initial setup.
 *
 * This is mostly useful as a sanity check, in code that shouldn't be run
 * until the object is ready. To wait for the object to be ready, call
 * becomeReady() and connect to the finished signal on the result.
 *
 * \param features Which features should be tested.
 * \return \c true if the object has finished initial setup.
 */
bool Connection::isReady(Features features) const
{
    return mPriv->ready
        && ((mPriv->features & features) == features);
}

/**
 * Return a pending operation which will succeed when this object finishes
 * its initial setup, or will fail if a fatal error occurs during this
 * initial setup.
 *
 * \param features Which features should be tested.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when this object has finished or failed its initial setup.
 */
PendingOperation *Connection::becomeReady(Features requestedFeatures)
{
    if (isReady(requestedFeatures)) {
        return new PendingSuccess(this);
    }

    debug() << "Calling becomeReady with requested features:"
            << requestedFeatures;
    foreach (Private::PendingReady *operation, mPriv->pendingOperations) {
        if (operation->features == requestedFeatures) {
            debug() << "Returning cached pending operation";
            return operation;
        }
    }

    Feature optionalFeatures[3] = { FeatureAliasing, FeaturePresence, FeatureSimplePresence };
    Feature optionalFeature;
    for (uint i = 0; i < sizeof(optionalFeatures) / sizeof(Feature); ++i) {
        optionalFeature = optionalFeatures[i];

        if (requestedFeatures & optionalFeature) {
            // as the feature is optional, if it's know to not be supported,
            // just finish silently
            if (requestedFeatures == (int) optionalFeature &&
                mPriv->missingFeatures & optionalFeature) {
                return new PendingSuccess(this);
            }

            // don't enqueue introspect funcs here, as they will be enqueued
            // when possible, depending on readiness, e.g. introspectMain needs
            // to be called before introspectAliasing, ...
        }
    }

    mPriv->pendingFeatures |= requestedFeatures;

    debug() << "Creating new pending operation";
    Private::PendingReady *operation =
        new Private::PendingReady(requestedFeatures, this);
    mPriv->pendingOperations.append(operation);

    mPriv->updatePendingOperations();
    return operation;
}

/**
 * Start an asynchronous request that the connection be connected.
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * \return A %PendingOperation, which will emit finished when the
 *         Disconnect D-Bus method returns.
 */
PendingOperation *Connection::requestConnect()
{
    return new PendingVoidMethodCall(this, baseInterface()->Connect());
}

/**
 * Start an asynchronous request that the connection be disconnected.
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * \return A %PendingOperation, which will emit finished when the
 *         Disconnect D-Bus method returns.
 */
PendingOperation *Connection::requestDisconnect()
{
    return new PendingVoidMethodCall(this, baseInterface()->Disconnect());
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

void Connection::continueIntrospection()
{
    if (mPriv->introspectQueue.isEmpty()) {
        if (mPriv->initialIntrospection) {
            mPriv->initialIntrospection = false;
            if (mPriv->readiness < Private::ReadinessNotYetConnected) {
                mPriv->changeReadiness(Private::ReadinessNotYetConnected);
            }
        }
        else {
            if (mPriv->readiness != Private::ReadinessDead) {
                mPriv->changeReadiness(Private::ReadinessFull);
                // we should have all interfaces now, so if an interface is not
                // present and we have a feature for it, add the feature to missing
                // features.
                if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING)) {
                    debug() << "adding FeatureAliasing to missing features";
                    mPriv->missingFeatures |= FeatureAliasing;
                }
                if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_PRESENCE)) {
                    debug() << "adding FeaturePresence to missing features";
                    mPriv->missingFeatures |= FeaturePresence;
                }
                if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
                    debug() << "adding FeatureSimplePresence to missing features";
                    mPriv->missingFeatures |= FeatureSimplePresence;
                }
            }
        }
    }
    else {
        (mPriv->*(mPriv->introspectQueue.dequeue()))();
    }

    mPriv->updatePendingOperations();
}

}
}
