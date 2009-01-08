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

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QDBusObjectPath>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Telepathy
{
namespace Client
{

class AccountManager;
class PendingConnection;
class PendingOperation;
class ProtocolInfo;

class Account : public StatelessDBusProxy,
        private OptionalInterfaceFactory
{
    Q_OBJECT

public:
    enum Feature {
        /** Get the avatar data */
        FeatureAvatar = 1,
        FeatureProtocolInfo = 2,
        _Padding = 0xFFFFFFFF
    };
    Q_DECLARE_FLAGS(Features, Feature)

    Account(AccountManager *am, const QDBusObjectPath &objectPath,
            QObject *parent = 0);

    virtual ~Account();

    QStringList interfaces() const;

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory::interface<DBus::PropertiesInterface>(
                *baseInterface());
    }

    AccountManager *manager() const;

    bool isValid() const;

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
    QByteArray avatarData() const;
    QString avatarMimeType() const;
    PendingOperation *setAvatar(const QByteArray &data, const QString &mimeType);

    QVariantMap parameters() const;
    PendingOperation *updateParameters(const QVariantMap &set,
            const QStringList &unset);

    // comes from the ConnectionManager
    ProtocolInfo *protocolInfo() const;

    Telepathy::SimplePresence automaticPresence() const;
    PendingOperation *setAutomaticPresence(
            const Telepathy::SimplePresence &value);

    bool connectsAutomatically() const;
    PendingOperation *setConnectsAutomatically(bool value);

    Telepathy::ConnectionStatus connectionStatus() const;
    Telepathy::ConnectionStatusReason connectionStatusReason() const;
    // not finished until the Connection is ready
    // PendingConnection *getConnection(Connection::Features features = 0) const;

    Telepathy::SimplePresence currentPresence() const;

    Telepathy::SimplePresence requestedPresence() const;
    PendingOperation *setRequestedPresence(
            const Telepathy::SimplePresence &value);

    // Unique per AccountManager implementation, i.e. at least per
    // QDBusConnection.
    //
    // returns the tail of the object path, i.e.
    // /org/freedesktop/Account/gabble/jabber/smcv_5eexample_2bcom ->
    // gabble/jabber/smcv_5eexample_2bcom
    QString uniqueIdentifier() const;

    // doc as for advanced users
    QString connectionObjectPath() const;

    QString normalizedName() const;

    PendingOperation *remove();

    bool isReady(Features features = 0) const;

    PendingOperation *becomeReady(Features features = 0);

Q_SIGNALS:
    void displayNameChanged(const QString &);
    void presenceChanged(const Telepathy::SimplePresence &) const;
    void avatarChanged(const QByteArray &data, const QString &mimeType);
    void connectionStatusChanged(Telepathy::ConnectionStatus,
            Telepathy::ConnectionStatusReason);

protected:
    AccountInterface *baseInterface() const;

private:
    Q_DISABLE_COPY(Account);

    class Private;
    friend class Private;
    Private *mPriv;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Account::Features)

} // Telepathy::Client
} // Telepathy

#endif
