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
#include <TelepathyQt4/StatelessDBusProxy>
#include <TelepathyQt4/Filter>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

#include <QDBusObjectPath>
#include <QSet>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class PendingAccount;

class TELEPATHY_QT4_EXPORT AccountManager : public StatelessDBusProxy,
                public OptionalInterfaceFactory<AccountManager>
{
    Q_OBJECT
    Q_DISABLE_COPY(AccountManager)

public:
    static const Feature FeatureCore;

    static AccountManagerPtr create(const QDBusConnection &bus);
    static AccountManagerPtr create(
            const AccountFactoryConstPtr &accountFactory =
                AccountFactory::create(QDBusConnection::sessionBus(), Account::FeatureCore),
            const ConnectionFactoryConstPtr &connectionFactory =
                ConnectionFactory::create(QDBusConnection::sessionBus()),
            const ChannelFactoryConstPtr &channelFactory =
                ChannelFactory::create(QDBusConnection::sessionBus()),
            const ContactFactoryConstPtr &contactFactory =
                ContactFactory::create());
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

    QList<AccountPtr> allAccounts() const;

    AccountSetPtr validAccounts() const;
    AccountSetPtr invalidAccounts() const;

    AccountSetPtr enabledAccounts() const;
    AccountSetPtr disabledAccounts() const;

    AccountSetPtr onlineAccounts() const;
    AccountSetPtr offlineAccounts() const;

    AccountSetPtr textChatAccounts() const;
    AccountSetPtr textChatroomAccounts() const;

    AccountSetPtr streamedMediaCallAccounts() const;
    AccountSetPtr streamedMediaAudioCallAccounts() const;
    AccountSetPtr streamedMediaVideoCallAccounts() const;
    AccountSetPtr streamedMediaVideoCallWithAudioAccounts() const;

    AccountSetPtr fileTransferAccounts() const;

    AccountSetPtr accountsByProtocol(const QString &protocolName) const;

    AccountSetPtr filterAccounts(const AccountFilterConstPtr &filter) const;
    AccountSetPtr filterAccounts(const QVariantMap &filter) const;

    AccountPtr accountForPath(const QString &path) const;
    QList<AccountPtr> accountsForPaths(const QStringList &paths) const;

    QStringList supportedAccountProperties() const;
    PendingAccount *createAccount(const QString &connectionManager,
            const QString &protocol, const QString &displayName,
            const QVariantMap &parameters,
            const QVariantMap &properties = QVariantMap());

Q_SIGNALS:
    void newAccount(const Tp::AccountPtr &account);

protected:
    AccountManager(const QDBusConnection &bus,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory,
            const Feature &coreFeature);

    Client::AccountManagerInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onAccountReady(Tp::PendingOperation *op);
    void onAccountValidityChanged(const QDBusObjectPath &objectPath,
            bool valid);
    void onAccountRemoved(const QDBusObjectPath &objectPath);

private:
    friend class PendingAccount;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
