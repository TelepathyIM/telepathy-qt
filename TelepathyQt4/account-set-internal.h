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

#include <TelepathyQt4/AccountPropertyFilter>

namespace Tp
{

class ConnectionCapabilities;

struct AccountSet::Private
{
    class AccountWrapper;

    Private(AccountSet *parent, const AccountManagerPtr &accountManager,
            const QList<AccountFilterConstPtr> &filters);
    Private(AccountSet *parent, const AccountManagerPtr &accountManager,
            const QVariantMap &filter);

    void init();
    bool checkFilters();
    void connectSignals();
    void insertAccounts();
    void insertAccount(const AccountPtr &account);
    void removeAccount(const AccountPtr &account);
    void wrapAccount(const AccountPtr &account);
    void filterAccount(const AccountPtr &account);
    bool accountMatchFilters(AccountWrapper *account);

    AccountSet *parent;
    AccountManagerPtr accountManager;
    QList<AccountFilterConstPtr> filters;
    QHash<QString, AccountWrapper *> wrappers;
    QHash<QString, AccountPtr> accounts;
    bool ready;
};

class TELEPATHY_QT4_NO_EXPORT AccountSet::Private::AccountWrapper : public QObject
{
    Q_OBJECT

public:
    AccountWrapper(const AccountPtr &account, QObject *parent = 0);
    ~AccountWrapper();

    AccountPtr account() const { return mAccount; }

    ConnectionCapabilities *capabilities() const;

Q_SIGNALS:
    void accountRemoved(const Tp::AccountPtr &account);
    void accountPropertyChanged(const Tp::AccountPtr &account,
            const QString &propertyName);
    void accountCapabilitiesChanged(const Tp::AccountPtr &account,
            Tp::ConnectionCapabilities *capabilities);

private Q_SLOTS:
    void onAccountRemoved();
    void onAccountPropertyChanged(const QString &propertyName);
    void onAccountCapalitiesChanged(Tp::ConnectionCapabilities *capabilities);

private:
    AccountPtr mAccount;
};

} // Tp
