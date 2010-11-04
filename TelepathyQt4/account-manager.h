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
#include <TelepathyQt4/Filter>
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

    // FIXME: (API/ABI break) Remove both constructors that don't take factories as params.
    static AccountManagerPtr create();
    static AccountManagerPtr create(const QDBusConnection &bus);

    // FIXME: (API/ABI break) Add a default parameter to accountFactory once the create methods
    //        above are removed
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

    // API/ABI break TODO: make const
    QList<AccountPtr> allAccounts();

    // FIXME: (API/ABI break) Rename all xxxSet methods to just xxx
    AccountSetPtr validAccountsSet() const;
    AccountSetPtr invalidAccountsSet() const;

    AccountSetPtr enabledAccountsSet() const;
    AccountSetPtr disabledAccountsSet() const;

    AccountSetPtr onlineAccountsSet() const;
    AccountSetPtr offlineAccountsSet() const;

    AccountSetPtr textChatAccountsSet() const;
    AccountSetPtr textChatRoomAccountsSet() const;

    // FIXME: (API/ABI break) Rename SM related methods to contain SM in the name, like
    //                        streamedMediaAudioCallAccounts(), etc.
    AccountSetPtr mediaCallAccountsSet() const;
    AccountSetPtr audioCallAccountsSet() const;
    AccountSetPtr videoCallAccountsSet(bool withAudio = true) const;

    AccountSetPtr fileTransferAccountsSet() const;

    AccountSetPtr accountsByProtocol(
            const QString &protocolName) const;

    AccountSetPtr filterAccounts(const AccountFilterConstPtr &filter) const;
    AccountSetPtr filterAccounts(const QList<AccountFilterConstPtr> &filters) const;
    AccountSetPtr filterAccounts(const QVariantMap &filter) const;

    // API/ABI break TODO: make const
    AccountPtr accountForPath(const QString &path);
    // API/ABI break TODO: make const
    QList<AccountPtr> accountsForPaths(const QStringList &paths);

    QStringList supportedAccountProperties() const;
    PendingAccount *createAccount(const QString &connectionManager,
            const QString &protocol, const QString &displayName,
            const QVariantMap &parameters,
            const QVariantMap &properties = QVariantMap());

    TELEPATHY_QT4_DEPRECATED inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory<AccountManager>::interface<Client::DBus::PropertiesInterface>();
    }

Q_SIGNALS:
    // FIXME: (API/ABI break) Remove accountCreated in favor of newAccount
    void accountCreated(const QString &path);
    // FIXME: (API/ABI break) Remove accountRemoved in favor of Account::removed
    void accountRemoved(const QString &path);
    // FIXME: (API/ABI break) Remove accountValidityChanged in favor of Account::validityChanged
    void accountValidityChanged(const QString &path,
            bool valid);

    // FIXME: (API/ABI break) Rename to accountCreated. Actually it should have being named
    //        accountCreated in the first place
    void newAccount(const Tp::AccountPtr &account);

protected:
    TELEPATHY_QT4_DEPRECATED AccountManager();
    TELEPATHY_QT4_DEPRECATED AccountManager(const QDBusConnection &bus);
    AccountManager(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    Client::AccountManagerInterface *baseInterface() const;

    // FIXME: (API/ABI break) Remove connectNotify
    void connectNotify(const char *);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);
    void onAccountReady(Tp::PendingOperation *);
    void onAccountValidityChanged(const QDBusObjectPath &, bool);
    void onAccountRemoved(const QDBusObjectPath &);

private:
    friend class PendingAccount;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
