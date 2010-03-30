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

#include <TelepathyQt4/AccountManager>

#include "TelepathyQt4/_gen/account-manager.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-manager.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-manager-body.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Constants>

#include <QQueue>
#include <QSet>
#include <QTimer>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT AccountManager::Private
{
    Private(AccountManager *parent);
    ~Private();

    void init();

    static void introspectMain(Private *self);

    void setAccountPaths(QSet<QString> &set, const QVariant &variant);

    // Public object
    AccountManager *parent;

    // Instance of generated interface class
    Client::AccountManagerInterface *baseInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    QSet<QString> validAccountPaths;
    QSet<QString> invalidAccountPaths;
    QMap<QString, AccountPtr> accounts;
    QStringList supportedAccountProperties;
};

AccountManager::Private::Private(AccountManager *parent)
    : parent(parent),
      baseInterface(new Client::AccountManagerInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      readinessHelper(parent->readinessHelper())
{
    debug() << "Creating new AccountManager:" << parent->busName();

    ReadinessHelper::Introspectables introspectables;

    // As AccountManager does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(Features() << FeatureCore);

    init();
}

AccountManager::Private::~Private()
{
    delete baseInterface;
}

void AccountManager::Private::setAccountPaths(QSet<QString> &set,
        const QVariant &variant)
{
    ObjectPathList paths = qdbus_cast<ObjectPathList>(variant);

    if (paths.size() == 0) {
        // maybe the AccountManager is buggy, like Mission Control
        // 5.0.beta45, and returns an array of strings rather than
        // an array of object paths?

        QStringList wronglyTypedPaths = qdbus_cast<QStringList>(variant);
        if (wronglyTypedPaths.size() > 0) {
            warning() << "AccountManager returned wrong type "
                "(expected 'ao', got 'as'); working around it";
            foreach (QString path, wronglyTypedPaths) {
                set << path;
            }
        }
    }
    else {
        foreach (const QDBusObjectPath &path, paths) {
            set << path.path();
        }
    }
}

/**
 * \class AccountManager
 * \ingroup clientam
 * \headerfile TelepathyQt4/account-manager.h <TelepathyQt4/AccountManager>
 *
 * \brief The AccountManager class provides an object representing a Telepathy
 * account manager.
 *
 * AccountManager adds the following features compared to using
 * Client::AccountManagerInterface directly:
 * <ul>
 *  <li>Account status tracking</li>
 *  <li>Getting the list of supported interfaces automatically</li>
 *  <li>Cache Account objects when they are requested the first time</li>
 * </ul>
 *
 * The remote object accessor functions on this object (allAccountPaths(),
 * allAccounts(), and so on) don't make any D-Bus calls; instead, they return/use
 * values cached from a previous introspection run. The introspection process
 * populates their values in the most efficient way possible based on what the
 * service implements. Their return value is mostly undefined until the
 * introspection process is completed, i.e. isReady() returns true. See the
 * individual accessor descriptions for more details.
 *
 * Signals are emitted to indicate that accounts are added/removed and when
 * accounts validity changes. See accountCreated(), accountRemoved(),
 * accountValidityChanged().
 *
 * \section am_usage_sec Usage
 *
 * \subsection am_create_sec Creating an account manager object
 *
 * One way to create an AccountManager object is to just call the create method.
 * For example:
 *
 * \code AccountManagerPtr am = AccountManager::create(); \endcode
 *
 * An AccountManagerPtr object is returned, which will automatically keep
 * track of object lifetime.
 *
 * You can also provide a D-Bus connection as a QDBusConnection:
 *
 * \code AccountManagerPtr am = AccountManager::create(QDBusConnection::sessionBus()); \endcode
 *
 * \subsection am_ready_sec Making account manager ready to use
 *
 * An AccountManager object needs to become ready before usage, meaning that the
 * introspection process finished and the object accessors can be used.
 *
 * To make the object ready, use becomeReady() and wait for the
 * PendingOperation::finished() signal to be emitted.
 *
 * \code
 *
 * class MyClass : public QObject
 * {
 *     QOBJECT
 *
 * public:
 *     MyClass(QObject *parent = 0);
 *     ~MyClass() { }
 *
 * private Q_SLOTS:
 *     void onAccountManagerReady(Tp::PendingOperation*);
 *
 * private:
 *     AccountManagerPtr am;
 * };
 *
 * MyClass::MyClass(QObject *parent)
 *     : QObject(parent)
 *       am(AccountManager::create())
 * {
 *     connect(am->becomeReady(),
 *             SIGNAL(finished(Tp::PendingOperation*)),
 *             SLOT(onAccountManagerReady(Tp::PendingOperation*)));
 * }
 *
 * void MyClass::onAccountManagerReady(Tp::PendingOperation *op)
 * {
 *     if (op->isError()) {
 *         qWarning() << "Account manager cannot become ready:" <<
 *             op->errorName() << "-" << op->errorMessage();
 *         return;
 *     }
 *
 *     // AccountManager is now ready
 *     qDebug() << "Valid accounts:";
 *     foreach (const QString &path, am->validAccountPaths()) {
 *         qDebug() << " path:" << path;
 *     }
 * }
 *
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * AccountManager object usable.
 *
 * Note that this feature must be enabled in order to use any AccountManager
 * method.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature AccountManager::FeatureCore = Feature(QLatin1String(AccountManager::staticMetaObject.className()), 0, true);

/**
 * Create a new AccountManager object using QDBusConnection::sessionBus().
 *
 * \return An AccountManagerPtr object pointing to the newly created
 *         AccountManager object.
 */
AccountManagerPtr AccountManager::create()
{
    return AccountManagerPtr(new AccountManager());
}

/**
 * Create a new AccountManager object using the given \a bus.
 *
 * \param bus QDBusConnection to use.
 * \return An AccountManagerPtr object pointing to the newly created
 *         AccountManager object.
 */
AccountManagerPtr AccountManager::create(const QDBusConnection &bus)
{
    return AccountManagerPtr(new AccountManager(bus));
}

/**
 * Construct a new AccountManager object using QDBusConnection::sessionBus().
 */
AccountManager::AccountManager()
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH)),
      OptionalInterfaceFactory<AccountManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
}

