/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include "TelepathyQt/connection-internal.h"

#include "TelepathyQt/_gen/cli-connection.moc.hpp"
#include "TelepathyQt/_gen/cli-connection-body.hpp"
#include "TelepathyQt/_gen/connection.moc.hpp"
#include "TelepathyQt/_gen/connection-internal.moc.hpp"
#include "TelepathyQt/_gen/connection-lowlevel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContactAttributes>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/ReferencedHandles>

#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QQueue>
#include <QString>
#include <QTimer>
#include <QtGlobal>

namespace Tp
{

struct TP_QT_NO_EXPORT Connection::Private
{
    Private(Connection *parent,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory);
    ~Private();

    void init();

    static void introspectMain(Private *self);
    void introspectMainFallbackStatus();
    void introspectMainFallbackInterfaces();
    void introspectMainFallbackSelfHandle();
    void introspectCapabilities();
    void introspectContactAttributeInterfaces();
    static void introspectSelfContact(Private *self);
    static void introspectSimplePresence(Private *self);
    static void introspectRoster(Private *self);
    static void introspectRosterGroups(Private *self);
    static void introspectBalance(Private *self);
    static void introspectConnected(Private *self);

    void continueMainIntrospection();
    void setCurrentStatus(uint status);
    void forceCurrentStatus(uint status);
    void setInterfaces(const QStringList &interfaces);

    // Should always be used instead of directly using baseclass invalidate()
    void invalidateResetCaps(const QString &errorName, const QString &errorMessage);

    struct HandleContext;

    // Public object
    Connection *parent;
    ConnectionLowlevelPtr lowlevel;

    // Factories
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Instance of generated interface class
    Client::ConnectionInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    // Optional interface proxies
    Client::ConnectionInterfaceSimplePresenceInterface *simplePresence;

    ReadinessHelper *readinessHelper;

    // Introspection
    QQueue<void (Private::*)()> introspectMainQueue;

    // FeatureCore
    // keep pendingStatus and pendingStatusReason until we emit statusChanged
    // so Connection::status() and Connection::statusReason() are consistent
    bool introspectingConnected;

    uint pendingStatus;
    uint pendingStatusReason;
    uint status;
    uint statusReason;
    ErrorDetails errorDetails;

    uint selfHandle;

    bool immortalHandles;

    ConnectionCapabilities caps;

    ContactManagerPtr contactManager;

    // FeatureSelfContact
    bool introspectingSelfContact;
    bool reintrospectSelfContactRequired;
    ContactPtr selfContact;
    QStringList contactAttributeInterfaces;

    // FeatureSimplePresence
    SimpleStatusSpecMap simplePresenceStatuses;
    uint maxPresenceStatusMessageLength;

    // FeatureAccountBalance
    CurrencyAmount accountBalance;

    // misc
    // (Bus connection name, service name) -> HandleContext
    static QHash<QPair<QString, QString>, HandleContext *> handleContexts;
    static QMutex handleContextsLock;
    HandleContext *handleContext;

    QString cmName;
    QString protocolName;
};

struct TP_QT_NO_EXPORT ConnectionLowlevel::Private
{
    Private(Connection *conn)
        : conn(conn)
    {
    }

