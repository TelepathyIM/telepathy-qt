/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt/PendingChannelRequest>
#include "TelepathyQt/pending-channel-request-internal.h"

#include "TelepathyQt/_gen/pending-channel-request.moc.hpp"
#include "TelepathyQt/_gen/pending-channel-request-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/ChannelDispatcher>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/ChannelRequestHints>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingReady>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingChannelRequest::Private
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
 * \ingroup clientchannelrequest
 * \headerfile TelepathyQt/pending-channel-request.h <TelepathyQt/PendingChannelRequest>
 *
 * \brief The PendingChannelRequest class represents the parameters of and
 * the reply to an asynchronous ChannelRequest request.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is through Account.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingChannelRequest object.
 *
 * \param account Account to use.
 * \param requestedProperties A dictionary containing the desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \param create Whether createChannel or ensureChannel should be called.
 * \param account The account the request was made through.
 */
PendingChannelRequest::PendingChannelRequest(const AccountPtr &account,
        const QVariantMap &requestedProperties, const QDateTime &userActionTime,
        const QString &preferredHandler, bool create, const ChannelRequestHints &hints)
    : PendingOperation(account),
      mPriv(new Private(account->dbusConnection()))
{
    QString channelDispatcherObjectPath =
        QString(QLatin1String("/%1")).arg(TP_QT_IFACE_CHANNEL_DISPATCHER);
    channelDispatcherObjectPath.replace(QLatin1String("."), QLatin1String("/"));
    Client::ChannelDispatcherInterface *channelDispatcherInterface =
        account->dispatcherInterface();

    QDBusPendingCallWatcher *watcher = 0;
    if (create) {
        if (hints.isValid()) {
            if (account->supportsRequestHints()) {
                watcher = new QDBusPendingCallWatcher(
                    channelDispatcherInterface->CreateChannelWithHints(
                        QDBusObjectPath(account->objectPath()),
                        requestedProperties,
                        userActionTime.isNull() ? 0 : userActionTime.toTime_t(),
                        preferredHandler, hints.allHints()), this);
            } else {
                warning() << "Hints passed to channel request won't have an effect"
                    << "because the Channel Dispatcher service in use is too old";
            }
        }

        if (!watcher) {
            watcher = new QDBusPendingCallWatcher(
                    channelDispatcherInterface->CreateChannel(
                        QDBusObjectPath(account->objectPath()),
                        requestedProperties,
                        userActionTime.isNull() ? 0 : userActionTime.toTime_t(),
                        preferredHandler), this);
        }
    } else {
        if (hints.isValid()) {
            if (account->supportsRequestHints()) {
                watcher = new QDBusPendingCallWatcher(
                    channelDispatcherInterface->EnsureChannelWithHints(
                        QDBusObjectPath(account->objectPath()),
                        requestedProperties,
                        userActionTime.isNull() ? 0 : userActionTime.toTime_t(),
                        preferredHandler, hints.allHints()), this);
            } else {
                warning() << "Hints passed to channel request won't have an effect"
                    << "because the Channel Dispatcher service in use is too old";
            }
        }

        if (!watcher) {
            watcher = new QDBusPendingCallWatcher(
                    channelDispatcherInterface->EnsureChannel(
                        QDBusObjectPath(account->objectPath()),
                        requestedProperties,
                        userActionTime.isNull() ? 0 : userActionTime.toTime_t(),
                        preferredHandler), this);
        }
    }

    // TODO: This is a Qt bug fixed upstream, should be in the next Qt release.
    //       We should not need to check watcher->isFinished() here, remove the
    //       check when we depend on the fixed Qt version.
    if (watcher->isFinished()) {
        onWatcherFinished(watcher);
    } else {
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onWatcherFinished(QDBusPendingCallWatcher*)));
    }
}

/**
 * Construct a new PendingChannelRequest object that always fails.
 *
 * \param account Account to use.
 * \param errorName The name of a D-Bus error.
 * \param errorMessage The error message.
 */
PendingChannelRequest::PendingChannelRequest(const AccountPtr &account,
        const QString &errorName, const QString &errorMessage)
    : PendingOperation(ConnectionPtr()),
      mPriv(new Private(account->dbusConnection()))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingChannelRequest::~PendingChannelRequest()
{
    delete mPriv;
}

/**
 * Return the account through which the request was made.
 *
 * \return A pointer to the Account object.
 */
AccountPtr PendingChannelRequest::account() const
{
    return AccountPtr(qobject_cast<Account*>((Account*) object().data()));
}

ChannelRequestPtr PendingChannelRequest::channelRequest() const
{
    return mPriv->channelRequest;
}

PendingOperation *PendingChannelRequest::cancel()
{
    if (isFinished()) {
        // CR has already succeeded or failed, so let's just fail here
        return new PendingFailure(TP_QT_DBUS_ERROR_UNKNOWN_METHOD,
                QLatin1String("ChannnelRequest already finished"),
                object());
    }

    if (!mPriv->cancelOperation) {
        mPriv->cancelOperation = new PendingChannelRequestCancelOperation();
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

        if (!account().isNull()) {
            mPriv->channelRequest = ChannelRequest::create(account(),
                    objectPath.path(), QVariantMap());
        }

        if (mPriv->cancelOperation) {
            mPriv->cancelOperation->go(mPriv->channelRequest);
        } else {
            emit channelRequestCreated(mPriv->channelRequest);

            connect(mPriv->channelRequest.data(),
                    SIGNAL(failed(QString,QString)),
                    SLOT(setFinishedWithError(QString,QString)));
            connect(mPriv->channelRequest.data(),
                    SIGNAL(succeeded(Tp::ChannelPtr)),
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
    if (!isFinished()) {
        setFinishedWithError(TP_QT_ERROR_CANCELLED,
                QLatin1String("ChannelRequest cancelled"));
    }
}

} // Tp
