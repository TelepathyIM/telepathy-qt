/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2010 Nokia Corporation
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

#include <TelepathyQt4/AccountSet>
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
    Private(AccountManager *parent, const AccountFactoryConstPtr &accFactory,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory);
    ~Private();

    void init();

    static void introspectMain(Private *self);

    void checkIntrospectionCompleted();

    QSet<QString> getAccountPathsFromProp(const QVariant &prop);
    QSet<QString> getAccountPathsFromProps(const QVariantMap &props);
    void addAccountForPath(const QString &accountObjectPath);

    AccountSetPtr filterAccountsByRequestableChannelClasses(
            const RequestableChannelClassList &rccs,
            const QVariantMap &additionalFilter) const;

    // Public object
    AccountManager *parent;

    // Instance of generated interface class
    Client::AccountManagerInterface *baseInterface;

    ReadinessHelper *readinessHelper;

    AccountFactoryConstPtr accFactory;
    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Introspection
    QHash<QString, AccountPtr> incompleteAccounts;
    QHash<QString, AccountPtr> accounts;
    QStringList supportedAccountProperties;
};

AccountManager::Private::Private(AccountManager *parent,
        const AccountFactoryConstPtr &accFactory, const ConnectionFactoryConstPtr &connFactory,
        const ChannelFactoryConstPtr &chanFactory, const ContactFactoryConstPtr &contactFactory)
    : parent(parent),
      baseInterface(new Client::AccountManagerInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      readinessHelper(parent->readinessHelper()),
      accFactory(accFactory),
      connFactory(connFactory),
      chanFactory(chanFactory),
      contactFactory(contactFactory)
{
    debug() << "Creating new AccountManager:" << parent->busName();

    if (accFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the account factory is not the proxy connection";
    }

    if (connFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the connection factory is not the proxy connection";
    }

    if (chanFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the channel factory is not the proxy connection";
    }

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

void AccountManager::Private::checkIntrospectionCompleted()
{
    if (!parent->isReady(FeatureCore) &&
        incompleteAccounts.size() == 0) {
        readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

QSet<QString> AccountManager::Private::getAccountPathsFromProp(
        const QVariant &prop)
{
    QSet<QString> set;

    ObjectPathList paths = qdbus_cast<ObjectPathList>(prop);
    if (paths.size() == 0) {
        /* maybe the AccountManager is buggy, like Mission Control
         * 5.0.beta45, and returns an array of strings rather than
         * an array of object paths? */
        QStringList wronglyTypedPaths = qdbus_cast<QStringList>(prop);
        if (wronglyTypedPaths.size() > 0) {
            warning() << "AccountManager returned wrong type for"
                "Valid/InvalidAccounts (expected 'ao', got 'as'); "
                "working around it";
            foreach (QString path, wronglyTypedPaths) {
                set << path;
            }
        }
    } else {
        foreach (const QDBusObjectPath &path, paths) {
            set << path.path();
        }
    }

    return set;
}

QSet<QString> AccountManager::Private::getAccountPathsFromProps(
        const QVariantMap &props)
{
    return getAccountPathsFromProp(props[QLatin1String("ValidAccounts")]).unite(
            getAccountPathsFromProp(props[QLatin1String("InvalidAccounts")]));
}

void AccountManager::Private::addAccountForPath(const QString &path)
{
    if (accounts.contains(path)) {
        return;
    }

    PendingReady *readyOp = accFactory->proxy(parent->busName(), path, connFactory,
            chanFactory, contactFactory);

    AccountPtr account(AccountPtr::dynamicCast(readyOp->proxy()));
    Q_ASSERT(!account.isNull());

    parent->connect(readyOp,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onAccountReady(Tp::PendingOperation *)));
    incompleteAccounts.insert(path, account);
}

AccountSetPtr AccountManager::Private::filterAccountsByRequestableChannelClasses(
        const RequestableChannelClassList &rccs,
        const QVariantMap &additionalFilter) const
{
    QVariantMap filter(additionalFilter);
    if (filter.contains(QLatin1String("rccSubset"))) {
        RequestableChannelClassList mergedRccs =
            filter[QLatin1String("rccSubset")].value<RequestableChannelClassList>();
        mergedRccs.append(rccs);
        filter.insert(QLatin1String("rccSubset"), qVariantFromValue(mergedRccs));
    } else {
        filter.insert(QLatin1String("rccSubset"), qVariantFromValue(rccs));
    }
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) parent), filter));
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
 * The instance will use an account factory creating Tp::Account objects with Account::FeatureCore
 * ready, a connection factory creating Tp::Connection objects with no features ready, and a channel
 * factory creating stock Telepathy-Qt4 channel subclasses, as appropriate, with no features ready.
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
 * The instance will use an account factory creating Tp::Account objects with Account::FeatureCore
 * ready, a connection factory creating Tp::Connection objects with no features ready, and a channel
 * factory creating stock Telepathy-Qt4 channel subclasses, as appropriate, with no features ready.
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
 * Create a new AccountManager using QDBusConnection::sessionBus() and the given factories.
 *
 * The connection and channel factories are passed to any Account objects created by this manager
 * object. In fact, they're not used directly by AccountManager at all.
 *
 * A warning is printed if the factories are for a bus different from QDBusConnection::sessionBus().
 *
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return An AccountManagerPtr object pointing to the newly created
 *         AccountManager object.
 */
