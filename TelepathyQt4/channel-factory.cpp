/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include "TelepathyQt4/channel-factory.h"

#include "TelepathyQt4/_gen/future-constants.h"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ContactSearchChannel>
#include <TelepathyQt4/FileTransferChannel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/OutgoingFileTransferChannel>
#include <TelepathyQt4/RoomListChannel>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/TextChannel>

namespace Tp
{

struct ChannelFactory::Private
{
    Private();

    typedef QPair<ChannelClassSpec, Features> FeaturePair;
    QList<FeaturePair> features;
};

ChannelFactory::Private::Private()
{
}

/**
 * \class ChannelFactory
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/channel-factory.h <TelepathyQt4/ChannelFactory>
 *
 * \brief Constructs Channel objects
 *
 * \todo This class is currently only a placeholder to enable using factories in general in other
 * classes. There is no actual configurability in the construction behavior, although a
 * factory-style construction API is provided.
 */

/**
 * Create a new ChannelFactory object for the given \a bus.
 *
 * The returned factory will construct channel subclasses provided by TelepathyQt4 as appropriate
 * for the channel immutable properties, but not make any features ready.
 *
 * \param bus The QDBusConnection the proxies constructed using this factory should use.
 * \return An ChannelFactoryPtr pointing to the newly created factory.
 */
ChannelFactoryPtr ChannelFactory::create(const QDBusConnection &bus)
{
    return ChannelFactoryPtr(new ChannelFactory(bus));
}

/**
 * Class constructor.
 *
 * The constructed factory will construct channel subclasses provided by TelepathyQt4 as appropriate
 * for the channel immutable properties, but not make any features ready.
 *
 * \param bus The QDBusConnection the proxies constructed using this factory should use.
 */
ChannelFactory::ChannelFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus), mPriv(new Private)
{
}

/**
 * Class destructor.
 */
ChannelFactory::~ChannelFactory()
{
    delete mPriv;
}

Features ChannelFactory::featuresFor(const ChannelClassSpec &channelClass) const
{
    Features features;

    foreach (const Private::FeaturePair &pair, mPriv->features) {
        if (pair.first.isSubsetOf(channelClass)) {
            features.unite(pair.second);
        }
    }

    return features;
}

void ChannelFactory::addFeaturesFor(const ChannelClassSpec &channelClass, const Features &features)
{
    QList<Private::FeaturePair>::iterator i;
    for (i = mPriv->features.begin(); i != mPriv->features.end(); ++i) {
        if (channelClass.allProperties().size() > i->first.allProperties().size()) {
            break;
        }

        if (i->first == channelClass) {
            i->second.unite(features);
            return;
        }
    }

    // We ran out of feature specifications (for the given size/specificity of a channel class)
    // before finding a matching one, so let's create a new entry
    mPriv->features.insert(i, qMakePair(channelClass, features));
}

/**
 * Constructs a Channel proxy and begins making it ready.
 *
 * If a valid proxy already exists in the factory cache for the given combination of \a busName and
 * \a objectPath, it is returned instead. All newly created proxies are automatically cached until
 * they're either DBusProxy::invalidated() or the last reference to them outside the factory has
 * been dropped.
 *
 * The proxy can be accessed immediately after this function returns using PendingReady::proxy().
 *
 * \todo Make it configurable which subclass is constructed and which features, if any, are made
 * ready on it.
 *
 * \param connection Proxy for the owning connection of the channel.
 * \param channelPath The object path of the channel.
 * \param immutableProperties The immutable properties of the channel.
 * \return A PendingReady operation with the proxy in PendingReady::proxy().
 */
PendingReady *ChannelFactory::proxy(const ConnectionPtr &connection, const QString &channelPath,
        const QVariantMap &immutableProperties) const
{
    SharedPtr<RefCounted> proxy = cachedProxy(connection->busName(), channelPath);
    if (!proxy) {
        proxy = create(connection, channelPath, immutableProperties);
    }

    return nowHaveProxy(proxy);
}

ChannelPtr ChannelFactory::create(const ConnectionPtr &connection,
        const QString &channelPath, const QVariantMap &immutableProperties)
{
    QString channelType = immutableProperties.value(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
    if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)) {
        return TextChannel::create(connection, channelPath, immutableProperties);
    } else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) ||
             channelType == QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL)) {
        return StreamedMediaChannel::create(connection, channelPath, immutableProperties);
    } else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_ROOM_LIST)) {
        return RoomListChannel::create(connection, channelPath, immutableProperties);
    } else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER)) {
        if (immutableProperties.contains(QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".Requested"))) {
            bool requested = immutableProperties.value(QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
            if (requested) {
                return OutgoingFileTransferChannel::create(connection, channelPath,
                        immutableProperties);
            } else {
                return IncomingFileTransferChannel::create(connection, channelPath,
                        immutableProperties);
            }
        } else {
            warning() << "Trying to create a channel of type FileTransfer "
                "without the " TELEPATHY_INTERFACE_CHANNEL ".Requested "
                "property set in immutableProperties, returning a "
                "FileTransferChannel instance";
            return FileTransferChannel::create(connection, channelPath,
                    immutableProperties);
        }
    } else if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH)) {
        return ContactSearchChannel::create(connection, channelPath, immutableProperties);
    }

    // ContactList, old-style Tubes, or a future channel type
    return Channel::create(connection, channelPath, immutableProperties);
}

/**
 * Transforms well-known names to the corresponding unique names, as is appropriate for Channel
 *
 * \param uniqueOrWellKnown The name to transform.
 * \return The unique name corresponding to \a uniqueOrWellKnown (which may be it itself).
 */
QString ChannelFactory::finalBusNameFrom(const QString &uniqueOrWellKnown) const
{
    return StatefulDBusProxy::uniqueNameFrom(dbusConnection(), uniqueOrWellKnown);
}

/**
 * Returns features as configured for the channel class given by the Channel::immutableProperties()
 * of \a proxy.
 *
 * \todo Make the features configurable - currently an empty set is always returned.
 *
 * \param proxy The Channel proxy to determine the features for.
 * \return The channel class-specific features.
 */
Features ChannelFactory::featuresFor(const SharedPtr<RefCounted> &proxy) const
{
    // API/ABI break TODO: change to qobjectCast once we have a saner object hierarchy
    ChannelPtr chan = ChannelPtr::dynamicCast(proxy);
    Q_ASSERT(!chan.isNull());

    return featuresFor(ChannelClassSpec(chan->immutableProperties()));
}

} // Tp
