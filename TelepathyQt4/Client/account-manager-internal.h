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

#ifndef _TelepathyQt4_cli_account_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt4_cli_account_manager_internal_h_HEADER_GUARD_

#include <TelepathyQt4/Client/PendingOperation>

#include <QQueue>
#include <QSet>
#include <QStringList>

namespace Telepathy
{
namespace Client
{

class AccountManager;
class AccountManagerInterface;

class AccountManager::Private : public QObject
{
    Q_OBJECT

public:
    Private(AccountManager *parent);
    ~Private();

    void callGetAll();
    void setAccountPaths(QSet<QString> &set, const QVariant &variant);

    class PendingReady;

    AccountManagerInterface *baseInterface;
    bool ready;
    PendingReady *pendingReady;
    QQueue<void (Private::*)()> introspectQueue;
    QStringList interfaces;
    AccountManager::Features features;
    QSet<QString> validAccountPaths;
    QSet<QString> invalidAccountPaths;

Q_SIGNALS:
    void accountCreated(const QString &path);
    void accountRemoved(const QString &path);
    void accountValidityChanged(const QString &path, bool valid);

private Q_SLOTS:
    void onGetAllAccountManagerReturn(QDBusPendingCallWatcher *);
    void onAccountValidityChanged(const QDBusObjectPath &, bool);
    void onAccountRemoved(const QDBusObjectPath &);
    void continueIntrospection();
};

class AccountManager::Private::PendingReady : public PendingOperation
{
    // AccountManager::Private is a friend so it can call finished() etc.
    friend class AccountManager::Private;

public:
    PendingReady(AccountManager *parent);
};

} // Telepathy::Client
} // Telepathy

#endif