AccountManagerPtr AccountManager::create(const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return AccountManagerPtr(new AccountManager(QDBusConnection::sessionBus(),
                accountFactory, connectionFactory, channelFactory, contactFactory));
}

/**
 * Create a new AccountManager using the given bus and the given factories.
 *
 * The connection and channel factories are passed to any Account objects created by this manager
 * object. In fact, they're not used directly by AccountManager at all.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \return An AccountManagerPtr object pointing to the newly created
 *         AccountManager object.
 */
AccountManagerPtr AccountManager::create(const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return AccountManagerPtr(new AccountManager(bus,
                accountFactory, connectionFactory, channelFactory, contactFactory));
}

/**
 * Construct a new AccountManager object using QDBusConnection::sessionBus().
 *
 * The instance will use an account factory creating Tp::Account objects with Account::FeatureCore
 * ready, a connection factory creating Tp::Connection objects with no features ready, and a channel
 * factory creating stock Telepathy-Qt4 channel subclasses, as appropriate, with no features ready.
 */
AccountManager::AccountManager()
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH)),
      OptionalInterfaceFactory<AccountManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(
                  this,
                  AccountFactory::create(QDBusConnection::sessionBus()),
                  ConnectionFactory::create(QDBusConnection::sessionBus()),
                  ChannelFactory::create(QDBusConnection::sessionBus()),
                  ContactFactory::create()))
{
}

/**
 * Construct a new AccountManager object using the given \a bus.
 *
 * The instance will use an account factory creating Tp::Account objects with Account::FeatureCore
 * ready, a connection factory creating Tp::Connection objects with no features ready, and a channel
 * factory creating stock Telepathy-Qt4 channel subclasses, as appropriate, with no features ready.
 *
 * \param bus QDBusConnection to use.
 */
AccountManager::AccountManager(const QDBusConnection& bus)
    : StatelessDBusProxy(bus,
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH)),
      OptionalInterfaceFactory<AccountManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(
                  this,
                  AccountFactory::create(bus),
                  ConnectionFactory::create(bus),
                  ChannelFactory::create(bus),
                  ContactFactory::create()))
{
}

/**
 * Construct a new AccountManager object using the given \a bus and the given factories.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 */
AccountManager::AccountManager(const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
    : StatelessDBusProxy(bus,
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH)),
      OptionalInterfaceFactory<AccountManager>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, accountFactory, connectionFactory, channelFactory, contactFactory))
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
 * Get the account factory used by this manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
AccountFactoryConstPtr AccountManager::accountFactory() const
{
    return mPriv->accFactory;
}

/**
 * Get the connection factory used by this manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ConnectionFactoryConstPtr AccountManager::connectionFactory() const
{
    return mPriv->connFactory;
}

/**
 * Get the channel factory used by this manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ChannelFactoryConstPtr AccountManager::channelFactory() const
{
    return mPriv->chanFactory;
}

/**
 * Get the contact factory used by this manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ContactFactoryConstPtr AccountManager::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return a list of object paths for all valid accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::validAccountPaths() const
{
    QStringList ret;
    foreach (const AccountPtr &account, mPriv->accounts) {
        if (account->isValid()) {
            ret << account->objectPath();
        }
    }
    return ret;
}

/**
 * Return a list of object paths for all invalid accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::invalidAccountPaths() const
{
    QStringList ret;
    foreach (const AccountPtr &account, mPriv->accounts) {
        if (!account->isValid()) {
            ret << account->objectPath();
        }
    }
    return ret;
}

/**
 * Return a list of object paths for all accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::allAccountPaths() const
{
    QStringList ret;
    foreach (const AccountPtr &account, mPriv->accounts) {
        ret << account->objectPath();
    }
    return ret;
}

/**
 * Return a list of AccountPtr objects for all valid accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A list of AccountPtr objects.
 * \sa invalidAccounts(), allAccounts(), accountsForPaths()
 */
QList<AccountPtr> AccountManager::validAccounts()
{
    QList<AccountPtr> ret;
    foreach (const AccountPtr &account, mPriv->accounts) {
        if (account->isValid()) {
            ret << account;
        }
    }
    return ret;
}

