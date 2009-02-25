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

#include "roster-item.h"
#include "_gen/roster-item.moc.hpp"

using namespace Telepathy::Client;

RosterItem::RosterItem(const QSharedPointer<Contact> &contact,
        QListWidget *parent)
    : QObject(parent),
      QListWidgetItem(parent),
      mContact(contact)
{
    setText(QString("%1 (%2)").arg(contact->id()).arg(contact->presenceStatus()));
    connect(contact.data(),
            SIGNAL(simplePresenceChanged(const QString &, uint, const QString &)),
            SLOT(onSimplePresenceChanged(const QString &, uint, const QString &)));
}

RosterItem::~RosterItem()
{
}

void RosterItem::onSimplePresenceChanged(const QString &status, uint type, const QString &presenceMessage)
{
    setText(QString("%1 (%2)").arg(mContact->id()).arg(status));
}
