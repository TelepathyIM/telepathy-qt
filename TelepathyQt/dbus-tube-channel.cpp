/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/DBusTubeChannel>

#include "TelepathyQt/_gen/dbus-tube-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/outgoing-stream-tube-channel-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusTubeChannel::Private
{
    Private(DBusTubeChannel *parent);

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
    QHash<QString, Tp::ContactPtr> contactsForBusNames;
    QString address;

    QHash<QUuid, QString> pendingNewBusNamesToAdd;
    QList<QUuid> pendingNewBusNamesToRemove;

    QueuedContactFactory *queuedContactFactory;
};

DBusTubeChannel::Private::Private(DBusTubeChannel *parent)
        : parent(parent),
          queuedContactFactory(new QueuedContactFactory(parent->connection()->contactManager(), parent))
{
    parent->connect(queuedContactFactory,
            SIGNAL(contactsRetrieved(QUuid,QList<Tp::ContactPtr>)),
            SLOT(onContactsRetrieved(QUuid,QList<Tp::ContactPtr>)));

    // Initialize readinessHelper + introspectables here
    readinessHelper = parent->readinessHelper();

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableDBusTube(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << TubeChannel::FeatureCore,                                 // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectDBusTube,
        this);
    introspectables[DBusTubeChannel::FeatureCore] = introspectableDBusTube;

    ReadinessHelper::Introspectable introspectableBusNamesMonitoring(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << DBusTubeChannel::FeatureCore,                         // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectBusNamesMonitoring,
        this);
    introspectables[DBusTubeChannel::FeatureBusNameMonitoring] = introspectableBusNamesMonitoring;

    readinessHelper->addIntrospectables(introspectables);
}

void DBusTubeChannel::Private::extractProperties(const QVariantMap &props)
{
    serviceName = qdbus_cast<QString>(props[TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName")]);
    accessControls = qdbus_cast<UIntList>(props[TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".SupportedAccessControls")]);
}

void DBusTubeChannel::Private::extractParticipants(const Tp::DBusTubeParticipants &participants)
{
    contactsForBusNames.clear();
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

        // Request the current DBusNames property
        connect(dbusTubeInterface->requestPropertyDBusNames(), SIGNAL(finished(Tp::PendingOperation*)),
                parent, SLOT(onRequestPropertyDBusNamesFinished(Tp::PendingOperation*)));
    } else {
        debug() << "FeatureBusNameMonitoring does not make sense in a P2P context";
        self->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureBusNameMonitoring, false);
    }
}

void DBusTubeChannel::Private::introspectDBusTube(DBusTubeChannel::Private *self)
{
    DBusTubeChannel *parent = self->parent;

    debug() << "Introspect dbus tube properties";

    if (parent->immutableProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".ServiceName")) &&
        parent->immutableProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1String(".SupportedAccessControls"))) {
        self->extractProperties(parent->immutableProperties());
        self->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureCore, true);
    } else {
        Client::ChannelTypeDBusTubeInterface *dbusTubeInterface =
                parent->interface<Client::ChannelTypeDBusTubeInterface>();

        parent->connect(dbusTubeInterface->requestAllProperties(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        parent,
                        SLOT(onRequestAllPropertiesFinished(Tp::PendingOperation*)));
    }
}

/**
 * \class DBusTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/dbus-tube-channel.h <TelepathyQt/DBusTubeChannel>
 *
 * \brief The DBusTubeChannel class represents a Telepathy channel of type DBusTube.
 *
 * It provides a private bus which can be used as a peer-to-peer connection in case of a
 * Contact Channel, or as a full-fledged bus in case of a Room Channel.
 *
 * DBusTubeChannel is an intermediate base class; OutgoingDBusTubeChannel and
 * IncomingDBusTubeChannel are the specialized classes used for locally and remotely initiated
 * tubes respectively.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
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
const Feature DBusTubeChannel::FeatureCore = Feature(QLatin1String(DBusTubeChannel::staticMetaObject.className()), 0);
/**
 * Feature used in order to monitor bus names in this DBus tube.
 *
 * See bus name monitoring specific methods' documentation for more details.
 *
 * \sa busNameAdded(), busNameRemoved()
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
}

/**
 * Class destructor.
 */
DBusTubeChannel::~DBusTubeChannel()
{
    delete mPriv;
}

/**
 * Returns the service name which will be used over the tube. This should be a
 * well-known and valid DBus service name, in the form "org.my.service".
 *
 * This method requires DBusTubeChannel::FeatureCore to be enabled.
 *
 * \return the service name that will be used over the tube
 */
QString DBusTubeChannel::serviceName() const
{
    if (!isReady(FeatureCore)) {
        warning() << "DBusTubeChannel::serviceName() used with "
            "FeatureCore not ready";
        return QString();
    }

    return mPriv->serviceName;
}

/**
 * Checks if this tube is capable to accept or offer a private bus which
 * will allow connections only from the current user
 *
 * This method is useful only if your appliance is really security-sensitive:
 * in general, this restriction is always enabled by default on all tubes offered
 * or accepted from Telepathy-Qt, falling back to a general connection allowance
 * if this feature is not available.
 *
 * If your application does not have specific needs regarding DBus credentials,
 * you can trust Telepathy-Qt to do the right thing - in any case, the most secure
 * method available will be used by default.
 *
 * This method requires DBusTubeChannel::FeatureCore to be enabled.
 *
 * \return Whether this DBus tube is capable to accept or offer a private bus
 *         restricting access to it to the current user only.
 *
 * \sa IncomingDBusTubeChannel::acceptTube
 * \sa OutgoingDBusTubeChannel::offerTube
 */
