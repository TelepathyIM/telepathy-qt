/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef BUILDING_TELEPATHY_QT4
#error "This file is a TpQt4 internal header not to be included by applications"
#endif

#include <QObject>
#include <QPair>
#include <QString>

#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/WeakPtr>

namespace Tp
{

class DBusProxy;

class TELEPATHY_QT4_NO_EXPORT DBusProxyFactory::Cache : public QObject
{
    Q_OBJECT

public:
    typedef QPair<QString /* serviceName */, QString /* objectPath */> Key;

    Cache();
    ~Cache();

    SharedPtr<RefCounted> get(const Key &key) const;
    void put(const SharedPtr<RefCounted> &obj);

private Q_SLOTS:
    void onProxyInvalidated(Tp::DBusProxy *proxy); // The error itself is not interesting

private:
    QHash<Key, WeakPtr<RefCounted> > proxies;
};

}
