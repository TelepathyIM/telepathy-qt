/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#include <TelepathyQt/AccountManager>

#include "TelepathyQt/_gen/account-manager.moc.hpp"
#include "TelepathyQt/_gen/cli-account-manager.moc.hpp"
#include "TelepathyQt/_gen/cli-account-manager-body.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/AccountCapabilityFilter>
#include <TelepathyQt/AccountFilter>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Constants>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReadinessHelper>

#include <QQueue>
#include <QSet>
#include <QTimer>

namespace Tp
{

struct TP_QT_NO_EXPORT AccountManager::Private
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

    // Public object
    AccountManager *parent;

    // Instance of generated interface class
    Client::AccountManagerInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    AccountFactoryConstPtr accFactory;
    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Introspection
    int reintrospectionRetries;
    bool gotInitialAccounts;
    QHash<QString, AccountPtr> incompleteAccounts;
    QHash<QString, AccountPtr> accounts;
    QStringList supportedAccountProperties;
};

static const int maxReintrospectionRetries = 5;
static const int reintrospectionRetryInterval = 3;

AccountManager::Private::Private(AccountManager *parent,
        const AccountFactoryConstPtr &accFactory, const ConnectionFactoryConstPtr &connFactory,
        const ChannelFactoryConstPtr &chanFactory, const ContactFactoryConstPtr &contactFactory)
    : parent(parent),
      baseInterface(new Client::AccountManagerInterface(parent)),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      accFactory(accFactory),
      connFactory(connFactory),
      chanFactory(chanFactory),
      contactFactory(contactFactory),
      reintrospectionRetries(0),
      gotInitialAccounts(false)
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
            SIGNAL(AccountValidityChanged(QDBusObjectPath,bool)),
            SLOT(onAccountValidityChanged(QDBusObjectPath,bool)));
    parent->connect(baseInterface,
            SIGNAL(AccountRemoved(QDBusObjectPath)),
            SLOT(onAccountRemoved(QDBusObjectPath)));
}

void AccountManager::Private::introspectMain(AccountManager::Private *self)
{
    debug() << "Calling Properties::GetAll(AccountManager)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            self->properties->GetAll(
                TP_QT_IFACE_ACCOUNT_MANAGER),
            self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
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
    // Also check incompleteAccounts, because otherwise we end up introspecting an account twice
    // when getting an AccountValidityChanged signal for a new account before we get the initial
    // introspection accounts list from the GetAll return (the GetAll return function
    // unconditionally calls addAccountForPath
    if (accounts.contains(path) || incompleteAccounts.contains(path)) {
        return;
    }

    PendingReady *readyOp = accFactory->proxy(parent->busName(), path, connFactory,
            chanFactory, contactFactory);
    AccountPtr account(AccountPtr::qObjectCast(readyOp->proxy()));
    Q_ASSERT(!account.isNull());

    parent->connect(readyOp,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountReady(Tp::PendingOperation*)));
    incompleteAccounts.insert(path, account);
}

/**
 * \class AccountManager
 * \ingroup clientam
 * \headerfile TelepathyQt/account-manager.h <TelepathyQt/AccountManager>
 *
 * \brief The AccountManager class represents a Telepathy account manager.
 *
 * The remote object accessor functions on this object (allAccounts(),
 * validAccounts(), and so on) don't make any D-Bus calls; instead, they return/use
 * values cached from a previous introspection run. The introspection process
 * populates their values in the most efficient way possible based on what the
 * service implements.
 *
 * To avoid unnecessary D-Bus traffic, some accessors only return valid
 * information after AccountManager::FeatureCore has been enabled.
 * See the individual methods descriptions for more details.
 *
 * AccountManager features can be enabled by calling becomeReady()
 * with the desired set of features as an argument (currently only AccountManager::FeatureCore is
 * supported), and waiting for the resulting PendingOperation to finish.
 *
 * All accounts returned by AccountManager are guaranteed to have the features set in the
 * AccountFactory used by it ready.
 *
 * A signal is emitted to indicate that accounts are added. See newCreated() for more details.
 *
 * \section am_usage_sec Usage
 *
 * \subsection am_create_sec Creating an AccountManager object
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
 * \subsection am_ready_sec Making AccountManager ready to use
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
 *     AccountManagerPtr mAM;
 * };
 *
 * MyClass::MyClass(QObject *parent)
 *     : QObject(parent)
 *       mAM(AccountManager::create())
 * {
 *     connect(mAM->becomeReady(),
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
 *     qDebug() << "All accounts:";
 *     foreach (const Tp::AccountPtr &acc, mAM->allAccounts()) {
 *         qDebug() << " path:" << acc->objectPath();
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
 * Note that this feature must be enabled in order to use most AccountManager
 * methods.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature AccountManager::FeatureCore = Feature(QLatin1String(AccountManager::staticMetaObject.className()), 0, true);

/**
 * Create a new AccountManager object using the given \a bus.
 *
 * The instance will use an account factory creating Tp::Account objects with Account::FeatureCore
 * ready, a connection factory creating Tp::Connection objects with no features ready, a channel
 * factory creating stock Tp::Channel subclasses, as appropriate, with no features ready, and a
 * contact factory creating Tp::Contact objects with no features ready.
 *
 * \param bus QDBusConnection to use.
 * \return An AccountManagerPtr object pointing to the newly created
 *         AccountManager object.
 */
