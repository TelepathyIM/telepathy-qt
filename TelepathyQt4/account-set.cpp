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

AccountSet::Private::Private(AccountSet *parent,
        const AccountManagerPtr &accountManager,
        const QVariantMap &filter)
    : parent(parent),
      accountManager(accountManager),
      filter(filter),
      ready(false)
{
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

AccountSet::AccountSet(const AccountManagerPtr &accountManager,
        const QVariantMap &filter)
    : QObject(),
      mPriv(new Private(this, accountManager, filter))
{
}

AccountSet::~AccountSet()
{
}

AccountManagerPtr AccountSet::accountManager() const
{
    return mPriv->accountManager;
}

QVariantMap AccountSet::filter() const
{
    return mPriv->filter;
}

QList<AccountPtr> AccountSet::accounts() const
{
    return mPriv->accounts.values();
}

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
