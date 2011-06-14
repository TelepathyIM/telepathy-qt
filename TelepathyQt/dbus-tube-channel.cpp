/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/DBusTubeChannel>

#include "TelepathyQt/_gen/dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/outgoing-stream-tube-channel-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusTubeChannel::Private
{
    Private(DBusTubeChannel *parent);
    virtual ~Private();

    void extractProperties(const QVariantMap &props);
    void extractParticipants(const Tp::DBusTubeParticipants &participants);

    static void introspectDBusTube(Private *self);
    static void introspectBusNamesMonitoring(Private *self);

    ReadinessHelper *readinessHelper;

    // Public object
    DBusTubeChannel *parent;

    // Properties
    UIntList accessControls;
    QString serviceName;
    QHash<ContactPtr, QString> busNames;
    QString address;

    QHash<QUuid, QString> pendingNewBusNamesToAdd;
    QList<QUuid> pendingNewBusNamesToRemove;

    QueuedContactFactory *queuedContactFactory;
};

DBusTubeChannel::Private::Private(DBusTubeChannel *parent)
        : parent(parent),
          queuedContactFactory(new QueuedContactFactory(parent->connection()->contactManager(), parent))
{
    // Initialize readinessHelper + introspectables here
    readinessHelper = parent->readinessHelper();

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableDBusTube(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << TubeChannel::FeatureCore,                                 // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectDBusTube,
        this);
    introspectables[DBusTubeChannel::FeatureDBusTube] = introspectableDBusTube;

    ReadinessHelper::Introspectable introspectableBusNamesMonitoring(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << DBusTubeChannel::FeatureDBusTube,                         // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectBusNamesMonitoring,
        this);
    introspectables[DBusTubeChannel::FeatureBusNameMonitoring] = introspectableBusNamesMonitoring;

    readinessHelper->addIntrospectables(introspectables);
}

DBusTubeChannel::Private::~Private()
{
}

void DBusTubeChannel::Private::extractProperties(const QVariantMap &props)
{
    serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
    accessControls = qdbus_cast<UIntList>(props[QLatin1String("SupportedAccessControls")]);
    extractParticipants(qdbus_cast<DBusTubeParticipants>(props[QLatin1String("DBusNames")]));
}

void DBusTubeChannel::Private::extractParticipants(const Tp::DBusTubeParticipants &participants)
{
    busNames.clear();
    for (DBusTubeParticipants::const_iterator i = participants.constBegin();
         i != participants.constEnd();
         ++i) {
        QUuid uuid = queuedContactFactory->appendNewRequest(UIntList() << i.key());
        pendingNewBusNamesToAdd.insert(uuid, i.value());
    }
}

void DBusTubeChannel::Private::introspectBusNamesMonitoring(DBusTubeChannel::Private *self)
{
    DBusTubeChannel *parent = self->parent;

    Client::ChannelTypeDBusTubeInterface *dbusTubeInterface =
            parent->interface<Client::ChannelTypeDBusTubeInterface>();

    // It must be present
    Q_ASSERT(dbusTubeInterface);

    // It makes sense only if this is a room, if that's not the case just spit a warning
    if (parent->targetHandleType() == static_cast<uint>(Tp::HandleTypeRoom)) {
        parent->connect(dbusTubeInterface, SIGNAL(DBusNamesChanged(Tp::DBusTubeParticipants,Tp::UIntList)),
                        parent, SLOT(onDBusNamesChanged(Tp::DBusTubeParticipants,Tp::UIntList)));
    } else {
        debug() << "FeatureBusNameMonitoring does not make sense in a P2P context";
    }

    self->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureBusNameMonitoring, true);
}

void DBusTubeChannel::Private::introspectDBusTube(DBusTubeChannel::Private *self)
{
    DBusTubeChannel *parent = self->parent;

    debug() << "Introspect dbus tube properties";

    Client::DBus::PropertiesInterface *properties =
            parent->interface<Client::DBus::PropertiesInterface>();

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(
                    QLatin1String(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE)),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            parent,
            SLOT(gotDBusTubeProperties(QDBusPendingCallWatcher*)));
}

