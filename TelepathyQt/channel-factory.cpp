/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#include <TelepathyQt/ChannelFactory>

#include "TelepathyQt/_gen/channel-factory.moc.hpp"

#include "TelepathyQt/_gen/future-constants.h"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/CallChannel>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassFeatures>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactSearchChannel>
#include <TelepathyQt/FileTransferChannel>
#include <TelepathyQt/IncomingDBusTubeChannel>
#include <TelepathyQt/IncomingFileTransferChannel>
#include <TelepathyQt/IncomingStreamTubeChannel>
#include <TelepathyQt/OutgoingDBusTubeChannel>
#include <TelepathyQt/OutgoingFileTransferChannel>
#include <TelepathyQt/OutgoingStreamTubeChannel>
#include <TelepathyQt/RoomListChannel>
#include <TelepathyQt/ServerAuthenticationChannel>
#include <TelepathyQt/StreamTubeChannel>
#include <TelepathyQt/StreamedMediaChannel>
#include <TelepathyQt/TextChannel>

namespace Tp
{

struct TP_QT_NO_EXPORT ChannelFactory::Private
{
    Private();

    QList<ChannelClassFeatures> features;

    typedef QPair<ChannelClassSpec, ConstructorConstPtr> CtorPair;
    QList<CtorPair> ctors;
};

ChannelFactory::Private::Private()
{
}

/**
 * \class ChannelFactory
 * \ingroup utils
 * \headerfile TelepathyQt/channel-factory.h <TelepathyQt/ChannelFactory>
 *
 * \brief The ChannelFactory class is responsible for constructing Channel
 * objects according to application-defined settings.
 */

/**
 * Create a new ChannelFactory object.
 *
 * The returned factory will construct channel subclasses provided by TelepathyQt as appropriate
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
 * Construct a new ChannelFactory object.
 *
 * The constructed factory will construct channel subclasses provided by TelepathyQt as appropriate
 * for the channel immutable properties, but not make any features ready.
 *
 * \param bus The QDBusConnection the proxies constructed using this factory should use.
 */
ChannelFactory::ChannelFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus), mPriv(new Private)
{
    setSubclassForTextChats<TextChannel>();
    setSubclassForTextChatrooms<TextChannel>();
    setSubclassForCalls<CallChannel>();
    setSubclassForStreamedMediaCalls<StreamedMediaChannel>();
    setSubclassForRoomLists<RoomListChannel>();
    setSubclassForIncomingDBusTubes<IncomingDBusTubeChannel>();
    setSubclassForOutgoingDBusTubes<OutgoingDBusTubeChannel>();
    setSubclassForIncomingRoomDBusTubes<IncomingDBusTubeChannel>();
    setSubclassForOutgoingRoomDBusTubes<OutgoingDBusTubeChannel>();
    setSubclassForIncomingFileTransfers<IncomingFileTransferChannel>();
    setSubclassForOutgoingFileTransfers<OutgoingFileTransferChannel>();
    setSubclassForIncomingStreamTubes<IncomingStreamTubeChannel>();
    setSubclassForOutgoingStreamTubes<OutgoingStreamTubeChannel>();
    setSubclassForIncomingRoomStreamTubes<IncomingStreamTubeChannel>();
    setSubclassForOutgoingRoomStreamTubes<OutgoingStreamTubeChannel>();
    setSubclassForContactSearches<ContactSearchChannel>();
    setSubclassForServerAuthentication<ServerAuthenticationChannel>();
    setFallbackSubclass<Channel>();
}

/**
 * Class destructor.
 */
ChannelFactory::~ChannelFactory()
{
    delete mPriv;
}

Features ChannelFactory::featuresForTextChats(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::textChat(additionalProps));
}

void ChannelFactory::addFeaturesForTextChats(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::textChat(additionalProps), features);
    addFeaturesFor(ChannelClassSpec::unnamedTextChat(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForTextChats(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::textChat(additionalProps));
}

void ChannelFactory::setConstructorForTextChats(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::textChat(additionalProps), ctor);
    setConstructorFor(ChannelClassSpec::unnamedTextChat(additionalProps), ctor);
}

Features ChannelFactory::featuresForTextChatrooms(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::textChatroom(additionalProps));
}

void ChannelFactory::addFeaturesForTextChatrooms(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::textChatroom(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForTextChatrooms(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::textChatroom(additionalProps));
}

void ChannelFactory::setConstructorForTextChatrooms(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::textChatroom(additionalProps), ctor);
}

Features ChannelFactory::featuresForCalls(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::audioCall(additionalProps));
}

void ChannelFactory::addFeaturesForCalls(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::audioCall(additionalProps), features);
    addFeaturesFor(ChannelClassSpec::videoCall(additionalProps), features);
}

