/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#ifndef BUILDING_TP_QT
#error "This file is a TpQt internal header not to be included by applications"
#endif

#include <QObject>
#include <QPair>
#include <QString>

#include <TelepathyQt/SharedPtr>

namespace Tp
{

class DBusProxy;

class TP_QT_NO_EXPORT DBusProxyFactory::Cache : public QObject
{
    Q_OBJECT

public:
    typedef QPair<QString /* serviceName */, QString /* objectPath */> Key;

    Cache();
    ~Cache();

    DBusProxyPtr get(const Key &key) const;
    void put(const DBusProxyPtr &proxy);

private Q_SLOTS:
    void onProxyInvalidated(Tp::DBusProxy *proxy); // The error itself is not interesting

private:
    QHash<Key, WeakPtr<DBusProxy> > proxies;
};

}
