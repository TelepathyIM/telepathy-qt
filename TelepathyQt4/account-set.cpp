/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#include <TelepathyQt4/AccountSet>
#include "TelepathyQt4/account-set-internal.h"

#include "TelepathyQt4/_gen/account-set.moc.hpp"
#include "TelepathyQt4/_gen/account-set-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>

namespace Tp
{

QStringList AccountSet::Private::supportedAccountProperties;

AccountSet::Private::Private(AccountSet *parent,
        const AccountManagerPtr &accountManager,
        const QVariantMap &filter)
    : parent(parent),
      accountManager(accountManager),
      filter(filter),
      ready(false)
{
    /* initialize supportedAccountProperties */
    if (supportedAccountProperties.isEmpty()) {
        const QMetaObject metaObject = Account::staticMetaObject;
        for (int i = metaObject.propertyOffset(); i < metaObject.propertyCount(); ++i) {
            supportedAccountProperties << QLatin1String(metaObject.property(i).name());
        }
    }

    /* check if filter is valid */
    QVariantMap::const_iterator i = filter.constBegin();
    QVariantMap::const_iterator end = filter.constEnd();
    while (i != end) {
        QString propertyName = i.key();

        if (!supportedAccountProperties.contains(propertyName)) {
            warning() << "Trying to filter accounts by" << propertyName <<
                "which is not a valid Account property";
            /* no need to check further or connect to signals as no account will
             * match filter */
            filterValid = false;
            return;
        }

        ++i;
    }

    filterValid = true;

    parent->connect(accountManager.data(),
            SIGNAL(newAccount(const Tp::AccountPtr &)),
            SLOT(onNewAccount(const Tp::AccountPtr &)));

    foreach (const Tp::AccountPtr &account, accountManager->allAccounts()) {
        insertAccount(account);
        ready = true;
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
    AccountWrapper *wrapper = wrappers[accountPath];
    delete wrapper;
    emit parent->accountRemoved(account);
}

void AccountSet::Private::wrapAccount(const AccountPtr &account)
{
    AccountWrapper *wrapper = new AccountWrapper(account, parent);
    parent->connect(wrapper,
            SIGNAL(accountRemoved(const Tp::AccountPtr &)),
            SLOT(onAccountRemoved(const Tp::AccountPtr &)));
    parent->connect(wrapper,
            SIGNAL(accountPropertyChanged(const Tp::AccountPtr &, const QString &)),
            SLOT(onAccountPropertyChanged(const Tp::AccountPtr &, const QString &)));
    wrappers.insert(account->objectPath(), wrapper);
}

void AccountSet::Private::filterAccount(const AccountPtr &account)
{
    /* account changed, let's check if it matches filter */
    if (accountMatchFilter(account, filter)) {
        if (!accounts.contains(account->objectPath())) {
            accounts.insert(account->objectPath(), account);
            if (ready) {
                emit parent->accountAdded(account);
            }
        }
    } else {
        /* QHash::remove is no-op if the item is not in the hash */
        accounts.remove(account->objectPath());
        if (ready) {
            emit parent->accountRemoved(account);
        }
    }
}

bool AccountSet::Private::accountMatchFilter(const AccountPtr &account,
        const QVariantMap &filter)
{
    if (filter.isEmpty()) {
        return true;
    }

    bool match = true;
    QVariantMap::const_iterator i = filter.constBegin();
    QVariantMap::const_iterator end = filter.constEnd();
    while (i != end) {
        QString propertyName = i.key();
        QVariant propertyValue = i.value();

        if (account->property(propertyName.toLatin1().constData()) != propertyValue) {
            match = false;
            break;
        }

        ++i;
    }

    return match;
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
            SIGNAL(propertyChanged(const QString &)),
            SLOT(onAccountPropertyChanged(const QString &)));
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

/**
 * \class AccountSet
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/account-set.h <TelepathyQt4/AccountSet>
 *
 * \brief The AccountSet class provides an object representing a set of
 * Telepathy accounts filtered by a given criteria.
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
 * AccountManager::validAccountsSet() to get a set of account objects
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
 *     validAccountsSet = am->validAccountsSet();
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
 *     QVariantMap filter;
 *     filter.insert(QLatin1String("protocol"), QLatin1String("jabber"));
 *     filter.insert(QLatin1String("protocolName"), QLatin1String("jabber"));
 *     filter.insert(QLatin1String("enabled"), true);
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
 * AccountSet can also be instantiated directly, but note that when doing it,
 * the AccountManager object passed as param must be ready for AccountSet
 * properly work.
 */

/**
 * Construct a new AccountSet object.
 *
 * \param accountManager An account manager object used to filter accounts.
 *                       The account manager object must be ready.
 * \param filter The desired filter.
 */
AccountSet::AccountSet(const AccountManagerPtr &accountManager,
        const QVariantMap &filter)
    : QObject(),
      mPriv(new Private(this, accountManager, filter))
{
}

/**
 * Class destructor.
 */
AccountSet::~AccountSet()
{
}

/**
 * Return the account manager object used to filter accounts.
 *
 * \return The AccountManager object used to filter accounts.
 */
AccountManagerPtr AccountSet::accountManager() const
{
    return mPriv->accountManager;
}

/**
 * Return whether the filter returned by filter() is valid.
 *
 * If the filter is invalid accounts() will always return an empty list.
 *
 * \return \c true if the filter returned by filter() is valid, otherwise \c
 *         false.
 */
bool AccountSet::isFilterValid() const
{
    return mPriv->filterValid;
}

/**
 * Return the filter used to filter accounts.
 *
 * The filter is composed by Account property names and values as map items.
 *
 * \return A QVariantMap representing the filter used to filter accounts.
 * \sa isFilterValid()
 */
QVariantMap AccountSet::filter() const
{
    return mPriv->filter;
}

/**
 * Return a list of account objects that match filter().
 *
 * \return A list of account objects that match filter().
 */
QList<AccountPtr> AccountSet::accounts() const
{
    return mPriv->accounts.values();
}

/**
 * \fn void AccountSet::accountAdded(const Tp::AccountPtr &account);
 *
 * This signal is emitted whenever an account that matches filter() is added to
 * this set.
 *
 * \param account The account that was added to this set.
 */

/**
 * \fn void AccountSet::accountRemoved(const Tp::AccountPtr &account);
 *
 * This signal is emitted whenever an account that matches filter() is removed
 * from this set.
 *
 * \param account The account that was removed from this set.
 */

void AccountSet::onNewAccount(const AccountPtr &account)
{
    mPriv->insertAccount(account);
}

void AccountSet::onAccountRemoved(const AccountPtr &account)
{
    mPriv->removeAccount(account);
}

void AccountSet::onAccountPropertyChanged(const AccountPtr &account,
        const QString &propertyName)
{
    mPriv->filterAccount(account);
}

} // Tp
