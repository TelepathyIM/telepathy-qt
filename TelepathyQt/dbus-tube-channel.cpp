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

#include "TelepathyQt/dbus-tube-channel-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>

#include "TelepathyQt/debug-internal.h"

namespace Tp
{

DBusTubeChannelPrivate::DBusTubeChannelPrivate(DBusTubeChannel *parent)
    : q_ptr(parent)
{
}

DBusTubeChannelPrivate::~DBusTubeChannelPrivate()
{
}

void DBusTubeChannelPrivate::init()
{
    Q_Q(DBusTubeChannel);
    // Initialize readinessHelper + introspectables here
    readinessHelper = q->readinessHelper();

    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableDBusTube(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << TubeChannel::FeatureCore,                                 // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &DBusTubeChannelPrivate::introspectDBusTube,
        this);
    introspectables[DBusTubeChannel::FeatureDBusTube] = introspectableDBusTube;

    ReadinessHelper::Introspectable introspectableBusNamesMonitoring(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << DBusTubeChannel::FeatureDBusTube,                         // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &DBusTubeChannelPrivate::introspectBusNamesMonitoring,
        this);
    introspectables[DBusTubeChannel::FeatureBusNamesMonitoring] = introspectableBusNamesMonitoring;

    readinessHelper->addIntrospectables(introspectables);
}

void DBusTubeChannelPrivate::extractProperties(const QVariantMap& props)
{
    serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
    accessControls = qdbus_cast<UIntList>(props[QLatin1String("SupportedAccessControls")]);
    extractParticipants(qdbus_cast<DBusTubeParticipants>(props[QLatin1String("DBusNames")]));
}

void DBusTubeChannelPrivate::extractParticipants(const Tp::DBusTubeParticipants& participants)
{
    Q_Q(DBusTubeChannel);

    busNames.clear();
    for (DBusTubeParticipants::const_iterator i = participants.constBegin();
         i != participants.constEnd();
         ++i) {
        busNames.insert(q->connection()->contactManager()->lookupContactByHandle(i.key()),
                        i.value());
    }
}


void DBusTubeChannelPrivate::__k__gotDBusTubeProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        extractProperties(props);
        debug() << "Got reply to Properties::GetAll(DBusTubeChannel)";
        readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureDBusTube, true);
    } else {
        warning().nospace() << "Properties::GetAll(DBusTubeChannel) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureDBusTube, false,
                reply.error());
    }
}

void DBusTubeChannelPrivate::__k__onDBusNamesChanged(
        const Tp::DBusTubeParticipants& added,
        const Tp::UIntList& removed)
{
    Q_Q(DBusTubeChannel);

    QHash< ContactPtr, QString > realAdded;
    QList< ContactPtr > realRemoved;

    for (DBusTubeParticipants::const_iterator i = added.constBegin();
         i != added.constEnd();
         ++i) {
        ContactPtr contact = q->connection()->contactManager()->lookupContactByHandle(i.key());
        realAdded.insert(contact, i.value());
        // Add it to our hash as well
        busNames.insert(contact, i.value());
    }

    foreach (uint handle, removed) {
        ContactPtr contact = q->connection()->contactManager()->lookupContactByHandle(handle);
        realRemoved << contact;
        // Remove it from our hash as well
        busNames.remove(contact);
    }

    // Emit the "real" signal
    emit q->busNamesChanged(realAdded, realRemoved);
}

void DBusTubeChannelPrivate::introspectBusNamesMonitoring(
        DBusTubeChannelPrivate* self)
{
    DBusTubeChannel *parent = self->q_func();

    Client::ChannelTypeDBusTubeInterface *dbusTubeInterface =
            parent->interface<Client::ChannelTypeDBusTubeInterface>();

    // It must be present
    Q_ASSERT(dbusTubeInterface);

    // It makes sense only if this is a room, if that's not the case just spit a warning
    if (parent->targetHandleType() == static_cast<uint>(Tp::HandleTypeRoom)) {
        parent->connect(dbusTubeInterface, SIGNAL(DBusNamesChanged(Tp::DBusTubeParticipants,Tp::UIntList)),
                        parent, SLOT(__k__onDBusNamesChanged(Tp::DBusTubeParticipants,Tp::UIntList)));
    } else {
        warning() << "FeatureBusNamesMonitoring does not make sense in a P2P context";
    }

    self->readinessHelper->setIntrospectCompleted(DBusTubeChannel::FeatureBusNamesMonitoring, true);
}

