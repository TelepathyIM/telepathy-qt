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

#include "cli-channel.h"

#include "_gen/cli-channel-body.hpp"
#include "_gen/cli-channel.moc.hpp"
#include "cli-channel.moc.hpp"

#include <QQueue>

#include "cli-dbus.h"
#include "constants.h"
#include "debug-internal.hpp"

namespace Telepathy
{
namespace Client
{

struct Channel::Private
{
    // Public object
    Channel& parent;

    // Optional interface proxies
    DBus::PropertiesInterface* properties;

    // Introspection
    Readiness readiness;
    QStringList interfaces;
    QQueue<void (Private::*)()> introspectQueue;

    // Introspected properties
    QString channelType;
    uint targetHandleType;
    uint targetHandle;
    uint groupFlags;
    HandleOwnerMap groupHandleOwners;
    QSet<uint> groupMembers;
    // TODO local pending
    QSet<uint> groupRemotePendingMembers;
    uint groupSelfHandle;

    Private(Channel& parent)
        : parent(parent)
    {
        debug() << "Creating new Channel";

        properties = 0;
        readiness = ReadinessJustCreated;
        targetHandleType = 0;
        targetHandle = 0;
        groupFlags = 0;
        groupSelfHandle = 0;

        debug() << "Connecting to Channel::Closed()";
        parent.connect(&parent,
                       SIGNAL(Closed()),
                       SLOT(onClosed()));

        introspectQueue.enqueue(&Private::introspectMain);
        continueIntrospection();
    }

    void introspectMain()
    {
        if (!properties) {
            properties = parent.propertiesInterface();
            Q_ASSERT(properties != 0);
        }

        debug() << "Calling Properties::GetAll(Channel)";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(
                    properties->GetAll(TELEPATHY_INTERFACE_CHANNEL), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackChannelType()
    {
        debug() << "Calling Channel::GetChannelType()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetChannelType(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotChannelType(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackHandle()
    {
        debug() << "Calling Channel::GetHandle()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetHandle(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotHandle(QDBusPendingCallWatcher*)));
    }

    void introspectMainFallbackInterfaces()
    {
        debug() << "Calling Channel::GetInterfaces()";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(parent.GetInterfaces(), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotInterfaces(QDBusPendingCallWatcher*)));
    }

    void introspectGroup()
    {
        Q_ASSERT(properties != 0);

        debug() << "Calling Properties::GetAll(Channel.Interface.Group)";
        QDBusPendingCallWatcher* watcher =
            new QDBusPendingCallWatcher(
                    properties->GetAll(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP), &parent);
        parent.connect(watcher,
                       SIGNAL(finished(QDBusPendingCallWatcher*)),
                       SLOT(gotGroupProperties(QDBusPendingCallWatcher*)));
    }

    void continueIntrospection()
    {
        if (introspectQueue.isEmpty()) {
            if (readiness < ReadinessFull)
                changeReadiness(ReadinessFull);
        } else {
            (this->*introspectQueue.dequeue())();
        }
    }

    void nowHaveInterfaces()
    {
        debug() << "Channel has" << interfaces.size() << "optional interfaces:" << interfaces;

        for (QStringList::const_iterator i = interfaces.begin();
                                         i != interfaces.end();
                                         ++i) {
            if (*i == TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP) {
                introspectQueue.enqueue(&Private::introspectGroup);
            }
        }

        continueIntrospection();
    }

    void changeReadiness(Readiness newReadiness)
    {
        Q_ASSERT(newReadiness != readiness);

        switch (readiness) {
            case ReadinessJustCreated:
                break;
            case ReadinessFull:
                Q_ASSERT(newReadiness == ReadinessDead);
                break;
            case ReadinessDead:
            default:
                Q_ASSERT(false);
                break;
        }

        debug() << "Channel readiness changed from" << readiness << "to" << newReadiness;

        if (newReadiness == ReadinessFull) {
            debug() << "Channel fully ready";
            debug() << " Channel type" << channelType;
            debug() << " Target handle" << targetHandle;
            debug() << " Target handle type" << targetHandleType;
        } else {
            debug() << "R.I.P. Channel.";
        }

        readiness = newReadiness;
        emit parent.readinessChanged(newReadiness);
    }
};

Channel::Channel(const QString& serviceName,
                 const QString& objectPath,
                 QObject* parent)
    : ChannelInterface(serviceName, objectPath, parent),
      mPriv(new Private(*this))
{
}

Channel::Channel(const QDBusConnection& connection,
                 const QString& serviceName,
                 const QString& objectPath,
                 QObject* parent)
    : ChannelInterface(connection, serviceName, objectPath, parent),
      mPriv(new Private(*this))
{
}

Channel::~Channel()
{
    delete mPriv;
}

Channel::Readiness Channel::readiness() const
{
    return mPriv->readiness;
}

QStringList Channel::interfaces() const
{
    return mPriv->interfaces;
}

QString Channel::channelType() const
{
    return mPriv->channelType;
}

uint Channel::targetHandleType() const
{
    return mPriv->targetHandleType;
}

uint Channel::targetHandle() const
{
    return mPriv->targetHandle;
}

void Channel::gotMainProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError())
        props = reply.value();

