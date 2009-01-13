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

#include <TelepathyQt4/Client/AccountManager>
#include "TelepathyQt4/Client/account-manager-internal.h"

#include "TelepathyQt4/Client/_gen/account-manager.moc.hpp"
#include "TelepathyQt4/Client/_gen/account-manager-internal.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-manager.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-manager-body.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/PendingAccount>
#include <TelepathyQt4/Client/PendingSuccess>
#include <TelepathyQt4/Constants>

#include <QQueue>
#include <QSet>
#include <QTimer>

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

/**
 * \defgroup clientaccount Account and Account Manager proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing the Telepathy Account Manager and the Accounts
 * that it manages, and their optional interfaces.
 */

namespace Telepathy
{
namespace Client
{

AccountManager::Private::PendingReady::PendingReady(AccountManager *parent)
    : PendingOperation(parent)
{
}

AccountManager::Private::Private(AccountManager *parent)
    : QObject(parent),
      baseInterface(new AccountManagerInterface(parent->dbusConnection(),
                            parent->busName(), parent->objectPath(), parent)),
      ready(false),
      pendingReady(0),
      features(0)
{
    debug() << "Creating new AccountManager:" << parent->busName();

    connect(baseInterface,
            SIGNAL(AccountValidityChanged(const QDBusObjectPath &, bool)),
            SLOT(onAccountValidityChanged(const QDBusObjectPath &, bool)));
    connect(baseInterface,
            SIGNAL(AccountRemoved(const QDBusObjectPath &)),
            SLOT(onAccountRemoved(const QDBusObjectPath &)));

    introspectQueue.enqueue(&Private::callGetAll);
    QTimer::singleShot(0, this, SLOT(continueIntrospection()));
}

AccountManager::Private::~Private()
{
    delete baseInterface;
}

void AccountManager::Private::callGetAll()
{
    debug() << "Calling Properties::GetAll(AccountManager)";
    AccountManager *am = static_cast<AccountManager *>(parent());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            am->propertiesInterface()->GetAll(
                TELEPATHY_INTERFACE_ACCOUNT_MANAGER), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onGetAllAccountManagerReturn(QDBusPendingCallWatcher *)));
}

void AccountManager::Private::setAccountPaths(QSet<QDBusObjectPath> &set,
        const QVariant &variant)
{
    Telepathy::ObjectPathList paths = qdbus_cast<Telepathy::ObjectPathList>(variant);

    if (paths.size() == 0) {
        // maybe the AccountManager is buggy, like Mission Control
        // 5.0.beta45, and returns an array of strings rather than
        // an array of object paths?

        QStringList wronglyTypedPaths = qdbus_cast<QStringList>(variant);
        if (wronglyTypedPaths.size() > 0) {
            warning() << "AccountManager returned wrong type "
                "(expected 'ao', got 'as'); working around it";
            Q_FOREACH (QString path, wronglyTypedPaths) {
                paths << QDBusObjectPath(path);
            }
        }
    }

    Q_FOREACH (const QDBusObjectPath &path, paths) {
        set << path;
    }
}

void AccountManager::Private::onGetAllAccountManagerReturn(
        QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(AccountManager)";
        props = reply.value();
    } else {
        warning().nospace() <<
            "GetAll(AccountManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    if (props.contains("Interfaces")) {
        interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    }

    if (props.contains("ValidAccounts")) {
        setAccountPaths(validAccountPaths,
                props["ValidAccounts"]);
    }

    if (props.contains("InvalidAccounts")) {
        setAccountPaths(invalidAccountPaths,
                props["InvalidAccounts"]);
    }

    continueIntrospection();

    watcher->deleteLater();
}

