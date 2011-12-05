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

#include "roster-window.h"
#include "_gen/roster-window.moc.hpp"

#include "roster-widget.h"

#include <TelepathyQt/Types>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>

#include <QDebug>

using namespace Tp;

RosterWindow::RosterWindow(const QString &accountName, QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QLatin1String("Roster"));

    setupGui();

    ChannelFactoryPtr channelFactory = ChannelFactory::create(
            QDBusConnection::sessionBus());
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(
            QDBusConnection::sessionBus(), Connection::FeatureConnected |
                Connection::FeatureRoster | Connection::FeatureRosterGroups);
    ContactFactoryPtr contactFactory = ContactFactory::create(
            Contact::FeatureAlias | Contact::FeatureSimplePresence);

    mAccount = Account::create(TP_QT_ACCOUNT_MANAGER_BUS_NAME,
            TP_QT_ACCOUNT_OBJECT_PATH_BASE + QLatin1Char('/') + accountName,
            connectionFactory, channelFactory, contactFactory);
    connect(mAccount->becomeReady(Account::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onAccountReady(Tp::PendingOperation *)));

    resize(240, 320);
}

RosterWindow::~RosterWindow()
{
}

void RosterWindow::setupGui()
{
    mRoster = new RosterWidget();
    setCentralWidget(mRoster);
}

void RosterWindow::onAccountReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Account cannot become ready - " <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "Account ready";
    connect(mAccount.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));

    if (mAccount->connection().isNull()) {
        qDebug() << "The account given has no Connection. Please set it online to continue.";
    }

    onAccountConnectionChanged(mAccount->connection());
}

void RosterWindow::onAccountConnectionChanged(const ConnectionPtr &conn)
{
    if (conn) {
        mRoster->setConnection(conn);
    } else {
        mRoster->unsetConnection();
    }
}