    QList<bool> conditions;

    conditions << (props.size() >= 4);
    conditions << (props.contains("ChannelType") && !qdbus_cast<QString>(props["ChannelType"]).isEmpty());
    conditions << props.contains("Interfaces");
    conditions << props.contains("TargetHandle");
    conditions << props.contains("TargetHandleType");

    if (conditions.contains(false)) {
        if (reply.isError())
            warning().nospace() << "Properties::GetAll(Channel) failed with " << reply.error().name() << ": " << reply.error().message();
        else
            warning() << "Reply to Properties::GetAll(Channel) didn't contain the expected properties";

        warning() << "Assuming a pre-0.17.7-spec service, falling back to serial inspection";

        mPriv->introspectQueue.enqueue(&Private::introspectMainFallbackChannelType);
        mPriv->introspectQueue.enqueue(&Private::introspectMainFallbackHandle);
        mPriv->introspectQueue.enqueue(&Private::introspectMainFallbackInterfaces);

        mPriv->continueIntrospection();
        return;
    }

    debug() << "Got reply to Properties::GetAll(Channel)";
    mPriv->channelType = qdbus_cast<QString>(props["ChannelType"]);
    mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    mPriv->targetHandle = qdbus_cast<uint>(props["TargetHandle"]);
    mPriv->targetHandleType = qdbus_cast<uint>(props["TargetHandleType"]);
    mPriv->nowHaveInterfaces();
}

void Channel::gotChannelType(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetChannelType() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if (mPriv->readiness != ReadinessDead)
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetChannelType()";
    mPriv->channelType = reply.value();
    mPriv->continueIntrospection();
}

void Channel::gotHandle(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetHandle() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if (mPriv->readiness != ReadinessDead)
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetHandle()";
    mPriv->targetHandleType = reply.argumentAt<0>();
    mPriv->targetHandle = reply.argumentAt<1>();
    mPriv->continueIntrospection();
}

void Channel::gotInterfaces(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;

    if (reply.isError()) {
        warning().nospace() << "Channel::GetInterfaces() failed with " << reply.error().name() << ": " << reply.error().message() << ", Channel officially dead";
        if (mPriv->readiness != ReadinessDead)
            mPriv->changeReadiness(ReadinessDead);
        return;
    }

    debug() << "Got reply to fallback Channel::GetInterfaces()";
    mPriv->interfaces = reply.value();
    mPriv->nowHaveInterfaces();
}

void Channel::onClosed()
{
    debug() << "Got Channel::Closed";

    if (mPriv->readiness != ReadinessDead)
        mPriv->changeReadiness(ReadinessDead);
}

void Channel::gotGroupProperties(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError())
        props = reply.value();

    QList<bool> conditions;

    conditions << (props.size() >= 6);
    conditions << (props.contains("GroupFlags") && (qdbus_cast<uint>(props["GroupFlags"]) & ChannelGroupFlagProperties));
    conditions << props.contains("HandleOwners");
    conditions << props.contains("LocalPendingMembers");
    conditions << props.contains("Members");
    conditions << props.contains("RemotePendingMembers");
    conditions << props.contains("SelfHandle");

    if (conditions.contains(false)) {
        if (reply.isError())
            warning().nospace() << "Properties::GetAll(Channel.Interface.Group) failed with " << reply.error().name() << ": " << reply.error().message();
        else
            warning() << "Reply to Properties::GetAll(Channel.Interface.Group) didn't contain the expected properties";

        warning() << "Assuming a pre-0.17.6-spec service, falling back to serial inspection";
        // Enqueue group fallbacks here
        mPriv->continueIntrospection();
        return;
    }

    debug() << "Got reply to Properties::GetAll(Channel.Interface.Group)";
    mPriv->groupFlags = qdbus_cast<uint>(props["GroupFlags"]);
    mPriv->groupHandleOwners = qdbus_cast<HandleOwnerMap>(props["HandleOwners"]);
    mPriv->groupMembers = QSet<uint>::fromList(qdbus_cast<UIntList>(props["Members"]));
    // TODO local pending
    mPriv->groupRemotePendingMembers = QSet<uint>::fromList(qdbus_cast<UIntList>(props["RemotePendingMembers"]));
    mPriv->groupSelfHandle = qdbus_cast<uint>(props["SelfHandle"]);
    mPriv->continueIntrospection();
}

}
}
