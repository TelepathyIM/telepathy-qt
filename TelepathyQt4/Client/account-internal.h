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

#ifndef _TelepathyQt4_cli_account_internal_h_HEADER_GUARD_
#define _TelepathyQt4_cli_account_internal_h_HEADER_GUARD_

#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QByteArray>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Telepathy
{
namespace Client
{

class Account;
class AccountInterface;
class ConnectionManager;
class ProtocolInfo;

class Account::Private : public QObject
{
    Q_OBJECT

public:
    Private(Account *parent);
    ~Private();

    void checkForAvatarInterface();
    void callGetAll();
    void callGetAvatar();
    void callGetProtocolInfo();
    void updateProperties(const QVariantMap &props);
    void retrieveAvatar();
    PendingOperation *becomeReady(Account::Features requestFeatures);

    class PendingReady;
    class PendingUpdateParameters;

    AccountInterface *baseInterface;
    bool ready;
    QList<PendingReady *> pendingOperations;
    QQueue<void (Private::*)()> introspectQueue;
    QStringList interfaces;
    Account::Features features;
    Account::Features pendingFeatures;
    Account::Features missingFeatures;
    QVariantMap parameters;
    bool valid;
    bool enabled;
    bool connectsAutomatically;
    QString cmName;
    QString protocol;
    QString displayName;
    QString nickname;
    QString icon;
    QString connectionObjectPath;
    QString normalizedName;
    QString avatarMimeType;
    QByteArray avatarData;
    ConnectionManager *cm;
    ProtocolInfo *protocolInfo;
    Telepathy::ConnectionStatus connectionStatus;
    Telepathy::ConnectionStatusReason connectionStatusReason;
    Telepathy::SimplePresence automaticPresence;
    Telepathy::SimplePresence currentPresence;
    Telepathy::SimplePresence requestedPresence;

Q_SIGNALS:
    void removed();
    void displayNameChanged(const QString &);
    void iconChanged(const QString &);
    void nicknameChanged(const QString &);
    void stateChanged(bool);
    void validityChanged(bool);
    void parametersChanged(const QVariantMap &);
    void presenceChanged(const Telepathy::SimplePresence &) const;
    void avatarChanged(const QByteArray &, const QString &);
    void connectionStatusChanged(Telepathy::ConnectionStatus,
            Telepathy::ConnectionStatusReason);

private Q_SLOTS:
    void onGetAllAccountReturn(QDBusPendingCallWatcher *);
    void onGetAvatarReturn(QDBusPendingCallWatcher *);
    void onAvatarChanged();
    void onConnectionManagerReady(Telepathy::Client::PendingOperation *);
    void onPropertyChanged(const QVariantMap &delta);
    void onRemoved();
    void continueIntrospection();
};

class Account::Private::PendingReady : public PendingOperation
{
    // Account::Private is a friend so it can call finished() etc.
    friend class Account::Private;

public:
    PendingReady(Account::Features features, QObject *parent = 0);

    Account::Features features;
};

} // Telepathy::Client
} // Telepathy

#endif