    WeakPtr<Connection> conn;
    HandleIdentifierMap contactsIds;
};

// Handle tracking
struct TP_QT_NO_EXPORT Connection::Private::HandleContext
{
    struct Type
    {
        QHash<uint, uint> refcounts;
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
    QHash<uint, Type> types;
};

Connection::Private::Private(Connection *parent,
        const ChannelFactoryConstPtr &chanFactory,
        const ContactFactoryConstPtr &contactFactory)
    : parent(parent),
      lowlevel(ConnectionLowlevelPtr(new ConnectionLowlevel(parent))),
      chanFactory(chanFactory),
      contactFactory(contactFactory),
      baseInterface(new Client::ConnectionInterface(parent)),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      simplePresence(0),
      readinessHelper(parent->readinessHelper()),
      introspectingConnected(false),
      pendingStatus((uint) -1),
      pendingStatusReason(ConnectionStatusReasonNoneSpecified),
      status((uint) -1),
      statusReason(ConnectionStatusReasonNoneSpecified),
      selfHandle(0),
      immortalHandles(false),
      contactManager(ContactManagerPtr(new ContactManager(parent))),
      introspectingSelfContact(false),
      reintrospectSelfContactRequired(false),
      maxPresenceStatusMessageLength(0),
      handleContext(0)
{
    accountBalance.amount = 0;
    accountBalance.scale = 0;

    Q_ASSERT(properties != 0);

    if (chanFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the channel factory is not the proxy connection";
    }

    init();

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << (uint) -1 <<
                        ConnectionStatusDisconnected <<
                        ConnectionStatusConnected,                                     // makesSenseForStatuses
        Features(),                                                                    // dependsOnFeatures (none)
        QStringList(),                                                                 // dependsOnInterfaces (none)
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableSelfContact(
        QSet<uint>() << ConnectionStatusConnected,                                     // makesSenseForStatuses
        Features() << FeatureCore,                                                     // dependsOnFeatures (core)
        QStringList(),                                                                 // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectSelfContact,
        this);
    introspectables[FeatureSelfContact] = introspectableSelfContact;

    ReadinessHelper::Introspectable introspectableSimplePresence(
        QSet<uint>() << ConnectionStatusDisconnected <<
                        ConnectionStatusConnected,                                                  // makesSenseForStatuses
        Features() << FeatureCore,                                                                  // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,   // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectSimplePresence,
        this);
    introspectables[FeatureSimplePresence] = introspectableSimplePresence;

    ReadinessHelper::Introspectable introspectableRoster(
        QSet<uint>() << ConnectionStatusConnected,                                                  // makesSenseForStatuses
        Features() << FeatureCore,                                                                  // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS,          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectRoster,
        this);
    introspectables[FeatureRoster] = introspectableRoster;

    ReadinessHelper::Introspectable introspectableRosterGroups(
        QSet<uint>() << ConnectionStatusConnected,                                                  // makesSenseForStatuses
        Features() << FeatureRoster,                                                                // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS,          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectRosterGroups,
        this);
    introspectables[FeatureRosterGroups] = introspectableRosterGroups;

    ReadinessHelper::Introspectable introspectableBalance(
        QSet<uint>() << ConnectionStatusConnected,                                                  // makesSenseForStatuses
        Features() << FeatureCore,                                                                  // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CONNECTION_INTERFACE_BALANCE,           // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectBalance,
        this);
    introspectables[FeatureAccountBalance] = introspectableBalance;

    ReadinessHelper::Introspectable introspectableConnected(
        QSet<uint>() << (uint) -1 <<
                        ConnectionStatusDisconnected <<
                        ConnectionStatusConnecting <<
                        ConnectionStatusConnected,                                                  // makesSenseForStatuses
        Features() << FeatureCore,                                                                  // dependsOnFeatures (none)
        QStringList(),                                                                              // dependsOnInterfaces (none)
        (ReadinessHelper::IntrospectFunc) &Private::introspectConnected,
        this);
    introspectables[FeatureConnected] = introspectableConnected;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->setCurrentStatus(status);
    parent->connect(readinessHelper,
            SIGNAL(statusReady(uint)),
            SLOT(onStatusReady(uint)));

    // FIXME: QRegExp probably isn't the most efficient possible way to parse
    //        this :-)
    QRegExp rx(QLatin1String("^") + TP_QT_CONNECTION_OBJECT_PATH_BASE + QLatin1String(
                "([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
                "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
                "/([_A-Za-z][_A-Za-z0-9]*)"  // account-specific part
                ));

    if (rx.exactMatch(parent->objectPath())) {
        cmName = rx.cap(1);
        protocolName = rx.cap(2);
    } else {
        warning() << "Connection object path is not spec-compliant, "
            "trying again with a different account-specific part check";

        rx = QRegExp(QLatin1String("^") + TP_QT_CONNECTION_OBJECT_PATH_BASE + QLatin1String(
                    "([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
                    "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
                    "/([_A-Za-z0-9]*)"  // account-specific part
                    ));
        if (rx.exactMatch(parent->objectPath())) {
            cmName = rx.cap(1);
            protocolName = rx.cap(2);
        } else {
            warning() << "Not a valid Connection object path:" <<
                parent->objectPath();
        }
    }
}

Connection::Private::~Private()
{
    contactManager->resetRoster();

    // Clear selfContact so its handle will be released cleanly before the
    // handleContext
    selfContact.reset();

    QMutexLocker locker(&handleContextsLock);
    // All handle contexts locked, so safe
    if (!--handleContext->refcount) {
        if (!immortalHandles) {
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

        }

        handleContexts.remove(qMakePair(baseInterface->connection().name(),
                    parent->objectPath()));
        delete handleContext;
    } else {
        Q_ASSERT(handleContext->refcount > 0);
    }
}

void Connection::Private::init()
{
    debug() << "Connecting to ConnectionError()";
    parent->connect(baseInterface,
            SIGNAL(ConnectionError(QString,QVariantMap)),
            SLOT(onConnectionError(QString,QVariantMap)));
    debug() << "Connecting to StatusChanged()";
    parent->connect(baseInterface,
            SIGNAL(StatusChanged(uint,uint)),
            SLOT(onStatusChanged(uint,uint)));
    debug() << "Connecting to SelfHandleChanged()";
    parent->connect(baseInterface,
            SIGNAL(SelfHandleChanged(uint)),
            SLOT(onSelfHandleChanged(uint)));

    QMutexLocker locker(&handleContextsLock);
    QString busConnectionName = baseInterface->connection().name();

    if (handleContexts.contains(qMakePair(busConnectionName, parent->objectPath()))) {
        debug() << "Reusing existing HandleContext for" << parent->objectPath();
        handleContext = handleContexts[
            qMakePair(busConnectionName, parent->objectPath())];
    } else {
        debug() << "Creating new HandleContext for" << parent->objectPath();
        handleContext = new HandleContext;
        handleContexts[
            qMakePair(busConnectionName, parent->objectPath())] = handleContext;
    }

    // All handle contexts locked, so safe
    ++handleContext->refcount;
}

void Connection::Private::introspectMain(Connection::Private *self)
{
    debug() << "Calling Properties::GetAll(Connection)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(TP_QT_IFACE_CONNECTION),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectMainFallbackStatus()
{
    debug() << "Calling GetStatus()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetStatus(),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotStatus(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectMainFallbackInterfaces()
{
    debug() << "Calling GetInterfaces()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetInterfaces(),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectMainFallbackSelfHandle()
{
    debug() << "Calling GetSelfHandle()";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(baseInterface->GetSelfHandle(),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotSelfHandle(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectCapabilities()
{
    debug() << "Retrieving capabilities";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->Get(
                TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS,
                QLatin1String("RequestableChannelClasses")), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotCapabilities(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectContactAttributeInterfaces()
{
    debug() << "Retrieving contact attribute interfaces";
    QDBusPendingCall call =
        properties->Get(
                TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS,
                QLatin1String("ContactAttributeInterfaces"));
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(call, parent);
    parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotContactAttributeInterfaces(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectSelfContact(Connection::Private *self)
{
    debug() << "Building self contact";

    Q_ASSERT(!self->introspectingSelfContact);

    self->introspectingSelfContact = true;
    self->reintrospectSelfContactRequired = false;

    PendingContacts *contacts = self->contactManager->contactsForHandles(
            UIntList() << self->selfHandle);
    self->parent->connect(contacts,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotSelfContact(Tp::PendingOperation*)));
}

void Connection::Private::introspectSimplePresence(Connection::Private *self)
{
    Q_ASSERT(self->properties != 0);

    debug() << "Calling Properties::Get("
        "Connection.I.SimplePresence.Statuses)";
    QDBusPendingCall call =
        self->properties->GetAll(
                TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE);
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(call, self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotSimpleStatuses(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectRoster(Connection::Private *self)
{
    debug() << "Introspecting roster";

    PendingOperation *op = self->contactManager->introspectRoster();
    self->parent->connect(op,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onIntrospectRosterFinished(Tp::PendingOperation*)));
}

void Connection::Private::introspectRosterGroups(Connection::Private *self)
{
    debug() << "Introspecting roster groups";

    PendingOperation *op = self->contactManager->introspectRosterGroups();
    self->parent->connect(op,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onIntrospectRosterGroupsFinished(Tp::PendingOperation*)));
}

void Connection::Private::introspectBalance(Connection::Private *self)
{
    debug() << "Introspecting balance";

    // we already checked if balance interface exists, so bypass requests
    // interface checking
    Client::ConnectionInterfaceBalanceInterface *iface =
        self->parent->interface<Client::ConnectionInterfaceBalanceInterface>();

    debug() << "Connecting to Balance.BalanceChanged";
    self->parent->connect(iface,
            SIGNAL(BalanceChanged(Tp::CurrencyAmount)),
            SLOT(onBalanceChanged(Tp::CurrencyAmount)));

    debug() << "Retrieving balance";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            self->properties->Get(
                TP_QT_IFACE_CONNECTION_INTERFACE_BALANCE,
                QLatin1String("AccountBalance")), self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotBalance(QDBusPendingCallWatcher*)));
}

void Connection::Private::introspectConnected(Connection::Private *self)
{
    Q_ASSERT(!self->introspectingConnected);
    self->introspectingConnected = true;

    if (self->pendingStatus == ConnectionStatusConnected) {
        self->readinessHelper->setIntrospectCompleted(FeatureConnected, true);
        self->introspectingConnected = false;
    }
}

void Connection::Private::continueMainIntrospection()
{
    if (!parent->isValid()) {
        debug() << parent << "stopping main introspection, as it has been invalidated";
        return;
    }

    if (introspectMainQueue.isEmpty()) {
        readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        (this->*(introspectMainQueue.dequeue()))();
    }
}

void Connection::Private::setCurrentStatus(uint status)
{
    // ReadinessHelper waits for all in-flight introspection ops to finish for the current status
    // before proceeding to a new one.
    //
    // Therefore we don't need any safeguarding here to prevent finishing introspection when there
    // is a pending status change. However, we can speed up the process slightly by canceling any
    // pending introspect ops from our local introspection queue when it's waiting for us.

    introspectMainQueue.clear();

    if (introspectingConnected) {
        // On the other hand, we have to finish the Connected introspection for now, as
        // ReadinessHelper would otherwise wait indefinitely for it to land
        debug() << "Finishing FeatureConnected for status" << this->status <<
            "to allow ReadinessHelper to introspect new status" << status;
        readinessHelper->setIntrospectCompleted(FeatureConnected, true);
        introspectingConnected = false;
    }

    readinessHelper->setCurrentStatus(status);
}

void Connection::Private::forceCurrentStatus(uint status)
{
    // only update the status if we did not get it from StatusChanged
    if (pendingStatus == (uint) -1) {
        debug() << "Got status:" << status;
        pendingStatus = status;
        // No need to re-run introspection as we just received the status. Let
        // the introspection continue normally but update readinessHelper with
        // the correct status.
        readinessHelper->forceCurrentStatus(status);
    }
}

void Connection::Private::setInterfaces(const QStringList &interfaces)
{
    debug() << "Got interfaces:" << interfaces;
    parent->setInterfaces(interfaces);
    readinessHelper->setInterfaces(interfaces);
}

void Connection::Private::invalidateResetCaps(const QString &error, const QString &message)
{
    caps.updateRequestableChannelClasses(RequestableChannelClassList());
    parent->invalidate(error, message);
}

/**
 * \class ConnectionLowlevel
 * \ingroup clientconn
 * \headerfile TelepathyQt/connection-lowlevel.h <TelepathyQt/ConnectionLowlevel>
 *
 * \brief The ConnectionLowlevel class extends Connection with support to
 * low-level features.
 */

ConnectionLowlevel::ConnectionLowlevel(Connection *conn)
    : mPriv(new Private(conn))
{
}

ConnectionLowlevel::~ConnectionLowlevel()
{
    delete mPriv;
}

bool ConnectionLowlevel::isValid() const
{
    return !(connection().isNull());
}

ConnectionPtr ConnectionLowlevel::connection() const
{
    return ConnectionPtr(mPriv->conn);
}

Connection::PendingConnect::PendingConnect(const ConnectionPtr &connection,
        const Features &requestedFeatures)
    : PendingReady(connection, requestedFeatures)
{
    if (!connection) {
        // Called when the connection had already been destroyed
        return;
    }

    QDBusPendingCall call = connection->baseInterface()->Connect();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

    connect(connection.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            this,
            SLOT(onConnInvalidated(Tp::DBusProxy*,QString,QString)));

    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(onConnectReply(QDBusPendingCallWatcher*)));
}

void Connection::PendingConnect::onConnectReply(QDBusPendingCallWatcher *watcher)
{
    ConnectionPtr connection = ConnectionPtr::qObjectCast(proxy());

    if (watcher->isError()) {
        debug() << "Connect failed with" <<
            watcher->error().name() << ": " << watcher->error().message();
        setFinishedWithError(watcher->error());
        connection->disconnect(
                this,
                SLOT(onConnInvalidated(Tp::DBusProxy*,QString,QString)));
    } else {
        if (connection->status() == ConnectionStatusConnected) {
            onStatusChanged(ConnectionStatusConnected);
        } else {
            // Wait for statusChanged()! Connect returning just means that the connection has
            // started to connect - spec quoted for truth:
            //
            // Connect () -> nothing
            // Request that the connection be established. This will be done asynchronously and
            // errors will be returned by emitting StatusChanged signals.
            //
            // Which should actually say progress and/or errors IMO, but anyway...
            connect(connection.data(),
                    SIGNAL(statusChanged(Tp::ConnectionStatus)),
                    SLOT(onStatusChanged(Tp::ConnectionStatus)));
        }
    }

    watcher->deleteLater();
}

void Connection::PendingConnect::onStatusChanged(ConnectionStatus newStatus)
{
    ConnectionPtr connection = ConnectionPtr::qObjectCast(proxy());

    if (newStatus == ConnectionStatusDisconnected) {
        debug() << "Connection became disconnected while a PendingConnect was underway";
        setFinishedWithError(connection->invalidationReason(), connection->invalidationMessage());

        connection->disconnect(this,
                SLOT(onConnInvalidated(Tp::DBusProxy*,QString,QString)));

        return;
    }

    if (newStatus == ConnectionStatusConnected) {
        // OK, the connection is Connected now - finally, we'll get down to business
        connect(connection->becomeReady(requestedFeatures()),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onBecomeReadyReply(Tp::PendingOperation*)));
    }
}

void Connection::PendingConnect::onBecomeReadyReply(Tp::PendingOperation *op)
{
    ConnectionPtr connection = ConnectionPtr::qObjectCast(proxy());

    // We don't care about future disconnects even if they happen before we are destroyed
    // (which happens two mainloop iterations from now)
    connection->disconnect(this,
            SLOT(onStatusChanged(Tp::ConnectionStatus)));
    connection->disconnect(this,
            SLOT(onConnInvalidated(Tp::DBusProxy*,QString,QString)));

    if (op->isError()) {
        debug() << "Connection->becomeReady failed with" <<
            op->errorName() << ": " << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        debug() << "Connected";

        if (connection->isValid()) {
            setFinished();
        } else {
            debug() << "  ... but the Connection was immediately invalidated!";
            setFinishedWithError(connection->invalidationReason(), connection->invalidationMessage());
        }
    }
}

void Connection::PendingConnect::onConnInvalidated(Tp::DBusProxy *proxy, const QString &error,
        const QString &message)
{
    ConnectionPtr connection = ConnectionPtr::qObjectCast(this->proxy());

    Q_ASSERT(proxy == connection.data());

    if (!isFinished()) {
        debug() << "Unable to connect. Connection invalidated";
        setFinishedWithError(error, message);
    }

    connection->disconnect(this,
            SLOT(onStatusChanged(Tp::ConnectionStatus)));
}

QHash<QPair<QString, QString>, Connection::Private::HandleContext*> Connection::Private::handleContexts;
QMutex Connection::Private::handleContextsLock;

/**
 * \class Connection
 * \ingroup clientconn
 * \headerfile TelepathyQt/connection.h <TelepathyQt/Connection>
 *
 * \brief The Connection class represents a Telepathy connection.
 *
 * This models a connection to a single user account on a communication service.
 *
 * Contacts, and server-stored lists (such as subscribed contacts,
 * block lists, or allow lists) on a service are all represented using the
 * ContactManager object on the connection, which is valid only for the lifetime
 * of the connection object.
 *
 * The remote object accessor functions on this object (status(),
 * statusReason(), and so on) don't make any D-Bus calls; instead, they return/use
 * values cached from a previous introspection run. The introspection process
 * populates their values in the most efficient way possible based on what the
 * service implements.
 *
 * To avoid unnecessary D-Bus traffic, some accessors only return valid
 * information after specific features have been enabled.
 * For instance, to retrieve the connection self contact, it is necessary to
 * enable the feature Connection::FeatureSelfContact.
 * See the individual methods descriptions for more details.
 *
 * Connection features can be enabled by constructing a ConnectionFactory and enabling
 * the desired features, and passing it to AccountManager, Account or ClientRegistrar
 * when creating them as appropriate. However, if a particular
 * feature is only ever used in a specific circumstance, such as an user opening
 * some settings dialog separate from the general view of the application,
 * features can be later enabled as needed by calling becomeReady() with the additional
 * features, and waiting for the resulting PendingOperation to finish.
 *
 * As an addition to accessors, signals are emitted to indicate that properties have changed,
 * for example statusChanged()(), selfContactChanged(), etc.
 *
 * \section conn_usage_sec Usage
 *
 * \subsection conn_create_sec Creating a connection object
 *
 * The easiest way to create connection objects is through Account. One can
 * just use the Account::connection method to get an account active connection.
 *
 * If you already know the object path, you can just call create().
 * For example:
 *
 * \code ConnectionPtr conn = Connection::create(busName, objectPath); \endcode
 *
 * A ConnectionPtr object is returned, which will automatically keep
 * track of object lifetime.
 *
 * You can also provide a D-Bus connection as a QDBusConnection:
 *
 * \code
 *
 * ConnectionPtr conn = Connection::create(QDBusConnection::sessionBus(),
 *         busName, objectPath);
 *
 * \endcode
 *
 * \subsection conn_ready_sec Making connection ready to use
 *
 * A Connection object needs to become ready before usage, meaning that the
 * introspection process finished and the object accessors can be used.
 *
 * To make the object ready, use becomeReady() and wait for the
 * PendingOperation::finished() signal to be emitted.
 *
 * \code
 *
 * class MyClass : public QObject
 * {
 *     QOBJECT
 *
 * public:
 *     MyClass(QObject *parent = 0);
 *     ~MyClass() { }
 *
 * private Q_SLOTS:
 *     void onConnectionReady(Tp::PendingOperation*);
 *
 * private:
 *     ConnectionPtr conn;
 * };
 *
 * MyClass::MyClass(const QString &busName, const QString &objectPath,
 *         QObject *parent)
 *     : QObject(parent)
 *       conn(Connection::create(busName, objectPath))
 * {
 *     // connect and become ready
 *     connect(conn->requestConnect(),
 *             SIGNAL(finished(Tp::PendingOperation*)),
 *             SLOT(onConnectionReady(Tp::PendingOperation*)));
 * }
 *
 * void MyClass::onConnectionReady(Tp::PendingOperation *op)
 * {
 *     if (op->isError()) {
 *         qWarning() << "Account cannot become ready:" <<
 *             op->errorName() << "-" << op->errorMessage();
 *         return;
 *     }
 *
 *     // Connection is now ready
 * }
 *
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * Connection object usable.
 *
 * Note that this feature must be enabled in order to use most Connection
 * methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature Connection::FeatureCore = Feature(QLatin1String(Connection::staticMetaObject.className()), 0, true);

/**
 * Feature used to retrieve the connection self contact.
 *
 * See self contact specific methods' documentation for more details.
 *
 * \sa selfContact(), selfContactChanged()
 */
const Feature Connection::FeatureSelfContact = Feature(QLatin1String(Connection::staticMetaObject.className()), 1);

/**
 * Feature used to retrieve/keep track of the connection self presence.
 *
 * See simple presence specific methods' documentation for more details.
 */
const Feature Connection::FeatureSimplePresence = Feature(QLatin1String(Connection::staticMetaObject.className()), 2);

/**
 * Feature used to enable roster support on Connection::contactManager.
 *
 * See ContactManager roster specific methods' documentation for more details.
 *
 * \sa ContactManager::allKnownContacts()
 */
const Feature Connection::FeatureRoster = Feature(QLatin1String(Connection::staticMetaObject.className()), 4);

/**
 * Feature used to enable roster groups support on Connection::contactManager.
 *
 * See ContactManager roster groups specific methods' documentation for more
 * details.
 *
 * \sa ContactManager::allKnownGroups()
 */
const Feature Connection::FeatureRosterGroups = Feature(QLatin1String(Connection::staticMetaObject.className()), 5);

/**
 * Feature used to retrieve/keep track of the connection account balance.
 *
 * See account balance specific methods' documentation for more details.
 *
 * \sa accountBalance(), accountBalanceChanged()
 */
const Feature Connection::FeatureAccountBalance = Feature(QLatin1String(Connection::staticMetaObject.className()), 6);

/**
 * When this feature is prepared, it means that the connection status() is
 * ConnectionStatusConnected.
 *
 * Note that if ConnectionFactory is being used with FeatureConnected set, Connection objects will
 * only be signalled by the library when the corresponding connection is in status()
 * ConnectionStatusConnected.
 */
const Feature Connection::FeatureConnected = Feature(QLatin1String(Connection::staticMetaObject.className()), 7);

/**
 * Create a new connection object using QDBusConnection::sessionBus().
 *
 * A warning is printed if the factories are not for QDBusConnection::sessionBus().
 *
 * \param busName The connection well-known bus name (sometimes called a
 *                "service name").
 * \param objectPath The connection object path.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ConnectionPtr object pointing to the newly created Connection object.
 */
ConnectionPtr Connection::create(const QString &busName,
        const QString &objectPath,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return ConnectionPtr(new Connection(QDBusConnection::sessionBus(),
                busName, objectPath, channelFactory, contactFactory,
                Connection::FeatureCore));
}

/**
 * Create a new connection object using the given \a bus.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param busName The connection well-known bus name (sometimes called a
 *                "service name").
 * \param objectPath The connection object path.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ConnectionPtr object pointing to the newly created Connection object.
 */
ConnectionPtr Connection::create(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return ConnectionPtr(new Connection(bus, busName, objectPath, channelFactory, contactFactory,
                Connection::FeatureCore));
}

/**
 * Construct a new connection object using the given \a bus.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param busName The connection's well-known bus name (sometimes called a
 *                "service name").
 * \param objectPath The connection object path.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \param coreFeature The core feature of the Connection subclass. The corresponding introspectable
 * should depend on Connection::FeatureCore.
 */
Connection::Connection(const QDBusConnection &bus,
        const QString &busName,
        const QString &objectPath,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const Feature &coreFeature)
    : StatefulDBusProxy(bus, busName, objectPath, coreFeature),
      OptionalInterfaceFactory<Connection>(this),
      mPriv(new Private(this, channelFactory, contactFactory))
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
 * Return the channel factory used by this connection.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ChannelFactory object.
 */
ChannelFactoryConstPtr Connection::channelFactory() const
{
    return mPriv->chanFactory;
}

/**
 * Return the contact factory used by this connection.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ContactFactory object.
 */
ContactFactoryConstPtr Connection::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return the connection manager name of this connection.
 *
 * \return The connection manager name.
 */
QString Connection::cmName() const
{
    return mPriv->cmName;
}

/**
 * Return the protocol name of this connection.
 *
 * \return The protocol name.
 */
QString Connection::protocolName() const
{
    return mPriv->protocolName;
}

/**
 * Return the status of this connection.
 *
 * Change notification is via the statusChanged() signal.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \return The status as #ConnectionStatus.
 * \sa statusChanged(), statusReason(), errorDetails()
 */
ConnectionStatus Connection::status() const
{
    return (ConnectionStatus) mPriv->status;
}

/**
 * Return the reason for this connection status.
 *
 * The validity and change rules are the same as for status().
 *
 * The status reason should be only used as a fallback in error handling when the application
 * doesn't understand an error name given as the invalidation reason, which may in some cases be
 * domain/UI-specific.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \return The status reason as #ConnectionStatusReason.
 * \sa invalidated(), invalidationReason()
 */
ConnectionStatusReason Connection::statusReason() const
{
    return (ConnectionStatusReason) mPriv->statusReason;
}

struct TP_QT_NO_EXPORT Connection::ErrorDetails::Private : public QSharedData
{
    Private(const QVariantMap &details)
        : details(details) {}

    QVariantMap details;
};

/**
 * \class Connection::ErrorDetails
 * \ingroup clientconn
 * \headerfile TelepathyQt/connection.h <TelepathyQt/Connection>
 *
 * \brief The Connection::ErrorDetails class represents the details of a connection error.
 *
 * It contains detailed information about the reason for the connection going invalidated().
 *
 * Some services may provide additional error information in the ConnectionError D-Bus signal, when
 * a Connection is disconnected / has become unusable. If the service didn't provide any, or has not
 * been invalidated yet, the instance will be invalid, as returned by isValid().
 *
 * The information provided by invalidationReason() and this class should always be used in error
 * handling in preference to statusReason(). The status reason can be used as a fallback, however,
 * if the client doesn't understand what a particular value returned by invalidationReason() means,
 * as it may be domain-specific with some services.
 *
 * Connection::errorDetails() can be used to return the instance containing the details for
 * invalidating that connection after invalidated() has been emitted.
 */

/**
 * Constructs a new invalid ErrorDetails instance.
 */
Connection::ErrorDetails::ErrorDetails()
    : mPriv(0)
{
}

/**
 * Construct a error details instance with the given details. The instance will indicate that
 * it is valid.
 */
Connection::ErrorDetails::ErrorDetails(const QVariantMap &details)
    : mPriv(new Private(details))
{
}

/**
 * Copy constructor.
 */
Connection::ErrorDetails::ErrorDetails(const ErrorDetails &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
Connection::ErrorDetails::~ErrorDetails()
{
}

/**
 * Assigns all information (validity, details) from other to this.
 */
Connection::ErrorDetails &Connection::ErrorDetails::operator=(
        const ErrorDetails &other)
{
    if (this->mPriv.constData() != other.mPriv.constData())
        this->mPriv = other.mPriv;

    return *this;
}

/**
 * \fn bool Connection::ErrorDetails::isValid() const
 *
 * Return whether or not the details are valid (have actually been received from the service).
 *
 * \return \c true if valid, \c false otherwise.
 */

/**
 * \fn bool Connection::ErrorDetails::hasDebugMessage() const
 *
 * Return whether or not the details specify a debug message.
 *
 * If present, the debug message will likely be the same string as the one returned by
 * invalidationMessage().
 *
 * The debug message is purely informational, offered for display for bug reporting purposes, and
 * should not be attempted to be parsed.
 *
 * \return \c true if debug message is present, \c false otherwise.
 * \sa debugMessage()
 */

/**
 * \fn QString Connection::ErrorDetails::debugMessage() const
 *
 * Return the debug message specified by the details, if any.
 *
 * If present, the debug message will likely be the same string as the one returned by
 * invalidationMessage().
 *
 * The debug message is purely informational, offered for display for bug reporting purposes, and
 * should not be attempted to be parsed.
 *
 * \return The debug message, or an empty string if there is none.
 * \sa hasDebugMessage()
 */

/**
 * Return a map containing all details given in the low-level ConnectionError signal.
 *
 * This is useful for accessing domain-specific additional details.
 *
 * \return The details of the connection error as QVariantMap.
 */
QVariantMap Connection::ErrorDetails::allDetails() const
{
    return isValid() ? mPriv->details : QVariantMap();
}

/**
 * Return detailed information about the reason for the connection going invalidated().
 *
 * Some services may provide additional error information in the ConnectionError D-Bus signal, when
 * a Connection is disconnected / has become unusable. If the service didn't provide any, or has not
 * been invalidated yet, an invalid instance is returned.
 *
 * The information provided by invalidationReason() and this method should always be used in error
 * handling in preference to statusReason(). The status reason can be used as a fallback, however,
 * if the client doesn't understand what a particular value returned by invalidationReason() means,
 * as it may be domain-specific with some services.
 *
 * \return The error details as a Connection::ErrorDetails object.
 * \sa status(), statusReason(), invalidationReason()
 */
const Connection::ErrorDetails &Connection::errorDetails() const
{
    if (isValid()) {
        warning() << "Connection::errorDetails() used on" << objectPath() << "which is valid";
    }

    return mPriv->errorDetails;
}

/**
 * Return the handle representing the user on this connection.
 *
 * Note that if the connection is not yet in the ConnectionStatusConnected state,
 * the value of this property may be zero.
 *
 * Change notification is via the selfHandleChanged() signal.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \return The user handle.
 * \sa selfHandleChanged(), selfContact()
 */
uint Connection::selfHandle() const
{
    return mPriv->selfHandle;
}

/**
 * Return a dictionary of presence statuses valid for use in this connection.
 *
 * The value may have changed arbitrarily during the time the
 * Connection spends in status ConnectionStatusConnecting,
 * again staying fixed for the entire time in ConnectionStatusConnected.
 *
 * This method requires Connection::FeatureSimplePresence to be ready.
 *
 * \return The allowed statuses as a map from string identifiers to SimpleStatusSpec objects.
 */
SimpleStatusSpecMap ConnectionLowlevel::allowedPresenceStatuses() const
{
    if (!isValid()) {
        warning() << "ConnectionLowlevel::selfHandle() "
            "called for a connection which is already destroyed";
        return SimpleStatusSpecMap();
    }

    ConnectionPtr conn(connection());
    if (!conn->isReady(Connection::FeatureSimplePresence)) {
        warning() << "Trying to retrieve allowed presence statuses from connection, but "
                     "simple presence is not supported or was not requested. "
                     "Enable FeatureSimplePresence in this connection";
    }

    return conn->mPriv->simplePresenceStatuses;
}

/**
 * Return the maximum length for a presence status message.
 *
 * The value may have changed arbitrarily during the time the
 * Connection spends in status ConnectionStatusConnecting,
 * again staying fixed for the entire time in ConnectionStatusConnected.
 *
 * This method requires Connection::FeatureSimplePresence to be ready.
 *
 * \return The maximum length, or 0 if there is no limit.
 */
uint ConnectionLowlevel::maxPresenceStatusMessageLength() const
{
    if (!isValid()) {
        warning() << "ConnectionLowlevel::maxPresenceStatusMessageLength() "
            "called for a connection which is already destroyed";
        return 0;
    }

    ConnectionPtr conn(connection());
    if (!conn->isReady(Connection::FeatureSimplePresence)) {
        warning() << "Trying to retrieve max presence status message length connection, but "
                     "simple presence is not supported or was not requested. "
                     "Enable FeatureSimplePresence in this connection";
    }

    return conn->mPriv->maxPresenceStatusMessageLength;
}

/**
 * Set the self presence status.
 *
 * This should generally only be called by an Account Manager. In typical usage,
 * Account::setRequestedPresence() should be used instead.
 *
 * \a status must be one of the allowed statuses returned by
 * allowedPresenceStatuses().
 *
 * Note that clients SHOULD set the status message for the local user to the
 * empty string, unless the user has actually provided a specific message (i.e.
 * one that conveys more information than the ConnectionStatus).
 *
 * \param status The desired status.
 * \param statusMessage The desired status message.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa allowedPresenceStatuses()
 */
PendingOperation *ConnectionLowlevel::setSelfPresence(const QString &status,
        const QString &statusMessage)
{
    if (!isValid()) {
        warning() << "ConnectionLowlevel::selfHandle() called for a connection which is already destroyed";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE, QLatin1String("Connection already destroyed"),
                ConnectionPtr());
    }

    ConnectionPtr conn(connection());
    if (!conn->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Connection does not support SimplePresence"), conn);
    }

    Client::ConnectionInterfaceSimplePresenceInterface *simplePresenceInterface =
        conn->interface<Client::ConnectionInterfaceSimplePresenceInterface>();
    return new PendingVoid(
            simplePresenceInterface->SetPresence(status, statusMessage), conn);
}

/**
 * Return the object representing the user on this connection.
 *
 * Note that if the connection is not yet in the ConnectionStatusConnected state, the value of this
 * property may be null.
 *
 * Change notification is via the selfContactChanged() signal.
 *
 * This method requires Connection::FeatureSelfContact to be ready.
 *
 * \return A pointer to the Contact object, or a null ContactPtr if unknown.
 * \sa selfContactChanged(), selfHandle()
 */
ContactPtr Connection::selfContact() const
{
    if (!isReady(FeatureSelfContact)) {
        warning() << "Connection::selfContact() used, but becomeReady(FeatureSelfContact) "
            "hasn't been completed!";
    }

    return mPriv->selfContact;
}

/**
 * Return the user's balance on the account corresponding to this connection.
 *
 * A negative amount may be possible on some services, and indicates that the user
 * owes money to the service provider.
 *
 * Change notification is via the accountBalanceChanged() signal.
 *
 * This method requires Connection::FeatureAccountBalance to be ready.
 *
 * \return The account balance as #CurrencyAmount.
 * \sa accountBalanceChanged()
 */
CurrencyAmount Connection::accountBalance() const
{
    if (!isReady(FeatureAccountBalance)) {
        warning() << "Connection::accountBalance() used before connection "
            "FeatureAccountBalance is ready";
    }

    return mPriv->accountBalance;
}

/**
 * Return the capabilities for this connection.
 *
 * User interfaces can use this information to show or hide UI components.
 *
 * This property cannot change after the connection has gone to state
 * ConnectionStatusConnected, so there is no change notification.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * @return The capabilities of this connection.
 */
ConnectionCapabilities Connection::capabilities() const
{
    if (!isReady(Connection::FeatureCore)) {
        warning() << "Connection::capabilities() used before connection "
            "FeatureCore is ready";
    }

    return mPriv->caps;
}

void Connection::onStatusReady(uint status)
{
    Q_ASSERT(status == mPriv->pendingStatus);

    if (mPriv->status == status) {
        return;
    }

    mPriv->status = status;
    mPriv->statusReason = mPriv->pendingStatusReason;

    if (isValid()) {
        emit statusChanged((ConnectionStatus) mPriv->status);
    } else {
        debug() << this << " not emitting statusChanged because it has been invalidated";
    }
}

void Connection::onStatusChanged(uint status, uint reason)
{
    debug() << "StatusChanged from" << mPriv->pendingStatus
            << "to" << status << "with reason" << reason;

    if (mPriv->pendingStatus == status) {
        warning() << "New status was the same as the old status! Ignoring"
            "redundant StatusChanged";
        return;
    }

    uint oldStatus = mPriv->pendingStatus;
    mPriv->pendingStatus = status;
    mPriv->pendingStatusReason = reason;

    switch (status) {
        case ConnectionStatusConnected:
            debug() << "Performing introspection for the Connected status";
            mPriv->setCurrentStatus(status);
            break;

        case ConnectionStatusConnecting:
            mPriv->setCurrentStatus(status);
            break;

        case ConnectionStatusDisconnected:
            {
                QString errorName = ConnectionHelper::statusReasonToErrorName(
                        (ConnectionStatusReason) reason,
                        (ConnectionStatus) oldStatus);

                // TODO should we signal statusChanged to Disconnected here or just
                //      invalidate?
                //      Also none of the pendingOperations will finish. The
                //      user should just consider them to fail as the connection
                //      is invalid
                onStatusReady(ConnectionStatusDisconnected);
                mPriv->invalidateResetCaps(errorName,
                        QString(QLatin1String("ConnectionStatusReason = %1")).arg(uint(reason)));
            }
            break;

        default:
            warning() << "Unknown connection status" << status;
            break;
    }
}

void Connection::onConnectionError(const QString &error,
        const QVariantMap &details)
{
    debug().nospace() << "Connection(" << objectPath() << ") got ConnectionError(" << error
        << ") with " << details.size() << " details";

    mPriv->errorDetails = details;
    mPriv->invalidateResetCaps(error,
            details.value(QLatin1String("debug-message")).toString());
}

void Connection::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        props = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Connection) failed with " <<
            reply.error().name() << ": " << reply.error().message();
        // let's try to fallback first before failing
    }

    uint status = static_cast<uint>(-1);
    if (props.contains(QLatin1String("Status"))
            && ((status = qdbus_cast<uint>(props[QLatin1String("Status")])) <=
                ConnectionStatusDisconnected)) {
        mPriv->forceCurrentStatus(status);
    } else {
        // only introspect status if we did not got it from StatusChanged
        if (mPriv->pendingStatus == (uint) -1) {
            mPriv->introspectMainQueue.enqueue(
                    &Private::introspectMainFallbackStatus);
        }
    }

    if (props.contains(QLatin1String("Interfaces"))) {
        mPriv->setInterfaces(qdbus_cast<QStringList>(
                    props[QLatin1String("Interfaces")]));
    } else {
        mPriv->introspectMainQueue.enqueue(
                &Private::introspectMainFallbackInterfaces);
    }

    if (props.contains(QLatin1String("SelfHandle"))) {
        mPriv->selfHandle = qdbus_cast<uint>(
                props[QLatin1String("SelfHandle")]);
    } else {
        mPriv->introspectMainQueue.enqueue(
                &Private::introspectMainFallbackSelfHandle);
    }

    if (props.contains(QLatin1String("HasImmortalHandles"))) {
        mPriv->immortalHandles = qdbus_cast<bool>(props[QLatin1String("HasImmortalHandles")]);
    }

    if (hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS)) {
        mPriv->introspectMainQueue.enqueue(
                &Private::introspectCapabilities);
    }

    if (hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS)) {
        mPriv->introspectMainQueue.enqueue(
                &Private::introspectContactAttributeInterfaces);
    }

    mPriv->continueMainIntrospection();

    watcher->deleteLater();
}

void Connection::gotStatus(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (!reply.isError()) {
        mPriv->forceCurrentStatus(reply.value());

        mPriv->continueMainIntrospection();
    } else {
        warning().nospace() << "GetStatus() failed with " <<
            reply.error().name() << ": " << reply.error().message();
        mPriv->invalidateResetCaps(reply.error().name(), reply.error().message());
    }

    watcher->deleteLater();
}

void Connection::gotInterfaces(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (!reply.isError()) {
        mPriv->setInterfaces(reply.value());
    }
    else {
        warning().nospace() << "GetInterfaces() failed with " <<
            reply.error().name() << ": " << reply.error().message() <<
            " - assuming no new interfaces";
        // let's not fail if GetInterfaces fail
    }

    mPriv->continueMainIntrospection();

    watcher->deleteLater();
}

void Connection::gotSelfHandle(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;

    if (!reply.isError()) {
        mPriv->selfHandle = reply.value();
        debug() << "Got self handle:" << mPriv->selfHandle;

        mPriv->continueMainIntrospection();
    } else {
        warning().nospace() << "GetSelfHandle() failed with " <<
            reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
    }

    watcher->deleteLater();
}

void Connection::gotCapabilities(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got capabilities";
        mPriv->caps.updateRequestableChannelClasses(
                qdbus_cast<RequestableChannelClassList>(reply.value().variant()));
    } else {
        warning().nospace() << "Getting capabilities failed with " <<
            reply.error().name() << ": " << reply.error().message();
        // let's not fail if retrieving capabilities fail
    }

    mPriv->continueMainIntrospection();

    watcher->deleteLater();
}

void Connection::gotContactAttributeInterfaces(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got contact attribute interfaces";
        mPriv->contactAttributeInterfaces = qdbus_cast<QStringList>(reply.value().variant());
    } else {
        warning().nospace() << "Getting contact attribute interfaces failed with " <<
            reply.error().name() << ": " << reply.error().message();
        // let's not fail if retrieving contact attribute interfaces fail
        // TODO should we remove Contacts interface from interfaces?
    }