AccountManagerPtr AccountManager::create(const QDBusConnection &bus)
{
    return AccountManagerPtr(new AccountManager(bus,
                AccountFactory::create(bus, Account::FeatureCore), ConnectionFactory::create(bus),
                ChannelFactory::create(bus), ContactFactory::create(),
                AccountManager::FeatureCore));
}

/**
 * Create a new AccountManager using QDBusConnection::sessionBus() and the given factories.
 *
 * The connection, channel and contact factories are passed to any Account objects created by this
 * account manager object. In fact, they're not used directly by AccountManager at all.
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
                accountFactory, connectionFactory, channelFactory, contactFactory,
                AccountManager::FeatureCore));
}

/**
 * Create a new AccountManager using the given \a bus and the given factories.
 *
 * The connection, channel and contact factories are passed to any Account objects created by this
 * account manager object. In fact, they're not used directly by AccountManager at all.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
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
                accountFactory, connectionFactory, channelFactory, contactFactory,
                AccountManager::FeatureCore));
}

/**
 * Construct a new AccountManager object using the given \a bus and the given factories.
 *
 * The connection, channel and contact factories are passed to any Account objects created by this
 * account manager object. In fact, they're not used directly by AccountManager at all.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \param coreFeature The core feature of the Account subclass. The corresponding introspectable
 * should depend on AccountManager::FeatureCore.
 */
AccountManager::AccountManager(const QDBusConnection &bus,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory,
        const Feature &coreFeature)
    : StatelessDBusProxy(bus,
            TP_QT_ACCOUNT_MANAGER_BUS_NAME,
            TP_QT_ACCOUNT_MANAGER_OBJECT_PATH, coreFeature),
      OptionalInterfaceFactory<AccountManager>(this),
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
 * Return the account factory used by this account manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the AccountFactory object.
 */
AccountFactoryConstPtr AccountManager::accountFactory() const
{
    return mPriv->accFactory;
}

/**
 * Return the connection factory used by this account manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ConnectionFactory object.
 */
ConnectionFactoryConstPtr AccountManager::connectionFactory() const
{
    return mPriv->connFactory;
}

/**
 * Return the channel factory used by this account manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ChannelFactory object.
 */
ChannelFactoryConstPtr AccountManager::channelFactory() const
{
    return mPriv->chanFactory;
}

/**
 * Return the contact factory used by this account manager.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the manager would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return A read-only pointer to the ContactFactory object.
 */
ContactFactoryConstPtr AccountManager::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return a list containing all accounts.
 *
 * Newly accounts added and/or discovered are signaled via newAccount().
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A list of pointers to Account objects.
 */
