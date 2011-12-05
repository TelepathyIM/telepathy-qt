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

#include "roster-widget.h"
#include "_gen/roster-widget.moc.hpp"

#include "roster-item.h"

#include <TelepathyQt/Types>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>

#include <QAction>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Tp;

RosterWidget::RosterWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QLatin1String("Roster"));

    createActions();
    setupGui();
}

RosterWidget::~RosterWidget()
{
}

void RosterWidget::setConnection(const ConnectionPtr &conn)
{
    if (mConn) {
        unsetConnection();
    }

    mConn = conn;
    connect(conn->contactManager().data(),
            SIGNAL(presencePublicationRequested(const Tp::Contacts &)),
            SLOT(onPresencePublicationRequested(const Tp::Contacts &)));
    // TODO listen to allKnownContactsChanged

    connect(conn->contactManager().data(),
            SIGNAL(stateChanged(Tp::ContactListState)),
            SLOT(onContactManagerStateChanged(Tp::ContactListState)));
    onContactManagerStateChanged(conn->contactManager()->state());
}

void RosterWidget::unsetConnection()
{
    while (mList->count() > 0) {
        RosterItem *item = (RosterItem *) mList->item(0);
        mList->takeItem(0);
        delete item;
    }
    mConn.reset();
    updateActions();
    mAddBtn->setEnabled(false);
}

void RosterWidget::createActions()
{
    mAuthAction = new QAction(QLatin1String("Authorize Contact"), this);
    mAuthAction->setEnabled(false);
    connect(mAuthAction,
            SIGNAL(triggered(bool)),
            SLOT(onAuthActionTriggered(bool)));
    mDenyAction = new QAction(QLatin1String("Deny Contact"), this);
    mDenyAction->setEnabled(false);
    connect(mDenyAction,
            SIGNAL(triggered(bool)),
            SLOT(onDenyActionTriggered(bool)));
    mRemoveAction = new QAction(QLatin1String("Remove Contact"), this);
    mRemoveAction->setEnabled(false);
    connect(mRemoveAction,
            SIGNAL(triggered(bool)),
            SLOT(onRemoveActionTriggered(bool)));
    mBlockAction = new QAction(QLatin1String("Block Contact"), this);
    mBlockAction->setEnabled(false);
    mBlockAction->setCheckable(true);
    connect(mBlockAction,
            SIGNAL(triggered(bool)),
            SLOT(onBlockActionTriggered(bool)));
}

void RosterWidget::setupGui()
{
    QVBoxLayout *vbox = new QVBoxLayout;

    mList = new QListWidget;
    connect(mList,
            SIGNAL(itemSelectionChanged()),
            SLOT(onItemSelectionChanged()));
    vbox->addWidget(mList);

    mList->setContextMenuPolicy(Qt::ActionsContextMenu);
    mList->addAction(mAuthAction);
    mList->addAction(mDenyAction);
    mList->addAction(mRemoveAction);
    mList->addAction(mBlockAction);

    QHBoxLayout *hbox = new QHBoxLayout;

    mAddBtn = new QPushButton(QLatin1String("+"));
    mAddBtn->setEnabled(false);
    connect(mAddBtn,
            SIGNAL(clicked(bool)),
            SLOT(onAddButtonClicked()));
    hbox->addWidget(mAddBtn);
    hbox->addStretch(1);

    vbox->addLayout(hbox);

    setLayout(vbox);

    mAddDlg = new QDialog(this);
    mAddDlg->setWindowTitle(QLatin1String("Add Contact"));
    QVBoxLayout *addDlgVBox = new QVBoxLayout;

    QHBoxLayout *addDlgEntryHBox = new QHBoxLayout;
    QLabel *label = new QLabel(QLatin1String("Username"));
    addDlgEntryHBox->addWidget(label);
    mAddDlgEdt = new QLineEdit();
    addDlgEntryHBox->addWidget(mAddDlgEdt);
    addDlgVBox->addLayout(addDlgEntryHBox);

    QDialogButtonBox *addDlgBtnBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
    connect(addDlgBtnBox, SIGNAL(accepted()), mAddDlg, SLOT(accept()));
    connect(addDlgBtnBox, SIGNAL(rejected()), mAddDlg, SLOT(reject()));
    addDlgVBox->addWidget(addDlgBtnBox);

    mAddDlg->setLayout(addDlgVBox);
}

RosterItem *RosterWidget::createItemForContact(const ContactPtr &contact,
        bool &exists)
{
    RosterItem *item;
    exists = false;
    for (int i = 0; i < mList->count(); ++i) {
        item = dynamic_cast<RosterItem*>(mList->item(i));
        if (item->contact() == contact) {
            exists = true;
            return item;
        }
    }

    return new RosterItem(contact, mList);
}