void AccountManager::Private::onAccountValidityChanged(const QDBusObjectPath &path,
        bool nowValid)
{
    bool newAccount = false;

    if (!validAccountPaths.contains(path) && !invalidAccountPaths.contains(path)) {
        newAccount = true;
    }

    if (nowValid) {
        debug() << "Account created or became valid:" << path.path();
        invalidAccountPaths.remove(path);
        validAccountPaths.insert(path);
    }
    else {
        debug() << "Account became invalid:" << path.path();
        validAccountPaths.remove(path);
        invalidAccountPaths.insert(path);
    }

    if (newAccount) {
        Q_EMIT accountCreated(path);
        // if the newly created account is invalid (shouldn't be the case)
        // emit also accountValidityChanged indicating this
        if (!nowValid) {
            Q_EMIT accountValidityChanged(path, nowValid);
        }
    }
    else {
        Q_EMIT accountValidityChanged(path, nowValid);
    }
}

void AccountManager::Private::onAccountRemoved(const QDBusObjectPath &path)
{
    debug() << "Account removed:" << path.path();
    validAccountPaths.remove(path);
    invalidAccountPaths.remove(path);

    Q_EMIT accountRemoved(path);
}

void AccountManager::Private::continueIntrospection()
{
    if (!ready) {
        if (introspectQueue.isEmpty()) {
            debug() << "AccountManager is ready";
            ready = true;

            if (pendingReady) {
                pendingReady->setFinished();
                // it will delete itself later
                pendingReady = 0;
            }
        }
        else {
            (this->*(introspectQueue.dequeue()))();
        }
    }
}

/**
 * \class AccountManager
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/Client/account-manager.h> <TelepathyQt4/Client/AccountManager>
 *
 * Object representing a Telepathy account manager.
 */

/**
 * Construct a new AccountManager object.
 *
 * \param parent Object parent.
 */
AccountManager::AccountManager(QObject* parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH), parent),
      mPriv(new Private(this))
{
    init();
}

/**
 * Construct a new AccountManager object.
 *
 * \param bus QDBusConnection to use.
 * \param parent Object parent.
 */
AccountManager::AccountManager(const QDBusConnection& bus,
        QObject* parent)
    : StatelessDBusProxy(bus,
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH), parent),
      mPriv(new Private(this))
{
    init();
}

/**
 * Class destructor.
 */
AccountManager::~AccountManager()
{
    delete mPriv;
}

QStringList AccountManager::interfaces() const
{
    return mPriv->interfaces;
}

/**
 * \fn DBus::propertiesInterface *AccountManager::propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * AccountManager interface relies on properties, so this interface is
 * always assumed to be present.
 */

/**
 * Return a list of QDBusObjectPath for all valid accounts.
 *
 * \return A list of QDBusObjectPath.
 */
Telepathy::ObjectPathList AccountManager::validAccountPaths() const
{
    Telepathy::ObjectPathList result;
    result.append(mPriv->validAccountPaths.values());
    return result;
}

/**
 * Return a list of QDBusObjectPath for all invalid accounts.
 *
 * \return A list of QDBusObjectPath.
 */
Telepathy::ObjectPathList AccountManager::invalidAccountPaths() const
{
    Telepathy::ObjectPathList result;
    result.append(mPriv->invalidAccountPaths.values());
    return result;
}

/**
 * Return a list of QDBusObjectPath for all accounts.
 *
 * \return A list of QDBusObjectPath.
 */
Telepathy::ObjectPathList AccountManager::allAccountPaths() const
{
    ObjectPathList result;
    result.append(mPriv->validAccountPaths.values());
    result.append(mPriv->invalidAccountPaths.values());
    return result;
}

/**
 * Return a list of Account objects for all valid accounts.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * \return A list of Account objects
 * \sa invalidAccounts(), allAccounts(), accountsForPaths()
 */
QList<Account *> AccountManager::validAccounts()
{
    return accountsForPaths(validAccountPaths());
}

/**
 * Return a list of Account objects for all invalid accounts.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * \return A list of Account objects
 * \sa validAccounts(), allAccounts(), accountsForPaths()
 */
