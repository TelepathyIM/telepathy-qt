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

#include "call-roster-widget.h"
#include "_gen/call-roster-widget.moc.hpp"

#include "call-handler.h"
#include "call-widget.h"

#include <examples/roster/roster-item.h>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/StreamedMediaChannel>

#include <QAction>
#include <QDebug>

using namespace Telepathy::Client;

CallRosterWidget::CallRosterWidget(CallHandler *callHandler, QWidget *parent)
    : RosterWidget(parent),
      mCallHandler(callHandler)
{
    createActions();
    setupGui();
}

CallRosterWidget::~CallRosterWidget()
{
}

RosterItem *CallRosterWidget::createItemForContact(const QSharedPointer<Contact> &contact,
        bool &exists)
{
    return RosterWidget::createItemForContact(contact, exists);
}

void CallRosterWidget::createActions()
{
    mCallAction = new QAction(QLatin1String("Call Contact"), this);
    mCallAction->setEnabled(false);
    connect(mCallAction,
            SIGNAL(triggered(bool)),
            SLOT(onCallActionTriggered(bool)));
}

void CallRosterWidget::setupGui()
{
    QListWidget *list = listWidget();
    list->insertAction(list->actions().first(), mCallAction);
}

void CallRosterWidget::updateActions(RosterItem *item)
{
    mCallAction->setEnabled(item);
}

void CallRosterWidget::onCallActionTriggered(bool checked)
{
    QList<QListWidgetItem *> selectedItems = listWidget()->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    Q_ASSERT(selectedItems.size() == 1);
    RosterItem *item = dynamic_cast<RosterItem*>(selectedItems.first());
    QSharedPointer<Contact> contact = item->contact();
    mCallHandler->addOutgoingCall(contact);
}
