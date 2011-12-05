/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2009 Nokia Corporation
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

#include <TelepathyQt/test-backdoors.h>

#include <TelepathyQt/DBusProxy>

namespace Tp
{

void TestBackdoors::invalidateProxy(DBusProxy *proxy, const QString &reason, const QString &message)
{
    Q_ASSERT(proxy != 0);
    Q_ASSERT(proxy->isValid());

    proxy->invalidate(reason, message);
}

ConnectionCapabilities TestBackdoors::createConnectionCapabilities(
        const RequestableChannelClassSpecList &rccSpecs)
{
    return ConnectionCapabilities(rccSpecs);
}

ContactCapabilities TestBackdoors::createContactCapabilities(
            const RequestableChannelClassSpecList &rccSpecs, bool specificToContact)
{
    return ContactCapabilities(rccSpecs, specificToContact);
}

} // Tp