QList<Account *> AccountManager::invalidAccounts()
{
    return accountsForPaths(invalidAccountPaths());
}

/**
 * Return a list of Account objects for all accounts.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * \return A list of Account objects
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
QList<Account *> AccountManager::allAccounts()
{
    return accountsForPaths(allAccountPaths());
}

/**
 * Return an Account object for the given \a path.
 *
 * Note that the Account object won't be cached by account manager, and
 * should be done by the application itself.
 *
 * \param path A QDBusObjectPath to create account for.
 * \return A list of Account objects
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
Account *AccountManager::accountForPath(const QDBusObjectPath &path)
{
    // TODO should we use AM as parent of account,
    //      or receive parent as a param?
    return new Account(this, path, this);
}

/**
 * Return a list of Account objects for the given \a paths.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * \param paths List of QDBusObjectPath to create accounts for.
 * \return A list of Account objects
 * \sa validAccounts(), invalidAccounts(), allAccounts()
 */
QList<Account *> AccountManager::accountsForPaths(const QList<QDBusObjectPath> &paths)
{
    QList<Account *> result;
    Q_FOREACH (const QDBusObjectPath &path, paths) {
        result << accountForPath(path);
    }
    return result;
}

/**
 * Create an Account with the given parameters.
 *
 * Return a pending operation representing the Account object which will succeed
 * when the account has been created or fail if an error occurred.
 *
 * \param connectionManager Name of the connection manager to create the account for.
 * \param protocol Name of the protocol to create the account for.
 * \param displayName Account display name.
 * \param parameters Account parameters.
 * \return A PendingOperation which will emit PendingAccount::finished
 *         when the account has been created of failed its creation process.
 */
PendingAccount *AccountManager::createAccount(const QString &connectionManager,
        const QString &protocol, const QString &displayName,
        const QVariantMap &parameters)
{
    return new PendingAccount(this, connectionManager,
            protocol, displayName, parameters);
}

/**
 * Return whether this object has finished its initial setup.
 *
 * This is mostly useful as a sanity check, in code that shouldn't be run
 * until the object is ready. To wait for the object to be ready, call
 * becomeReady() and connect to the finished signal on the result.
 *
 * \param features Which features should be tested.
 * \return \c true if the object has finished initial setup.
 */
bool AccountManager::isReady(Features features) const
{
    return mPriv->ready
        && ((mPriv->features & features) == features);
}

/**
 * Return a pending operation which will succeed when this object finishes
 * its initial setup, or will fail if a fatal error occurs during this
 * initial setup.
 *
 * \param features Which features should be tested.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when this object has finished or failed its initial setup.
 */
PendingOperation *AccountManager::becomeReady(Features features)
{
    if (isReady(features)) {
        return new PendingSuccess(this);
    }

    if (features != 0) {
        return new PendingFailure(this, "org.freedesktop.Telepathy.Qt.DoesntWork",
                "Unimplemented");
    }

    if (!mPriv->pendingReady) {
        mPriv->pendingReady = new Private::PendingReady(this);
    }
    return mPriv->pendingReady;
}

/**
 * Get the AccountManagerInterface for this AccountManager. This
 * method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing AccountManagerInterface for this
 *         AccountManager.
 */
AccountManagerInterface *AccountManager::baseInterface() const
{
    return mPriv->baseInterface;
}

void AccountManager::init()
{
    connect(mPriv,
            SIGNAL(accountCreated(const QDBusObjectPath &)),
            SIGNAL(accountCreated(const QDBusObjectPath &)));
    connect(mPriv,
            SIGNAL(accountRemoved(const QDBusObjectPath &)),
            SIGNAL(accountRemoved(const QDBusObjectPath &)));
    connect(mPriv,
            SIGNAL(accountValidityChanged(const QDBusObjectPath &, bool)),
            SIGNAL(accountValidityChanged(const QDBusObjectPath &, bool)));
}

} // Telepathy::Client
} // Telepathy
