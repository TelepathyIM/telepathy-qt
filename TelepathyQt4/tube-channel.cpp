/*
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/TubeChannel>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT TubeChannel::Private
{
    Private(TubeChannel *parent);
    ~Private();

    void extractProperties(const QVariantMap &props);

    static void introspectTube(Private *self);

    // Public object
    TubeChannel *parent;

    ReadinessHelper *readinessHelper;

    // Properties
    TubeChannelState state;
    TubeType type;
    QVariantMap parameters;

    // Private slots
    void __k__onTubeChannelStateChanged(uint newstate);
    void __k__gotTubeProperties(QDBusPendingCallWatcher *watcher);

};

TubeChannel::Private::Private(TubeChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper())
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableTube(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectTube,
        this);
    introspectables[FeatureTube] = introspectableTube;

    readinessHelper->addIntrospectables(introspectables);
}

TubeChannel::Private::~Private()
{
}

void TubeChannel::Private::extractProperties(const QVariantMap& props)
{
    state = (Tp::TubeChannelState)qdbus_cast<uint>(props[QLatin1String("State")]);
    parameters = qdbus_cast<QVariantMap>(props[QLatin1String("Parameters")]);
}

void TubeChannel::Private::__k__gotTubeProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        extractProperties(props);
        debug() << "Got reply to Properties::GetAll(TubeChannel)";
        readinessHelper->setIntrospectCompleted(FeatureTube, true);
    }
    else {
        warning().nospace() << "Properties::GetAll(TubeChannel) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        readinessHelper->setIntrospectCompleted(FeatureTube, false,
                reply.error());
    }
}

void TubeChannel::Private::__k__onTubeChannelStateChanged(uint newstate)
{
    state = (Tp::TubeChannelState)newstate;
    emit parent->tubeStateChanged((Tp::TubeChannelState)newstate);
}

void TubeChannel::Private::introspectTube(TubeChannel::Private* self)
{
    TubeChannel *parent = self->parent;

    debug() << "Introspect tube state";

    Client::ChannelInterfaceTubeInterface *tubeInterface = 
            parent->interface<Client::ChannelInterfaceTubeInterface>();

    // It must be present
    Q_ASSERT(tubeInterface);

    parent->connect(tubeInterface, SIGNAL(TubeChannelStateChanged(uint)),
                    parent, SLOT(__k__onTubeChannelStateChanged(uint)));

    Client::DBus::PropertiesInterface *properties =
            parent->interface<Client::DBus::PropertiesInterface>();

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                properties->GetAll(
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_TUBE)),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(__k__gotTubeProperties(QDBusPendingCallWatcher *)));
}

/**
 * \class TubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/tube-channel.h <TelepathyQt4/TubeChannel>
 *
 * \brief A class representing an abstract Tube
 *
 * \c TubeChannel is an high level wrapper for managing Telepathy interface
 * org.freedesktop.Telepathy.Channel.Interface.Tube.
 * A tube is a mechanism for arbitrary data transfer between two or more IM users,
 * used to allow applications on the users' systems to communicate without having
 * to establish network connections themselves.
 *
 * \note You should \b never create an abstract tube: you should use one of its
 * subclasses instead. At the moment, \c StreamTube and \c DBusTube are available.
 *
 * For more details, please refer to Telepathy spec.
 */

// Features declaration and documentation
/**
 * Feature representing the core that needs to become ready to make the
 * TubeChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * TubeChannel methods.
 * See specific methods documentation for more details.
 */
const Feature TubeChannel::FeatureTube = Feature(QLatin1String(TubeChannel::staticMetaObject.className()), 0);

// Signals documentation
/**
 * \fn void TubeChannel::tubeStateChanged(Tp::TubeChannelState state)
 *
 * Emitted when the state of the tube has changed, if
 * FeatureTube has been enabled.
 */

/**
 * Create a new TubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A TubeChannelPtr object pointing to the newly created
 *         TubeChannel object.
 */
TubeChannelPtr TubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return TubeChannelPtr(new TubeChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new TubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
TubeChannel::TubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
TubeChannel::~TubeChannel()
{
    delete mPriv;
}

/**
 * \returns A dictionary of arbitrary parameters. Please refer to the spec for more details.
 */
QVariantMap TubeChannel::parameters() const
{
    return mPriv->parameters;
}

/**
 * \returns The State of the tube in this channel.
 */
TubeChannelState TubeChannel::tubeState() const
{
    if (!isReady(FeatureTube)) {
        warning() << "TubeChannel::tubeState() used with "
            "FeatureTube not ready";
        return TubeChannelStateNotOffered;
    }

    return mPriv->state;
}

void TubeChannel::connectNotify(const char* signal)
{
    if (QLatin1String(signal) == SIGNAL(tubeStateChanged(Tp::TubeChannelState)) &&
        !isReady(FeatureTube)) {
        warning() << "Connected to the signal tubeStateChanged, but FeatureTube is "
            "not ready. The signal won't be emitted until the mentioned feature is ready.";
    }

    Channel::connectNotify(signal);
}

} // Tp


#include "TelepathyQt4/_gen/tube-channel.moc.hpp"
