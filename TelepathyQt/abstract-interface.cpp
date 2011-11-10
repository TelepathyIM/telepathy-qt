/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/AbstractInterface>

#include "TelepathyQt/_gen/abstract-interface.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/Types>

#include <QDBusPendingCall>
#include <QDBusVariant>

namespace Tp
{

struct TP_QT_NO_EXPORT AbstractInterface::Private
{
    QString mError;
    QString mMessage;
};

/**
 * \class AbstractInterface
 * \ingroup clientsideproxies
 * \headerfile TelepathyQt/abstract-interface.h <TelepathyQt/AbstractInterface>
 *
 * \brief The AbstractInterface class is the base class for all client side
 * D-Bus interfaces, allowing access to remote methods/properties/signals.
 */

AbstractInterface::AbstractInterface(const QString &busName,
        const QString &path, const QLatin1String &interface,
        const QDBusConnection &dbusConnection, QObject *parent)
    : QDBusAbstractInterface(busName, path, interface.latin1(), dbusConnection, parent),
      mPriv(new Private)
{
}

AbstractInterface::AbstractInterface(DBusProxy *parent, const QLatin1String &interface)
    : QDBusAbstractInterface(parent->busName(), parent->objectPath(),
            interface.latin1(), parent->dbusConnection(), parent),
      mPriv(new Private)
{
    connect(parent, SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            this, SLOT(invalidate(Tp::DBusProxy*,QString,QString)));
}

AbstractInterface::~AbstractInterface()
{
    delete mPriv;
}

bool AbstractInterface::isValid() const
{
    return QDBusAbstractInterface::isValid() && mPriv->mError.isEmpty();
}

QString AbstractInterface::invalidationReason() const
{
    return mPriv->mError;
}

QString AbstractInterface::invalidationMessage() const
{
    return mPriv->mMessage;
}

void AbstractInterface::invalidate(DBusProxy *proxy,
        const QString &error, const QString &message)
{
    Q_ASSERT(!error.isEmpty());

    if (mPriv->mError.isEmpty()) {
        mPriv->mError = error;
        mPriv->mMessage = message;
    }
}

PendingVariant *AbstractInterface::internalRequestProperty(const QString &name) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(),
            TP_QT_IFACE_PROPERTIES, QLatin1String("Get"));
    msg << interface() << name;
    QDBusPendingCall pendingCall = connection().asyncCall(msg);
    DBusProxy *proxy = qobject_cast<DBusProxy*>(parent());
    return new PendingVariant(pendingCall, DBusProxyPtr(proxy));
}

PendingOperation *AbstractInterface::internalSetProperty(const QString &name,
        const QVariant &newValue)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(),
            TP_QT_IFACE_PROPERTIES, QLatin1String("Set"));
    msg << interface() << name << QVariant::fromValue(QDBusVariant(newValue));
    QDBusPendingCall pendingCall = connection().asyncCall(msg);
    DBusProxy *proxy = qobject_cast<DBusProxy*>(parent());
    return new PendingVoid(pendingCall, DBusProxyPtr(proxy));
}

PendingVariantMap *AbstractInterface::internalRequestAllProperties() const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(),
            TP_QT_IFACE_PROPERTIES, QLatin1String("GetAll"));
    msg << interface();
    QDBusPendingCall pendingCall = connection().asyncCall(msg);
    DBusProxy *proxy = qobject_cast<DBusProxy*>(parent());
    return new PendingVariantMap(pendingCall, DBusProxyPtr(proxy));
}

} // Tp