QList<AccountPtr> AccountManager::allAccounts() const
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
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::validAccounts() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("valid"), true);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all invalid accounts.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::invalidAccounts() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("valid"), false);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all enabled accounts.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::enabledAccounts() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("enabled"), true);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all disabled accounts.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::disabledAccounts() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("enabled"), false);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all online accounts.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::onlineAccounts() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("online"), true);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all offline accounts.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::offlineAccounts() const
{
    QVariantMap filter;
    filter.insert(QLatin1String("online"), false);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support text chats by
 * providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::textChatAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::textChat());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support text chat
 * rooms.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::textChatroomAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::textChatroom());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support audio calls (using the
 * Call interface) by providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::audioCallAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::audioCall());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support video calls (using the
 * Call interface) by providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::videoCallAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::videoCall());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support media calls (using the
 * StreamedMedia interface) by providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::streamedMediaCallAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::streamedMediaCall());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support audio calls (using the
 * StreamedMedia interface) by providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::streamedMediaAudioCallAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::streamedMediaAudioCall());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support video calls (using the
 * StreamedMedia interface) by providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::streamedMediaVideoCallAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::streamedMediaVideoCall());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support video calls with audio (using the
 * StreamedMedia interface) by providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::streamedMediaVideoCallWithAudioAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(
            RequestableChannelClassSpec::streamedMediaVideoCallWithAudio());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that support file transfers by
 * providing a contact identifier.
 *
 * For this method to work, you must use an AccountFactory which makes Account::FeatureCapabilities
 * ready.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::fileTransferAccounts() const
{
    if (!accountFactory()->features().contains(Account::FeatureCapabilities)) {
        warning() << "Account filtering by capabilities can only be used with an AccountFactory"
            << "which makes Account::FeatureCapabilities ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    AccountCapabilityFilterPtr filter = AccountCapabilityFilter::create();
    filter->addRequestableChannelClassSubset(RequestableChannelClassSpec::fileTransfer());
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts for the given \a
 * protocolName.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \param protocolName The name of the protocol used to filter accounts.
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::accountsByProtocol(
        const QString &protocolName) const
{
    if (!isReady(FeatureCore)) {
        warning() << "Account filtering requires AccountManager to be ready";
        return filterAccounts(AccountFilterConstPtr());
    }

    QVariantMap filter;
    filter.insert(QLatin1String("protocolName"), protocolName);
    return filterAccounts(filter);
}

/**
 * Return a set of accounts containing all accounts that match the given \a
 * filter criteria.
 *
 * For AccountCapabilityFilter filtering, an AccountFactory which makes
 * Account::FeatureCapabilities ready must be used.
 *
 * See AccountSet documentation for more details.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \param filter The desired filter.
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::filterAccounts(const AccountFilterConstPtr &filter) const
{
    if (!isReady(FeatureCore)) {
        warning() << "Account filtering requires AccountManager to be ready";
        return AccountSetPtr(new AccountSet(AccountManagerPtr(
                        (AccountManager *) this), AccountFilterConstPtr()));
    }

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
 *     mAM = AccountManager::create();
 *     connect(mAM->becomeReady(),
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
 *     filteredAccountSet = mAM->filterAccounts(filter);
 *     // connect to AccountSet::accountAdded/accountRemoved signals
 *     QList<AccountPtr> accounts = filteredAccountSet->accounts();
 *     // do something with accounts
 * }
 *
 * \endcode
 *
 * See AccountSet documentation for more details.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \param filter The desired filter.
 * \return A pointer to an AccountSet object containing the matching accounts.
 */
AccountSetPtr AccountManager::filterAccounts(const QVariantMap &filter) const
{
    if (!isReady(FeatureCore)) {
        warning() << "Account filtering requires AccountManager to be ready";
        return AccountSetPtr(new AccountSet(AccountManagerPtr(
                        (AccountManager *) this), QVariantMap()));
    }

    return AccountSetPtr(new AccountSet(AccountManagerPtr(
                    (AccountManager *) this), filter));
}

/**
 * Return the account for the given \a path.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \param path The account object path.
 * \return A pointer to an AccountSet object containing the matching accounts.
 * \sa allAccounts(), accountsForObjectPaths()
 */
AccountPtr AccountManager::accountForObjectPath(const QString &path) const
{
    if (!isReady(FeatureCore)) {
        return AccountPtr();
    }

    return mPriv->accounts.value(path);
}

/**
 * \deprecated See accountForObjectPath()
 */
AccountPtr AccountManager::accountForPath(const QString &path) const
{
    return accountForObjectPath(path);
}

/**
 * Return a list of accounts for the given \a paths.
 *
 * The returned list will have one AccountPtr object for each given path. If
 * a given path is invalid the returned AccountPtr object will point to 0.
 * AccountPtr::isNull() will return true.
 *
 * This method requires AccountManager::FeatureCore to be ready.
 *
 * \param paths List of accounts object paths.
 * \return A list of pointers to Account objects for the given
 *         \a paths. Null AccountPtr objects will be used as list elements for each invalid path.
 * \sa allAccounts(), accountForObjectPath()
 */
QList<AccountPtr> AccountManager::accountsForObjectPaths(const QStringList &paths) const
{
    if (!isReady(FeatureCore)) {
        return QList<AccountPtr>();
    }

    QList<AccountPtr> result;
    foreach (const QString &path, paths) {
        result << accountForObjectPath(path);
    }
    return result;
}

/**
 * \deprecated See accountsForObjectPaths()
 */
QList<AccountPtr> AccountManager::accountsForPaths(const QStringList &paths) const
{
    return accountsForObjectPaths(paths);
}

/**
 * Return a list of the fully qualified names of properties that can be set
 * when calling createAccount().
 *
 * \return A list of fully qualified D-Bus property names,
 *         such as "org.freedesktop.Telepathy.Account.Enabled".
 * \sa createAccount()
 */
QStringList AccountManager::supportedAccountProperties() const
{
    return mPriv->supportedAccountProperties;
}

/**
 * Create an account with the given parameters.
 *
 * The optional \a properties argument can be used to set any property listed in
 * supportedAccountProperties() at the time the account is created.
 *
 * \param connectionManager The name of the connection manager to create the account
 *                          for.
 * \param protocol The name of the protocol to create the account for.
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

void AccountManager::introspectMain()
{
    mPriv->introspectMain(mPriv);
}

void AccountManager::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        mPriv->gotInitialAccounts = true;

        debug() << "Got reply to Properties.GetAll(AccountManager)";
        props = reply.value();

        if (props.contains(QLatin1String("Interfaces"))) {
            setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
            mPriv->readinessHelper->setInterfaces(interfaces());
        }

        if (props.contains(QLatin1String("SupportedAccountProperties"))) {
            mPriv->supportedAccountProperties =
                qdbus_cast<QStringList>(props[QLatin1String("SupportedAccountProperties")]);
        }

        QSet<QString> paths = mPriv->getAccountPathsFromProps(props);
        foreach (const QString &path, paths) {
            mPriv->addAccountForPath(path);
        }

        mPriv->checkIntrospectionCompleted();
    } else {
        if (mPriv->reintrospectionRetries++ < maxReintrospectionRetries) {
            int retryInterval = reintrospectionRetryInterval;
            if (reply.error().type() == QDBusError::TimedOut) {
                retryInterval = 0;
            }
            QTimer::singleShot(retryInterval, this, SLOT(introspectMain()));
        } else {
            warning() << "GetAll(AccountManager) failed with" <<
                reply.error().name() << ":" << reply.error().message();
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                    false, reply.error());
        }
    }

    watcher->deleteLater();
}

void AccountManager::onAccountReady(Tp::PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    AccountPtr account(AccountPtr::qObjectCast(pr->proxy()));
    QString path = account->objectPath();

    /* Some error occurred or the account was removed before become ready */
    if (op->isError() || !mPriv->incompleteAccounts.contains(path)) {
        mPriv->incompleteAccounts.remove(path);
        mPriv->checkIntrospectionCompleted();
        return;
    }

    mPriv->incompleteAccounts.remove(path);

    // We shouldn't end up here twice for the same account - that would also mean newAccount being
    // emitted twice for an account, and AccountSets getting confused as a result
    Q_ASSERT(!mPriv->accounts.contains(path));
    mPriv->accounts.insert(path, account);

    if (isReady(FeatureCore)) {
        emit newAccount(account);
    }

    mPriv->checkIntrospectionCompleted();
}

void AccountManager::onAccountValidityChanged(const QDBusObjectPath &objectPath,
        bool valid)
{
    if (!mPriv->gotInitialAccounts) {
        return;
    }

    QString path = objectPath.path();

    if (!mPriv->incompleteAccounts.contains(path) &&
        !mPriv->accounts.contains(path)) {
        debug() << "New account" << path;
        mPriv->addAccountForPath(path);
    }
}

void AccountManager::onAccountRemoved(const QDBusObjectPath &objectPath)
{
    if (!mPriv->gotInitialAccounts) {
        return;
    }

    QString path = objectPath.path();

    /* the account is either in mPriv->incompleteAccounts or mPriv->accounts */
    if (mPriv->accounts.contains(path)) {
        mPriv->accounts.remove(path);

        if (isReady(FeatureCore)) {
            debug() << "Account" << path << "removed";
        } else {
            debug() << "Account" << path << "removed while the AM "
                "was not completely introspected";
        }
    } else if (mPriv->incompleteAccounts.contains(path)) {
        mPriv->incompleteAccounts.remove(path);
        debug() << "Account" << path << "was removed, but it was "
            "not completely introspected, ignoring";
    } else {
        debug() << "Got AccountRemoved for unknown account" << path << ", ignoring";
    }
}

/**
 * \fn void AccountManager::newAccount(const Tp::AccountPtr &account)
 *
 * Emitted when a new account is created.
 *
 * The new \a account will have the features set in the AccountFactory used by this
 * account manager ready and the same connection, channel and contact factories as used by this
 * account manager.
 *
 * \param account The newly created account.
 */

} // Tp