/**
 * \class DBusTubeChannel
 * \headerfile TelepathyQt/stream-tube-channel.h <TelepathyQt/DBusTubeChannel>
 *
 * A class representing a DBus Tube
 *
 * \c DBusTubeChannel is an high level wrapper for managing Telepathy interface
 * #TELEPATHY_INTERFACE_CHANNEL_TYPE_DBUS_TUBE.
 * It provides a private DBus connection, either peer-to-peer or between a group of contacts, which
 * can be used just like a standard DBus connection. As such, services exposed MUST adhere to the
 * DBus specification.
 *
 * This class provides high level methods for managing both incoming and outgoing tubes - however,
 * you probably want to use one of its subclasses, #OutgoingDBusTubeChannel or
 * #IncomingDBusTubeChannel, which both provide higher level methods for accepting
 * or offering tubes.
 *
 * For more details, please refer to Telepathy spec.
 */

// Features declaration and documentation
/**
 * Feature representing the core that needs to become ready to make the
 * DBusTubeChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * DBusTubeChannel methods.
 * See specific methods documentation for more details.
 */
const Feature DBusTubeChannel::FeatureDBusTube = Feature(QLatin1String(DBusTubeChannel::staticMetaObject.className()), 0);
/**
 * Feature used in order to monitor connections to this tube.
 * Please note that this feature makes sense only in Group tubes.
 *
 * %busNamesChanged will be emitted when the participants of this tube change
 */
const Feature DBusTubeChannel::FeatureBusNameMonitoring = Feature(QLatin1String(DBusTubeChannel::staticMetaObject.className()), 1);

/**
 * Create a new DBusTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A DBusTubeChannelPtr object pointing to the newly created
 *         DBusTubeChannel object.
 */
DBusTubeChannelPtr DBusTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return DBusTubeChannelPtr(new DBusTubeChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new DBusTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
DBusTubeChannel::DBusTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : TubeChannel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
    connect(mPriv->queuedContactFactory,
            SIGNAL(contactsRetrieved(QUuid,QList<Tp::ContactPtr>)),
            this,
            SLOT(onContactsRetrieved(QUuid,QList<Tp::ContactPtr>)));
}

/**
 * Class destructor.
 */
DBusTubeChannel::~DBusTubeChannel()
{
    delete mPriv;
}

/**
 * Returns the service name which will be used over the tube.
 *
 * This method requires DBusTubeChannel::FeatureDBusTube to be enabled.
 *
 * \return the service name that will be used over the tube
 */
QString DBusTubeChannel::serviceName() const
{
    if (!isReady(FeatureDBusTube)) {
        warning() << "DBusTubeChannel::service() used with "
            "FeatureDBusTube not ready";
        return QString();
    }

    return mPriv->serviceName;
}

/**
 * Checks if this tube is capable to accept or offer a private bus which
 * will require credentials upon connection.
 *
 * When this capability is available and enabled, the connecting process must send a byte when
 * it first connects, which is not considered to be part of the data stream.
 * If the operating system uses sendmsg() with SCM_CREDS or SCM_CREDENTIALS to pass
 * credentials over sockets, the connecting process must do so if possible;
 * if not, it must still send the byte.
 *
 * The listening process will disconnect the connection unless it can determine
 * by OS-specific means that the connecting process has the same user ID as the listening process.
 *
 * This method requires DBusTubeChannel::FeatureDBusTube to be enabled.
 *
 * \note It is strongly advised to call this method before attempting to call
 *       #IncomingDBusTubeChannel::acceptTube or
 *       #OutgoingDBusTubeChannel::offerTube requiring
 *       credentials to prevent failures, as the spec implies
 *       this feature is not compulsory for connection managers.
 *
 * \return Whether this DBus tube is capable to accept or offer a private bus
 *         requiring credentials for connecting to it.
 *
 * \sa IncomingDBusTubeChannel::acceptTube
 * \sa OutgoingDBusTubeChannel::offerTube
 */
bool DBusTubeChannel::supportsCredentials() const
{
    if (!isReady(FeatureDBusTube)) {
        warning() << "DBusTubeChannel::supportsCredentials() used with "
            "FeatureDBusTube not ready";
        return false;
    }

    return mPriv->accessControls.contains(static_cast<uint>(Tp::SocketAccessControlCredentials));
}

