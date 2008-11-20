/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 basysKom GmbH
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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
#ifndef TelepathyQt4_Prototype_DBusInterface_h_
#define TelepathyQt4_Prototype_DBusInterface_h_

#include <QDBusAbstractInterface>
#include <QDBusReply>
#include <QStringList>

namespace TpPrototype
{

/**
 * Class to access function provided by dbus daemon.
 */
class DBusInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    DBusInterface( QObject* parent = NULL );

    /**
     * Return a list of all registered service names. These names can be activated by calling the service.
     * @return List of service names.
     */
    QDBusReply<QStringList> listActivatableNames();
    
};

}
#endif

