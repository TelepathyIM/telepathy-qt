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

#ifndef _TelepathyQt4_cli_account_h_HEADER_GUARD_
#define _TelepathyQt4_cli_account_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-account.h>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/SharedPtr>

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Telepathy
{

class Account;
class Connection;
class PendingConnection;
class PendingOperation;
class PendingReady;
class ProtocolInfo;

class Account : public StatelessDBusProxy,
                private OptionalInterfaceFactory<Account>,
                public ReadyObject,
                public RefCounted

{
    Q_OBJECT
    Q_DISABLE_COPY(Account)

public:
    static const Feature FeatureCore;
    static const Feature FeatureAvatar;
    static const Feature FeatureProtocolInfo;

    static AccountPtr create(const QDBusConnection &bus,
            const QString &busName, const QString &objectPath);
    static AccountPtr create(const QString &busName,
            const QString &objectPath);

    virtual ~Account();

    bool isValidAccount() const;

    bool isEnabled() const;
    PendingOperation *setEnabled(bool value);

    QString cmName() const;

    QString protocol() const;

    QString displayName() const;
    PendingOperation *setDisplayName(const QString &value);

    QString icon() const;
    PendingOperation *setIcon(const QString &value);

    QString nickname() const;
    PendingOperation *setNickname(const QString &value);

    // requires spec 0.17.16
    const Telepathy::Avatar &avatar() const;
    PendingOperation *setAvatar(const Telepathy::Avatar &avatar);

    QVariantMap parameters() const;
    PendingOperation *updateParameters(const QVariantMap &set,
            const QStringList &unset);

    // comes from the ConnectionManager
    ProtocolInfo *protocolInfo() const;

    bool connectsAutomatically() const;
    PendingOperation *setConnectsAutomatically(bool value);

    Telepathy::ConnectionStatus connectionStatus() const;
    Telepathy::ConnectionStatusReason connectionStatusReason() const;
    bool haveConnection() const;
    ConnectionPtr connection() const;

    Telepathy::SimplePresence automaticPresence() const;
    PendingOperation *setAutomaticPresence(
            const Telepathy::SimplePresence &value);

    Telepathy::SimplePresence currentPresence() const;

    Telepathy::SimplePresence requestedPresence() const;
    PendingOperation *setRequestedPresence(
            const Telepathy::SimplePresence &value);

    QString uniqueIdentifier() const;

    // doc as for advanced users
    QString connectionObjectPath() const;

    QString normalizedName() const;

    PendingOperation *remove();

    QStringList interfaces() const;

    template <class Interface>
    inline Interface *optionalInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && !interfaces().contains(name)) {
            return 0;
        }

        // If present or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory<Account>::interface<Interface>();
    }

    inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<Client::DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

    inline Client::AccountInterfaceAvatarInterface *avatarInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        return optionalInterface<Client::AccountInterfaceAvatarInterface>(check);
    }

Q_SIGNALS:
    void displayNameChanged(const QString &);
    void iconChanged(const QString &);
    void nicknameChanged(const QString &);
    void normalizedNameChanged(const QString &);
    void validityChanged(bool);
    void stateChanged(bool);
    void connectsAutomaticallyPropertyChanged(bool);
    void parametersChanged(const QVariantMap &);
    void automaticPresenceChanged(const Telepathy::SimplePresence &) const;
    void currentPresenceChanged(const Telepathy::SimplePresence &) const;
    void requestedPresenceChanged(const Telepathy::SimplePresence &) const;
    void avatarChanged(const Telepathy::Avatar &);
    void connectionStatusChanged(Telepathy::ConnectionStatus,
            Telepathy::ConnectionStatusReason);
    void haveConnectionChanged(bool haveConnection);

protected:
    Account(const QString &busName, const QString &objectPath);
    Account(const QDBusConnection &bus,
            const QString &busName, const QString &objectPath);

    Client::AccountInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);
    void gotAvatar(QDBusPendingCallWatcher *);
    void onAvatarChanged();
    void onConnectionManagerReady(Telepathy::PendingOperation *);
    void onPropertyChanged(const QVariantMap &delta);
    void onRemoved();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy

#endif
