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

#ifndef _TelepathyQt4_examples_call_call_roster_widget_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_call_roster_widget_h_HEADER_GUARD_

#include <TelepathyQt4/Client/Contact>

#include <examples/roster/roster-widget.h>

namespace Telepathy {
namespace Client {
class Channel;
}
}

class CallHandler;
class CallWidget;

class CallRosterWidget : public RosterWidget
{
    Q_OBJECT

public:
    CallRosterWidget(CallHandler *callHandler, QWidget *parent = 0);
    virtual ~CallRosterWidget();

protected:
    virtual RosterItem *createItemForContact(
            const Telepathy::Client::ContactPtr &contact,
            bool &exists);
    virtual void updateActions(RosterItem *item);

private Q_SLOTS:
    void onCallActionTriggered(bool);

private:
    void createActions();
    void setupGui();

    CallHandler *mCallHandler;
    QAction *mCallAction;
};

#endif