    mPriv->continueMainIntrospection();

    watcher->deleteLater();
}

void Connection::gotSimpleStatuses(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();

        mPriv->simplePresenceStatuses = qdbus_cast<SimpleStatusSpecMap>(
                props[QLatin1String("Statuses")]);
        mPriv->maxPresenceStatusMessageLength = qdbus_cast<uint>(
                props[QLatin1String("MaximumStatusMessageLength")]);

        debug() << "Got" << mPriv->simplePresenceStatuses.size() <<
            "simple presence statuses - max status message length is" <<
            mPriv->maxPresenceStatusMessageLength;

        mPriv->readinessHelper->setIntrospectCompleted(FeatureSimplePresence, true);
    }
    else {
        warning().nospace() << "Getting simple presence statuses failed with " <<
            reply.error().name() << ":" << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureSimplePresence, false, reply.error());
    }

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

            if (!isReady(FeatureSelfContact)) {
                mPriv->readinessHelper->setIntrospectCompleted(FeatureSelfContact, true);
            }

            emit selfContactChanged();
        }
    } else {
        warning().nospace() << "Getting self contact failed with " <<
            pending->errorName() << ":" << pending->errorMessage();

        // check if the feature is already there, and for some reason introspectSelfContact
        // failed when called the second time
        if (!isReady(FeatureSelfContact)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureSelfContact, false,
                    op->errorName(), op->errorMessage());
        }

        if (mPriv->selfContact) {
            mPriv->selfContact.reset();
            emit selfContactChanged();
        }
    }

    mPriv->introspectingSelfContact = false;

    if (mPriv->reintrospectSelfContactRequired) {
        mPriv->introspectSelfContact(mPriv);
    }
}