void DBusTubeChannelPrivate::introspectDBusTube(
        DBusTubeChannelPrivate* self)
{
    DBusTubeChannel *parent = self->q_func();

    debug() << "Introspect dbus tube properties";

    Client::DBus::PropertiesInterface *properties =
            parent->interface<Client::DBus::PropertiesInterface>();

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(
                    QLatin1String(TP_QT_IFACE_CHANNEL_TYPE_DBUS_TUBE)),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            parent,
            SLOT(__k__gotDBusTubeProperties(QDBusPendingCallWatcher *)));
}

/**
 * \class DBusTubeChannel
 * \headerfile TelepathyQt/stream-tube-channel.h <TelepathyQt/DBusTubeChannel>
 *
 * \brief A class representing a Stream Tube
 *
 * \c DBusTubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Type.DBusTubeChannel.
 * It provides a transport for reliable and ordered data transfer, similar to SOCK_STREAM sockets.
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
const Feature DBusTubeChannel::FeatureDBusTube =
Feature(QLatin1String(DBusTubeChannel::staticMetaObject.className()), 0);
/**
 * Feature used in order to monitor connections to this tube.
 * Please note that this feature makes sense only in Group tubes.
 *
 * %busNamesChanged will be emitted when the participants of this tube change
 */
const Feature DBusTubeChannel::FeatureBusNamesMonitoring =
Feature(QLatin1String(DBusTubeChannel::staticMetaObject.className()), 1);

// Signals documentation
/**
 * \fn void DBusTubeChannel::busNamesChanged(const QHash< ContactPtr, QString > &added, const QList< ContactPtr >
&removed)
 *
 * Emitted when the participants of this tube change
 *
 * \param added An hash containing the contacts who joined this tube, with their respective bus name.
 * \param removed A list containing the contacts who left this tube.
 */

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
      d_ptr(new DBusTubeChannelPrivate(this))
{
    // Initialize
    Q_D(DBusTubeChannel);
    d->init();
}

DBusTubeChannel::DBusTubeChannel(const ConnectionPtr& connection,
        const QString& objectPath,
        const QVariantMap& immutableProperties,
        DBusTubeChannelPrivate& dd)
    : TubeChannel(connection, objectPath, immutableProperties)
    , d_ptr(&dd)
{
    // Initialize
    Q_D(DBusTubeChannel);
    d->init();
}


/**
 * Class destructor.
 */
DBusTubeChannel::~DBusTubeChannel()
{
    delete d_ptr;
}

/**
 * \returns the service name that will be used over the tube
 */
QString DBusTubeChannel::serviceName() const
{
    if (!isReady(FeatureDBusTube)) {
        warning() << "DBusTubeChannel::service() used with "
            "FeatureDBusTube not ready";
        return QString();
    }

    Q_D(const DBusTubeChannel);

    return d->serviceName;
}


/**
 * \returns Whether this Stream tube supports offering or accepting it as an Unix socket and requiring
 *          credentials for connecting to it.
 *
 * \see IncomingDBusTubeChannel::acceptTubeAsUnixSocket
 * \see OutgoingDBusTubeChannel::offerTubeAsUnixSocket
 * \see supportsUnixSockets
 */
bool DBusTubeChannel::supportsCredentials() const
{
    if (!isReady(FeatureDBusTube)) {
        warning() << "DBusTubeChannel::supportsCredentials() used with "
            "FeatureDBusTube not ready";
        return false;
    }

    Q_D(const DBusTubeChannel);

    return d->accessControls.contains(static_cast<uint>(Tp::SocketAccessControlCredentials));
}


void DBusTubeChannel::connectNotify(const char* signal)
{
    TubeChannel::connectNotify(signal);
}


/**
 * If the tube has been opened, this function returns the private bus address you should be listening
 * to for using this tube. Please note that specialized superclasses such as IncomingDBusTubeChannel and
 * OutgoingDBusTubeChannel have more convenient methods for accessing the resulting connection.
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

    Q_D(const DBusTubeChannel);

    return d->address;
}


/**
 * This function returns all the known active connections since FeatureConnectionMonitoring has
 * been enabled. For this method to return all known connections, you need to make
 * FeatureConnectionMonitoring ready before accepting or offering the tube.
 *
 * \returns A list of active connection ids known to this tube
 */
QHash< ContactPtr, QString > DBusTubeChannel::busNames() const
{
    if (!isReady(FeatureBusNamesMonitoring)) {
        warning() << "DBusTubeChannel::busNames() used with "
            "FeatureBusNamesMonitoring not ready";
        return QHash< ContactPtr, QString >();
    }

    Q_D(const DBusTubeChannel);

    return d->busNames;
}

}

#include "TelepathyQt/_gen/dbus-tube-channel.moc.hpp"
