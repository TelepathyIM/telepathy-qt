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

#include <QString>
#include <QVariantMap>

namespace Telepathy
{
namespace Client
{

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

    Telepathy::ObjectPathList validAccountPaths() const;
    Telepathy::ObjectPathList invalidAccountPaths() const;
    Telepathy::ObjectPathList allAccountPaths() const;

    PendingAccounts *validAccounts() const;
    PendingAccounts *invalidAccounts() const;
    PendingAccounts *allAccounts() const;

    PendingAccount *accountForPath(const QString &path) const;

    PendingAccount *createAccount(const QString &connectionManager,
            const QString &protocol, const QString &displayName,
            const QVariantMap &parameters);

    // TODO: enabledAccounts(), accountsByProtocol(), ... ?

    bool isReady(Features features = 0) const;

    PendingOperation *becomeReady(Features features = 0);

Q_SIGNALS:
    void accountCreated(const QDBusObjectPath &path);
    void accountRemoved(const QDBusObjectPath &path);
    void accountValidityChanged(const QDBusObjectPath &path, bool valid);

protected:
    AccountManagerInterface *baseInterface() const;

private:
    void init();

    class Private;
    friend class Private;
    Private *mPriv;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AccountManager::Features)

} // Telepathy::Client
} // Telepathy

#endif