void Connection::onIntrospectRosterFinished(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "Introspecting roster failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureRoster, false,
                op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Introspecting roster finished";
    mPriv->readinessHelper->setIntrospectCompleted(FeatureRoster, true);
}

void Connection::onIntrospectRosterGroupsFinished(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "Introspecting roster groups failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureRosterGroups, false,
                op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Introspecting roster groups finished";
    mPriv->readinessHelper->setIntrospectCompleted(FeatureRosterGroups, true);
}

void Connection::gotBalance(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got balance";
        mPriv->accountBalance = qdbus_cast<CurrencyAmount>(reply.value());
        mPriv->readinessHelper->setIntrospectCompleted(FeatureAccountBalance, true);
    } else {
        warning().nospace() << "Getting balance failed with " <<
            reply.error().name() << ":" << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureAccountBalance, false,
                reply.error().name(), reply.error().message());
    }

    watcher->deleteLater();
}

/**
 * Return the Client::ConnectionInterface interface proxy object for this connection.
 * This method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing Client::ConnectionInterface object for this
 *         Connection object.
 */
Client::ConnectionInterface *Connection::baseInterface() const
{
    return mPriv->baseInterface;
}

/**
 * Same as \c createChannel(request, -1)
 */
PendingChannel *ConnectionLowlevel::createChannel(const QVariantMap &request)
{
    return createChannel(request, -1);
}

