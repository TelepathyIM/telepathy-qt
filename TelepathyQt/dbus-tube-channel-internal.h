/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_dbus_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt_dbus_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt/DBusTubeChannel>

namespace Tp {

class TP_QT_NO_EXPORT DBusTubeChannelPrivate
{
    Q_DECLARE_PUBLIC(DBusTubeChannel)
protected:
    DBusTubeChannel * const q_ptr;

public:
    DBusTubeChannelPrivate(DBusTubeChannel *parent);
    virtual ~DBusTubeChannelPrivate();

    void init();

    void extractProperties(const QVariantMap &props);
    void extractParticipants(const Tp::DBusTubeParticipants &participants);

    static void introspectDBusTube(DBusTubeChannelPrivate *self);
    static void introspectBusNamesMonitoring(DBusTubeChannelPrivate *self);

    ReadinessHelper *readinessHelper;

    // Properties
    UIntList accessControls;
    QString serviceName;
    QHash< ContactPtr, QString > busNames;
    QString address;

    // Private slots
    void gotDBusTubeProperties(QDBusPendingCallWatcher *watcher);
    void onDBusNamesChanged(const Tp::DBusTubeParticipants &added, const Tp::UIntList &removed);

};

}

#endif
