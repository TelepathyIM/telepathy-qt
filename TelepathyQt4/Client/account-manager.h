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

#ifndef _TelepathyQt4_cli_account_manager_h_HEADER_GUARD_
#define _TelepathyQt4_cli_account_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-account-manager.h>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

#include <QDBusObjectPath>
#include <QString>
#include <QVariantMap>

namespace Telepathy
{
namespace Client
{

class Account;
class PendingAccount;
class PendingAccounts;
class PendingOperation;

class AccountManager : public StatelessDBusProxy,
        private OptionalInterfaceFactory
{
    Q_OBJECT

public:
    enum Feature {
        _Padding = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(Features, Feature)

    AccountManager(QObject *parent = 0);
    AccountManager(const QDBusConnection &bus, QObject *parent = 0);

    virtual ~AccountManager();

    QStringList interfaces() const;

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory::interface<DBus::PropertiesInterface>(
                *baseInterface());
    }

    QStringList validAccountPaths() const;
    QStringList invalidAccountPaths() const;
    QStringList allAccountPaths() const;

    QList<Account *> validAccounts();
    QList<Account *> invalidAccounts();
    QList<Account *> allAccounts();

    Account *accountForPath(const QString &path);
    QList<Account *> accountsForPaths(const QStringList &paths);

    PendingAccount *createAccount(const QString &connectionManager,
            const QString &protocol, const QString &displayName,
            const QVariantMap &parameters);

    // TODO: enabledAccounts(), accountsByProtocol(), ... ?

    bool isReady(Features features = 0) const;

    PendingOperation *becomeReady(Features features = 0);

Q_SIGNALS:
    void accountCreated(const QString &path);
    void accountRemoved(const QString &path);
    void accountValidityChanged(const QString &path, bool valid);

protected:
    AccountManagerInterface *baseInterface() const;

private Q_SLOTS:
    void onGetAllAccountManagerReturn(QDBusPendingCallWatcher *);
    void onAccountValidityChanged(const QDBusObjectPath &, bool);
    void onAccountRemoved(const QDBusObjectPath &);
    void continueIntrospection();

private:
    void init();
    void callGetAll();

    struct Private;
    friend struct Private;
    friend class PendingAccount;
    Private *mPriv;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AccountManager::Features)

} // Telepathy::Client
} // Telepathy

#endif