/**
 * Asynchronously creates a channel satisfying the given request.
 *
 * In typical usage, only the Channel Dispatcher should call this. Ordinary
 * applications should use the Account::createChannel() family of methods
 * (which invoke the Channel Dispatcher's services).
 *
 * The request MUST contain the following keys:
 *   org.freedesktop.Telepathy.Channel.ChannelType
 *   org.freedesktop.Telepathy.Channel.TargetHandleType
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingChannel object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to get
 * notification of the request finishing processing. See the documentation
 * for that class for more info.
 *
 * \param request A dictionary containing the desirable properties.
 * \param timeout The D-Bus timeout in milliseconds used for the method call.
 *                If timeout is -1, a default implementation-defined value that
 *                is suitable for inter-process communications (generally,
 *                25 seconds) will be used.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         when the channel has been created, or an error occurred.
 * \sa PendingChannel, ensureChannel(),
 *     Account::createChannel(), Account::createAndHandleChannel(),
 *     Account::ensureChannel(), Account::ensureAndHandleChannel()
 */
PendingChannel *ConnectionLowlevel::createChannel(const QVariantMap &request,
        int timeout)
{
    if (!isValid()) {
        return new PendingChannel(ConnectionPtr(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"));
    }

    ConnectionPtr conn(connection());
    if (conn->mPriv->pendingStatus != ConnectionStatusConnected) {
        warning() << "Calling createChannel with connection not yet connected";
        return new PendingChannel(conn,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection not yet connected"));
    }

    if (!conn->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS)) {
        warning() << "Requests interface is not support by this connection";
        return new PendingChannel(conn,
                TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Connection does not support Requests Interface"));
    }

    if (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))) {
        return new PendingChannel(conn,
                TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Invalid 'request' argument"));
    }

    debug() << "Creating a Channel";
    PendingChannel *channel = new PendingChannel(conn, request, true, timeout);
    return channel;
}

/**
 * Same as \c ensureChannel(request, -1)
 */
PendingChannel *ConnectionLowlevel::ensureChannel(const QVariantMap &request)
{
    return ensureChannel(request, -1);
}

/**
 * Asynchronously ensures a channel exists satisfying the given request.
 *
 * In typical usage, only the Channel Dispatcher should call this. Ordinary
 * applications should use the Account::ensureChannel() family of methods
 * (which invoke the Channel Dispatcher's services).
 *
 * The request MUST contain the following keys:
 *   org.freedesktop.Telepathy.Channel.ChannelType
 *   org.freedesktop.Telepathy.Channel.TargetHandleType
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingChannel object. The object also provides access to the
 * parameters with which the call was made and a signal to connect to get
 * notification of the request finishing processing. See the documentation
 * for that class for more info.
 *
 * \param request A dictionary containing the desirable properties.
 * \param timeout The D-Bus timeout in milliseconds used for the method call.
 *                If timeout is -1, a default implementation-defined value that
 *                is suitable for inter-process communications (generally,
 *                25 seconds) will be used.
 * \return A PendingChannel which will emit PendingChannel::finished
 *         when the channel is ensured to exist, or an error occurred.
 * \sa PendingChannel, createChannel(),
 *     Account::createChannel(), Account::createAndHandleChannel(),
 *     Account::ensureChannel(), Account::ensureAndHandleChannel()
 */
PendingChannel *ConnectionLowlevel::ensureChannel(const QVariantMap &request,
        int timeout)
{
    if (!isValid()) {
        return new PendingChannel(ConnectionPtr(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"));
    }

    ConnectionPtr conn(connection());
    if (conn->mPriv->pendingStatus != ConnectionStatusConnected) {
        warning() << "Calling ensureChannel with connection not yet connected";
        return new PendingChannel(conn,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection not yet connected"));
    }

    if (!conn->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS)) {
        warning() << "Requests interface is not support by this connection";
        return new PendingChannel(conn,
                TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Connection does not support Requests Interface"));
    }

    if (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))) {
        return new PendingChannel(conn,
                TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Invalid 'request' argument"));
    }

    debug() << "Creating a Channel";
    PendingChannel *channel = new PendingChannel(conn, request, false, timeout);
    return channel;
}

/**
 * Request handles of the given type for the given entities (contacts,
 * rooms, lists, etc.).
 *
 * Typically one doesn't need to request and use handles directly; instead, string identifiers
 * and/or Contact objects are used in most APIs. File a bug for APIs in which there is no
 * alternative to using handles. In particular however using low-level DBus interfaces for which
 * there is no corresponding high-level (or one is implementing that abstraction) functionality does
 * and will always require using bare handles.
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
 * \param handleType Type for the handles to request, as specified in
 *                   #HandleType.
 * \param names Names of the entities to request handles for.
 * \return A PendingHandles which will emit PendingHandles::finished
 *         when the handles have been requested, or an error occurred.
 * \sa PendingHandles
 */
PendingHandles *ConnectionLowlevel::requestHandles(HandleType handleType, const QStringList &names)
{
    debug() << "Request for" << names.length() << "handles of type" << handleType;

    if (!isValid()) {
        return new PendingHandles(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"));
    }

    ConnectionPtr conn(connection());
    if (!hasImmortalHandles()) {
        Connection::Private::HandleContext *handleContext = conn->mPriv->handleContext;
        QMutexLocker locker(&handleContext->lock);
        handleContext->types[handleType].requestsInFlight++;
    }

    PendingHandles *pending =
        new PendingHandles(conn, handleType, names);
    return pending;
}

/**
 * Request a reference to the given handles. Handles not explicitly
 * requested (via requestHandles()) but eg. observed in a signal need to be
 * referenced to guarantee them staying valid.
 *
 * Typically one doesn't need to reference and use handles directly; instead, string identifiers
 * and/or Contact objects are used in most APIs. File a bug for APIs in which there is no
 * alternative to using handles. In particular however using low-level DBus interfaces for which
 * there is no corresponding high-level (or one is implementing that abstraction) functionality does
 * and will always require using bare handles.
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
 * \return A PendingHandles which will emit PendingHandles::finished
 *         when the handles have been referenced, or an error occurred.
 */
PendingHandles *ConnectionLowlevel::referenceHandles(HandleType handleType, const UIntList &handles)
{
    debug() << "Reference of" << handles.length() << "handles of type" << handleType;

    if (!isValid()) {
        return new PendingHandles(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"));
    }

    ConnectionPtr conn(connection());
    UIntList alreadyHeld;
    UIntList notYetHeld;
    if (!hasImmortalHandles()) {
        Connection::Private::HandleContext *handleContext = conn->mPriv->handleContext;
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

        debug() << " Already holding" << alreadyHeld.size() <<
            "of the handles -" << notYetHeld.size() << "to go";
    } else {
        alreadyHeld = handles;
    }

    PendingHandles *pending =
        new PendingHandles(conn, handleType, handles, alreadyHeld, notYetHeld);
    return pending;
}

/**
 * Start an asynchronous request that the connection be connected.
 *
 * When using a full-fledged Telepathy setup with an Account Manager service, the Account methods
 * Account::setRequestedPresence() and Account::reconnect() must be used instead.
 *
 * The returned PendingOperation will finish successfully when the connection
 * has reached ConnectionStatusConnected and the requested \a features are all ready, or
 * finish with an error if a fatal error occurs during that process.
 *
 * \param requestedFeatures The features which should be enabled
 * \return A PendingReady which will emit PendingReady::finished
 *         when the Connection has reached #ConnectionStatusConnected, and initial setup
 *         for basic functionality, plus the given features, has succeeded or
 *         failed.
 */
PendingReady *ConnectionLowlevel::requestConnect(const Features &requestedFeatures)
{
    if (!isValid()) {
        Connection::PendingConnect *pending = new Connection::PendingConnect(ConnectionPtr(),
                requestedFeatures);
        pending->setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"));
        return pending;
    }

    return new Connection::PendingConnect(connection(), requestedFeatures);
}

/**
 * Start an asynchronous request that the connection be disconnected.
 * The returned PendingOperation object will signal the success or failure
 * of this request; under normal circumstances, it can be expected to
 * succeed.
 *
 * When using a full-fledged Telepathy setup with an Account Manager service,
 * Account::setRequestedPresence() with Presence::offline() as an argument should generally be used
 * instead.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 */
PendingOperation *ConnectionLowlevel::requestDisconnect()
{
    if (!isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"), ConnectionPtr());
    }

    ConnectionPtr conn(connection());

    return new PendingVoid(conn->baseInterface()->Disconnect(), conn);
}

/**
 * Requests attributes for contacts. Optionally, the handles of the contacts
 * will be referenced automatically. Essentially, this method wraps
 * ConnectionInterfaceContactsInterface::GetContactAttributes(), integrating it
 * with the rest of the handle-referencing machinery.
 *
 * This is very low-level API the Contact/ContactManager API provides a higher level of abstraction
 * for the same functionality.
 *
 * Upon completion, the reply to the request can be retrieved through the
 * returned PendingContactAttributes object. The object also provides access to
 * the parameters with which the call was made and a signal to connect to to get
 * notification of the request finishing processing. See the documentation for
 * that class for more info.
 *
 * If the remote object doesn't support the Contacts interface (as signified by
 * the list returned by interfaces() not containing
 * #TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS), the returned
 * PendingContactAttributes instance will fail instantly with the error
 * #TP_QT_ERROR_NOT_IMPLEMENTED.
 *
 * Similarly, if the connection isn't both connected and ready
 * (<code>status() == ConnectionStatusConnected && isReady(Connection::FeatureCore)</code>),
 * the returned PendingContactAttributes instance will fail instantly with the
 * error #TP_QT_ERROR_NOT_AVAILABLE.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \sa PendingContactAttributes
 *
 * \param handles A list of handles of type HandleTypeContact
 * \param interfaces D-Bus interfaces for which the client requires information
 * \param reference Whether the handles should additionally be referenced.
 * \return A PendingContactAttributes which will emit PendingContactAttributes::fininshed
 *         when the contact attributes have been retrieved, or an error occurred.
 */
PendingContactAttributes *ConnectionLowlevel::contactAttributes(const UIntList &handles,
        const QStringList &interfaces, bool reference)
{
    debug() << "Request for attributes for" << handles.size() << "contacts";

    if (!isValid()) {
        PendingContactAttributes *pending = new PendingContactAttributes(ConnectionPtr(),
                handles, interfaces, reference);
        pending->failImmediately(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection has been destroyed"));
        return pending;
    }

    ConnectionPtr conn(connection());
    PendingContactAttributes *pending =
        new PendingContactAttributes(conn,
                handles, interfaces, reference);
    if (!conn->isReady(Connection::FeatureCore)) {
        warning() << "ConnectionLowlevel::contactAttributes() used when not ready";
        pending->failImmediately(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection isn't ready"));
        return pending;
    } else if (conn->mPriv->pendingStatus != ConnectionStatusConnected) {
        warning() << "ConnectionLowlevel::contactAttributes() used with status" << conn->status() << "!= ConnectionStatusConnected";
        pending->failImmediately(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("The connection isn't Connected"));
        return pending;
    } else if (!conn->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS)) {
        warning() << "ConnectionLowlevel::contactAttributes() used without the remote object supporting"
                  << "the Contacts interface";
        pending->failImmediately(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("The connection doesn't support the Contacts interface"));
        return pending;
    }

    if (!hasImmortalHandles()) {
        Connection::Private::HandleContext *handleContext = conn->mPriv->handleContext;
        QMutexLocker locker(&handleContext->lock);
        handleContext->types[HandleTypeContact].requestsInFlight++;
    }

    Client::ConnectionInterfaceContactsInterface *contactsInterface =
        conn->interface<Client::ConnectionInterfaceContactsInterface>();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(contactsInterface->GetContactAttributes(handles, interfaces,
                    reference));
    pending->connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                              SLOT(onCallFinished(QDBusPendingCallWatcher*)));
    return pending;
}

QStringList ConnectionLowlevel::contactAttributeInterfaces() const
{
    if (!isValid()) {
        warning() << "ConnectionLowlevel::contactAttributeInterfaces() called for a destroyed Connection";
        return QStringList();
    }

    ConnectionPtr conn(connection());
    if (conn->mPriv->pendingStatus != ConnectionStatusConnected) {
        warning() << "ConnectionLowlevel::contactAttributeInterfaces() used with status"
            << conn->status() << "!= ConnectionStatusConnected";
    } else if (!conn->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS)) {
        warning() << "ConnectionLowlevel::contactAttributeInterfaces() used without the remote object supporting"
                  << "the Contacts interface";
    }

    return conn->mPriv->contactAttributeInterfaces;
}

void ConnectionLowlevel::injectContactIds(const HandleIdentifierMap &contactIds)
{
    if (!hasImmortalHandles()) {
        return;
    }

    for (HandleIdentifierMap::const_iterator i = contactIds.constBegin();
            i != contactIds.constEnd(); ++i) {
        uint handle = i.key();
        QString id = i.value();

        if (!id.isEmpty()) {
            QString currentId = mPriv->contactsIds.value(handle);

            if (!currentId.isEmpty() && id != currentId) {
                warning() << "Trying to overwrite contact id from" << currentId << "to" << id
                    << "for the same handle" << handle << ", ignoring";
            } else {
                mPriv->contactsIds.insert(handle, id);
            }
        }
    }
}

void ConnectionLowlevel::injectContactId(uint handle, const QString &contactId)
{
    HandleIdentifierMap contactIds;
    contactIds.insert(handle, contactId);
    injectContactIds(contactIds);
}

bool ConnectionLowlevel::hasContactId(uint handle) const
{
    return mPriv->contactsIds.contains(handle);
}

QString ConnectionLowlevel::contactId(uint handle) const
{
    return mPriv->contactsIds.value(handle);
}

/**
 * Return whether the handles last for the whole lifetime of the connection.
 *
 * \return \c true if handles are immortal, \c false otherwise.
 */
bool ConnectionLowlevel::hasImmortalHandles() const
{
    return connection()->mPriv->immortalHandles;
}

/**
 * Return the ContactManager object for this connection.
 *
 * The contact manager is responsible for all contact handling in this
 * connection, including adding, removing, authorizing, etc.
 *
 * \return A pointer to the ContactManager object.
 */
ContactManagerPtr Connection::contactManager() const
{
    return mPriv->contactManager;
}

ConnectionLowlevelPtr Connection::lowlevel()
{
    return mPriv->lowlevel;
}

ConnectionLowlevelConstPtr Connection::lowlevel() const
{
    return mPriv->lowlevel;
}

void Connection::refHandle(HandleType handleType, uint handle)
{
    if (mPriv->immortalHandles) {
        return;
    }

    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    if (handleContext->types[handleType].toRelease.contains(handle)) {
        handleContext->types[handleType].toRelease.remove(handle);
    }

    handleContext->types[handleType].refcounts[handle]++;
}

void Connection::unrefHandle(HandleType handleType, uint handle)
{
    if (mPriv->immortalHandles) {
        return;
    }

    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    Q_ASSERT(handleContext->types.contains(handleType));
    Q_ASSERT(handleContext->types[handleType].refcounts.contains(handle));

    if (!--handleContext->types[handleType].refcounts[handle]) {
        handleContext->types[handleType].refcounts.remove(handle);
        handleContext->types[handleType].toRelease.insert(handle);

        if (!handleContext->types[handleType].releaseScheduled) {
            if (!handleContext->types[handleType].requestsInFlight) {
                debug() << "Lost last reference to at least one handle of type" <<
                    handleType <<
                    "and no requests in flight for that type - scheduling a release sweep";
                QMetaObject::invokeMethod(this, "doReleaseSweep",
                        Qt::QueuedConnection, Q_ARG(uint, handleType));
                handleContext->types[handleType].releaseScheduled = true;
            }
        }
    }
}

void Connection::doReleaseSweep(uint handleType)
{
    if (mPriv->immortalHandles) {
        return;
    }

    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    Q_ASSERT(handleContext->types.contains(handleType));
    Q_ASSERT(handleContext->types[handleType].releaseScheduled);

    debug() << "Entering handle release sweep for type" << handleType;
    handleContext->types[handleType].releaseScheduled = false;

    if (handleContext->types[handleType].requestsInFlight > 0) {
        debug() << " There are requests in flight, deferring sweep to when they have been completed";
        return;
    }

    if (handleContext->types[handleType].toRelease.isEmpty()) {
        debug() << " No handles to release - every one has been resurrected";
        return;
    }

    debug() << " Releasing" << handleContext->types[handleType].toRelease.size() << "handles";

    mPriv->baseInterface->ReleaseHandles(handleType, handleContext->types[handleType].toRelease.toList());
    handleContext->types[handleType].toRelease.clear();
}

void Connection::handleRequestLanded(HandleType handleType)
{
    if (mPriv->immortalHandles) {
        return;
    }

    Private::HandleContext *handleContext = mPriv->handleContext;
    QMutexLocker locker(&handleContext->lock);

    Q_ASSERT(handleContext->types.contains(handleType));
    Q_ASSERT(handleContext->types[handleType].requestsInFlight > 0);

    if (!--handleContext->types[handleType].requestsInFlight &&
        !handleContext->types[handleType].toRelease.isEmpty() &&
        !handleContext->types[handleType].releaseScheduled) {
        debug() << "All handle requests for type" << handleType <<
            "landed and there are handles of that type to release - scheduling a release sweep";
        QMetaObject::invokeMethod(this, "doReleaseSweep", Qt::QueuedConnection, Q_ARG(uint, handleType));
        handleContext->types[handleType].releaseScheduled = true;
    }
}

void Connection::onSelfHandleChanged(uint handle)
{
    if (mPriv->selfHandle == handle) {
        return;
    }

    if (mPriv->pendingStatus != ConnectionStatusConnected || !mPriv->selfHandle) {
        debug() << "Got a self handle change before we have the initial self handle, ignoring";
        return;
    }

    debug() << "Connection self handle changed to" << handle;
    mPriv->selfHandle = handle;
    emit selfHandleChanged(handle);

    if (mPriv->introspectingSelfContact) {
        // We're currently introspecting the SelfContact feature, but have started building the
        // contact with the old handle, so we need to do it again with the new handle.

        debug() << "The self contact is being built, will rebuild with the new handle shortly";
        mPriv->reintrospectSelfContactRequired = true;
    } else if (isReady(FeatureSelfContact)) {
        // We've already introspected the SelfContact feature, so we can reinvoke the introspection
        // logic directly to rebuild with the new handle.

        debug() << "Re-building self contact for handle" << handle;
        Private::introspectSelfContact(mPriv);
    }

    // If ReadinessHelper hasn't started introspecting SelfContact yet for the Connected state, we
    // don't need to do anything. When it does start the introspection, it will do so using the
    // correct handle.
}

void Connection::onBalanceChanged(const Tp::CurrencyAmount &value)
{
    mPriv->accountBalance = value;
    emit accountBalanceChanged(value);
}

QString ConnectionHelper::statusReasonToErrorName(Tp::ConnectionStatusReason reason,
        Tp::ConnectionStatus oldStatus)
{
    QString errorName;

    switch (reason) {
        case ConnectionStatusReasonNoneSpecified:
            errorName = TP_QT_ERROR_DISCONNECTED;
            break;

        case ConnectionStatusReasonRequested:
            errorName = TP_QT_ERROR_CANCELLED;
            break;

        case ConnectionStatusReasonNetworkError:
            errorName = TP_QT_ERROR_NETWORK_ERROR;
            break;

        case ConnectionStatusReasonAuthenticationFailed:
            errorName = TP_QT_ERROR_AUTHENTICATION_FAILED;
            break;

        case ConnectionStatusReasonEncryptionError:
            errorName = TP_QT_ERROR_ENCRYPTION_ERROR;
            break;

        case ConnectionStatusReasonNameInUse:
            if (oldStatus == ConnectionStatusConnected) {
                errorName = TP_QT_ERROR_CONNECTION_REPLACED;
            } else {
                errorName = TP_QT_ERROR_ALREADY_CONNECTED;
            }
            break;

        case ConnectionStatusReasonCertNotProvided:
            errorName = TP_QT_ERROR_CERT_NOT_PROVIDED;
            break;

        case ConnectionStatusReasonCertUntrusted:
            errorName = TP_QT_ERROR_CERT_UNTRUSTED;
            break;

        case ConnectionStatusReasonCertExpired:
            errorName = TP_QT_ERROR_CERT_EXPIRED;
            break;

        case ConnectionStatusReasonCertNotActivated:
            errorName = TP_QT_ERROR_CERT_NOT_ACTIVATED;
            break;

        case ConnectionStatusReasonCertHostnameMismatch:
            errorName = TP_QT_ERROR_CERT_HOSTNAME_MISMATCH;
            break;

        case ConnectionStatusReasonCertFingerprintMismatch:
            errorName = TP_QT_ERROR_CERT_FINGERPRINT_MISMATCH;
            break;

        case ConnectionStatusReasonCertSelfSigned:
            errorName = TP_QT_ERROR_CERT_SELF_SIGNED;
            break;

        case ConnectionStatusReasonCertOtherError:
            errorName = TP_QT_ERROR_CERT_INVALID;
            break;

        default:
            errorName = TP_QT_ERROR_DISCONNECTED;
            break;
    }

    return errorName;
}

/**
 * \fn void Connection::statusChanged(Tp::ConnectionStatus newStatus)
 *
 * Indicates that the connection's status has changed and that all previously requested features are
 * now ready to use for the new status.
 *
 * Legitimate uses for this signal, apart from waiting for a given connection status to be ready,
 * include updating an animation based on the connection being in ConnectionStatusConnecting,
 * ConnectionStatusConnected and ConnectionStatusDisconnected, and otherwise showing progress
 * indication to the user. It should, however, NEVER be used for error handling:
 *
 * This signal doesn't contain the status reason as an argument, because statusChanged() shouldn't
 * be used for error-handling. There are numerous cases in which a Connection may become unusable
 * without there being a status change to ConnectionStatusDisconnected. All of these cases, and
 * being disconnected itself, are signaled by invalidated() with appropriate error names. On the
 * other hand, the reason for the status going to ConnectionStatusConnecting or
 * ConnectionStatusConnected will always be ConnectionStatusReasonRequested, so signaling that would
 * be useless.
 *
 * The status reason, as returned by statusReason(), may however be used as a fallback for error
 * handling in slots connected to the invalidated() signal, if the client doesn't understand a
 * particular (likely domain-specific if so) error name given by invalidateReason().
 *
 * \param newStatus The new status of the connection, as would be returned by status().
 */

/**
 * \fn void Connection::selfHandleChanged(uint newHandle)
 *
 * Emitted when the value of selfHandle() changes.
 *
 * \param newHandle The new connection self handle.
 * \sa selfHandle()
 */

/**
 * \fn void Connection::selfContactChanged()
 *
 * Emitted when the value of selfContact() changes.
 *
 * \sa selfContact()
 */

/**
 * \fn void Connection::accountBalanceChanged(const Tp::CurrencyAmount &accountBalance)
 *
 * Emitted when the value of accountBalance() changes.
 *
 * \param accountBalance The new user's balance of this connection.
 * \sa accountBalance()
 */

} // Tp
