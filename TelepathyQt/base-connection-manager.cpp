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

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseProtocol>
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
    QHash<QString, BaseProtocolPtr> protocols;
    QSet<BaseConnectionPtr> connections;
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
    // No interfaces suitable for listing in this property are currently defined;
    // it's provided as a hook for possible future functionality.
    return QStringList();
}

ProtocolPropertiesMap BaseConnectionManager::Adaptee::protocols() const
{
    ProtocolPropertiesMap ret;
    foreach (const BaseProtocolPtr &protocol, mCM->protocols()) {
        ret.insert(protocol->name(), protocol->immutableProperties());
    }
    return ret;
}

void BaseConnectionManager::Adaptee::getParameters(const QString &protocolName,
        const Tp::Service::ConnectionManagerAdaptor::GetParametersContextPtr &context)
{
    if (!checkValidProtocolName(protocolName)) {
        context->setFinishedWithError(TP_QT_ERROR_INVALID_ARGUMENT,
                protocolName + QLatin1String("is not a valid protocol name"));
        return;
    }

    if (!mCM->hasProtocol(protocolName)) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("unknown protocol") + protocolName);
        return;
    }

    BaseProtocolPtr protocol = mCM->protocol(protocolName);
    ParamSpecList ret;
    foreach (const ProtocolParameter &param, protocol->parameters()) {
        ret << param.bareParameter();
    }
    context->setFinished(ret);
}

void BaseConnectionManager::Adaptee::listProtocols(
        const Tp::Service::ConnectionManagerAdaptor::ListProtocolsContextPtr &context)
{
    QStringList ret;
    QList<BaseProtocolPtr> protocols = mCM->protocols();
    foreach (const BaseProtocolPtr &protocol, protocols) {
        ret << protocol->name();
    }
    context->setFinished(ret);
}

void BaseConnectionManager::Adaptee::requestConnection(const QString &protocolName,
        const QVariantMap &parameters,
        const Tp::Service::ConnectionManagerAdaptor::RequestConnectionContextPtr &context)
{
    if (!checkValidProtocolName(protocolName)) {
        context->setFinishedWithError(TP_QT_ERROR_INVALID_ARGUMENT,
                protocolName + QLatin1String("is not a valid protocol name"));
        return;
    }

    if (!mCM->hasProtocol(protocolName)) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("unknown protocol") + protocolName);
        return;
    }

    BaseProtocolPtr protocol = mCM->protocol(protocolName);

    DBusError error;
    BaseConnectionPtr connection;
    // connection = protocol->createConnection(parameters, &error);
    QMetaObject::invokeMethod(protocol.data(), "createConnection",
        Q_RETURN_ARG(Tp::BaseConnectionPtr, connection),
        Q_ARG(QVariantMap, parameters),
        Q_ARG(Tp::DBusError*, &error));
    if (!connection) {
        context->setFinishedWithError(error);
        return;
    }

    if (!connection->registerObject(&error)) {
        context->setFinishedWithError(error);
        return;
    }

    mCM->addConnection(connection);

    emit newConnection(connection->busName(), QDBusObjectPath(connection->objectPath()),
            protocol->name());
    context->setFinished(connection->busName(), QDBusObjectPath(connection->objectPath()));
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

QList<BaseProtocolPtr> BaseConnectionManager::protocols() const
{
    return mPriv->protocols.values();
}

BaseProtocolPtr BaseConnectionManager::protocol(const QString &protocolName) const
{
    return mPriv->protocols.value(protocolName);
}

bool BaseConnectionManager::hasProtocol(const QString &protocolName) const
{
    return mPriv->protocols.contains(protocolName);
}

bool BaseConnectionManager::addProtocol(const BaseProtocolPtr &protocol)
{
    if (isRegistered()) {
        warning() << "Unable to add protocol" << protocol->name() <<
            "- CM already registered";
        return false;
    }

    if (protocol->dbusConnection().name() != dbusConnection().name()) {
        warning() << "Unable to add protocol" << protocol->name() <<
            "- protocol must have the same D-Bus connection as the owning CM";
        return false;
    }

    if (mPriv->protocols.contains(protocol->name())) {
        warning() << "Unable to add protocol" << protocol->name() <<
            "- another protocol with same name already added";
        return false;
    }

    debug() << "Protocol" << protocol->name() << "added to CM";
    mPriv->protocols.insert(protocol->name(), protocol);
    return true;
}

bool BaseConnectionManager::registerObject(DBusError *error)
{
    QString busName = TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE;
    busName.append(mPriv->name);
    QString objectPath = TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE;
    objectPath.append(mPriv->name);
    DBusError _error;
    bool ret = registerObject(busName, objectPath, &_error);
    if (!ret && error) {
        error->set(_error.name(), _error.message());
    }
    return ret;
}

bool BaseConnectionManager::registerObject(const QString &busName, const QString &objectPath,
        DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    // register protocols
    foreach (const BaseProtocolPtr &protocol, protocols()) {
        QString escapedProtocolName = protocol->name();
        escapedProtocolName.replace(QLatin1Char('-'), QLatin1Char('_'));
        QString protoObjectPath = QString(
                QLatin1String("%1/%2")).arg(objectPath).arg(escapedProtocolName);
        debug() << "Registering protocol" << protocol->name() << "at path" << protoObjectPath <<
            "for CM" << objectPath << "at bus name" << busName;
        if (!protocol->registerObject(busName, protoObjectPath, error)) {
            return false;
        }
    }

    debug() << "Registering CM" << objectPath << "at bus name" << busName;
    // Only call DBusService::registerObject after registering the protocols as we don't want to
    // advertise isRegistered if some protocol cannot be registered
    if (!DBusService::registerObject(busName, objectPath, error)) {
        return false;
    }

    return true;
}

QList<BaseConnectionPtr> BaseConnectionManager::connections() const
{
    return mPriv->connections.toList();
}

void BaseConnectionManager::addConnection(const BaseConnectionPtr &connection)
{
    Q_ASSERT(!mPriv->connections.contains(connection));
    mPriv->connections.insert(connection);
    connect(connection.data(),
            SIGNAL(disconnected()),
            SLOT(removeConnection()));
    emit newConnection(connection);
}

void BaseConnectionManager::removeConnection()
{
    BaseConnectionPtr connection = BaseConnectionPtr(
            qobject_cast<BaseConnection*>(sender()));
    Q_ASSERT(connection);
    Q_ASSERT(mPriv->connections.contains(connection));
    mPriv->connections.remove(connection);
}

}
