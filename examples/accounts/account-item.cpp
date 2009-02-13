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

#include "account-item.h"
#include "_gen/account-item.moc.hpp"

#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/PendingReadyAccount>

#include <QDebug>
#include <QComboBox>
#include <QTableWidget>

AccountItem::AccountItem(Telepathy::Client::AccountManager *am,
        const QString &objectPath, QTableWidget *table, int row, QObject *parent)
    : QObject(parent),
      acc(new Telepathy::Client::Account(am, objectPath, this)),
      mTable(table),
      mRow(row)
{
    connect(acc->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onReady(Telepathy::Client::PendingOperation *)));
}

AccountItem::~AccountItem()
{
}

void AccountItem::setupGui()
{
    mTable->setItem(mRow, ColumnValid, new QTableWidgetItem(acc->isValid() ? "true" : "false"));
    mTable->setItem(mRow, ColumnEnabled, new QTableWidgetItem(acc->isEnabled() ? "true" : "false"));
    mTable->setItem(mRow, ColumnConnectionManager, new QTableWidgetItem(acc->cmName()));
    mTable->setItem(mRow, ColumnProtocol, new QTableWidgetItem(acc->protocol()));
    mTable->setItem(mRow, ColumnDisplayName, new QTableWidgetItem(acc->displayName()));
    mTable->setItem(mRow, ColumnNickname, new QTableWidgetItem(acc->nickname()));
    mTable->setItem(mRow, ColumnConnectsAutomatically, new QTableWidgetItem(acc->connectsAutomatically() ? "true" : "false"));
    mTable->setItem(mRow, ColumnAutomaticPresence, new QTableWidgetItem(acc->automaticPresence().status));
    mTable->setItem(mRow, ColumnCurrentPresence, new QTableWidgetItem(acc->currentPresence().status));
    mTable->setItem(mRow, ColumnRequestedPresence, new QTableWidgetItem(acc->requestedPresence().status));
    mTable->setItem(mRow, ColumnConnectionStatus, new QTableWidgetItem(QString::number(acc->connectionStatus())));
    mTable->setItem(mRow, ColumnConnection, new QTableWidgetItem(acc->connectionObjectPath()));
}

void AccountItem::onReady(Telepathy::Client::PendingOperation *op)
{
    setupGui();

    connect(acc,
            SIGNAL(validityChanged(bool)),
            SLOT(onValidityChanged(bool)));
    connect(acc,
            SIGNAL(stateChanged(bool)),
            SLOT(onStateChanged(bool)));
    connect(acc,
            SIGNAL(displayNameChanged(const QString &)),
            SLOT(onDisplayNameChanged(const QString &)));
    connect(acc,
            SIGNAL(nicknameChanged(const QString &)),
            SLOT(onNicknameChanged(const QString &)));
    connect(acc,
            SIGNAL(connectsAutomaticallyPropertyChanged(bool)),
            SLOT(onConnectsAutomaticallyPropertyChanged(bool)));
    connect(acc,
            SIGNAL(automaticPresenceChanged(const Telepathy::SimplePresence &)),
            SLOT(onAutomaticPresenceChanged(const Telepathy::SimplePresence &)));
    connect(acc,
            SIGNAL(currentPresenceChanged(const Telepathy::SimplePresence &)),
            SLOT(onCurrentPresenceChanged(const Telepathy::SimplePresence &)));
    connect(acc,
            SIGNAL(requestedPresenceChanged(const Telepathy::SimplePresence &)),
            SLOT(onRequestedPresenceChanged(const Telepathy::SimplePresence &)));
    connect(acc,
            SIGNAL(connectionStatusChanged(Telepathy::ConnectionStatus, Telepathy::ConnectionStatusReason)),
            SLOT(onConnectionStatusChanged(Telepathy::ConnectionStatus, Telepathy::ConnectionStatusReason)));
    connect(acc,
            SIGNAL(haveConnectionChanged(bool)),
            SLOT(onHaveConnectionChanged(bool)));
}

void AccountItem::onValidityChanged(bool valid)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnValid);
    item->setText((valid ? "true" : "false"));
}

void AccountItem::onStateChanged(bool enabled)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnEnabled);
    item->setText((enabled ? "true" : "false"));
}

void AccountItem::onDisplayNameChanged(const QString &name)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnDisplayName);
    item->setText(name);
}

void AccountItem::onNicknameChanged(const QString &name)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnNickname);
    item->setText(name);
}

void AccountItem::onConnectsAutomaticallyPropertyChanged(bool value)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnConnectsAutomatically);
    item->setText((value ? "true" : "false"));
}

void AccountItem::onAutomaticPresenceChanged(const Telepathy::SimplePresence &presence)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnAutomaticPresence);
    item->setText(presence.status);
}

void AccountItem::onCurrentPresenceChanged(const Telepathy::SimplePresence &presence)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnCurrentPresence);
    item->setText(presence.status);
}

void AccountItem::onRequestedPresenceChanged(const Telepathy::SimplePresence &presence)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnRequestedPresence);
    item->setText(presence.status);
}

void AccountItem::onConnectionStatusChanged(Telepathy::ConnectionStatus status,
        Telepathy::ConnectionStatusReason reason)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnConnectionStatus);
    item->setText(QString::number(status));
}

void AccountItem::onHaveConnectionChanged(bool haveConnection)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnConnection);
    item->setText(acc->connectionObjectPath());
}