/**
 * Construct a new AccountManager object using the given \a bus.
 *
 * \param bus QDBusConnection to use.
 */
AccountManager::AccountManager(const QDBusConnection& bus)
    : StatelessDBusProxy(bus,
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH)),
      OptionalInterfaceFactory<AccountManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
AccountManager::~AccountManager()
{
    delete mPriv;
}

/**
 * Return a list of object paths for all valid accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::validAccountPaths() const
{
    return mPriv->validAccountPaths.values();
}

/**
 * Return a list of object paths for all invalid accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::invalidAccountPaths() const
{
    return mPriv->invalidAccountPaths.values();
}

/**
 * Return a list of object paths for all accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::allAccountPaths() const
{
    QStringList result;
    result.append(mPriv->validAccountPaths.values());
    result.append(mPriv->invalidAccountPaths.values());
    return result;
}

/**
 * Return a list of AccountPtr objects for all valid accounts.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using them.
 *
 * \return A list of AccountPtr objects.
 * \sa invalidAccounts(), allAccounts(), accountsForPaths()
 */
QList<AccountPtr> AccountManager::validAccounts()
{
    return accountsForPaths(validAccountPaths());
}

/**
 * Return a list of AccountPtr objects for all invalid accounts.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using them.
 *
 * \return A list of AccountPtr objects.
 * \sa validAccounts(), allAccounts(), accountsForPaths()
 */
QList<AccountPtr> AccountManager::invalidAccounts()
{
    return accountsForPaths(invalidAccountPaths());
}

/**
 * Return a list of AccountPtr objects for all accounts.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using them.
 *
 * \return A list of AccountPtr objects.
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
QList<AccountPtr> AccountManager::allAccounts()
{
    return accountsForPaths(allAccountPaths());
}

/**
 * Return an AccountPtr object for the given \a path.
 *
 * Remember to call Account::becomeReady on the new account, to
 * make sure it is ready before using it.
 *
 * \param path The account object path.
 * \return An AccountPtr object pointing to the Account object for the given
 *         \a path, or a null AccountPtr if \a path is invalid.
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
AccountPtr AccountManager::accountForPath(const QString &path)
{
    if (mPriv->accounts.contains(path)) {
        return mPriv->accounts[path];
    }

    if (!mPriv->validAccountPaths.contains(path) &&
        !mPriv->invalidAccountPaths.contains(path)) {
        return AccountPtr();
    }

    AccountPtr account = Account::create(dbusConnection(), busName(), path);
    mPriv->accounts[path] = account;
    return account;
}

/**
 * Return a list of AccountPtr objects for the given \a paths.
 *
 * The returned list will have one AccountPtr object for each given path. If
 * a given path is invalid the returned AccountPtr object will point to 0.
 * AccountPtr::isNull() will return true.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using them.
 *
 * \param paths List of accounts object paths.
 * \return A list of AccountPtr objects.
 * \sa validAccounts(), invalidAccounts(), allAccounts(), accountForPath()
 */
QList<AccountPtr> AccountManager::accountsForPaths(const QStringList &paths)
{
    QList<AccountPtr> result;
    foreach (const QString &path, paths) {
        result << accountForPath(path);
    }
    return result;
}

/**
 * Return a list of the fully qualified names of properties that can be set
 * when calling createAccount().
 *
 * \return A list of fully qualified D-Bus property names,
 *         such as "org.freedesktop.Telepathy.Account.Enabled"
 * \sa createAccount()
 */
QStringList AccountManager::supportedAccountProperties() const
{
    return mPriv->supportedAccountProperties;
}

/**
 * Create an account with the given parameters.
 *
 * The optional properties argument can be used to set any property listed in
 * supportedAccountProperties() at the time the account is created.
 *
 * \param connectionManager Name of the connection manager to create the account
 *                          for.
 * \param protocol Name of the protocol to create the account for.
 * \param displayName The account display name.
 * \param parameters The account parameters.
 * \param properties An optional map from fully qualified D-Bus property
 *                   names such as "org.freedesktop.Telepathy.Account.Enabled"
 *                   to their values.
 * \return A PendingAccount object which will emit PendingAccount::finished
 *         when the account has been created of failed its creation process.
 * \sa supportedAccountProperties()
 */