void ChannelFactory::setConstructorForCalls(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::audioCall(additionalProps), ctor);
    setConstructorFor(ChannelClassSpec::videoCall(additionalProps), ctor);
}

Features ChannelFactory::featuresForStreamedMediaCalls(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::streamedMediaCall(additionalProps));
}

void ChannelFactory::addFeaturesForStreamedMediaCalls(const Features &features,
        const QVariantMap &additionalProps)
{
    ChannelClassSpec smSpec = ChannelClassSpec::streamedMediaCall(additionalProps);
    ChannelClassSpec unnamedSMSpec = ChannelClassSpec::unnamedStreamedMediaCall(additionalProps);

    addFeaturesFor(smSpec, features);
    addFeaturesFor(unnamedSMSpec, features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForStreamedMediaCalls(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::streamedMediaCall(additionalProps));
}

void ChannelFactory::setConstructorForStreamedMediaCalls(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    ChannelClassSpec smSpec = ChannelClassSpec::streamedMediaCall(additionalProps);
    ChannelClassSpec unnamedSMSpec = ChannelClassSpec::unnamedStreamedMediaCall(additionalProps);

    setConstructorFor(smSpec, ctor);
    setConstructorFor(unnamedSMSpec, ctor);
}

Features ChannelFactory::featuresForRoomLists(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::roomList(additionalProps));
}

void ChannelFactory::addFeaturesForRoomLists(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::roomList(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForRoomLists(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::roomList(additionalProps));
}

void ChannelFactory::setConstructorForRoomLists(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::roomList(additionalProps), ctor);
}

Features ChannelFactory::featuresForOutgoingFileTransfers(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::outgoingFileTransfer(additionalProps));
}

void ChannelFactory::addFeaturesForOutgoingFileTransfers(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::outgoingFileTransfer(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForOutgoingFileTransfers(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::outgoingFileTransfer(additionalProps));
}

void ChannelFactory::setConstructorForOutgoingFileTransfers(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::outgoingFileTransfer(additionalProps), ctor);
}

Features ChannelFactory::featuresForIncomingFileTransfers(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::incomingFileTransfer(additionalProps));
}

void ChannelFactory::addFeaturesForIncomingFileTransfers(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::incomingFileTransfer(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForIncomingFileTransfers(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::incomingFileTransfer(additionalProps));
}

void ChannelFactory::setConstructorForIncomingFileTransfers(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::incomingFileTransfer(additionalProps), ctor);
}

Features ChannelFactory::featuresForOutgoingStreamTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::outgoingStreamTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForOutgoingStreamTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::outgoingStreamTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForOutgoingStreamTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::outgoingStreamTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForOutgoingStreamTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::outgoingStreamTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForIncomingStreamTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::incomingStreamTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForIncomingStreamTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::incomingStreamTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForIncomingStreamTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::incomingStreamTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForIncomingStreamTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::incomingStreamTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForOutgoingRoomStreamTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::outgoingRoomStreamTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForOutgoingRoomStreamTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::outgoingRoomStreamTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForOutgoingRoomStreamTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::outgoingRoomStreamTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForOutgoingRoomStreamTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::outgoingRoomStreamTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForIncomingRoomStreamTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::incomingRoomStreamTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForIncomingRoomStreamTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::incomingRoomStreamTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForIncomingRoomStreamTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::incomingRoomStreamTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForIncomingRoomStreamTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::incomingRoomStreamTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForOutgoingDBusTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::outgoingDBusTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForOutgoingDBusTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::outgoingDBusTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForOutgoingDBusTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::outgoingDBusTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForOutgoingDBusTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::outgoingDBusTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForIncomingDBusTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::incomingDBusTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForIncomingDBusTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::incomingDBusTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForIncomingDBusTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::incomingDBusTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForIncomingDBusTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::incomingDBusTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForOutgoingRoomDBusTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::outgoingRoomDBusTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForOutgoingRoomDBusTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::outgoingRoomDBusTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForOutgoingRoomDBusTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::outgoingRoomDBusTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForOutgoingRoomDBusTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::outgoingRoomDBusTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForIncomingRoomDBusTubes(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::incomingRoomDBusTube(QString(), additionalProps));
}

void ChannelFactory::addFeaturesForIncomingRoomDBusTubes(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::incomingRoomDBusTube(QString(), additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForIncomingRoomDBusTubes(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::incomingRoomDBusTube(QString(), additionalProps));
}

void ChannelFactory::setConstructorForIncomingRoomDBusTubes(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::incomingRoomDBusTube(QString(), additionalProps), ctor);
}

Features ChannelFactory::featuresForContactSearches(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::contactSearch(additionalProps));
}

