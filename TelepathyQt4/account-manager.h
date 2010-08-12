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

#ifndef _TelepathyQt4_account_manager_h_HEADER_GUARD_
#define _TelepathyQt4_account_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-account-manager.h>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

#include <QDBusObjectPath>
#include <QSet>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class AccountManager;
class PendingAccount;
class PendingReady;

class TELEPATHY_QT4_EXPORT AccountManager : public StatelessDBusProxy,
                       public OptionalInterfaceFactory<AccountManager>,
                       public ReadyObject,
                       public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(AccountManager)

public:
    static const Feature FeatureCore;
    static const Feature FeatureFilterByCapabilities;

    // API/ABI break TODO: Remove these and have just all-default-argument versions with the factory
    // params
    static AccountManagerPtr create();
    static AccountManagerPtr create(const QDBusConnection &bus);

    // Needs to not have an accountFactory default param to not conflict with the above variants
    // until the API/ABI break
    static AccountManagerPtr create(
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());

    // The bus-taking variant should never have default factories unless the bus is the last param
    // which would be illogical?
    static AccountManagerPtr create(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());

    virtual ~AccountManager();

    AccountFactoryConstPtr accountFactory() const;
    ConnectionFactoryConstPtr connectionFactory() const;
    ChannelFactoryConstPtr channelFactory() const;
    ContactFactoryConstPtr contactFactory() const;

    TELEPATHY_QT4_DEPRECATED QStringList validAccountPaths() const;
    TELEPATHY_QT4_DEPRECATED QStringList invalidAccountPaths() const;
    TELEPATHY_QT4_DEPRECATED QStringList allAccountPaths() const;

    TELEPATHY_QT4_DEPRECATED QList<AccountPtr> validAccounts();
    TELEPATHY_QT4_DEPRECATED QList<AccountPtr> invalidAccounts();

    QList<AccountPtr> allAccounts();

    AccountSetPtr validAccountsSet() const;
    AccountSetPtr invalidAccountsSet() const;

    AccountSetPtr enabledAccountsSet() const;
    AccountSetPtr disabledAccountsSet() const;

    AccountSetPtr onlineAccountsSet() const;
    AccountSetPtr offlineAccountsSet() const;

    AccountSetPtr supportsTextChatsAccountsSet() const;
    AccountSetPtr supportsTextChatroomsAccountsSet() const;
    AccountSetPtr supportsMediaCallsAccountsSet() const;
    AccountSetPtr supportsAudioCallsAccountsSet() const;
    AccountSetPtr supportsVideoCallsAccountsSet(bool withAudio = true) const;
    AccountSetPtr supportsFileTransfersAccountsSet() const;

    AccountSetPtr accountsByProtocol(
            const QString &protocolName) const;

    AccountSetPtr filterAccounts(const QVariantMap &filter) const;

    TELEPATHY_QT4_DEPRECATED AccountPtr accountForPath(const QString &path);
    TELEPATHY_QT4_DEPRECATED QList<AccountPtr> accountsForPaths(const QStringList &paths);

    QStringList supportedAccountProperties() const;
    PendingAccount *createAccount(const QString &connectionManager,
            const QString &protocol, const QString &displayName,
            const QVariantMap &parameters,
            const QVariantMap &properties = QVariantMap());

    inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory<AccountManager>::interface<Client::DBus::PropertiesInterface>();
    }

Q_SIGNALS:
    TELEPATHY_QT4_DEPRECATED void accountCreated(const QString &path);
    TELEPATHY_QT4_DEPRECATED void accountRemoved(const QString &path);
    TELEPATHY_QT4_DEPRECATED void accountValidityChanged(const QString &path,
            bool valid);

    void newAccount(const Tp::AccountPtr &account);

protected:
    AccountManager();
    AccountManager(const QDBusConnection &bus);
    AccountManager(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    Client::AccountManagerInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);
    void onAccountReady(Tp::PendingOperation *);
    void onAccountValidityChanged(const QDBusObjectPath &, bool);
    void onAccountRemoved(const QDBusObjectPath &);
    void onAccountsCapabilitiesReady(Tp::PendingOperation *);

private:
    friend class PendingAccount;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
