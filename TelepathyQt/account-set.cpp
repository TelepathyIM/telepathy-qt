/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#include <TelepathyQt/AccountSet>
#include "TelepathyQt/account-set-internal.h"

#include "TelepathyQt/_gen/account-set.moc.hpp"
#include "TelepathyQt/_gen/account-set-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFilter>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ConnectionManager>

namespace Tp
{

AccountSet::Private::Private(AccountSet *parent,
        const AccountManagerPtr &accountManager,
        const AccountFilterConstPtr &filter)
    : parent(parent),
      accountManager(accountManager),
      filter(filter),
      ready(false)
{
    init();
}

AccountSet::Private::Private(AccountSet *parent,
        const AccountManagerPtr &accountManager,
        const QVariantMap &filterMap)
    : parent(parent),
      accountManager(accountManager),
      ready(false)
{
    AccountPropertyFilterPtr propertyFilter = AccountPropertyFilter::create();
    for (QVariantMap::const_iterator i = filterMap.constBegin();
            i != filterMap.constEnd(); ++i) {
        propertyFilter->addProperty(i.key(), i.value());
    }
    filter = AccountFilterPtr::dynamicCast(propertyFilter);
    init();
}

void AccountSet::Private::init()
{
    if (filter->isValid()) {
        connectSignals();
        insertAccounts();
        ready = true;
    }
}

void AccountSet::Private::connectSignals()
{
    parent->connect(accountManager.data(),
            SIGNAL(newAccount(Tp::AccountPtr)),
            SLOT(onNewAccount(Tp::AccountPtr)));
}

void AccountSet::Private::insertAccounts()
{
    foreach (const Tp::AccountPtr &account, accountManager->allAccounts()) {
        insertAccount(account);
    }
}

void AccountSet::Private::insertAccount(const Tp::AccountPtr &account)
{
    QString accountPath = account->objectPath();
    Q_ASSERT(!wrappers.contains(accountPath));
    wrapAccount(account);
    filterAccount(account);
}

void AccountSet::Private::removeAccount(const Tp::AccountPtr &account)
{
    QString accountPath = account->objectPath();
    Q_ASSERT(wrappers.contains(accountPath));
    accounts.remove(accountPath);

    AccountWrapper *wrapper = wrappers.take(accountPath);
    Q_ASSERT(wrapper->disconnect(parent));
    wrapper->deleteLater();

    emit parent->accountRemoved(account);
}

void AccountSet::Private::wrapAccount(const AccountPtr &account)
{
    AccountWrapper *wrapper = new AccountWrapper(account, parent);
    parent->connect(wrapper,
            SIGNAL(accountRemoved(Tp::AccountPtr)),
            SLOT(onAccountRemoved(Tp::AccountPtr)));
    parent->connect(wrapper,
            SIGNAL(accountPropertyChanged(Tp::AccountPtr,QString)),
            SLOT(onAccountChanged(Tp::AccountPtr)));
    parent->connect(wrapper,
            SIGNAL(accountCapabilitiesChanged(Tp::AccountPtr,Tp::ConnectionCapabilities)),
            SLOT(onAccountChanged(Tp::AccountPtr)));
    wrappers.insert(account->objectPath(), wrapper);
}

void AccountSet::Private::filterAccount(const AccountPtr &account)
{
    QString accountPath = account->objectPath();
    Q_ASSERT(wrappers.contains(accountPath));
    AccountWrapper *wrapper = wrappers[accountPath];

    /* account changed, let's check if it matches filter */
    if (accountMatchFilter(wrapper)) {
        if (!accounts.contains(account->objectPath())) {
            accounts.insert(account->objectPath(), account);
            if (ready) {
                emit parent->accountAdded(account);
            }
        }
    } else {
        if (accounts.contains(account->objectPath())) {
            accounts.remove(account->objectPath());
            if (ready) {
                emit parent->accountRemoved(account);
            }
        }
    }
}

bool AccountSet::Private::accountMatchFilter(AccountWrapper *wrapper)
{
    if (!filter) {
        return true;
    }

    return filter->matches(wrapper->account());
}

AccountSet::Private::AccountWrapper::AccountWrapper(
        const AccountPtr &account, QObject *parent)
    : QObject(parent),
      mAccount(account)
{
    connect(account.data(),
            SIGNAL(removed()),
            SLOT(onAccountRemoved()));
    connect(account.data(),
            SIGNAL(propertyChanged(QString)),
            SLOT(onAccountPropertyChanged(QString)));
    connect(account.data(),
            SIGNAL(capabilitiesChanged(Tp::ConnectionCapabilities)),
            SLOT(onAccountCapalitiesChanged(Tp::ConnectionCapabilities)));
}

AccountSet::Private::AccountWrapper::~AccountWrapper()
{
}

void AccountSet::Private::AccountWrapper::onAccountRemoved()
{
    emit accountRemoved(mAccount);
}

void AccountSet::Private::AccountWrapper::onAccountPropertyChanged(
        const QString &propertyName)
{
    emit accountPropertyChanged(mAccount, propertyName);
}

void AccountSet::Private::AccountWrapper::onAccountCapalitiesChanged(
        const ConnectionCapabilities &caps)
{
    emit accountCapabilitiesChanged(mAccount, caps);
}

/**
 * \class AccountSet
 * \ingroup clientaccount
 * \headerfile TelepathyQt/account-set.h <TelepathyQt/AccountSet>
 *
 * \brief The AccountSet class represents a set of Telepathy accounts
 * filtered by a given criteria.
 *
 * AccountSet is automatically updated whenever accounts that match the given
 * criteria are added, removed or updated.
 *
 * \section account_set_usage_sec Usage
 *
 * \subsection account_set_create_sec Creating an AccountSet object
 *
 * The easiest way to create AccountSet objects is through AccountManager. One
 * can just use the AccountManager convenience methods such as
 * AccountManager::validAccounts() to get a set of account objects
 * representing valid accounts.
 *
 * For example:
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
 *     void onAccountManagerReady(Tp::PendingOperation *);
 *     void onValidAccountAdded(const Tp::AccountPtr &);
 *     void onValidAccountRemoved(const Tp::AccountPtr &);
 *
 * private:
 *     AccountManagerPtr am;
 *     AccountSetPtr validAccountsSet;
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
 *     validAccountsSet = am->validAccounts();
 *     connect(validAccountsSet.data(),
 *             SIGNAL(accountAdded(const Tp::AccountPtr &)),
 *             SLOT(onValidAccountAdded(const Tp::AccountPtr &)));
 *     connect(validAccountsSet.data(),
 *             SIGNAL(accountRemoved(const Tp::AccountPtr &)),
 *             SLOT(onValidAccountRemoved(const Tp::AccountPtr &)));
 *
 *     QList<AccountPtr> accounts = validAccountsSet->accounts();
 *     // do something with accounts
 * }
 *
 * void MyClass::onValidAccountAdded(const Tp::AccountPtr &account)
 * {
 *     // do something with account
 * }
 *
 * void MyClass::onValidAccountRemoved(const Tp::AccountPtr &account)
 * {
 *     // do something with account
 * }
 *
 * \endcode
 *
 * You can also define your own filter using AccountManager::filterAccounts:
 *
 * \code
 *
 * void MyClass::onAccountManagerReady(Tp::PendingOperation *op)
 * {
 *     ...
 *
 *     AccountPropertyFilterPtr filter = AccountPropertyFilter::create();
 *     filter->addProperty(QLatin1String("protocolName"), QLatin1String("jabber"));
 *     filter->addProperty(QLatin1String("enabled"), true);
 *
 *     AccountSetPtr filteredAccountSet = am->filterAccounts(filter);
 *     // connect to AccountSet::accountAdded/accountRemoved signals
 *     QList<AccountPtr> accounts = filteredAccountSet->accounts();
 *     // do something with accounts
 *
 *     ....
 * }
 *
 * \endcode
 *
 * Note that for AccountSet to property work with AccountCapabilityFilter
 * objects, the feature Account::FeatureCapabilities need to be enabled in all
 * accounts return by the AccountManager passed as param in the constructor.
 * The easiest way to do this is to enable AccountManager feature
 * AccountManager::FeatureFilterByCapabilities.
 *
 * AccountSet can also be instantiated directly, but when doing it,
 * the AccountManager object passed as param in the constructor must be ready
 * for AccountSet properly work.
 */

/**
 * Construct a new AccountSet object.
 *
 * \param accountManager An account manager object used to filter accounts.
 *                       The account manager object must be ready.
 * \param filter The desired filter.
 */
AccountSet::AccountSet(const AccountManagerPtr &accountManager,
        const AccountFilterConstPtr &filter)
    : Object(),
      mPriv(new Private(this, accountManager, filter))
{
}

/**
 * Construct a new AccountSet object.
 *
 * The \a filter must contain Account property names and values as map items.
 *
 * \param accountManager An account manager object used to filter accounts.
 *                       The account manager object must be ready.
 * \param filter The desired filter.
 */
AccountSet::AccountSet(const AccountManagerPtr &accountManager,
        const QVariantMap &filter)
    : Object(),
      mPriv(new Private(this, accountManager, filter))
{
}

/**
 * Class destructor.
 */
AccountSet::~AccountSet()
{
    delete mPriv;
}

/**
 * Return the account manager object used to filter accounts.
 *
 * \return A pointer to the AccountManager object.
 */
AccountManagerPtr AccountSet::accountManager() const
{
    return mPriv->accountManager;
}

/**
 * Return the filter used to filter accounts.
 *
 * \return A read-only pointer the AccountFilter object.
 */
AccountFilterConstPtr AccountSet::filter() const
{
    return mPriv->filter;
}

/**
 * Return a list of account objects that match filter.
 *
 * Change notification is via the accountAdded() and accountRemoved() signals.
 *
 * \return A list of pointers to Account objects.
 * \sa accountAdded(), accountRemoved()
 */
QList<AccountPtr> AccountSet::accounts() const
{
    return mPriv->accounts.values();
}

/**
 * \fn void AccountSet::accountAdded(const Tp::AccountPtr &account)
 *
 * Emitted whenever an account that matches filter is added to
 * this set.
 *
 * \param account The account that was added to this set.
 * \sa accounts()
 */

/**
 * \fn void AccountSet::accountRemoved(const Tp::AccountPtr &account)
 *
 * Emitted whenever an account that matches filter is removed
 * from this set.
 *
 * \param account The account that was removed from this set.
 * \sa accounts()
 */

void AccountSet::onNewAccount(const AccountPtr &account)
{
    mPriv->insertAccount(account);
}

void AccountSet::onAccountRemoved(const AccountPtr &account)
{
    mPriv->removeAccount(account);
}

void AccountSet::onAccountChanged(const AccountPtr &account)
{
    mPriv->filterAccount(account);
}

} // Tp
