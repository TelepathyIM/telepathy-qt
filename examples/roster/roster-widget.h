/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_examples_roster_roster_widget_h_HEADER_GUARD_
#define _TelepathyQt4_examples_roster_roster_widget_h_HEADER_GUARD_

#include <QWidget>

#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Connection>

namespace Telepathy {
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

    QList<Telepathy::ConnectionPtr> connections() const { return mConns; }
    void addConnection(const Telepathy::ConnectionPtr &conn);
    void removeConnection(const Telepathy::ConnectionPtr &conn);

    QListWidget *listWidget() const { return mList; }

protected:
    virtual RosterItem *createItemForContact(
            const Telepathy::ContactPtr &contact,
            bool &exists);
    virtual void updateActions(RosterItem *item) { }

private Q_SLOTS:
    void onConnectionReady(Telepathy::PendingOperation *);
    void onPresencePublicationRequested(const Telepathy::Contacts &);
    void onItemSelectionChanged();
    void onAddButtonClicked();
    void onAuthActionTriggered(bool);
    void onDenyActionTriggered(bool);
    void onRemoveActionTriggered(bool);
    void onBlockActionTriggered(bool);
    void onContactRetrieved(Telepathy::PendingOperation *op);
    void updateActions();

private:
    void createActions();
    void setupGui();

    QList<Telepathy::ConnectionPtr> mConns;
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
