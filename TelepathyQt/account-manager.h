/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt_account_manager_h_HEADER_GUARD_
#define _TelepathyQt_account_manager_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-account-manager.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/StatelessDBusProxy>
#include <TelepathyQt/Filter>
#include <TelepathyQt/OptionalInterfaceFactory>
#include <TelepathyQt/SharedPtr>
#include <TelepathyQt/Types>

#include <QDBusObjectPath>
#include <QSet>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class PendingAccount;

class TP_QT_EXPORT AccountManager : public StatelessDBusProxy,
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

    AccountSetPtr audioCallAccounts() const;
    AccountSetPtr videoCallAccounts() const;

    TP_QT_DEPRECATED AccountSetPtr streamedMediaCallAccounts() const;
    TP_QT_DEPRECATED AccountSetPtr streamedMediaAudioCallAccounts() const;
    TP_QT_DEPRECATED AccountSetPtr streamedMediaVideoCallAccounts() const;
    TP_QT_DEPRECATED AccountSetPtr streamedMediaVideoCallWithAudioAccounts() const;

    AccountSetPtr fileTransferAccounts() const;

    AccountSetPtr accountsByProtocol(const QString &protocolName) const;

    AccountSetPtr filterAccounts(const AccountFilterConstPtr &filter) const;
    AccountSetPtr filterAccounts(const QVariantMap &filter) const;

    AccountPtr accountForObjectPath(const QString &path) const;
    TP_QT_DEPRECATED AccountPtr accountForPath(const QString &path) const;
    QList<AccountPtr> accountsForObjectPaths(const QStringList &paths) const;
    TP_QT_DEPRECATED QList<AccountPtr> accountsForPaths(const QStringList &paths) const;

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
    TP_QT_NO_EXPORT void introspectMain();
    TP_QT_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onAccountReady(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onAccountValidityChanged(const QDBusObjectPath &objectPath,
            bool valid);
    TP_QT_NO_EXPORT void onAccountRemoved(const QDBusObjectPath &objectPath);

private:
    friend class PendingAccount;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
