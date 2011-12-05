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

#ifndef _TelepathyQt_examples_accounts_accounts_window_h_HEADER_GUARD_
#define _TelepathyQt_examples_accounts_accounts_window_h_HEADER_GUARD_

#include <QMainWindow>

#include <TelepathyQt/Types>

namespace Tp {
class PendingOperation;
}

class QTableWidget;
class QTableWidgetItem;

class AccountsWindow : public QMainWindow
{
    Q_OBJECT

public:
    AccountsWindow(QWidget *parent = 0);
    virtual ~AccountsWindow();

private Q_SLOTS:
    void onAMReady(Tp::PendingOperation *);
    void onNewAccount(const Tp::AccountPtr &);

private:
    void setupGui();

    Tp::AccountManagerPtr mAM;
    QTableWidget *mTable;
};

#endif
