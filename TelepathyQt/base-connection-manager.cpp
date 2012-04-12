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
#include <TelepathyQt/DBusObject>
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
                protocolName + QLatin1String(" is not a valid protocol name"));
        return;
    }

    if (!mCM->hasProtocol(protocolName)) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("unknown protocol ") + protocolName);
        return;
    }

    BaseProtocolPtr protocol = mCM->protocol(protocolName);
    ParamSpecList ret;
    foreach (const ProtocolParameter &param, protocol->parameters()) {
         ParamSpec paramSpec = param.bareParameter();
         if (!(paramSpec.flags & ConnMgrParamFlagHasDefault)) {
             // we cannot pass QVariant::Invalid over D-Bus, lets build a dummy value
             // that should be ignored according to the spec
             paramSpec.defaultValue = QDBusVariant(
                     parseValueWithDBusSignature(QString(), paramSpec.signature));
         }
         ret << paramSpec;
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
                protocolName + QLatin1String(" is not a valid protocol name"));
        return;
    }

    if (!mCM->hasProtocol(protocolName)) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("unknown protocol ") + protocolName);
        return;
    }

    BaseProtocolPtr protocol = mCM->protocol(protocolName);

    DBusError error;
    BaseConnectionPtr connection;
    connection = protocol->createConnection(parameters, &error);
    if (!connection) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }

    if (!connection->registerObject(&error)) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }

    mCM->addConnection(connection);

    emit newConnection(connection->busName(), QDBusObjectPath(connection->objectPath()),
            protocol->name());
    context->setFinished(connection->busName(), QDBusObjectPath(connection->objectPath()));
}

/**
 * \class BaseConnectionManager
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection-manager.h <TelepathyQt/BaseConnectionManager>
 *
 * \brief Base class for connection manager implementations.
 */

/**
 * Constructs a new BaseConnectionManager object that implements a connection manager
 * on the given \a dbusConnection and has the given \a name.
 *
 * \param dbusConnection The QDBusConnection to use.
 * \param name The name of the connection manager.
 */
BaseConnectionManager::BaseConnectionManager(const QDBusConnection &dbusConnection,
        const QString &name)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, name))
{
}

/**
 * Class destructor.
 */
BaseConnectionManager::~BaseConnectionManager()
{
    delete mPriv;
}

/**
 * Return the connection manager's name, as given on the constructor.
 *
 * \return The connection manager's name.
 */
QString BaseConnectionManager::name() const
{
    return mPriv->name;
}

/**
 * Return the immutable properties of this connection manager object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this connection manager object.
 */
QVariantMap BaseConnectionManager::immutableProperties() const
{
    QVariantMap ret;
    ret.insert(TP_QT_IFACE_CONNECTION_MANAGER + QLatin1String(".Protocols"),
            QVariant::fromValue(mPriv->adaptee->protocols()));
    return ret;
}

/**
 * Return a list of all protocols that this connection manager implements.
 *
 * This property is immutable and cannot change after the connection manager
 * has been registered on the bus with registerObject().
 *
 * \return A list of all protocols that this connection manager implements.
 * \sa addProtocol(), hasProtocol(), protocol()
 */
QList<BaseProtocolPtr> BaseConnectionManager::protocols() const
{
    return mPriv->protocols.values();
}

/**
 * Return a pointer to the BaseProtocol instance that implements the protocol
 * with the given \a protocolName, or a null BaseProtocolPtr if no such
 * protocol has been added to the connection manager.
 *
 * \param protocolName The name of the protocol in interest.
 * \return The BaseProtocol instance that implements the protocol with
 * the given \a protocolName.
 * \sa hasProtocol(), protocols(), addProtocol()
 */
BaseProtocolPtr BaseConnectionManager::protocol(const QString &protocolName) const
{
    return mPriv->protocols.value(protocolName);
}

/**
 * Return whether a protocol with the given \a protocolName has been
 * added to the connection manager.
 *
 * \param protocolName The name of the protocol in interest.
 * \return \c true if a protocol with the given \a protocolName has been
 * added to the connection manager, or \c false otherwise.
 * \sa addProtocol(), protocol(), protocols()
 */
bool BaseConnectionManager::hasProtocol(const QString &protocolName) const
{
    return mPriv->protocols.contains(protocolName);
}

/**
 * Add a new \a protocol to the list of protocols that this connection
 * manager implements.
 *
 * Note that you cannot add new protocols after the connection manager
 * has been registered on the bus with registerObject(). In addition,
 * you cannot add two protocols with the same name. If any of these
 * conditions is not met, this function will return false and print
 * a suitable warning.
 *
 * \param protocol The protocol to add.
 * \return \c true on success or \c false otherwise.
 */
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

    if (protocol->isRegistered()) {
        warning() << "Unable to add protocol" << protocol->name() <<
            "- protocol already registered";
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

/**
 * Register this connection manager on the bus.
 *
 * A connection manager can only be registered once, and it
 * should be registered only after all the protocols it implements
 * have been added with addProtocol().
 *
 * If \a error is passed, any D-Bus error that may occur will
 * be stored there.
 *
 * \param error A pointer to an empty DBusError where any
 * possible D-Bus error will be stored.
 * \return \c true on success and \c false if there was an error
 * or this connection manager is already registered.
 * \sa isRegistered()
 */
bool BaseConnectionManager::registerObject(DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

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

/**
 * Reimplemented from DBusService.
 */
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

/**
 * Return a list of all connections that have currently been made.
 *
 * \return A list of all connections that have currently been made.
 */
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

/**
 * \fn void newConnection(const BaseConnectionPtr &connection)
 *
 * Emitted when a new connection has been requested by a client and
 * the connection object has been constructed.
 *
 * To handle the connection request before a connection has been created,
 * use BaseProtocol::setCreateConnectionCallback().
 *
 * \param connection The newly created connection
 */

}