/**
 * If the tube has been opened, this function returns the private bus address you should be connecting
 * to for using this tube.
 *
 * Please note this function will return a meaningful value only if the tube has already
 * been opened successfully: in case of failure or the tube being still pending, an empty QString will be
 * returned.
 *
 * \returns The address of the private bus opened by this tube
 */
QString DBusTubeChannel::address() const
{
    if (state() != TubeChannelStateOpen) {
        warning() << "DBusTubeChannel::address() can be called only if "
            "the tube has already been opened";
        return QString();
    }

    return mPriv->address;
}

/**
 * This function returns all the known active connections since FeatureBusNameMonitoring has
 * been enabled. For this method to return all known connections, you need to make
 * FeatureBusNameMonitoring ready before accepting or offering the tube.
 *
 * This function will always return an empty hash in case the tube is p2p, even if
 * FeatureBusNameMonitoring has been activated.
 *
 * \returns A list of active connection ids known to this tube
 */
QHash<ContactPtr, QString> DBusTubeChannel::busNames() const
{
    if (!isReady(FeatureBusNameMonitoring)) {
        warning() << "DBusTubeChannel::busNames() used with "
            "FeatureBusNameMonitoring not ready";
        return QHash<ContactPtr, QString>();
    }

    return mPriv->busNames;
}

void DBusTubeChannel::gotDBusTubeProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        debug() << "Got reply to Properties::GetAll(DBusTubeChannel)";
        mPriv->extractProperties(props);
        mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureDBusTube, true);
    } else {
        warning().nospace() << "Properties::GetAll(DBusTubeChannel) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureDBusTube, false,
                reply.error());
    }
}

void DBusTubeChannel::onDBusNamesChanged(const Tp::DBusTubeParticipants &added,
        const Tp::UIntList &removed)
{
    for (DBusTubeParticipants::const_iterator i = added.constBegin();
         i != added.constEnd();
         ++i) {
        QUuid uuid = mPriv->queuedContactFactory->appendNewRequest(UIntList() << i.key());
        // Add it to our hash as well
        mPriv->pendingNewBusNamesToAdd.insert(uuid, i.value());
    }

    foreach (uint handle, removed) {
        QUuid uuid = mPriv->queuedContactFactory->appendNewRequest(UIntList() << handle);
        // Add it to pending removed as well
        mPriv->pendingNewBusNamesToRemove << uuid;
    }
}

void DBusTubeChannel::onContactsRetrieved(const QUuid &uuid, const QList<ContactPtr> &contacts)
{
    // Retrieve our hash
    if (mPriv->pendingNewBusNamesToAdd.contains(uuid)) {
        QString busName = mPriv->pendingNewBusNamesToAdd.take(uuid);
        QHash<ContactPtr, QString> added;

        // Add it to our connections hash
        foreach (const Tp::ContactPtr &contact, contacts) {
            mPriv->busNames.insert(contact, busName);
            added.insert(contact, busName);
        }

        // Time for us to emit the signal
        emit busNamesChanged(added, QList<ContactPtr>());
    } else if (mPriv->pendingNewBusNamesToRemove.contains(uuid)) {
        mPriv->pendingNewBusNamesToRemove.removeOne(uuid);
        QList<ContactPtr> removed;

        // Remove it from our connections hash
        foreach (const Tp::ContactPtr &contact, contacts) {
            if (mPriv->busNames.contains(contact)) {
                mPriv->busNames.remove(contact);
                removed << contact;
            } else {
                warning() << "Trying to remove a bus name which has not been retrieved previously!";
            }
        }

        // Time for us to emit the signal
        emit busNamesChanged(QHash<ContactPtr, QString>(), removed);
    } else {
        warning() << "Contacts retrieved but no pending bus names were found";
        return;
    }
}

void DBusTubeChannel::setAddress(const QString& address)
{
    mPriv->address = address;
}

// Signals documentation
/**
 * \fn void DBusTubeChannel::busNamesChanged(const QHash< ContactPtr, QString > &added, const QList< ContactPtr > &removed)
 *
 * Emitted when the participants of this tube change.
 *
 * This signal will be emitted only if the tube is a group tube (not p2p), and if the
 * FeatureBusNameMonitoring feature has been enabled.
 *
 * \param added An hash containing the contacts who joined this tube, with their respective bus name.
 * \param removed A list containing the contacts who left this tube.
 */

}