/**
 * Return a list of AccountPtr objects for all invalid accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A list of AccountPtr objects.
 * \sa validAccounts(), allAccounts(), accountsForPaths()
 */
QList<AccountPtr> AccountManager::invalidAccounts()
{
    QList<AccountPtr> ret;
    foreach (const AccountPtr &account, mPriv->accounts) {
        if (!account->isValid()) {
            ret << account;
        }
    }
    return ret;
}

/**
 * Return a list of AccountPtr objects for all accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A list of AccountPtr objects.
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
/* FIXME (API-BREAK) Add const qualifier */
QList<AccountPtr> AccountManager::allAccounts()
{
    QList<AccountPtr> ret;
    foreach (const AccountPtr &account, mPriv->accounts) {
        ret << account;
    }
    return ret;
}

/**
 * Return a set of accounts containing all valid accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A set of accounts containing all valid accounts.
 */
AccountSetPtr AccountManager::validAccountsSet() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("valid"), true);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all invalid accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A set of accounts containing all invalid accounts.
 */
AccountSetPtr AccountManager::invalidAccountsSet() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("valid"), false);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all enabled accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A set of accounts containing all enabled accounts.
 */
AccountSetPtr AccountManager::enabledAccountsSet() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("enabled"), true);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all disabled accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A set of accounts containing all disabled accounts.
 */
AccountSetPtr AccountManager::disabledAccountsSet() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("enabled"), false);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all online accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A set of accounts containing all online accounts.
 */
AccountSetPtr AccountManager::onlineAccountsSet() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("online"), true);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all offline accounts.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \return A set of accounts containing all offline accounts.
 */
AccountSetPtr AccountManager::offlineAccountsSet() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("online"), false);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier and that match the given \a additionalFilter
 * criteria.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param additionalFilter Additional filter.
 * \return A set of accounts containing all accounts that support text chats by
 *         providing a contact identifier.
 */
