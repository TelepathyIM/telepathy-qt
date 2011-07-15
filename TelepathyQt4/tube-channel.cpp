/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt4/TubeChannel>

#include "TelepathyQt4/_gen/tube-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/PendingVariantMap>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT TubeChannel::Private
{
    Private(TubeChannel *parent);

    static void introspectTube(TubeChannel::Private *self);

    void extractTubeProperties(const QVariantMap &props);

    // Public object
    TubeChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection
    TubeChannelState state;
    QVariantMap parameters;
};

TubeChannel::Private::Private(TubeChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      state((TubeChannelState) -1)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableTube(
            QSet<uint>() << 0,                                                          // makesSenseForStatuses
            Features() << Channel::FeatureCore,                                         // dependsOnFeatures (core)
            QStringList() << QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_TUBE), // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &TubeChannel::Private::introspectTube,
            this);
    introspectables[TubeChannel::FeatureCore] = introspectableTube;

    readinessHelper->addIntrospectables(introspectables);
}

void TubeChannel::Private::introspectTube(TubeChannel::Private *self)
{
    TubeChannel *parent = self->parent;

    debug() << "Introspecting tube properties";
    Client::ChannelInterfaceTubeInterface *tubeInterface =
            parent->interface<Client::ChannelInterfaceTubeInterface>();

    parent->connect(tubeInterface,
            SIGNAL(TubeChannelStateChanged(uint)),
            SLOT(onTubeChannelStateChanged(uint)));

    PendingVariantMap *pvm = tubeInterface->requestAllProperties();
    parent->connect(pvm,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(gotTubeProperties(Tp::PendingOperation *)));
}

void TubeChannel::Private::extractTubeProperties(const QVariantMap &props)
{
    state = (Tp::TubeChannelState) qdbus_cast<uint>(props[QLatin1String("State")]);
    parameters = qdbus_cast<QVariantMap>(props[QLatin1String("Parameters")]);
}

/**
 * \class TubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/tube-channel.h <TelepathyQt4/TubeChannel>
 *
 * \brief The TubeChannel class is a base class for all tube types.
 *
 * A tube is a mechanism for arbitrary data transfer between two or more IM users,
 * used to allow applications on the users' systems to communicate without having
 * to establish network connections themselves.
 *
 * Note that TubeChannel should never be instantiated directly, instead one of its
 * subclasses (e.g. IncomingStreamTubeChannel or OutgoingStreamTubeChannel) should be used.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * TubeChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * TubeChannel methods.
 * See specific methods documentation for more details.
 */
const Feature TubeChannel::FeatureCore = Feature(QLatin1String(TubeChannel::staticMetaObject.className()), 0);

/**
 * \deprecated Use TubeChannel::FeatureCore instead.
 */
const Feature TubeChannel::FeatureTube = TubeChannel::FeatureCore;

/**
 * Create a new TubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
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
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on TubeChannel::FeatureCore.
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
 * Return the parameters associated with this tube, if any.
 *
 * Note that for outgoing tubes, this function will only return a valid value after the tube has
 * been offered successfully.
 *
 * This method requires TubeChannel::FeatureCore to be ready.
 *
 * \return The parameters as QVariantMap.
 *         For more details, please refer to \telepathy_spec.
 */
QVariantMap TubeChannel::parameters() const
{
    if (!isReady(FeatureCore)) {
        warning() << "TubeChannel::parameters() used with FeatureCore not ready";
        return QVariantMap();
    }

    return mPriv->parameters;
}

/**
 * Return the state of this tube.
 *
 * Change notification is via the stateChanged() signal.
 *
 * This method requires TubeChannel::FeatureCore to be ready.
 *
 * \return The state as #TubeChannelState.
 * \sa stateChanged()
 */
TubeChannelState TubeChannel::state() const
{
    if (!isReady(FeatureCore)) {
        warning() << "TubeChannel::state() used with FeatureCore not ready";
        return TubeChannelStateNotOffered;
    }

    return mPriv->state;
}

/**
 * \deprecated Use state() instead.
 */
TubeChannelState TubeChannel::tubeState() const
{
    return state();
}

void TubeChannel::setParameters(const QVariantMap &parameters)
{
    mPriv->parameters = parameters;
}

void TubeChannel::onTubeChannelStateChanged(uint newState)
{
    if (newState == mPriv->state) {
        return;
    }

    uint oldState = mPriv->state;

    debug() << "Tube state changed to" << newState;
    mPriv->state = (Tp::TubeChannelState) newState;

    /* only emit stateChanged if we already received the state from initial introspection */
    if (oldState != (uint) -1) {
        emit stateChanged((Tp::TubeChannelState) newState);
        // FIXME (API/ABI break) Remove tubeStateChanged call
        emit tubeStateChanged((Tp::TubeChannelState) newState);
    }
}

void TubeChannel::gotTubeProperties(PendingOperation *op)
{
    if (!op->isError()) {
        PendingVariantMap *pvm = qobject_cast<PendingVariantMap *>(op);

        mPriv->extractTubeProperties(pvm->result());

        debug() << "Got reply to Properties::GetAll(TubeChannel)";
        mPriv->readinessHelper->setIntrospectCompleted(TubeChannel::FeatureCore, true);
    } else {
        warning().nospace() << "Properties::GetAll(TubeChannel) failed "
            "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(TubeChannel::FeatureCore, false,
                op->errorName(), op->errorMessage());
    }
}

/**
 * \fn void TubeChannel::stateChanged(Tp::TubeChannelState state)
 *
 * Emitted when the value of state() changes.
 *
 * \sa state The new state of this tube.
 * \sa state()
 */

/**
 * \fn void TubeChannel::tubeStateChanged(Tp::TubeChannelState state)
 *
 * \deprecated Use stateChanged() instead.
 */

void TubeChannel::connectNotify(const char *signalName)
{
    if (qstrcmp(signalName, SIGNAL(tubeStateChanged(Tp::TubeChannelState))) == 0) {
        warning() << "Connecting to deprecated signal tubeStateChanged(Tp::TubeChannelState)";
    }
}


} // Tp
