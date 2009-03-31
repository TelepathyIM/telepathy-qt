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

#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>
#include <TelepathyQt4/Client/ReadinessHelper>
#include <TelepathyQt4/Client/ReadyObject>
#include <TelepathyQt4/Client/Types>
#include <TelepathyQt4/SharedPtr>

#include <QDBusObjectPath>
#include <QSet>
#include <QString>
#include <QVariantMap>

namespace Telepathy
{
namespace Client
{

class AccountManager;
class PendingAccount;
class PendingReady;

class AccountManager : public StatelessDBusProxy,
                       private OptionalInterfaceFactory<AccountManager>,
                       public ReadyObject,
                       public SharedData
{
    Q_OBJECT
    Q_DISABLE_COPY(AccountManager)

public:
    static const Feature FeatureCore;

    AccountManager(QObject *parent = 0);
    AccountManager(const QDBusConnection &bus, QObject *parent = 0);

    virtual ~AccountManager();

    QStringList interfaces() const;

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory<AccountManager>::interface<DBus::PropertiesInterface>();
    }

    QStringList validAccountPaths() const;
    QStringList invalidAccountPaths() const;
    QStringList allAccountPaths() const;

    QList<AccountPtr> validAccounts();
    QList<AccountPtr> invalidAccounts();
    QList<AccountPtr> allAccounts();

    AccountPtr accountForPath(const QString &path);
    QList<AccountPtr> accountsForPaths(const QStringList &paths);

    PendingAccount *createAccount(const QString &connectionManager,
            const QString &protocol, const QString &displayName,
            const QVariantMap &parameters);

    // TODO: enabledAccounts(), accountsByProtocol(), ... ?

Q_SIGNALS:
    void accountCreated(const QString &path);
    void accountRemoved(const QString &path);
    void accountValidityChanged(const QString &path, bool valid);

protected:
    AccountManagerInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);
    void onAccountValidityChanged(const QDBusObjectPath &, bool);
    void onAccountRemoved(const QDBusObjectPath &);

private:
    struct Private;
    friend struct Private;
    friend class PendingAccount;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