AccountSetPtr AccountManager::supportsTextChatsAccountsSet(
        const QVariantMap &additionalFilter) const
{
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rccs.append(rcc);

    return mPriv->filterAccountsByRequestableChannelClasses(rccs, additionalFilter);
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier and that match the given \a additionalFilter
 * criteria.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param additionalFilter Additional filter.
 * \return A set of accounts containing all accounts that support text chat rooms.
 */
AccountSetPtr AccountManager::supportsTextChatroomsAccountsSet(
        const QVariantMap &additionalFilter) const
{
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeRoom);
    rccs.append(rcc);

    return mPriv->filterAccountsByRequestableChannelClasses(rccs, additionalFilter);
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier and that match the given \a additionalFilter
 * criteria.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param additionalFilter Additional filter.
 * \return A set of accounts containing all accounts that support media calls by
 *         providing a contact identifier.
 */
AccountSetPtr AccountManager::supportsMediaCallsAccountsSet(
        const QVariantMap &additionalFilter) const
{
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rccs.append(rcc);

    return mPriv->filterAccountsByRequestableChannelClasses(rccs, additionalFilter);
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier and that match the given \a additionalFilter
 * criteria.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param additionalFilter Additional filter.
 * \return A set of accounts containing all accounts that support audio calls by
 *         providing a contact identifier.
 */
AccountSetPtr AccountManager::supportsAudioCallsAccountsSet(
        const QVariantMap &additionalFilter) const
{
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio"));
    rccs.append(rcc);

    return mPriv->filterAccountsByRequestableChannelClasses(rccs, additionalFilter);
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier and that match the given \a additionalFilter
 * criteria.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param additionalFilter Additional filter.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \return A set of accounts containing all accounts that support video calls by
 *         providing a contact identifier.
 */
AccountSetPtr AccountManager::supportsVideoCallsAccountsSet(bool withAudio,
        const QVariantMap &additionalFilter) const
{
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rcc.allowedProperties.append(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialVideo"));
    if (withAudio) {
        rcc.allowedProperties.append(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio"));
    }
    rccs.append(rcc);

    return mPriv->filterAccountsByRequestableChannelClasses(rccs, additionalFilter);
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier and that match the given \a additionalFilter
 * criteria.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param additionalFilter Additional filter.
 * \return A set of accounts containing all accounts that support file transfers by
 *         providing a contact identifier.
 */
AccountSetPtr AccountManager::supportsFileTransfersAccountsSet(
        const QVariantMap &additionalFilter) const
{
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rccs.append(rcc);

    return mPriv->filterAccountsByRequestableChannelClasses(rccs, additionalFilter);
}

/**
 * Return a set of accounts containing all accounts for the given \a
 * protocolName.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param protocolName The name of the protocol used to filter accounts.
 * \return A set of accounts containing all accounts for the given \a
 *         protocolName.
 */
AccountSetPtr AccountManager::accountsByProtocol(
        const QString &protocolName) const
{
    QVariantMap filter;
    filter.insert(QLatin1String("protocolName"), protocolName);
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return a set of accounts containing all accounts that match the given \a
 * filter criteria.
 *
 * The \a filter is composed by Account property names and values as map items.
 *
 * The following example will return all jabber accounts that are enabled:
 *
 * \code
 *
 * void MyClass::init()
 * {
 *     am = AccountManager::create();
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
 *     QVariantMap filter;
 *     filter.insert(QLatin1String("protocolName"), QLatin1String("jabber"));
 *     filter.insert(QLatin1String("enabled"), true);
 *     filteredAccountSet = am->filterAccounts(filter);
 *     // connect to AccountSet::accountAdded/accountRemoved signals
 *     QList<AccountPtr> accounts = filteredAccountSet->accounts();
 *     // do something with accounts
 * }
 *
 * \endcode
 *
 * See AccountSet documentation for more details.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param filter The desired filter
 * \return A set of accounts containing all accounts that match the given \a
 *         filter criteria.
 */
AccountSetPtr AccountManager::filterAccounts(const QVariantMap &filter) const
{
    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return an AccountPtr object for the given \a path.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
 *
 * \param path The account object path.
 * \return An AccountPtr object pointing to the Account object for the given
 *         \a path, or a null AccountPtr if \a path is invalid.
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
AccountPtr AccountManager::accountForPath(const QString &path)
{
    if (!mPriv->accounts.contains(path)) {
        return AccountPtr();
    }

    return mPriv->accounts[path];
}

/**
 * Return a list of AccountPtr objects for the given \a paths.
 *
 * The returned list will have one AccountPtr object for each given path. If
 * a given path is invalid the returned AccountPtr object will point to 0.
 * AccountPtr::isNull() will return true.
 *
 * Note that the returned accounts already have the Account::FeatureCore and
 * Account::FeatureProtocolInfo enabled.
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

        if (props.contains(QLatin1String("SupportedAccountProperties"))) {
            mPriv->supportedAccountProperties =
                qdbus_cast<QStringList>(props[QLatin1String("SupportedAccountProperties")]);
        }

        QSet<QString> paths = mPriv->getAccountPathsFromProps(props);
        foreach (const QString &path, paths) {
            mPriv->addAccountForPath(path);
        }
    } else {
        warning().nospace() <<
            "GetAll(AccountManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    watcher->deleteLater();

    mPriv->checkIntrospectionCompleted();
}

void AccountManager::onAccountReady(Tp::PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    AccountPtr account = AccountPtr::dynamicCast(pr->proxy());
    QString path = account->objectPath();

    /* Some error occurred or the account was removed before become ready */
    if (op->isError() || !mPriv->incompleteAccounts.contains(path)) {
        mPriv->incompleteAccounts.remove(path);
        mPriv->checkIntrospectionCompleted();
        return;
    }

    mPriv->incompleteAccounts.remove(path);
    mPriv->accounts.insert(path, account);

    if (isReady(FeatureCore)) {
        /* FIXME (API-BREAK) Remove accountCreated signal */
        emit accountCreated(path);
        emit newAccount(account);
    }

    mPriv->checkIntrospectionCompleted();
}

void AccountManager::onAccountValidityChanged(const QDBusObjectPath &objectPath,
        bool valid)
{
    QString path = objectPath.path();
    bool newAccount = false;

    if (!mPriv->incompleteAccounts.contains(path) &&
        !mPriv->accounts.contains(path)) {
        newAccount = true;
    }

    if (newAccount) {
        mPriv->addAccountForPath(path);
    } else {
        /* Only emit accountValidityChanged if both the AM and the account
         * are ready */
        if (isReady(FeatureCore) && mPriv->accounts.contains(path)) {
            /* FIXME (API-BREAK) Remove accountValidityChanged signal */
            emit accountValidityChanged(path, valid);
        }
    }
}

void AccountManager::onAccountRemoved(const QDBusObjectPath &objectPath)
{
    QString path = objectPath.path();

    /* the account is either in mPriv->incompleteAccounts or mPriv->accounts */
    if (mPriv->accounts.contains(path)) {
        mPriv->accounts.remove(path);

        /* Only emit accountRemoved if both the AM and the account are ready */
        if (isReady(FeatureCore)) {
            /* FIXME (API-BREAK) Remove accountRemoved signal */
            emit accountRemoved(path);
            debug() << "Account" << path << "removed";
        } else {
            debug() << "Account" << path << "removed while the AM "
                "or the account itself were not completely introspected, "
                "ignoring";
        }
    } else if (mPriv->incompleteAccounts.contains(path)) {
        mPriv->incompleteAccounts.remove(path);
        debug() << "Account" << path << "was removed, but it was "
            "not completely introspected, ignoring";
    }
}

} // Tp