PendingAccount *AccountManager::createAccount(const QString &connectionManager,
        const QString &protocol, const QString &displayName,
        const QVariantMap &parameters, const QVariantMap &properties)
{
    return new PendingAccount(AccountManagerPtr(this), connectionManager,
            protocol, displayName, parameters, properties);
}

/**
 * \fn void AccountManager::accountCreated(const QString &path)
 *
 * This signal is emitted when a new account is created.
 *
 * \param path The object path of the newly created account.
 * \sa accountForPath()
 */

/**
 * \fn void AccountManager::accountRemoved(const QString &path)
 *
 * This signal is emitted when an account gets removed.
 *
 * \param path The object path of the removed account.
 * \sa accountForPath()
 */

/**
 * \fn void AccountManager::accountValidityChanged(const QString &path, bool valid)
 *
 * This signal is emitted when an account validity changes.
 *
 * \param path The object path of the account in which the validity changed.
 * \param valid Whether the account is valid or not.
 * \sa accountForPath()
 */

/**
 * \fn Client::DBus::PropertiesInterface *AccountManager::propertiesInterface() const
 *
 * Convenience function for getting a PropertiesInterface interface proxy object
 * for this account manager. The AccountManager interface relies on properties,
 * so this interface is always assumed to be present.
 *
 * \return A pointer to the existing Client::DBus::PropertiesInterface object
 *         for this AccountManager object.
 */

/**
 * Return the Client::AccountManagerInterface interface proxy object for this
 * account manager. This method is protected since the convenience methods
 * provided by this class should generally be used instead of calling D-Bus
 * methods directly.
 *
 * \return A pointer to the existing Client::AccountManagerInterface object for
 *         this AccountManager object.
 */
Client::AccountManagerInterface *AccountManager::baseInterface() const
{
    return mPriv->baseInterface;
}

/**** Private ****/
void AccountManager::Private::init()
{
    if (!parent->isValid()) {
        return;
    }

    parent->connect(baseInterface,
            SIGNAL(AccountValidityChanged(const QDBusObjectPath &, bool)),
            SLOT(onAccountValidityChanged(const QDBusObjectPath &, bool)));
    parent->connect(baseInterface,
            SIGNAL(AccountRemoved(const QDBusObjectPath &)),
            SLOT(onAccountRemoved(const QDBusObjectPath &)));
}

void AccountManager::Private::introspectMain(AccountManager::Private *self)
{
    Client::DBus::PropertiesInterface *properties = self->parent->propertiesInterface();
    Q_ASSERT(properties != 0);

    debug() << "Calling Properties::GetAll(AccountManager)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_MANAGER)),
            self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher *)));
}

void AccountManager::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(AccountManager)";
        props = reply.value();

        if (props.contains(QLatin1String("Interfaces"))) {
            setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
        }
        if (props.contains(QLatin1String("ValidAccounts"))) {
            mPriv->setAccountPaths(mPriv->validAccountPaths,
                    props[QLatin1String("ValidAccounts")]);
        }
        if (props.contains(QLatin1String("InvalidAccounts"))) {
            mPriv->setAccountPaths(mPriv->invalidAccountPaths,
                    props[QLatin1String("InvalidAccounts")]);
        }
        if (props.contains(QLatin1String("SupportedAccountProperties"))) {
            mPriv->supportedAccountProperties =
                qdbus_cast<QStringList>(props[QLatin1String("SupportedAccountProperties")]);
        }
    } else {
        warning().nospace() <<
            "GetAll(AccountManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);

    watcher->deleteLater();
}

void AccountManager::onAccountValidityChanged(const QDBusObjectPath &objectPath,
        bool nowValid)
{
    QString path = objectPath.path();
    bool newAccount = false;

    if (!mPriv->validAccountPaths.contains(path) &&
        !mPriv->invalidAccountPaths.contains(path)) {
        newAccount = true;
    }

    if (nowValid) {
        debug() << "Account created or became valid:" << path;
        mPriv->invalidAccountPaths.remove(path);
        mPriv->validAccountPaths.insert(path);
    }
    else {
        debug() << "Account became invalid:" << path;
        mPriv->validAccountPaths.remove(path);
        mPriv->invalidAccountPaths.insert(path);
    }

    if (newAccount) {
        emit accountCreated(path);
        // if the newly created account is invalid (shouldn't be the case)
        // emit also accountValidityChanged indicating this
        if (!nowValid) {
            emit accountValidityChanged(path, nowValid);
        }
    }
    else {
        emit accountValidityChanged(path, nowValid);
    }
}

void AccountManager::onAccountRemoved(const QDBusObjectPath &objectPath)
{
    QString path = objectPath.path();

    debug() << "Account removed:" << path;
    mPriv->validAccountPaths.remove(path);
    mPriv->invalidAccountPaths.remove(path);
    if (mPriv->accounts.contains(path)) {
        mPriv->accounts.remove(path);
    }
    emit accountRemoved(path);
}

} // Tp