void ChannelFactory::addFeaturesForContactSearches(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::contactSearch(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForContactSearches(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::contactSearch(additionalProps));
}

void ChannelFactory::setConstructorForContactSearches(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::contactSearch(additionalProps), ctor);
}

Features ChannelFactory::featuresForServerAuthentication(const QVariantMap &additionalProps) const
{
    return featuresFor(ChannelClassSpec::serverAuthentication(additionalProps));
}

void ChannelFactory::addFeaturesForServerAuthentication(const Features &features,
        const QVariantMap &additionalProps)
{
    addFeaturesFor(ChannelClassSpec::serverAuthentication(additionalProps), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorForServerAuthentication(
        const QVariantMap &additionalProps) const
{
    return constructorFor(ChannelClassSpec::serverAuthentication(additionalProps));
}

void ChannelFactory::setConstructorForServerAuthentication(const ConstructorConstPtr &ctor,
        const QVariantMap &additionalProps)
{
    setConstructorFor(ChannelClassSpec::serverAuthentication(additionalProps), ctor);
}

Features ChannelFactory::commonFeatures() const
{
    return featuresFor(ChannelClassSpec());
}

void ChannelFactory::addCommonFeatures(const Features &features)
{
    addFeaturesFor(ChannelClassSpec(), features);
}

ChannelFactory::ConstructorConstPtr ChannelFactory::fallbackConstructor() const
{
    return constructorFor(ChannelClassSpec());
}

void ChannelFactory::setFallbackConstructor(const ConstructorConstPtr &ctor)
{
    setConstructorFor(ChannelClassSpec(), ctor);
}

Features ChannelFactory::featuresFor(const ChannelClassSpec &channelClass) const
{
    Features features;

    foreach (const ChannelClassFeatures &pair, mPriv->features) {
        if (pair.first.isSubsetOf(channelClass)) {
            features.unite(pair.second);
        }
    }

    return features;
}

void ChannelFactory::addFeaturesFor(const ChannelClassSpec &channelClass, const Features &features)
{
    QList<ChannelClassFeatures>::iterator i;
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

ChannelFactory::ConstructorConstPtr ChannelFactory::constructorFor(const ChannelClassSpec &cc) const
{
    QList<Private::CtorPair>::iterator i;
    for (i = mPriv->ctors.begin(); i != mPriv->ctors.end(); ++i) {
        if (i->first.isSubsetOf(cc)) {
            return i->second;
        }
    }

    // If this is reached, we didn't have a proper fallback constructor
    Q_ASSERT(false);
    return ConstructorConstPtr();
}

void ChannelFactory::setConstructorFor(const ChannelClassSpec &channelClass,
        const ConstructorConstPtr &ctor)
{
    if (ctor.isNull()) {
        warning().nospace() << "Tried to set a NULL ctor for ChannelClass("
            << channelClass.channelType() << ", " << channelClass.targetHandleType() << ", "
            << channelClass.allProperties().size() << "props in total)";
        return;
    }

    QList<Private::CtorPair>::iterator i;
    for (i = mPriv->ctors.begin(); i != mPriv->ctors.end(); ++i) {
        if (channelClass.allProperties().size() > i->first.allProperties().size()) {
            break;
        }

        if (i->first == channelClass) {
            i->second = ctor;
            return;
        }
    }

    // We ran out of constructors (for the given size/specificity of a channel class)
    // before finding a matching one, so let's create a new entry
    mPriv->ctors.insert(i, qMakePair(channelClass, ctor));
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
 * \param connection Proxy for the owning connection of the channel.
 * \param channelPath The object path of the channel.
 * \param immutableProperties The immutable properties of the channel.
 * \return A PendingReady operation with the proxy in PendingReady::proxy().
 */
PendingReady *ChannelFactory::proxy(const ConnectionPtr &connection, const QString &channelPath,
        const QVariantMap &immutableProperties) const
{
    DBusProxyPtr proxy = cachedProxy(connection->busName(), channelPath);
    if (proxy.isNull()) {
        proxy = constructorFor(ChannelClassSpec(immutableProperties))->construct(connection,
                channelPath, immutableProperties);
    }

    return nowHaveProxy(proxy);
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
 * Return features as configured for the channel class given by the Channel::immutableProperties()
 * of \a proxy.
 *
 * \param proxy The Channel proxy to determine the features for.
 * \return A list of Feature objects.
 */
Features ChannelFactory::featuresFor(const DBusProxyPtr &proxy) const
{
    ChannelPtr chan = ChannelPtr::qObjectCast(proxy);
    Q_ASSERT(!chan.isNull());

    return featuresFor(ChannelClassSpec(chan->immutableProperties()));
}

} // Tp