bool DBusTubeChannel::supportsRestrictingToCurrentUser() const
{
    if (!isReady(FeatureCore)) {
        warning() << "DBusTubeChannel::supportsRestrictingToCurrentUser() used with "
            "FeatureCore not ready";
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
 * \note If you plan to use QtDBus for the DBus connection, please note you should always use
 *       QDBusConnection::connectToPeer(), regardless of the fact this tube is a p2p or a group one.
 *       The above function has been introduced in Qt 4.8, previous versions of Qt do not allow the use
 *       of DBus Tubes through QtDBus.
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
 * This function returns all the known active bus names in this tube. It requires
 * FeatureBusNameMonitoring to be activated; however, even a late activation of the
 * feature will make this function return a full list of all the connected bus
 * names, including the ones which appeared before the activation of the feature
 * itself.
 *
 * This function will always return an empty hash in case the tube is p2p, even if
 * FeatureBusNameMonitoring has been activated.
 *
 * This method requires FeatureBusNameMonitoring to be enabled.
 *
 * \returns A list of active connection ids known to this tube
 */
QHash<QString, Tp::ContactPtr> DBusTubeChannel::contactsForBusNames() const
{
    if (!isReady(FeatureBusNameMonitoring)) {
        warning() << "DBusTubeChannel::contactsForBusNames() used with "
            "FeatureBusNameMonitoring not ready";
        return QHash<QString, Tp::ContactPtr>();
    }

    return mPriv->contactsForBusNames;
}

void DBusTubeChannel::onRequestAllPropertiesFinished(PendingOperation *op)
{
    if (!op->isError()) {
        debug() << "RequestAllProperties succeeded";
        PendingVariantMap *result = qobject_cast<PendingVariantMap*>(op);

        QVariantMap map = result->result();
        QVariantMap qualifiedMap;

        for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
            qualifiedMap.insert(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE + QLatin1Char('.') + i.key(),
                                i.value());
        }

        mPriv->extractProperties(qualifiedMap);
        mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureCore, true);
    } else {
        warning().nospace() << "RequestAllProperties failed "
            "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureCore, false);
    }
}

void DBusTubeChannel::onRequestPropertyDBusNamesFinished(PendingOperation *op)
{
    if (!op->isError()) {
        debug() << "RequestPropertyDBusNames succeeded";
        PendingVariant *result = qobject_cast<PendingVariant*>(op);
        DBusTubeParticipants participants = qdbus_cast<DBusTubeParticipants>(result->result());

        if (participants.isEmpty()) {
            // Nothing to do actually, simply mark the feature as ready.
            mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureBusNameMonitoring, true);
        } else {
            // Wait for the queue to complete
            connect(mPriv->queuedContactFactory, SIGNAL(queueCompleted()),
                    this, SLOT(onQueueCompleted()));

            // Extract the participants, populating the QueuedContactFactory
            mPriv->extractParticipants(participants);
        }
    } else {
        warning().nospace() << "RequestPropertyDBusNames failed "
            "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureBusNameMonitoring, false);
    }
}

void DBusTubeChannel::onQueueCompleted()
{
    debug() << "Queue was completed";

    // Set the feature as completed, and disconnect the signal as it's no longer useful
    mPriv->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureBusNameMonitoring, true);

    disconnect(mPriv->queuedContactFactory, SIGNAL(queueCompleted()),
               this, SLOT(onQueueCompleted()));
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

        // Add it to our connections hash
        foreach (const Tp::ContactPtr &contact, contacts) {
            mPriv->contactsForBusNames.insert(busName, contact);

            // Time for us to emit the signal - if the feature is ready
            if (isReady(FeatureBusNameMonitoring)) {
                emit busNameAdded(busName, contact);
            }
        }
    } else if (mPriv->pendingNewBusNamesToRemove.contains(uuid)) {
        mPriv->pendingNewBusNamesToRemove.removeOne(uuid);

        // Remove it from our connections hash
        foreach (const Tp::ContactPtr &contact, contacts) {
            if (mPriv->contactsForBusNames.values().contains(contact)) {
                QString busName = mPriv->contactsForBusNames.key(contact);
                mPriv->contactsForBusNames.remove(busName);

                // Time for us to emit the signal - if the feature is ready
                if (isReady(FeatureBusNameMonitoring)) {
                    emit busNameRemoved(busName, contact);
                }
            } else {
                warning() << "Trying to remove a bus name for contact " << contact->id()
                          << " which has not been retrieved previously!";
            }
        }
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
 * \fn void DBusTubeChannel::busNameAdded(const QString &busName, const Tp::ContactPtr &contact)
 *
 * Emitted when a new participant joins this tube.
 *
 * This signal will be emitted only if the tube is a group tube (not p2p), and if the
 * FeatureBusNameMonitoring feature has been enabled.
 *
 * \param busName The bus name of the new participant
 * \param contact The ContactPtr identifying the participant
 */

/**
 * \fn void DBusTubeChannel::busNameRemoved(const QString &busName, const Tp::ContactPtr &contact)
 *
 * Emitted when a participant leaves this tube.
 *
 * This signal will be emitted only if the tube is a group tube (not p2p), and if the
 * FeatureBusNameMonitoring feature has been enabled.
 *
 * \param busName The bus name of the participant leaving
 * \param contact The ContactPtr identifying the participant
 */

}
