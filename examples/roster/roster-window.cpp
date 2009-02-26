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
#include <TelepathyQt4/Client/PendingReadyConnectionManager>

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
    setupGui();

    mCM = new ConnectionManager("gabble", this);
    connect(mCM->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onCMReady(Telepathy::Client::PendingOperation *)));
}

RosterWindow::~RosterWindow()
{
}

void RosterWindow::setupGui()
{
    QWidget *frame = new QWidget;

    QVBoxLayout *vbox = new QVBoxLayout;

    mList = new QListWidget;
    vbox->addWidget(mList);

    QHBoxLayout *hbox = new QHBoxLayout;

    mAddBtn = new QPushButton("Add contact");
    mAddBtn->setEnabled(false);
    connect(mAddBtn,
            SIGNAL(clicked(bool)),
            SLOT(onAddButtonClicked()));
    hbox->addWidget(mAddBtn);

    mAuthBtn = new QPushButton("Authorize contact");
    mAuthBtn->setEnabled(false);
    connect(mAuthBtn,
            SIGNAL(clicked(bool)),
            SLOT(onAuthButtonClicked()));
    hbox->addWidget(mAuthBtn);

    mRemoveBtn = new QPushButton("Remove contact");
    mRemoveBtn->setEnabled(false);
    connect(mRemoveBtn,
            SIGNAL(clicked(bool)),
            SLOT(onRemoveButtonClicked()));
    hbox->addWidget(mRemoveBtn);

    mDenyBtn = new QPushButton("Deny contact");
    mDenyBtn->setEnabled(false);
    connect(mDenyBtn,
            SIGNAL(clicked(bool)),
            SLOT(onDenyButtonClicked()));
    hbox->addWidget(mDenyBtn);

    vbox->addLayout(hbox);

    frame->setLayout(vbox);

    setCentralWidget(frame);

    mAddDlg = new QDialog(this);
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
            SIGNAL(presencePublicationRequested(const QSet<QSharedPointer<Telepathy::Client::Contact> > &)),
            SLOT(onPresencePublicationRequested(const QSet<QSharedPointer<Telepathy::Client::Contact> > &)));

    qDebug() << "Connection ready";
    foreach (const QSharedPointer<Contact> &contact, mConn->contactManager()->allKnownContacts()) {
        (void) new RosterItem(contact, mList);
    }

    mAddBtn->setEnabled(true);
    mAuthBtn->setEnabled(true);
    mRemoveBtn->setEnabled(true);
    mDenyBtn->setEnabled(true);
}

void RosterWindow::onPresencePublicationRequested(const QSet<QSharedPointer<Contact> > &contacts)
{
    qDebug() << "Presence publication requested";
    foreach (const QSharedPointer<Contact> &contact, contacts) {
        (void) new RosterItem(contact, mList);
    }
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

void RosterWindow::onAuthButtonClicked()
{
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

void RosterWindow::onRemoveButtonClicked()
{
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

void RosterWindow::onDenyButtonClicked()
{
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
    //
    bool found = false;
    for (int i = 0; i < mList->count(); ++i) {
        RosterItem *item = dynamic_cast<RosterItem*>(mList->item(i));
        if (item->contact() == contact) {
            found = true;
        }
    }
    if (!found) {
        (void) new RosterItem(contact, mList);
    }
    contact->requestPresenceSubscription();
}
