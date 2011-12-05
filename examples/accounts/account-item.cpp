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

#include "account-item.h"
#include "_gen/account-item.moc.hpp"

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/PendingReady>

#include <QDebug>
#include <QComboBox>
#include <QTableWidget>

AccountItem::AccountItem(Tp::AccountPtr acc, QTableWidget *table, int row, QObject *parent)
    : QObject(parent),
      mAcc(acc),
      mTable(table),
      mRow(row)
{
    init();
}

AccountItem::~AccountItem()
{
}

void AccountItem::setupGui()
{
    mTable->setItem(mRow, ColumnValid, new QTableWidgetItem(mAcc->isValid() ?
                QLatin1String("true") : QLatin1String("false")));
    mTable->setItem(mRow, ColumnEnabled, new QTableWidgetItem(mAcc->isEnabled() ?
                QLatin1String("true") : QLatin1String("false")));
    mTable->setItem(mRow, ColumnConnectionManager, new QTableWidgetItem(mAcc->cmName()));
    mTable->setItem(mRow, ColumnProtocol, new QTableWidgetItem(mAcc->protocolName()));
    mTable->setItem(mRow, ColumnDisplayName, new QTableWidgetItem(mAcc->displayName()));
    mTable->setItem(mRow, ColumnNickname, new QTableWidgetItem(mAcc->nickname()));
    mTable->setItem(mRow, ColumnConnectsAutomatically, new QTableWidgetItem(mAcc->connectsAutomatically() ?
                QLatin1String("true") : QLatin1String("false")));
    mTable->setItem(mRow, ColumnAutomaticPresence, new QTableWidgetItem(mAcc->automaticPresence().status()));
    mTable->setItem(mRow, ColumnCurrentPresence, new QTableWidgetItem(mAcc->currentPresence().status()));
    mTable->setItem(mRow, ColumnRequestedPresence, new QTableWidgetItem(mAcc->requestedPresence().status()));
    mTable->setItem(mRow, ColumnChangingPresence, new QTableWidgetItem(mAcc->isChangingPresence() ?
                QLatin1String("true") : QLatin1String("false")));
    mTable->setItem(mRow, ColumnConnectionStatus, new QTableWidgetItem(QString::number(mAcc->connectionStatus())));
    mTable->setItem(mRow, ColumnConnection, new QTableWidgetItem(
                mAcc->connection().isNull() ? QLatin1String("") : mAcc->connection()->objectPath()));
}

void AccountItem::init()
{
    setupGui();

    Tp::Account *acc = mAcc.data();
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
            SIGNAL(changingPresence(bool)),
            SLOT(onChangingPresenceChanged(bool)));
    connect(acc,
            SIGNAL(automaticPresenceChanged(const Tp::SimplePresence &)),
            SLOT(onAutomaticPresenceChanged(const Tp::SimplePresence &)));
    connect(acc,
            SIGNAL(currentPresenceChanged(const Tp::SimplePresence &)),
            SLOT(onCurrentPresenceChanged(const Tp::SimplePresence &)));
    connect(acc,
            SIGNAL(requestedPresenceChanged(const Tp::SimplePresence &)),
            SLOT(onRequestedPresenceChanged(const Tp::SimplePresence &)));
    connect(acc,
            SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason,
                    const QString &, const QVariantMap &)),
            SLOT(onStatusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason,
                    const QString &, const QVariantMap &)));
    connect(acc,
            SIGNAL(haveConnectionChanged(bool)),
            SLOT(onHaveConnectionChanged(bool)));
}

void AccountItem::onValidityChanged(bool valid)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnValid);
    item->setText((valid ? QLatin1String("true") : QLatin1String("false")));
}

void AccountItem::onStateChanged(bool enabled)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnEnabled);
    item->setText((enabled ? QLatin1String("true") : QLatin1String("false")));
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
    item->setText((value ? QLatin1String("true") : QLatin1String("false")));
}

void AccountItem::onChangingPresenceChanged(bool value)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnChangingPresence);
    item->setText((value ? QLatin1String("true") : QLatin1String("false")));
}

void AccountItem::onAutomaticPresenceChanged(const Tp::SimplePresence &presence)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnAutomaticPresence);
    item->setText(presence.status);
}

void AccountItem::onCurrentPresenceChanged(const Tp::SimplePresence &presence)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnCurrentPresence);
    item->setText(presence.status);
}

void AccountItem::onRequestedPresenceChanged(const Tp::SimplePresence &presence)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnRequestedPresence);
    item->setText(presence.status);
}

void AccountItem::onStatusChanged(Tp::ConnectionStatus status,
        Tp::ConnectionStatusReason reason, const QString &error,
        const QVariantMap &errorDetails)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnConnectionStatus);
    item->setText(QString::number(status));
}

void AccountItem::onHaveConnectionChanged(bool haveConnection)
{
    QTableWidgetItem *item = mTable->item(mRow, ColumnConnection);
    item->setText(mAcc->connection().isNull() ?
            QLatin1String("") : mAcc->connection()->objectPath());
}
