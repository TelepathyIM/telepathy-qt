/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt_pending_account_h_HEADER_GUARD_
#define _TelepathyQt_pending_account_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Account>
#include <TelepathyQt/PendingOperation>

#include <QString>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace Tp
{

class AccountManager;

class TP_QT_EXPORT PendingAccount : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingAccount);

public:
    ~PendingAccount();

    AccountManagerPtr manager() const;

    AccountPtr account() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onCallFinished(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onAccountBuilt(Tp::PendingOperation *readyOp);
    TP_QT_NO_EXPORT void onNewAccount(const Tp::AccountPtr &account);

private:
    friend class AccountManager;

    TP_QT_NO_EXPORT PendingAccount(const AccountManagerPtr &manager,
            const QString &connectionManager, const QString &protocol,
            const QString &displayName, const QVariantMap &parameters,
            const QVariantMap &properties = QVariantMap());

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
