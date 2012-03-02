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

#include <TelepathyQt/BaseConnectionManager>
#include "TelepathyQt/base-connection-manager-internal.h"

#include "TelepathyQt/_gen/base-connection-manager.moc.hpp"
#include "TelepathyQt/_gen/base-connection-manager-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/Utils>

#include <QDBusObjectPath>
#include <QString>
#include <QStringList>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseConnectionManager::Private
{
    Private(BaseConnectionManager *parent, const QDBusConnection &dbusConnection,
            const QString &name)
        : parent(parent),
          name(name),
          adaptee(new BaseConnectionManager::Adaptee(dbusConnection, parent))
    {
    }

    BaseConnectionManager *parent;
    QString name;

    BaseConnectionManager::Adaptee *adaptee;
};

BaseConnectionManager::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
        BaseConnectionManager *cm)
    : QObject(cm),
      mCM(cm)
{
    mAdaptor = new Service::ConnectionManagerAdaptor(dbusConnection, this, cm->dbusObject());
}

BaseConnectionManager::Adaptee::~Adaptee()
{
}

QStringList BaseConnectionManager::Adaptee::interfaces() const
{
    return QStringList() << QString(QLatin1String("ofdT.Test"));
}

ProtocolPropertiesMap BaseConnectionManager::Adaptee::protocols() const
{
    return ProtocolPropertiesMap();
}

void BaseConnectionManager::Adaptee::getParameters(const QString &protocol,
        const Tp::Service::ConnectionManagerAdaptor::GetParametersContextPtr &context)
{
    qDebug() << __FUNCTION__ << "called";
    context->setFinished(ParamSpecList());
}

void BaseConnectionManager::Adaptee::listProtocols(
        const Tp::Service::ConnectionManagerAdaptor::ListProtocolsContextPtr &context)
{
    qDebug() << __FUNCTION__ << "called";
    context->setFinished(QStringList());
}

void BaseConnectionManager::Adaptee::requestConnection(const QString &protocol,
        const QVariantMap &params,
        const Tp::Service::ConnectionManagerAdaptor::RequestConnectionContextPtr &context)
{
    // emit newConnection()
    qDebug() << __FUNCTION__ << "called";
    context->setFinished(QString(), QDBusObjectPath(QLatin1String("/")));
}

BaseConnectionManager::BaseConnectionManager(const QDBusConnection &dbusConnection,
        const QString &name)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, name))
{
}

BaseConnectionManager::~BaseConnectionManager()
{
    delete mPriv;
}

QString BaseConnectionManager::name() const
{
    return mPriv->name;
}

bool BaseConnectionManager::registerObject()
{
    QString busName = TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE;
    busName.append(mPriv->name);
    QString objectPath = TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE;
    objectPath.append(mPriv->name);
    return registerObject(busName, objectPath);
}

bool BaseConnectionManager::registerObject(const QString &busName,
        const QString &objectPath)
{
    if (isRegistered()) {
        return true;
    }

    // register protocols

    // Only call DBusService::registerObject after registering the protocols as we don't want to
    // advertise isRegistered if some protocol cannot be registered
    if (!DBusService::registerObject(busName, objectPath)) {
        return false;
    }

    return true;
}

}