void RosterWidget::onContactManagerStateChanged(ContactListState state)
{
    if (state == ContactListStateSuccess) {
        qDebug() << "Loading contacts";
        RosterItem *item;
        bool exists;
        foreach (const ContactPtr &contact, mConn->contactManager()->allKnownContacts()) {
            exists = false;
            item = createItemForContact(contact, exists);
            if (!exists) {
                connect(item, SIGNAL(changed()), SLOT(updateActions()));
            }
        }

        mAddBtn->setEnabled(true);
    }
}

void RosterWidget::onPresencePublicationRequested(const Contacts &contacts)
{
    qDebug() << "Presence publication requested";
    RosterItem *item;
    bool exists;
    foreach (const ContactPtr &contact, contacts) {
        exists = false;
        item = createItemForContact(contact, exists);
        if (!exists) {
            connect(item, SIGNAL(changed()), SLOT(updateActions()));
        }
    }
}

void RosterWidget::onItemSelectionChanged()
{
    updateActions();
}

void RosterWidget::onAddButtonClicked()
{
    mAddDlgEdt->clear();
    int ret = mAddDlg->exec();
    if (ret == QDialog::Rejected) {
        return;
    }

    QString username = mAddDlgEdt->text();
    PendingContacts *pcontacts = mConn->contactManager()->contactsForIdentifiers(
            QStringList() << username);
    connect(pcontacts,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContactRetrieved(Tp::PendingOperation *)));
}

void RosterWidget::onAuthActionTriggered(bool checked)
{
    Q_UNUSED(checked);

    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    if (item->contact()->publishState() != Contact::PresenceStateYes) {
        item->contact()->authorizePresencePublication();
    }
}

void RosterWidget::onDenyActionTriggered(bool checked)
{
    Q_UNUSED(checked);

    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    if (item->contact()->publishState() != Contact::PresenceStateNo) {
        // The contact can't see my presence
        item->contact()->removePresencePublication();
    }
}

void RosterWidget::onRemoveActionTriggered(bool checked)
{
    Q_UNUSED(checked);

    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    if (item->contact()->subscriptionState() != Contact::PresenceStateNo) {
        // The contact can't see my presence and I can't see his/her presence
        item->contact()->removePresencePublication();
        item->contact()->removePresenceSubscription();
    }
}

void RosterWidget::onBlockActionTriggered(bool checked)
{
    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    if (checked) {
        item->contact()->block();
    } else {
        item->contact()->unblock();
    }
}

void RosterWidget::onContactRetrieved(Tp::PendingOperation *op)
{
    PendingContacts *pcontacts = qobject_cast<PendingContacts *>(op);
    QList<ContactPtr> contacts = pcontacts->contacts();
    Q_ASSERT(pcontacts->identifiers().size() == 1);
    QString username = pcontacts->identifiers().first();
    if (contacts.size() != 1 || !contacts.first()) {
        QMessageBox msgBox;
        msgBox.setText(QString(QLatin1String("Unable to add contact \"%1\"")).arg(username));
        msgBox.exec();
        return;
    }

    ContactPtr contact = contacts.first();
    qDebug() << "Request presence subscription for contact" << username;
    bool exists = false;
    RosterItem *item = createItemForContact(contact, exists);
    if (!exists) {
        connect(item, SIGNAL(changed()), SLOT(updateActions()));
    }
    contact->requestPresenceSubscription();
}

void RosterWidget::updateActions()
{
    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        mAuthAction->setEnabled(false);
        mDenyAction->setEnabled(false);
        mRemoveAction->setEnabled(false);
        mBlockAction->setEnabled(false);
        updateActions(0);
        return;
    }
    Q_ASSERT(selectedItems.size() == 1);

    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    ContactPtr contact = item->contact();

    ContactManagerPtr manager = contact->manager();
    qDebug() << "Contact" << contact->id() << "selected";
    qDebug() << " subscription state:" << contact->subscriptionState();
    qDebug() << " publish state     :" << contact->publishState();
    qDebug() << " blocked           :" << contact->isBlocked();

    if (manager->canAuthorizePresencePublication() &&
        contact->publishState() == Contact::PresenceStateAsk) {
        mAuthAction->setEnabled(true);
    } else {
        mAuthAction->setEnabled(false);
    }

    if (manager->canRemovePresencePublication() &&
        contact->publishState() != Contact::PresenceStateNo) {
        mDenyAction->setEnabled(true);
    } else {
        mDenyAction->setEnabled(false);
    }

    if (manager->canRemovePresenceSubscription() &&
        contact->subscriptionState() != Contact::PresenceStateNo) {
        mRemoveAction->setEnabled(true);
    } else {
        mRemoveAction->setEnabled(false);
    }

    if (manager->canBlockContacts() &&
        contact->publishState() == Contact::PresenceStateYes) {
        mBlockAction->setEnabled(true);
    } else {
        mBlockAction->setEnabled(false);
    }

    mBlockAction->setChecked(contact->isBlocked());

    updateActions(item);
}
