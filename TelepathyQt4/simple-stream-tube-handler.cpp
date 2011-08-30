/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include "TelepathyQt4/simple-stream-tube-handler.h"

#include "TelepathyQt4/_gen/simple-stream-tube-handler.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/PendingComposite>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/StreamTubeChannel>

namespace Tp
{

namespace
{
    ChannelClassSpecList buildFilter(const QStringList &p2pServices,
            const QStringList &roomServices, bool requested)
    {
        Q_ASSERT(!p2pServices.isEmpty() || !roomServices.isEmpty());

        ChannelClassSpecList filter;

        foreach (QString service, p2pServices)
        {
            filter.append(requested ?
                    ChannelClassSpec::outgoingStreamTube(service) :
                    ChannelClassSpec::incomingStreamTube(service));
        }

        foreach (QString service, roomServices)
        {
            filter.append(requested ?
                    ChannelClassSpec::outgoingRoomStreamTube(service) :
                    ChannelClassSpec::incomingRoomStreamTube(service));
        }

        return filter;
    }
}

SharedPtr<SimpleStreamTubeHandler> SimpleStreamTubeHandler::create(
        const QStringList &p2pServices,
        const QStringList &roomServices,
        bool requested,
        bool monitorConnections,
        bool bypassApproval)
{
    return SharedPtr<SimpleStreamTubeHandler>(
            new SimpleStreamTubeHandler(
                p2pServices,
                roomServices,
                requested,
                monitorConnections,
                bypassApproval));
}

SimpleStreamTubeHandler::SimpleStreamTubeHandler(
        const QStringList &p2pServices,
        const QStringList &roomServices,
        bool requested,
        bool monitorConnections,
        bool bypassApproval)
: AbstractClientHandler(buildFilter(p2pServices, roomServices, requested)),
    mMonitorConnections(monitorConnections),
    mBypassApproval(bypassApproval)
{
}

SimpleStreamTubeHandler::~SimpleStreamTubeHandler()
{
    if (!mTubes.empty()) {
        debug() << "~SSTubeHandler(): Closing" << mTubes.size() << "leftover tubes";

        foreach (StreamTubeChannelPtr tube, mTubes.keys()) {
            tube->requestClose();
        }
    }
}

void SimpleStreamTubeHandler::handleChannels(
        const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const HandlerInfo &handlerInfo)
{
    debug() << "SimpleStreamTubeHandler::handleChannels() invoked for " <<
        channels.size() << "channels on account" << account->objectPath();

    SharedPtr<InvocationData> invocation(new InvocationData());
    QList<PendingOperation *> readyOps;

    foreach (ChannelPtr chan, channels) {
        StreamTubeChannelPtr tube = StreamTubeChannelPtr::qObjectCast(chan);

        if (!tube) {
            const QString channelType =
                chan->immutableProperties()[QLatin1String(
                        TELEPATHY_INTERFACE_CHANNEL ".ChannelType")].toString();

            if (channelType != TP_QT4_IFACE_CHANNEL_TYPE_STREAM_TUBE) {
                debug() << "We got a non-StreamTube channel" << chan->objectPath() <<
                    "of type" << channelType << ", ignoring";
            } else {
                warning() << "The channel factory used for a simple StreamTube handler must" <<
                    "construct StreamTubeChannel subclasses for stream tubes";
            }
            continue;
        }

        Features features = StreamTubeChannel::FeatureStreamTube;
        if (mMonitorConnections) {
            features.insert(StreamTubeChannel::FeatureConnectionMonitoring);
        }
        readyOps.append(tube->becomeReady(features));

        invocation->tubes.append(tube);
    }

    invocation->ctx = context;
    invocation->acc = account;
    invocation->time = userActionTime;

    if (!requestsSatisfied.isEmpty()) {
        invocation->hints = requestsSatisfied.first()->hints();
    }

    invocation->readyOp = new PendingComposite(readyOps, SharedPtr<SimpleStreamTubeHandler>(this));
    connect(invocation->readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onReadyOpFinished(Tp::PendingOperation*)));

    mInvocations.append(invocation);
}

void SimpleStreamTubeHandler::onReadyOpFinished(Tp::PendingOperation *op)
{
    Q_ASSERT(!mInvocations.isEmpty());
    Q_ASSERT(op->isFinished());

    for (QLinkedList<SharedPtr<InvocationData> >::iterator i = mInvocations.begin();
            i != mInvocations.end(); ++i) {
        if ((*i)->readyOp != op) {
            continue;
        }

        (*i)->readyOp = 0;

        if (op->isError()) {
            warning() << "Preparing proxies for SSTubeHandler failed with" << op->errorName()
                << op->errorMessage();
            (*i)->error = op->errorName();
            (*i)->message = op->errorMessage();
        }

        break;
    }

    while (!mInvocations.isEmpty() && !mInvocations.first()->readyOp) {
        SharedPtr<InvocationData> invocation = mInvocations.takeFirst();

        if (!invocation->error.isEmpty()) {
            // We guarantee that the proxies were ready - so we can't invoke the client if they
            // weren't made ready successfully. Fix the introspection code if this happens :)
            invocation->ctx->setFinishedWithError(invocation->error, invocation->message);
            continue;
        }

        debug() << "Emitting SSTubeHandler::invokedForTube for" << invocation->tubes.size()
            << "tubes";

        foreach (StreamTubeChannelPtr tube, invocation->tubes) {
            if (!tube->isValid()) {
                debug() << "Skipping already invalidated tube" << tube->objectPath();
                continue;
            }

            if (!mTubes.contains(tube)) {
                connect(tube.data(),
                        SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                        SLOT(onTubeInvalidated(Tp::DBusProxy*,QString,QString)));

                mTubes.insert(tube, invocation->acc);
            }

            emit invokedForTube(
                    invocation->acc,
                    tube,
                    invocation->time,
                    invocation->hints);
        }

        invocation->ctx->setFinished();
    }
}

void SimpleStreamTubeHandler::onTubeInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    StreamTubeChannelPtr tube(qobject_cast<StreamTubeChannel *>(proxy));

    Q_ASSERT(!tube.isNull());
    Q_ASSERT(mTubes.contains(tube));

    debug() << "Tube" << tube->objectPath() << "invalidated - " << errorName << ':' << errorMessage;

    AccountPtr acc = mTubes.value(tube);
    mTubes.remove(tube);

    emit tubeInvalidated(
            acc,
            tube,
            errorName,
            errorMessage);
}

} // Tp
