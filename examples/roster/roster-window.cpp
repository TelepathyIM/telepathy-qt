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
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingConnection>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/PendingReadyConnectionManager>

#include <QDebug>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>

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
    mList = new QListWidget;
    setCentralWidget(mList);
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

    qDebug() << "Connection ready";
    foreach (const QSharedPointer<Contact> &contact, mConn->contactManager()->allKnownContacts()) {
        (void) new RosterItem(contact, mList);
    }
}
