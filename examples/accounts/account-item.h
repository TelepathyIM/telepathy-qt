/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_examples_accounts_account_item_h_HEADER_GUARD_
#define _TelepathyQt_examples_accounts_account_item_h_HEADER_GUARD_

#include <TelepathyQt/Types>
#include <TelepathyQt/Account>
#include <TelepathyQt/Types>

#include <QString>

namespace Tp {
class AccountManager;
class PendingOperation;
}

class QTableWidget;

class AccountItem : public QObject
{
    Q_OBJECT

public:
    enum Columns {
        ColumnValid = 0,
        ColumnEnabled,
        ColumnConnectionManager,
        ColumnProtocol,
        ColumnDisplayName,
        ColumnNickname,
        ColumnConnectsAutomatically,
        ColumnChangingPresence,
        ColumnAutomaticPresence,
        ColumnCurrentPresence,
        ColumnRequestedPresence,
        ColumnConnectionStatus,
        ColumnConnection,
        NumColumns
    };
    Q_ENUMS(Columns)

    AccountItem(Tp::AccountPtr acc, QTableWidget *table, int row, QObject *parent = 0);
    virtual ~AccountItem();

    int row() const { return mRow; }

private Q_SLOTS:
    void onValidityChanged(bool);
    void onStateChanged(bool);
    void onDisplayNameChanged(const QString &);
    void onNicknameChanged(const QString &);
    void onConnectsAutomaticallyPropertyChanged(bool);
    void onChangingPresenceChanged(bool);
    void onAutomaticPresenceChanged(const Tp::SimplePresence &);
    void onCurrentPresenceChanged(const Tp::SimplePresence &);
    void onRequestedPresenceChanged(const Tp::SimplePresence &);
    void onStatusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason,
            const QString &error, const QVariantMap &errorDetails);
    void onHaveConnectionChanged(bool);

private:
    void init();
    void setupGui();

    Tp::AccountPtr mAcc;
    QTableWidget *mTable;
    int mRow;
};

#endif
