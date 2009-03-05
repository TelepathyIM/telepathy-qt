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

#include "roster-window.h"
#include "_gen/roster-window.moc.hpp"

#include "roster-item.h"

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingConnection>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/PendingReady>

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

using namespace Telepathy::Client;

RosterWindow::RosterWindow(const QString &username, const QString &password,
        QWidget *parent)
    : QMainWindow(parent),
      mUsername(username),
      mPassword(password)
{
    setWindowTitle("Roster");

    createActions();
    setupGui();

    mCM = new ConnectionManager("gabble", this);
    connect(mCM->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onCMReady(Telepathy::Client::PendingOperation *)));

    resize(240, 320);
}

RosterWindow::~RosterWindow()
{
}

void RosterWindow::createActions()
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

void RosterWindow::setupGui()
{
    QWidget *frame = new QWidget;

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

    mAddBtn = new QPushButton("+");
    mAddBtn->setEnabled(false);
    connect(mAddBtn,
            SIGNAL(clicked(bool)),
            SLOT(onAddButtonClicked()));
    hbox->addWidget(mAddBtn);
    hbox->addStretch(1);

    vbox->addLayout(hbox);

    frame->setLayout(vbox);

    setCentralWidget(frame);

    mAddDlg = new QDialog(this);
    mAddDlg->setWindowTitle("Add Contact");
    QVBoxLayout *addDlgVBox = new QVBoxLayout;

    QHBoxLayout *addDlgEntryHBox = new QHBoxLayout;
    QLabel *label = new QLabel("Username");
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

void RosterWindow::createItemForContact(const QSharedPointer<Contact> &contact,
        bool checkExists)
{
    bool found = false;
    if (checkExists) {
        for (int i = 0; i < mList->count(); ++i) {
            RosterItem *item = dynamic_cast<RosterItem*>(mList->item(i));
            if (item->contact() == contact) {
                found = true;
            }
        }
    }

    if (!found) {
        RosterItem *item = new RosterItem(contact, mList);
        connect(item, SIGNAL(changed()), SLOT(updateActions()));
    }
}

void RosterWindow::onCMReady(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CM cannot become ready";
        return;
    }

    qDebug() << "CM ready";
    QVariantMap params;
    params.insert("account", QVariant(mUsername));
    params.insert("password", QVariant(mPassword));
    PendingConnection *pconn = mCM->requestConnection("jabber", params);
    connect(pconn,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onConnectionCreated(Telepathy::Client::PendingOperation *)));
}

void RosterWindow::onConnectionCreated(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create connection";
        return;
    }

    qDebug() << "Connection created";
    PendingConnection *pconn =
        qobject_cast<PendingConnection *>(op);
    mConn = pconn->connection();
    QSet<uint> features = QSet<uint>() << Connection::FeatureRoster;
    connect(mConn->requestConnect(features),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onConnectionReady(Telepathy::Client::PendingOperation *)));
}

void RosterWindow::onConnectionReady(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Connection cannot become ready";
        return;
    }

    connect(mConn->contactManager(),
            SIGNAL(presencePublicationRequested(const Telepathy::Client::Contacts &)),
            SLOT(onPresencePublicationRequested(const Telepathy::Client::Contacts &)));

    qDebug() << "Connection ready";
    foreach (const QSharedPointer<Contact> &contact, mConn->contactManager()->allKnownContacts()) {
        createItemForContact(contact);
    }

    mAddBtn->setEnabled(true);
}

void RosterWindow::onPresencePublicationRequested(const Contacts &contacts)
{
    qDebug() << "Presence publication requested";
    foreach (const QSharedPointer<Contact> &contact, contacts) {
        createItemForContact(contact, true);
    }
}

void RosterWindow::onItemSelectionChanged()
{
    updateActions();
}

void RosterWindow::onAddButtonClicked()
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
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onContactRetrieved(Telepathy::Client::PendingOperation *)));
}

void RosterWindow::onAuthActionTriggered(bool checked)
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

void RosterWindow::onDenyActionTriggered(bool checked)
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

void RosterWindow::onRemoveActionTriggered(bool checked)
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

void RosterWindow::onBlockActionTriggered(bool checked)
{
    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    item->contact()->block(checked);
}

void RosterWindow::onContactRetrieved(Telepathy::Client::PendingOperation *op)
{
    PendingContacts *pcontacts = qobject_cast<PendingContacts *>(op);
    QList<QSharedPointer<Contact> > contacts = pcontacts->contacts();
    Q_ASSERT(pcontacts->identifiers().size() == 1);
    QString username = pcontacts->identifiers().first();
    if (contacts.size() != 1 || !contacts.first()) {
        QMessageBox msgBox;
        msgBox.setText(QString("Unable to add contact \"%1\"").arg(username));
        msgBox.exec();
        return;
    }

    QSharedPointer<Contact> contact = contacts.first();
    qDebug() << "Request presence subscription for contact" << username;
    // TODO should we have a signal on ContactManager to signal that a contact was
    //      added to subscribe list?
    createItemForContact(contact, true);
    contact->requestPresenceSubscription();
}

void RosterWindow::updateActions()
{
    QList<QListWidgetItem *> selectedItems = mList->selectedItems();
    if (selectedItems.isEmpty()) {
        mAuthAction->setEnabled(false);
        mDenyAction->setEnabled(false);
        mRemoveAction->setEnabled(false);
        mBlockAction->setEnabled(false);
        return;
    }
    Q_ASSERT(selectedItems.size() == 1);

    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    QSharedPointer<Contact> contact = item->contact();

    ContactManager *manager = contact->manager();
    qDebug() << "Contact" << contact->id() << "selected";
    qDebug() << " subscription state:" << contact->subscriptionState();
    qDebug() << " publish state     :" << contact->publishState();
    qDebug() << " blocked           :" << contact->isBlocked();

    if (manager->canAuthorizeContactsPresencePublication() &&
        contact->publishState() == Contact::PresenceStateAsk) {
        mAuthAction->setEnabled(true);
    } else {
        mAuthAction->setEnabled(false);
    }

    if (manager->canRemoveContactsPresencePublication() &&
        contact->publishState() != Contact::PresenceStateNo) {
        mDenyAction->setEnabled(true);
    } else {
        mDenyAction->setEnabled(false);
    }

    if (manager->canRemoveContactsPresenceSubscription() &&
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
}
