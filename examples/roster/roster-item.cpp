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

RosterItem::RosterItem(const ContactPtr &contact,
        QListWidget *parent)
    : QObject(parent),
      QListWidgetItem(parent),
      mContact(contact)
{
    onContactChanged();

    connect(contact.data(),
            SIGNAL(simplePresenceChanged(const QString &, uint, const QString &)),
            SLOT(onContactChanged()));
    connect(contact.data(),
            SIGNAL(subscriptionStateChanged(Telepathy::Client::Contact::PresenceState)),
            SLOT(onContactChanged()));
    connect(contact.data(),
            SIGNAL(publishStateChanged(Telepathy::Client::Contact::PresenceState)),
            SLOT(onContactChanged()));
    connect(contact.data(),
            SIGNAL(blockStatusChanged(bool)),
            SLOT(onContactChanged()));
}

RosterItem::~RosterItem()
{
}

void RosterItem::onContactChanged()
{
    QString status = mContact->presenceStatus();
    // I've asked to see the contact presence
    if (mContact->subscriptionState() == Contact::PresenceStateAsk) {
        setText(QString("%1 (%2) (awaiting approval)").arg(mContact->id()).arg(status));
    // The contact asked to see my presence
    } else if (mContact->publishState() == Contact::PresenceStateAsk) {
        setText(QString("%1 (%2) (pending approval)").arg(mContact->id()).arg(status));
    } else if (mContact->subscriptionState() == Contact::PresenceStateNo &&
               mContact->publishState() == Contact::PresenceStateNo) {
        setText(QString("%1 (unknown)").arg(mContact->id()));
    } else {
        if (mContact->isBlocked()) {
            setText(QString("%1 (%2) (blocked)").arg(mContact->id()).arg(status));
        } else {
            setText(QString("%1 (%2)").arg(mContact->id()).arg(status));
        }
    }

    emit changed();
}
