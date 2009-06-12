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

#include <TelepathyQt4/PendingChannelRequest>
#include "TelepathyQt4/pending-channel-request-internal.h"

#include "TelepathyQt4/_gen/pending-channel-request.moc.hpp"
#include "TelepathyQt4/_gen/pending-channel-request-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ChannelDispatcher>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingReady>

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

namespace Tp
{

struct PendingChannelRequest::Private
{
    Private(const QDBusConnection &dbusConnection)
        : dbusConnection(dbusConnection),
          cancelOperation(0)
    {
    }

    QDBusConnection dbusConnection;
    ChannelRequestPtr channelRequest;
    PendingChannelRequestCancelOperation *cancelOperation;
};

/**
 * \class PendingChannelRequest
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/pending-channel-request.h> <TelepathyQt4/PendingChannelRequest>
 *
 * Class containing the parameters of and the reply to an asynchronous
 * ChannelRequest request. Instances of this class cannot be constructed
 * directly; the only way to get one is trough Account.
 */

/**
 * Construct a new PendingChannelRequest object.
 *
 * \param dbusConnection QDBusConnection to use.
 * \param accountObjectPath Account object path.
 * \param requestedProperties A dictionary containing the desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param create Whether createChannel or ensureChannel should be called.
 * \param parent Parent object
 */
PendingChannelRequest::PendingChannelRequest(const QDBusConnection &dbusConnection,
        const QString &accountObjectPath,
        const QVariantMap &requestedProperties,
        const QDateTime &userActionTime,
        const QString &preferredHandler,
        bool create,
        QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(dbusConnection))
{
    QString channelDispatcherObjectPath =
        QString("/%1").arg(TELEPATHY_INTERFACE_CHANNEL_DISPATCHER);
    channelDispatcherObjectPath.replace('.', '/');
    Client::ChannelDispatcherInterface *channelDispatcherInterface =
        new Client::ChannelDispatcherInterface(mPriv->dbusConnection,
                TELEPATHY_INTERFACE_CHANNEL_DISPATCHER,
                channelDispatcherObjectPath,
                this);
    QDBusPendingCallWatcher *watcher;
    if (create) {
        watcher = new QDBusPendingCallWatcher(
                channelDispatcherInterface->CreateChannel(
                    QDBusObjectPath(accountObjectPath),
                    requestedProperties,
                    userActionTime.isNull() ? 0 : userActionTime.toTime_t(),
                    preferredHandler), this);
    }
    else {
        watcher = new QDBusPendingCallWatcher(
                channelDispatcherInterface->EnsureChannel(
                    QDBusObjectPath(accountObjectPath),
                    requestedProperties,
                    userActionTime.isNull() ? 0 : userActionTime.toTime_t(),
                    preferredHandler), this);
    }

    // FIXME: This is a Qt bug fixed upstream, should be in the next Qt release.
    //        We should not need to check watcher->isFinished() here, remove the
    //        check when a fixed Qt version is released.
    if (watcher->isFinished()) {
        onWatcherFinished(watcher);
    } else {
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onWatcherFinished(QDBusPendingCallWatcher *)));
    }
}

/**
 * Class destructor.
 */
PendingChannelRequest::~PendingChannelRequest()
{
    delete mPriv;
}

ChannelRequestPtr PendingChannelRequest::channelRequest() const
{
    return mPriv->channelRequest;
}

PendingOperation *PendingChannelRequest::cancel()
{
    if (isFinished()) {
        // CR has already succeeded or failed, so let's just fail here
        return new PendingFailure(this,
                TELEPATHY_DBUS_ERROR_UNKNOWN_METHOD,
                "ChannnelRequest already finished");
    }

    if (!mPriv->cancelOperation) {
        mPriv->cancelOperation = new PendingChannelRequestCancelOperation(this);
        connect(mPriv->cancelOperation,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onCancelOperationFinished(Tp::PendingOperation*)));

        if (mPriv->channelRequest) {
            mPriv->cancelOperation->go(mPriv->channelRequest);
        }
    }

    return mPriv->cancelOperation;
}

void PendingChannelRequest::onWatcherFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    if (!reply.isError()) {
        QDBusObjectPath objectPath = reply.argumentAt<0>();
        debug() << "Got reply to ChannelDispatcher.Ensure/CreateChannel "
            "- object path:" << objectPath.path();

        mPriv->channelRequest = ChannelRequest::create(mPriv->dbusConnection,
                objectPath.path(), QVariantMap());

        if (mPriv->cancelOperation) {
            mPriv->cancelOperation->go(mPriv->channelRequest);
            setFinishedWithError(TELEPATHY_ERROR_CANCELLED,
                    "ChannelRequest cancelled");
        } else {
            emit channelRequestCreated(mPriv->channelRequest);

            connect(mPriv->channelRequest.data(),
                    SIGNAL(failed(const QString &, const QString &)),
                    SLOT(setFinishedWithError(const QString &, const QString &)));
            connect(mPriv->channelRequest.data(),
                    SIGNAL(succeeded()),
                    SLOT(setFinished()));

            connect(mPriv->channelRequest->proceed(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onProceedOperationFinished(Tp::PendingOperation*)));
        }
    } else {
        debug().nospace() << "Ensure/CreateChannel failed:" <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingChannelRequest::onProceedOperationFinished(PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
    }
}

void PendingChannelRequest::onCancelOperationFinished(PendingOperation *op)
{
    mPriv->cancelOperation = 0;
}

} // Tp
