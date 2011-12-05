/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_examples_roster_roster_widget_h_HEADER_GUARD_
#define _TelepathyQt_examples_roster_roster_widget_h_HEADER_GUARD_

#include <QWidget>

#include <TelepathyQt/Contact>
#include <TelepathyQt/Connection>

namespace Tp {
class Connection;
class PendingOperation;
}

class QAction;
class QDialog;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class RosterItem;

class RosterWidget : public QWidget
{
    Q_OBJECT

public:
    RosterWidget(QWidget *parent = 0);
    virtual ~RosterWidget();

    Tp::ConnectionPtr connection() const { return mConn; }
    void setConnection(const Tp::ConnectionPtr &conn);
    void unsetConnection();

    QListWidget *listWidget() const { return mList; }

protected:
    virtual RosterItem *createItemForContact(
            const Tp::ContactPtr &contact,
            bool &exists);
    virtual void updateActions(RosterItem *item) { }

private Q_SLOTS:
    void onContactManagerStateChanged(Tp::ContactListState state);
    void onPresencePublicationRequested(const Tp::Contacts &);
    void onItemSelectionChanged();
    void onAddButtonClicked();
    void onAuthActionTriggered(bool);
    void onDenyActionTriggered(bool);
    void onRemoveActionTriggered(bool);
    void onBlockActionTriggered(bool);
    void onContactRetrieved(Tp::PendingOperation *op);
    void updateActions();

private:
    void createActions();
    void setupGui();

    Tp::ConnectionPtr mConn;
    QAction *mAuthAction;
    QAction *mRemoveAction;
    QAction *mDenyAction;
    QAction *mBlockAction;
    QListWidget *mList;
    QPushButton *mAddBtn;
    QDialog *mAddDlg;
    QLineEdit *mAddDlgEdt;
};

#endif
