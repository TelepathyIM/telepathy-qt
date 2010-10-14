/* Abstract base class for Telepathy D-Bus interfaces.
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/AbstractInterface>

#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/PendingVariantMap>
#include <TelepathyQt4/PendingVoid>
#include <TelepathyQt4/Constants>

#include "TelepathyQt4/_gen/abstract-interface.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

#include <QDBusPendingCall>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT AbstractInterface::Private
{
    QString mError;
    QString mMessage;
};

AbstractInterface::AbstractInterface(const QString &busName,
        const QString &path, const char *interface,
        const QDBusConnection &dbusConnection, QObject *parent)
    : QDBusAbstractInterface(busName, path, interface, dbusConnection, parent),
      mPriv(new Private)
{
}

AbstractInterface::AbstractInterface(DBusProxy *parent, const char *interface)
    : QDBusAbstractInterface(parent->busName(), parent->objectPath(),
            interface, parent->dbusConnection(), parent),
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

PendingVariant *AbstractInterface::internalRequestProperty(const char *name)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(),
            QLatin1String(TELEPATHY_INTERFACE_PROPERTIES), QLatin1String("Get"));
    msg << interface() << QString::fromUtf8(name);
    QDBusPendingCall pendingCall = connection().asyncCall(msg);
    return new PendingVariant(pendingCall);
}

PendingOperation *AbstractInterface::internalSetProperty(const char *name,
        const QVariant &newValue)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(),
            QLatin1String(TELEPATHY_INTERFACE_PROPERTIES), QLatin1String("Set"));
    msg << interface() << QString::fromUtf8(name) << newValue;
    QDBusPendingCall pendingCall = connection().asyncCall(msg);
    return new PendingVoid(pendingCall, this);
}

PendingVariantMap *AbstractInterface::internalRequestAllProperties()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(),
            QLatin1String(TELEPATHY_INTERFACE_PROPERTIES), QLatin1String("GetAll"));
    msg << interface();
    QDBusPendingCall pendingCall = connection().asyncCall(msg);
    return new PendingVariantMap(pendingCall);
}

} // Tp
