/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#ifndef _TelepathyQt_Service_base_connection_manager_h_HEADER_GUARD_
#define _TelepathyQt_Service_base_connection_manager_h_HEADER_GUARD_

#include <TelepathyQt/Service/Global>

class QDBusConnection;
class QString;

namespace Tp
{
namespace Service
{

class TP_QT_SVC_EXPORT BaseConnectionManager
{
public:
    BaseConnectionManager(const QDBusConnection &dbusConnection, const QString &cmName);
    ~BaseConnectionManager();

    QDBusConnection dbusConnection() const;

    bool registerObject();

private:
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

}
}

#endif
