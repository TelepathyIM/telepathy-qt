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

#include <TelepathyQt/PendingAccount>

#include "TelepathyQt/_gen/pending-account.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/PendingReady>

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingAccount::Private
{
    AccountPtr account;
};

/**
 * \class PendingAccount
 * \ingroup clientaccount
 * \headerfile TelepathyQt/pending-account.h <TelepathyQt/PendingAccount>
 *
 * \brief The PendingAccount class represents the parameters of and the reply to
 * an asynchronous account request.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via AccountManager.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingAccount object.
 *
 * \param manager AccountManager to use.
 * \param connectionManager Name of the connection manager to create the account
 *                          for.
 * \param protocol Name of the protocol to create the account for.
 * \param displayName Account display name.
 * \param parameters Account parameters.
 * \param properties An optional map from fully qualified D-Bus property
 *                   names such as "org.freedesktop.Telepathy.Account.Enabled"
 *                   to their values.
 */
PendingAccount::PendingAccount(const AccountManagerPtr &manager,
        const QString &connectionManager, const QString &protocol,
        const QString &displayName, const QVariantMap &parameters,
        const QVariantMap &properties)
    : PendingOperation(manager),
      mPriv(new Private)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            manager->baseInterface()->CreateAccount(connectionManager,
                protocol, displayName, parameters, properties), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onCallFinished(QDBusPendingCallWatcher*)));
}

/**
 * Class destructor.
 */
PendingAccount::~PendingAccount()
{
    delete mPriv;
}

/**
 * Return the account manager through which the request was made.
 *
 * \return A pointer to the AccountManager object.
 */
AccountManagerPtr PendingAccount::manager() const
{
    return AccountManagerPtr(qobject_cast<AccountManager *>((AccountManager*) object().data()));
}

/**
 * Return the newly created account.
 *
 * \return A pointer to an Account object, or a null AccountPtr if an error occurred.
 */
AccountPtr PendingAccount::account() const
{
    if (!isFinished()) {
        warning() << "PendingAccount::account called before finished, returning 0";
        return AccountPtr();
    } else if (!isValid()) {
        warning() << "PendingAccount::account called when not valid, returning 0";
        return AccountPtr();
    }

    return mPriv->account;
}

void PendingAccount::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    if (!reply.isError()) {
        QString objectPath = reply.value().path();

        debug() << "Got reply to AccountManager.CreateAccount - object path:" << objectPath;

        PendingReady *readyOp = manager()->accountFactory()->proxy(manager()->busName(),
                objectPath, manager()->connectionFactory(),
                manager()->channelFactory(), manager()->contactFactory());
        mPriv->account = AccountPtr::qObjectCast(readyOp->proxy());
        connect(readyOp,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAccountBuilt(Tp::PendingOperation*)));
    } else {
        debug().nospace() <<
            "CreateAccount failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

void PendingAccount::onAccountBuilt(Tp::PendingOperation *op)
{
    Q_ASSERT(op->isFinished());

    if (op->isError()) {
        warning() << "Making account ready using the factory failed:" <<
            op->errorName() << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        // AM is stateless, so the only way for it to become invalid is in the introspection phase,
        // and a PendingAccount should never be created if AM introspection hasn't succeeded
        Q_ASSERT(!manager().isNull() && manager()->isValid());

        if (manager()->allAccounts().contains(mPriv->account)) {
            setFinished();
            debug() << "New account" << mPriv->account->objectPath() << "built";
        } else {
            // Have to wait for the AM to pick up the change and signal it so the world can be
            // assumed to be ~round when we finish
            connect(manager().data(),
                    SIGNAL(newAccount(Tp::AccountPtr)),
                    SLOT(onNewAccount(Tp::AccountPtr)));
        }
    }
}

void PendingAccount::onNewAccount(const AccountPtr &account)
{
    if (account != mPriv->account) {
        return;
    }

    debug() << "Account" << account->objectPath() << "added to AM, finishing PendingAccount";
    setFinished();
}

} // Tp
