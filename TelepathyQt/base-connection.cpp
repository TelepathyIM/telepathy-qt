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

#include <TelepathyQt/BaseConnection>
#include "TelepathyQt/base-connection-internal.h"

#include "TelepathyQt/_gen/base-connection.moc.hpp"
#include "TelepathyQt/_gen/base-connection-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>
#include <TelepathyQt/AbstractProtocolInterface>
#include <QString>
#include <QVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseConnection::Private {
    Private(BaseConnection *parent, const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters)
        : parent(parent),
          cmName(cmName),
          protocolName(protocolName),
          parameters(parameters),
          status(Tp::ConnectionStatusDisconnected),
          selfHandle(0),
          adaptee(new BaseConnection::Adaptee(dbusConnection, parent)) {
    }

    BaseConnection *parent;
    QString cmName;
    QString protocolName;
    QVariantMap parameters;
    uint status;
    QHash<QString, AbstractConnectionInterfacePtr> interfaces;
    QSet<BaseChannelPtr> channels;
    CreateChannelCallback createChannelCB;
    RequestHandlesCallback requestHandlesCB;
    ConnectCallback connectCB;
    InspectHandlesCallback inspectHandlesCB;
    uint selfHandle;
    BaseConnection::Adaptee *adaptee;
};

BaseConnection::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
                                 BaseConnection *connection)
    : QObject(connection),
      mConnection(connection)
{
    mAdaptor = new Service::ConnectionAdaptor(dbusConnection, this, connection->dbusObject());
}

BaseConnection::Adaptee::~Adaptee()
{
}

void BaseConnection::Adaptee::disconnect(const Tp::Service::ConnectionAdaptor::DisconnectContextPtr &context)
{
    debug() << "BaseConnection::Adaptee::disconnect";
    /* This will remove the connection from the connection manager
     * and destroy this object. */
    emit mConnection->disconnected();
    context->setFinished();
}

void BaseConnection::Adaptee::getSelfHandle(const Tp::Service::ConnectionAdaptor::GetSelfHandleContextPtr &context)
{
    context->setFinished(mConnection->mPriv->selfHandle);
}

uint BaseConnection::Adaptee::selfHandle() const
{
    return mConnection->mPriv->selfHandle;
}

void BaseConnection::Adaptee::getStatus(const Tp::Service::ConnectionAdaptor::GetStatusContextPtr &context)
{
    context->setFinished(mConnection->status());
}

void BaseConnection::Adaptee::connect(const Tp::Service::ConnectionAdaptor::ConnectContextPtr &context)
{
    if (!mConnection->mPriv->connectCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mConnection->mPriv->connectCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnection::Adaptee::inspectHandles(uint handleType,
                                             const Tp::UIntList &handles,
                                             const Tp::Service::ConnectionAdaptor::InspectHandlesContextPtr &context)
{
    if (!mConnection->mPriv->inspectHandlesCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    QStringList ret = mConnection->mPriv->inspectHandlesCB(handleType, handles, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(ret);
}
QStringList BaseConnection::Adaptee::interfaces() const
{
    QStringList ret;
    foreach(const AbstractConnectionInterfacePtr & iface, mConnection->interfaces()) {
        ret << iface->interfaceName();
    }
    return ret;
}

void BaseConnection::Adaptee::requestChannel(const QString &type, uint handleType, uint handle, bool suppressHandler,
        const Tp::Service::ConnectionAdaptor::RequestChannelContextPtr &context)
{
    debug() << "BaseConnection::Adaptee::requestChannel (deprecated)";
    DBusError error;
    bool yours;
    BaseChannelPtr channel = mConnection->ensureChannel(type,
                             handleType,
                             handle,
                             yours,
                             selfHandle(),
                             suppressHandler,
                             &error);
    if (error.isValid() || !channel) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(QDBusObjectPath(channel->objectPath()));
}

void BaseConnection::Adaptee::requestHandles(uint handleType, const QStringList &identifiers,
        const Tp::Service::ConnectionAdaptor::RequestHandlesContextPtr &context)
{
    DBusError error;
    Tp::UIntList handles = mConnection->requestHandles(handleType, identifiers, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(handles);
}

/**
 * \class BaseConnection
 * \ingroup serviceconn
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for Connection implementations.
 */

/**
 * Construct a BaseConnection.
 *
 * \param dbusConnection The D-Bus connection that will be used by this object.
 * \param cmName The name of the connection manager associated with this connection.
 * \param protocolName The name of the protocol associated with this connection.
 * \param parameters The parameters of this connection.
 */
BaseConnection::BaseConnection(const QDBusConnection &dbusConnection,
                               const QString &cmName, const QString &protocolName,
                               const QVariantMap &parameters)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, cmName, protocolName, parameters))
{
}

/**
 * Class destructor.
 */
BaseConnection::~BaseConnection()
{
    foreach (BaseChannelPtr channel, mPriv->channels) {
        channel->close();
    }

    delete mPriv;
}

/**
 * Return the name of the connection manager associated with this connection.
 *
 * \return The name of the connection manager associated with this connection.
 */
QString BaseConnection::cmName() const
{
    return mPriv->cmName;
}

/**
 * Return the name of the protocol associated with this connection.
 *
 * \return The name of the protocol associated with this connection.
 */
QString BaseConnection::protocolName() const
{
    return mPriv->protocolName;
}

/**
 * Return the parameters of this connection.
 *
 * \return The parameters of this connection.
 */
QVariantMap BaseConnection::parameters() const
{
    return mPriv->parameters;
}

/**
 * Return the immutable properties of this connection object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this connection object.
 */
QVariantMap BaseConnection::immutableProperties() const
{
    // FIXME
    return QVariantMap();
}

/**
 * Return a unique name for this connection.
 *
 * \return A unique name for this connection.
 */
QString BaseConnection::uniqueName() const
{
    return QString(QLatin1String("connection_%1")).arg((quintptr) this, 0, 16);
}

uint BaseConnection::status() const
{
    debug() << "BaseConnection::status = " << mPriv->status << " " << this;
    return mPriv->status;
}

void BaseConnection::setStatus(uint newStatus, uint reason)
{
    debug() << "BaseConnection::setStatus " << newStatus << " " << reason << " " << this;
    bool changed = (newStatus != mPriv->status);
    mPriv->status = newStatus;
    if (changed)
        QMetaObject::invokeMethod(mPriv->adaptee, "statusChanged", Q_ARG(uint, newStatus), Q_ARG(uint, reason)); //Can simply use emit in Qt5
}

void BaseConnection::setCreateChannelCallback(const CreateChannelCallback &cb)
{
    mPriv->createChannelCB = cb;
}

Tp::BaseChannelPtr BaseConnection::createChannel(const QString &channelType,
        uint targetHandleType,
        uint targetHandle,
        uint initiatorHandle,
        bool suppressHandler,
        DBusError *error)
{
    if (!mPriv->createChannelCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseChannelPtr();
    }
    if (!mPriv->inspectHandlesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseChannelPtr();
    }

    BaseChannelPtr channel = mPriv->createChannelCB(channelType, targetHandleType, targetHandle, error);
    if (error->isValid())
        return BaseChannelPtr();

    QString targetID;
    if (targetHandle != 0) {
        QStringList list = mPriv->inspectHandlesCB(targetHandleType,  UIntList() << targetHandle, error);
        if (error->isValid()) {
            debug() << "BaseConnection::createChannel: could not resolve handle " << targetHandle;
            return BaseChannelPtr();
        } else {
            debug() << "BaseConnection::createChannel: found targetID " << *list.begin();
            targetID = *list.begin();
        }
    }
    QString initiatorID;
    if (initiatorHandle != 0) {
        QStringList list = mPriv->inspectHandlesCB(HandleTypeContact, UIntList() << initiatorHandle, error);
        if (error->isValid()) {
            debug() << "BaseConnection::createChannel: could not resolve handle " << initiatorHandle;
            return BaseChannelPtr();
        } else {
            debug() << "BaseConnection::createChannel: found initiatorID " << *list.begin();
            initiatorID = *list.begin();
        }
    }
    channel->setInitiatorHandle(initiatorHandle);
    channel->setInitiatorID(initiatorID);
    channel->setTargetID(targetID);
    channel->setRequested(initiatorHandle == mPriv->selfHandle);

    channel->registerObject(error);
    if (error->isValid())
        return BaseChannelPtr();

    mPriv->channels.insert(channel);

    BaseConnectionRequestsInterfacePtr reqIface =
        BaseConnectionRequestsInterfacePtr::dynamicCast(interface(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS));

    if (!reqIface.isNull())
        //emit after return
        QMetaObject::invokeMethod(reqIface.data(), "newChannels",
                                  Qt::QueuedConnection,
                                  Q_ARG(Tp::ChannelDetailsList, ChannelDetailsList() << channel->details()));


    //emit after return
    QMetaObject::invokeMethod(mPriv->adaptee, "newChannel",
                              Qt::QueuedConnection,
                              Q_ARG(QDBusObjectPath, QDBusObjectPath(channel->objectPath())),
                              Q_ARG(QString, channel->channelType()),
                              Q_ARG(uint, channel->targetHandleType()),
                              Q_ARG(uint, channel->targetHandle()),
                              Q_ARG(bool, suppressHandler));

    QObject::connect(channel.data(),
                     SIGNAL(closed()),
                     SLOT(removeChannel()));

    return channel;
}

void BaseConnection::setRequestHandlesCallback(const RequestHandlesCallback &cb)
{
    mPriv->requestHandlesCB = cb;
}

UIntList BaseConnection::requestHandles(uint handleType, const QStringList &identifiers, DBusError* error)
{
    if (!mPriv->requestHandlesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return UIntList();
    }
    return mPriv->requestHandlesCB(handleType, identifiers, error);
}

Tp::ChannelInfoList BaseConnection::channelsInfo()
{
    qDebug() << "BaseConnection::channelsInfo:";
    Tp::ChannelInfoList list;
    foreach(const BaseChannelPtr & c, mPriv->channels) {
        Tp::ChannelInfo info;
        info.channel = QDBusObjectPath(c->objectPath());
        info.channelType = c->channelType();
        info.handle = c->targetHandle();
        info.handleType = c->targetHandleType();
        qDebug() << "BaseConnection::channelsInfo " << info.channel.path();
        list << info;
    }
    return list;
}

Tp::ChannelDetailsList BaseConnection::channelsDetails()
{
    Tp::ChannelDetailsList list;
    foreach(const BaseChannelPtr & c, mPriv->channels)
    list << c->details();
    return list;
}

BaseChannelPtr BaseConnection::ensureChannel(const QString &channelType, uint targetHandleType,
        uint targetHandle, bool &yours, uint initiatorHandle,
        bool suppressHandler,
        DBusError* error)
{
    foreach(BaseChannelPtr channel, mPriv->channels) {
        if (channel->channelType() == channelType
                && channel->targetHandleType() == targetHandleType
                && channel->targetHandle() == targetHandle) {
            yours = false;
            return channel;
        }
    }
    yours = true;
    return createChannel(channelType, targetHandleType, targetHandle, initiatorHandle, suppressHandler, error);
}

void BaseConnection::addChannel(BaseChannelPtr channel)
{
    if (mPriv->channels.contains(channel)) {
        qDebug() << "BaseConnection::addChannel: Channel already added.";
        return;
    }

    mPriv->channels.insert(channel);

    BaseConnectionRequestsInterfacePtr reqIface =
        BaseConnectionRequestsInterfacePtr::dynamicCast(interface(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS));

    if (!reqIface.isNull()) {
        //emit after return
        QMetaObject::invokeMethod(reqIface.data(), "newChannels",
                                  Qt::QueuedConnection,
                                  Q_ARG(Tp::ChannelDetailsList, ChannelDetailsList() << channel->details()));
    }

    //emit after return
    QMetaObject::invokeMethod(mPriv->adaptee, "newChannel",
                              Qt::QueuedConnection,
                              Q_ARG(QDBusObjectPath, QDBusObjectPath(channel->objectPath())),
                              Q_ARG(QString, channel->channelType()),
                              Q_ARG(uint, channel->targetHandleType()),
                              Q_ARG(uint, channel->targetHandle()),
                              Q_ARG(bool, false));

    QObject::connect(channel.data(),
                     SIGNAL(closed()),
                     SLOT(removeChannel()));
}

void BaseConnection::removeChannel()
{
    BaseChannelPtr channel = BaseChannelPtr(
                                 qobject_cast<BaseChannel*>(sender()));
    Q_ASSERT(channel);
    Q_ASSERT(mPriv->channels.contains(channel));

    BaseConnectionRequestsInterfacePtr reqIface =
        BaseConnectionRequestsInterfacePtr::dynamicCast(interface(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS));

    if (!reqIface.isNull()) {
        reqIface->channelClosed(QDBusObjectPath(channel->objectPath()));
    }

    mPriv->channels.remove(channel);
}

/**
 * Return a list of interfaces that have been plugged into this Protocol
 * D-Bus object with plugInterface().
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return A list containing all the Protocol interface implementation objects.
 * \sa plugInterface(), interface()
 */
QList<AbstractConnectionInterfacePtr> BaseConnection::interfaces() const
{
    return mPriv->interfaces.values();
}

/**
 * Return a pointer to the interface with the given name.
 *
 * \param interfaceName The D-Bus name of the interface,
 * ex. TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING.
 * \return A pointer to the AbstractConnectionInterface object that implements
 * the D-Bus interface with the given name, or a null pointer if such an interface
 * has not been plugged into this object.
 * \sa plugInterface(), interfaces()
 */
AbstractConnectionInterfacePtr BaseConnection::interface(const QString &interfaceName) const
{
    return mPriv->interfaces.value(interfaceName);
}

/**
 * Plug a new interface into this Connection D-Bus object.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param interface An AbstractConnectionInterface instance that implements
 * the interface that is to be plugged.
 * \return \c true on success or \c false otherwise
 * \sa interfaces(), interface()
 */
bool BaseConnection::plugInterface(const AbstractConnectionInterfacePtr &interface)
{
    if (isRegistered()) {
        warning() << "Unable to plug protocol interface " << interface->interfaceName() <<
                  "- protocol already registered";
        return false;
    }

    if (interface->isRegistered()) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
                  "- interface already registered";
        return false;
    }

    if (mPriv->interfaces.contains(interface->interfaceName())) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
                  "- another interface with same name already plugged";
        return false;
    }

    debug() << "Interface" << interface->interfaceName() << "plugged";
    mPriv->interfaces.insert(interface->interfaceName(), interface);
    return true;
}

/**
 * Register this connection object on the bus.
 *
 * If \a error is passed, any D-Bus error that may occur will
 * be stored there.
 *
 * \param error A pointer to an empty DBusError where any
 * possible D-Bus error will be stored.
 * \return \c true on success and \c false if there was an error
 * or this connection object is already registered.
 * \sa isRegistered()
 */
bool BaseConnection::registerObject(DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    if (!checkValidProtocolName(mPriv->protocolName)) {
        if (error) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT,
                       mPriv->protocolName + QLatin1String("is not a valid protocol name"));
        }
        debug() << "Unable to register connection - invalid protocol name";
        return false;
    }

    QString escapedProtocolName = mPriv->protocolName;
    escapedProtocolName.replace(QLatin1Char('-'), QLatin1Char('_'));
    QString name = uniqueName();
    debug() << "cmName: " << mPriv->cmName << " escapedProtocolName: " << escapedProtocolName << " name:" << name;
    QString busName = QString(QLatin1String("%1%2.%3.%4"))
                      .arg(TP_QT_CONNECTION_BUS_NAME_BASE, mPriv->cmName, escapedProtocolName, name);
    QString objectPath = QString(QLatin1String("%1%2/%3/%4"))
                         .arg(TP_QT_CONNECTION_OBJECT_PATH_BASE, mPriv->cmName, escapedProtocolName, name);
    debug() << "busName: " << busName << " objectName: " << objectPath;
    DBusError _error;

    debug() << "Connection: registering interfaces  at " << dbusObject();
    foreach(const AbstractConnectionInterfacePtr & iface, mPriv->interfaces) {
        if (!iface->registerInterface(dbusObject())) {
            // lets not fail if an optional interface fails registering, lets warn only
            warning() << "Unable to register interface" << iface->interfaceName();
        }
    }

    bool ret = registerObject(busName, objectPath, &_error);
    if (!ret && error) {
        error->set(_error.name(), _error.message());
    }
    return ret;
}

/**
 * Reimplemented from DBusService.
 */
bool BaseConnection::registerObject(const QString &busName,
                                    const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

void BaseConnection::setSelfHandle(uint selfHandle)
{
    mPriv->selfHandle = selfHandle;
}

uint BaseConnection::selfHandle() const
{
    return mPriv->selfHandle;
}

void BaseConnection::setConnectCallback(const ConnectCallback &cb)
{
    mPriv->connectCB = cb;
}

void BaseConnection::setInspectHandlesCallback(const InspectHandlesCallback &cb)
{
    mPriv->inspectHandlesCB = cb;
}

/**
 * \fn void BaseConnection::disconnected()
 *
 * Emitted when this connection has been disconnected.
 */

/**
 * \class AbstractConnectionInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for all the Connection object interface implementations.
 */

AbstractConnectionInterface::AbstractConnectionInterface(const QString &interfaceName)
    : AbstractDBusServiceInterface(interfaceName)
{
}

AbstractConnectionInterface::~AbstractConnectionInterface()
{
}

// Conn.I.Requests
BaseConnectionRequestsInterface::Adaptee::Adaptee(BaseConnectionRequestsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionRequestsInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionRequestsInterface::Adaptee::ensureChannel(const QVariantMap &request,
        const Tp::Service::ConnectionInterfaceRequestsAdaptor::EnsureChannelContextPtr &context)
{
    DBusError error;
    bool yours;
    QDBusObjectPath channel;
    QVariantMap details;

    mInterface->ensureChannel(request, yours, channel, details, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(yours, channel, details);
}

void BaseConnectionRequestsInterface::Adaptee::createChannel(const QVariantMap &request,
        const Tp::Service::ConnectionInterfaceRequestsAdaptor::CreateChannelContextPtr &context)
{
    DBusError error;
    QDBusObjectPath channel;
    QVariantMap details;

    mInterface->createChannel(request, channel, details, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(channel, details);
}

struct TP_QT_NO_EXPORT BaseConnectionRequestsInterface::Private {
    Private(BaseConnectionRequestsInterface *parent, BaseConnection *connection_)
        : connection(connection_), adaptee(new BaseConnectionRequestsInterface::Adaptee(parent)) {
    }
    BaseConnection *connection;
    BaseConnectionRequestsInterface::Adaptee *adaptee;
};

/**
 * \class BaseConnectionRequestsInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.Requests
 */

/**
 * Class constructor.
 */
BaseConnectionRequestsInterface::BaseConnectionRequestsInterface(BaseConnection *connection)
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS),
      mPriv(new Private(this, connection))
{
}

/**
 * Class destructor.
 */
BaseConnectionRequestsInterface::~BaseConnectionRequestsInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionRequestsInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS + QLatin1String(".RequestableChannelClasses"),
               QVariant::fromValue(mPriv->adaptee->requestableChannelClasses()));
    return map;
}

void BaseConnectionRequestsInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceRequestsAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

Tp::ChannelDetailsList BaseConnectionRequestsInterface::Adaptee::channels() const
{
    return mInterface->mPriv->connection->channelsDetails();
}

void BaseConnectionRequestsInterface::newChannels(const Tp::ChannelDetailsList &channels)
{
    QMetaObject::invokeMethod(mPriv->adaptee,"newChannels", Q_ARG(Tp::ChannelDetailsList,channels)); //Can replace by a direct call in Qt5
}

void BaseConnectionRequestsInterface::channelClosed(const QDBusObjectPath &removed)
{
    QMetaObject::invokeMethod(mPriv->adaptee,"channelClosed", Q_ARG(QDBusObjectPath, removed)); //Can replace by a direct call in Qt5
}

void BaseConnectionRequestsInterface::ensureChannel(const QVariantMap &request, bool &yours,
        QDBusObjectPath &objectPath, QVariantMap &details, DBusError *error)
{
    if (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))
            || !request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"))
            || (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))
                && !request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")))) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Missing parameters"));
        return;
    }

    QString channelType = request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")].toString();
    uint targetHandleType = request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")].toUInt();
    uint targetHandle;
    if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")))
        targetHandle = request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")].toUInt();
    else {
        QString targetID = request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")].toString();
        Tp::UIntList list = mPriv->connection->requestHandles(targetHandleType, QStringList() << targetID, error);
        if (error->isValid()) {
            warning() << "BBaseConnectionRequestsInterface::ensureChannel: could not resolve ID " << targetID;
            return;
        }
        targetHandle = *list.begin();
    }

    bool suppressHandler = true;
    BaseChannelPtr channel = mPriv->connection->ensureChannel(channelType, targetHandleType,
                             targetHandle, yours,
                             mPriv->connection->selfHandle(),
                             suppressHandler,
                             error);
    if (error->isValid())
        return;

    objectPath = QDBusObjectPath(channel->objectPath());
    details = channel->details().properties;
}

void BaseConnectionRequestsInterface::createChannel(const QVariantMap &request,
        QDBusObjectPath &objectPath,
        QVariantMap &details, DBusError *error)
{
    if (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))
            || !request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"))
            || !request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Missing parameters"));
        return;
    }

    QString channelType = request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")].toString();
    uint targetHandleType = request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")].toUInt();
    uint targetHandle = request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")].toUInt();

    bool suppressHandler = true;
    BaseChannelPtr channel = mPriv->connection->createChannel(channelType, targetHandleType,
                             targetHandle,
                             mPriv->connection->selfHandle(),
                             suppressHandler,
                             error);
    if (error->isValid())
        return;

    objectPath = QDBusObjectPath(channel->objectPath());
    details = channel->details().properties;
}


// Conn.I.Contacts
BaseConnectionContactsInterface::Adaptee::Adaptee(BaseConnectionContactsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionContactsInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionContactsInterface::Adaptee::getContactAttributes(const Tp::UIntList &handles,
        const QStringList &interfaces, bool /*hold*/,
        const Tp::Service::ConnectionInterfaceContactsAdaptor::GetContactAttributesContextPtr &context)
{
    DBusError error;
    ContactAttributesMap contactAttributes = mInterface->getContactAttributes(handles, interfaces, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(contactAttributes);
}

struct TP_QT_NO_EXPORT BaseConnectionContactsInterface::Private {
    Private(BaseConnectionContactsInterface *parent)
        : adaptee(new BaseConnectionContactsInterface::Adaptee(parent)) {
    }
    QStringList contactAttributeInterfaces;
    GetContactAttributesCallback getContactAttributesCallback;
    BaseConnectionContactsInterface::Adaptee *adaptee;
};

QStringList BaseConnectionContactsInterface::Adaptee::contactAttributeInterfaces() const
{
    return mInterface->mPriv->contactAttributeInterfaces;
}

/**
 * \class BaseConnectionContactsInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.Contacts
 */

/**
 * Class constructor.
 */
BaseConnectionContactsInterface::BaseConnectionContactsInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionContactsInterface::~BaseConnectionContactsInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionContactsInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS + QLatin1String(".ContactAttributeInterfaces"),
               QVariant::fromValue(mPriv->adaptee->contactAttributeInterfaces()));
    return map;
}

void BaseConnectionContactsInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceContactsAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionContactsInterface::setContactAttributeInterfaces(const QStringList &contactAttributeInterfaces)
{
    mPriv->contactAttributeInterfaces = contactAttributeInterfaces;
}

void BaseConnectionContactsInterface::setGetContactAttributesCallback(const GetContactAttributesCallback &cb)
{
    mPriv->getContactAttributesCallback = cb;
}

ContactAttributesMap BaseConnectionContactsInterface::getContactAttributes(const Tp::UIntList &handles,
        const QStringList &interfaces,
        DBusError *error)
{
    if (!mPriv->getContactAttributesCallback.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return ContactAttributesMap();
    }
    return mPriv->getContactAttributesCallback(handles, interfaces, error);
}

// Conn.I.SimplePresence
BaseConnectionSimplePresenceInterface::Adaptee::Adaptee(BaseConnectionSimplePresenceInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionSimplePresenceInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseConnectionSimplePresenceInterface::Private {
    Private(BaseConnectionSimplePresenceInterface *parent)
        : maxmimumStatusMessageLength(0),
          adaptee(new BaseConnectionSimplePresenceInterface::Adaptee(parent)) {
    }
    SetPresenceCallback setPresenceCB;
    SimpleStatusSpecMap statuses;
    uint maxmimumStatusMessageLength;
    /* The current presences */
    SimpleContactPresences presences;
    BaseConnectionSimplePresenceInterface::Adaptee *adaptee;
};

/**
 * \class BaseConnectionSimplePresenceInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.SimplePresence
 */

/**
 * Class constructor.
 */
BaseConnectionSimplePresenceInterface::BaseConnectionSimplePresenceInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionSimplePresenceInterface::~BaseConnectionSimplePresenceInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionSimplePresenceInterface::immutableProperties() const
{
    QVariantMap map;
    //FIXME
    return map;
}

void BaseConnectionSimplePresenceInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceSimplePresenceAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}



void BaseConnectionSimplePresenceInterface::setPresences(const Tp::SimpleContactPresences &presences)
{
    foreach(uint handle, presences.keys()) {
        mPriv->presences[handle] = presences[handle];
    }
    QMetaObject::invokeMethod(mPriv->adaptee, "presencesChanged", Q_ARG(Tp::SimpleContactPresences, presences)); //Can simply use emit in Qt5
}

void BaseConnectionSimplePresenceInterface::setSetPresenceCallback(const SetPresenceCallback &cb)
{
    mPriv->setPresenceCB = cb;
}

Tp::SimpleStatusSpecMap BaseConnectionSimplePresenceInterface::statuses() const
{
    return mPriv->statuses;
}

void BaseConnectionSimplePresenceInterface::setStatuses(const SimpleStatusSpecMap &statuses)
{
    mPriv->statuses = statuses;
}

int BaseConnectionSimplePresenceInterface::maximumStatusMessageLength() const
{
    return mPriv->maxmimumStatusMessageLength;
}

void BaseConnectionSimplePresenceInterface::setMaxmimumStatusMessageLength(uint maxmimumStatusMessageLength)
{
    mPriv->maxmimumStatusMessageLength = maxmimumStatusMessageLength;
}

Tp::SimpleStatusSpecMap BaseConnectionSimplePresenceInterface::Adaptee::statuses() const
{
    return mInterface->mPriv->statuses;
}

int BaseConnectionSimplePresenceInterface::Adaptee::maximumStatusMessageLength() const
{
    return mInterface->mPriv->maxmimumStatusMessageLength;
}

void BaseConnectionSimplePresenceInterface::Adaptee::setPresence(const QString &status, const QString &statusMessage_,
        const Tp::Service::ConnectionInterfaceSimplePresenceAdaptor::SetPresenceContextPtr &context)
{
    if (!mInterface->mPriv->setPresenceCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    SimpleStatusSpecMap::Iterator i = mInterface->mPriv->statuses.find(status);
    if (i == mInterface->mPriv->statuses.end()) {
        warning() << "BaseConnectionSimplePresenceInterface::Adaptee::setPresence: status is not in statuses";
        context->setFinishedWithError(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("status not in statuses"));
        return;
    }

    QString statusMessage = statusMessage_;
    if ((uint)statusMessage.length() > mInterface->mPriv->maxmimumStatusMessageLength) {
        debug() << "BaseConnectionSimplePresenceInterface::Adaptee::setPresence: "
                << "truncating status to " << mInterface->mPriv->maxmimumStatusMessageLength;
        statusMessage = statusMessage.left(mInterface->mPriv->maxmimumStatusMessageLength);
    }

    DBusError error;
    uint selfHandle = mInterface->mPriv->setPresenceCB(status, statusMessage, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    Tp::SimplePresence presence;
    presence.type = i->type;
    presence.status = status;
    presence.statusMessage = statusMessage;
    mInterface->mPriv->presences[selfHandle] = presence;

    /* Emit PresencesChanged */
    SimpleContactPresences presences;
    presences[selfHandle] = presence;
    //emit after return
    QMetaObject::invokeMethod(mInterface->mPriv->adaptee, "presencesChanged",
                              Qt::QueuedConnection,
                              Q_ARG(Tp::SimpleContactPresences, presences));
    context->setFinished();
}

void BaseConnectionSimplePresenceInterface::Adaptee::getPresences(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceSimplePresenceAdaptor::GetPresencesContextPtr &context)
{
    Tp::SimpleContactPresences presences;
    foreach(uint handle, contacts) {
        SimpleContactPresences::iterator i = mInterface->mPriv->presences.find(handle);
        if (i == mInterface->mPriv->presences.end()) {
            Tp::SimplePresence presence;
            presence.type = ConnectionPresenceTypeUnknown;
            presence.status = QLatin1String("unknown");
            presences[handle] = presence;
        } else
            presences[handle] = *i;
    }
    context->setFinished(presences);
}

// Conn.I.ContactList
struct TP_QT_NO_EXPORT BaseConnectionContactListInterface::Private {
    Private(BaseConnectionContactListInterface *parent)
        : contactListState(ContactListStateNone),
          contactListPersists(false),
          canChangeContactList(true),
          requestUsesMessage(false),
          downloadAtConnection(false),
          adaptee(new BaseConnectionContactListInterface::Adaptee(parent))
    {
    }

    uint contactListState;
    bool contactListPersists;
    bool canChangeContactList;
    bool requestUsesMessage;
    bool downloadAtConnection;
    GetContactListAttributesCallback getContactListAttributesCB;
    RequestSubscriptionCallback requestSubscriptionCB;
    AuthorizePublicationCallback authorizePublicationCB;
    RemoveContactsCallback removeContactsCB;
    UnsubscribeCallback unsubscribeCB;
    UnpublishCallback unpublishCB;
    DownloadCallback downloadCB;
    BaseConnectionContactListInterface::Adaptee *adaptee;
};

BaseConnectionContactListInterface::Adaptee::Adaptee(BaseConnectionContactListInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionContactListInterface::Adaptee::~Adaptee()
{
}

uint BaseConnectionContactListInterface::Adaptee::contactListState() const
{
    return mInterface->contactListState();
}

bool BaseConnectionContactListInterface::Adaptee::contactListPersists() const
{
    return mInterface->contactListPersists();
}

bool BaseConnectionContactListInterface::Adaptee::canChangeContactList() const
{
    return mInterface->canChangeContactList();
}

bool BaseConnectionContactListInterface::Adaptee::requestUsesMessage() const
{
    return mInterface->requestUsesMessage();
}

bool BaseConnectionContactListInterface::Adaptee::downloadAtConnection() const
{
    return mInterface->downloadAtConnection();
}

void BaseConnectionContactListInterface::Adaptee::getContactListAttributes(const QStringList &interfaces, bool hold,
        const Tp::Service::ConnectionInterfaceContactListAdaptor::GetContactListAttributesContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::getContactListAttributes";
    DBusError error;
    Tp::ContactAttributesMap attributes = mInterface->getContactListAttributes(interfaces, hold, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(attributes);
}

void BaseConnectionContactListInterface::Adaptee::requestSubscription(const Tp::UIntList &contacts, const QString &message,
        const Tp::Service::ConnectionInterfaceContactListAdaptor::RequestSubscriptionContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::requestSubscription";
    DBusError error;
    mInterface->requestSubscription(contacts, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactListInterface::Adaptee::authorizePublication(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceContactListAdaptor::AuthorizePublicationContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::authorizePublication";
    DBusError error;
    mInterface->authorizePublication(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactListInterface::Adaptee::removeContacts(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceContactListAdaptor::RemoveContactsContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::removeContacts";
    DBusError error;
    mInterface->removeContacts(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactListInterface::Adaptee::unsubscribe(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceContactListAdaptor::UnsubscribeContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::unsubscribe";
    DBusError error;
    mInterface->unsubscribe(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactListInterface::Adaptee::unpublish(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceContactListAdaptor::UnpublishContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::unpublish";
    DBusError error;
    mInterface->unpublish(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactListInterface::Adaptee::download(
        const Tp::Service::ConnectionInterfaceContactListAdaptor::DownloadContextPtr &context)
{
    qDebug() << "BaseConnectionContactListInterface::Adaptee::download";
    DBusError error;
    mInterface->download(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseConnectionContactListInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.ContactList
 */

/**
 * Class constructor.
 */
BaseConnectionContactListInterface::BaseConnectionContactListInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionContactListInterface::~BaseConnectionContactListInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionContactListInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

uint BaseConnectionContactListInterface::contactListState() const
{
    return mPriv->contactListState;
}

void BaseConnectionContactListInterface::setContactListState(uint contactListState)
{
    if (mPriv->contactListState == contactListState) {
        return;
    }

    mPriv->contactListState = contactListState;
    QMetaObject::invokeMethod(mPriv->adaptee, "contactListStateChanged", Q_ARG(uint, contactListState)); //Can simply use emit in Qt5
}

bool BaseConnectionContactListInterface::contactListPersists() const
{
    return mPriv->contactListPersists;
}

void BaseConnectionContactListInterface::setContactListPersists(bool contactListPersists)
{
    mPriv->contactListPersists = contactListPersists;
}

bool BaseConnectionContactListInterface::canChangeContactList() const
{
    return mPriv->canChangeContactList;
}

void BaseConnectionContactListInterface::setCanChangeContactList(bool canChangeContactList)
{
    mPriv->canChangeContactList = canChangeContactList;
}

bool BaseConnectionContactListInterface::requestUsesMessage() const
{
    return mPriv->requestUsesMessage;
}

void BaseConnectionContactListInterface::setRequestUsesMessage(bool requestUsesMessage)
{
    mPriv->requestUsesMessage = requestUsesMessage;
}

bool BaseConnectionContactListInterface::downloadAtConnection() const
{
    return mPriv->downloadAtConnection;
}

void BaseConnectionContactListInterface::setDownloadAtConnection(bool downloadAtConnection)
{
    mPriv->downloadAtConnection = downloadAtConnection;
}

void BaseConnectionContactListInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceContactListAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionContactListInterface::setGetContactListAttributesCallback(const BaseConnectionContactListInterface::GetContactListAttributesCallback &cb)
{
    mPriv->getContactListAttributesCB = cb;
}

Tp::ContactAttributesMap BaseConnectionContactListInterface::getContactListAttributes(const QStringList &interfaces, bool hold, DBusError *error)
{
    if (!mPriv->getContactListAttributesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ContactAttributesMap();
    }
    return mPriv->getContactListAttributesCB(interfaces, hold, error);
}

void BaseConnectionContactListInterface::setRequestSubscriptionCallback(const BaseConnectionContactListInterface::RequestSubscriptionCallback &cb)
{
    mPriv->requestSubscriptionCB = cb;
}

void BaseConnectionContactListInterface::requestSubscription(const Tp::UIntList &contacts, const QString &message, DBusError *error)
{
    if (!mPriv->requestSubscriptionCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->requestSubscriptionCB(contacts, message, error);
}

void BaseConnectionContactListInterface::setAuthorizePublicationCallback(const BaseConnectionContactListInterface::AuthorizePublicationCallback &cb)
{
    mPriv->authorizePublicationCB = cb;
}

void BaseConnectionContactListInterface::authorizePublication(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->authorizePublicationCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->authorizePublicationCB(contacts, error);
}

void BaseConnectionContactListInterface::setRemoveContactsCallback(const BaseConnectionContactListInterface::RemoveContactsCallback &cb)
{
    mPriv->removeContactsCB = cb;
}

void BaseConnectionContactListInterface::removeContacts(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->removeContactsCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->removeContactsCB(contacts, error);
}

void BaseConnectionContactListInterface::setUnsubscribeCallback(const BaseConnectionContactListInterface::UnsubscribeCallback &cb)
{
    mPriv->unsubscribeCB = cb;
}

void BaseConnectionContactListInterface::unsubscribe(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->unsubscribeCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->unsubscribeCB(contacts, error);
}

void BaseConnectionContactListInterface::setUnpublishCallback(const BaseConnectionContactListInterface::UnpublishCallback &cb)
{
    mPriv->unpublishCB = cb;
}

void BaseConnectionContactListInterface::unpublish(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->unpublishCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->unpublishCB(contacts, error);
}

void BaseConnectionContactListInterface::setDownloadCallback(const BaseConnectionContactListInterface::DownloadCallback &cb)
{
    mPriv->downloadCB = cb;
}

void BaseConnectionContactListInterface::download(DBusError *error)
{
    if (!mPriv->downloadCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->downloadCB(error);
}

void BaseConnectionContactListInterface::contactsChangedWithID(const Tp::ContactSubscriptionMap &changes, const Tp::HandleIdentifierMap &identifiers, const Tp::HandleIdentifierMap &removals)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "contactsChangedWithID", Q_ARG(Tp::ContactSubscriptionMap, changes), Q_ARG(Tp::HandleIdentifierMap, identifiers), Q_ARG(Tp::HandleIdentifierMap, removals)); //Can simply use emit in Qt5
}

// Conn.I.ContactInfo
struct TP_QT_NO_EXPORT BaseConnectionContactInfoInterface::Private {
    Private(BaseConnectionContactInfoInterface *parent)
        : adaptee(new BaseConnectionContactInfoInterface::Adaptee(parent))
    {
    }

    Tp::ContactInfoFlags contactInfoFlags;
    Tp::FieldSpecs supportedFields;
    GetContactInfoCallback getContactInfoCB;
    RefreshContactInfoCallback refreshContactInfoCB;
    RequestContactInfoCallback requestContactInfoCB;
    SetContactInfoCallback setContactInfoCB;
    BaseConnectionContactInfoInterface::Adaptee *adaptee;
};

BaseConnectionContactInfoInterface::Adaptee::Adaptee(BaseConnectionContactInfoInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionContactInfoInterface::Adaptee::~Adaptee()
{
}

uint BaseConnectionContactInfoInterface::Adaptee::contactInfoFlags() const
{
    return mInterface->contactInfoFlags();
}

Tp::FieldSpecs BaseConnectionContactInfoInterface::Adaptee::supportedFields() const
{
    return mInterface->supportedFields();
}

void BaseConnectionContactInfoInterface::Adaptee::getContactInfo(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceContactInfoAdaptor::GetContactInfoContextPtr &context)
{
    qDebug() << "BaseConnectionContactInfoInterface::Adaptee::getContactInfo";
    DBusError error;
    Tp::ContactInfoMap contactInfo = mInterface->getContactInfo(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(contactInfo);
}

void BaseConnectionContactInfoInterface::Adaptee::refreshContactInfo(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceContactInfoAdaptor::RefreshContactInfoContextPtr &context)
{
    qDebug() << "BaseConnectionContactInfoInterface::Adaptee::refreshContactInfo";
    DBusError error;
    mInterface->refreshContactInfo(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactInfoInterface::Adaptee::requestContactInfo(uint contact,
        const Tp::Service::ConnectionInterfaceContactInfoAdaptor::RequestContactInfoContextPtr &context)
{
    qDebug() << "BaseConnectionContactInfoInterface::Adaptee::requestContactInfo";
    DBusError error;
    Tp::ContactInfoFieldList contactInfo = mInterface->requestContactInfo(contact, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(contactInfo);
}

void BaseConnectionContactInfoInterface::Adaptee::setContactInfo(const Tp::ContactInfoFieldList &contactInfo,
        const Tp::Service::ConnectionInterfaceContactInfoAdaptor::SetContactInfoContextPtr &context)
{
    qDebug() << "BaseConnectionContactInfoInterface::Adaptee::setContactInfo";
    DBusError error;
    mInterface->setContactInfo(contactInfo, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseConnectionContactInfoInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.Contact.Info
 */

/**
 * Class constructor.
 */
BaseConnectionContactInfoInterface::BaseConnectionContactInfoInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_INFO),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionContactInfoInterface::~BaseConnectionContactInfoInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionContactInfoInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

Tp::ContactInfoFlags BaseConnectionContactInfoInterface::contactInfoFlags() const
{
    return mPriv->contactInfoFlags;
}

void BaseConnectionContactInfoInterface::setContactInfoFlags(const Tp::ContactInfoFlags &contactInfoFlags)
{
    mPriv->contactInfoFlags = contactInfoFlags;
}

Tp::FieldSpecs BaseConnectionContactInfoInterface::supportedFields() const
{
    return mPriv->supportedFields;
}

void BaseConnectionContactInfoInterface::setSupportedFields(const Tp::FieldSpecs &supportedFields)
{
    mPriv->supportedFields = supportedFields;
}

void BaseConnectionContactInfoInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceContactInfoAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionContactInfoInterface::setGetContactInfoCallback(const BaseConnectionContactInfoInterface::GetContactInfoCallback &cb)
{
    mPriv->getContactInfoCB = cb;
}

Tp::ContactInfoMap BaseConnectionContactInfoInterface::getContactInfo(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->getContactInfoCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ContactInfoMap();
    }
    return mPriv->getContactInfoCB(contacts, error);
}

void BaseConnectionContactInfoInterface::setRefreshContactInfoCallback(const BaseConnectionContactInfoInterface::RefreshContactInfoCallback &cb)
{
    mPriv->refreshContactInfoCB = cb;
}

void BaseConnectionContactInfoInterface::refreshContactInfo(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->refreshContactInfoCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->refreshContactInfoCB(contacts, error);
}

void BaseConnectionContactInfoInterface::setRequestContactInfoCallback(const BaseConnectionContactInfoInterface::RequestContactInfoCallback &cb)
{
    mPriv->requestContactInfoCB = cb;
}

Tp::ContactInfoFieldList BaseConnectionContactInfoInterface::requestContactInfo(uint contact, DBusError *error)
{
    if (!mPriv->requestContactInfoCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ContactInfoFieldList();
    }
    return mPriv->requestContactInfoCB(contact, error);
}

void BaseConnectionContactInfoInterface::setSetContactInfoCallback(const BaseConnectionContactInfoInterface::SetContactInfoCallback &cb)
{
    mPriv->setContactInfoCB = cb;
}

void BaseConnectionContactInfoInterface::setContactInfo(const Tp::ContactInfoFieldList &contactInfo, DBusError *error)
{
    if (!mPriv->setContactInfoCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->setContactInfoCB(contactInfo, error);
}

void BaseConnectionContactInfoInterface::contactInfoChanged(uint contact, const Tp::ContactInfoFieldList &contactInfo)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "contactInfoChanged", Q_ARG(uint, contact), Q_ARG(Tp::ContactInfoFieldList, contactInfo)); //Can simply use emit in Qt5
}

// Conn.I.Addressing
BaseConnectionAddressingInterface::Adaptee::Adaptee(BaseConnectionAddressingInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionAddressingInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseConnectionAddressingInterface::Private {
    Private(BaseConnectionAddressingInterface *parent)
        : adaptee(new BaseConnectionAddressingInterface::Adaptee(parent)) {
    }
    GetContactsByVCardFieldCallback getContactsByVCardFieldCB;
    GetContactsByURICallback getContactsByURICB;
    BaseConnectionAddressingInterface::Adaptee *adaptee;
};

/**
 * \class BaseConnectionAddressingInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.Addressing
 */

/**
 * Class constructor.
 */
BaseConnectionAddressingInterface::BaseConnectionAddressingInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionAddressingInterface::~BaseConnectionAddressingInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionAddressingInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionAddressingInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceAddressingAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionAddressingInterface::setGetContactsByVCardFieldCallback(const GetContactsByVCardFieldCallback &cb)
{
    mPriv->getContactsByVCardFieldCB = cb;
}

void BaseConnectionAddressingInterface::setGetContactsByURICallback(const GetContactsByURICallback &cb)
{
    mPriv->getContactsByURICB = cb;
}

void BaseConnectionAddressingInterface::Adaptee::getContactsByVCardField(const QString &field,
        const QStringList &addresses,
        const QStringList &interfaces,
        const Tp::Service::ConnectionInterfaceAddressingAdaptor::GetContactsByVCardFieldContextPtr &context)
{
    if (!mInterface->mPriv->getContactsByVCardFieldCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::AddressingNormalizationMap addressingNormalizationMap;
    Tp::ContactAttributesMap contactAttributesMap;

    DBusError error;
    mInterface->mPriv->getContactsByVCardFieldCB(field, addresses, interfaces,
            addressingNormalizationMap, contactAttributesMap,
            &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(addressingNormalizationMap, contactAttributesMap);
}

void BaseConnectionAddressingInterface::Adaptee::getContactsByURI(const QStringList &URIs,
        const QStringList &interfaces,
        const Tp::Service::ConnectionInterfaceAddressingAdaptor::GetContactsByURIContextPtr &context)
{
    if (!mInterface->mPriv->getContactsByURICB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::AddressingNormalizationMap addressingNormalizationMap;
    Tp::ContactAttributesMap contactAttributesMap;

    DBusError error;
    mInterface->mPriv->getContactsByURICB(URIs, interfaces,
                                          addressingNormalizationMap, contactAttributesMap,
                                          &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(addressingNormalizationMap, contactAttributesMap);
}

// Conn.I.Aliasing
struct TP_QT_NO_EXPORT BaseConnectionAliasingInterface::Private {
    Private(BaseConnectionAliasingInterface *parent)
        : adaptee(new BaseConnectionAliasingInterface::Adaptee(parent))
    {
    }

    GetAliasFlagsCallback getAliasFlagsCB;
    RequestAliasesCallback requestAliasesCB;
    GetAliasesCallback getAliasesCB;
    SetAliasesCallback setAliasesCB;
    BaseConnectionAliasingInterface::Adaptee *adaptee;
};

BaseConnectionAliasingInterface::Adaptee::Adaptee(BaseConnectionAliasingInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionAliasingInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionAliasingInterface::Adaptee::getAliasFlags(
        const Tp::Service::ConnectionInterfaceAliasingAdaptor::GetAliasFlagsContextPtr &context)
{
    qDebug() << "BaseConnectionAliasingInterface::Adaptee::getAliasFlags";
    DBusError error;
    Tp::ConnectionAliasFlags aliasFlags = mInterface->getAliasFlags(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(aliasFlags);
}

void BaseConnectionAliasingInterface::Adaptee::requestAliases(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceAliasingAdaptor::RequestAliasesContextPtr &context)
{
    qDebug() << "BaseConnectionAliasingInterface::Adaptee::requestAliases";
    DBusError error;
    QStringList aliases = mInterface->requestAliases(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(aliases);
}

void BaseConnectionAliasingInterface::Adaptee::getAliases(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceAliasingAdaptor::GetAliasesContextPtr &context)
{
    qDebug() << "BaseConnectionAliasingInterface::Adaptee::getAliases";
    DBusError error;
    Tp::AliasMap aliases = mInterface->getAliases(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(aliases);
}

void BaseConnectionAliasingInterface::Adaptee::setAliases(const Tp::AliasMap &aliases,
        const Tp::Service::ConnectionInterfaceAliasingAdaptor::SetAliasesContextPtr &context)
{
    qDebug() << "BaseConnectionAliasingInterface::Adaptee::setAliases";
    DBusError error;
    mInterface->setAliases(aliases, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseConnectionAliasingInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.Aliasing
 */

/**
 * Class constructor.
 */
BaseConnectionAliasingInterface::BaseConnectionAliasingInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionAliasingInterface::~BaseConnectionAliasingInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseConnectionAliasingInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionAliasingInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceAliasingAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionAliasingInterface::setGetAliasFlagsCallback(const BaseConnectionAliasingInterface::GetAliasFlagsCallback &cb)
{
    mPriv->getAliasFlagsCB = cb;
}

Tp::ConnectionAliasFlags BaseConnectionAliasingInterface::getAliasFlags(DBusError *error)
{
    if (!mPriv->getAliasFlagsCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ConnectionAliasFlags();
    }
    return mPriv->getAliasFlagsCB(error);
}

void BaseConnectionAliasingInterface::setRequestAliasesCallback(const BaseConnectionAliasingInterface::RequestAliasesCallback &cb)
{
    mPriv->requestAliasesCB = cb;
}

QStringList BaseConnectionAliasingInterface::requestAliases(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->requestAliasesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QStringList();
    }
    return mPriv->requestAliasesCB(contacts, error);
}

void BaseConnectionAliasingInterface::setGetAliasesCallback(const BaseConnectionAliasingInterface::GetAliasesCallback &cb)
{
    mPriv->getAliasesCB = cb;
}

Tp::AliasMap BaseConnectionAliasingInterface::getAliases(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->getAliasesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::AliasMap();
    }
    return mPriv->getAliasesCB(contacts, error);
}

void BaseConnectionAliasingInterface::setSetAliasesCallback(const BaseConnectionAliasingInterface::SetAliasesCallback &cb)
{
    mPriv->setAliasesCB = cb;
}

void BaseConnectionAliasingInterface::setAliases(const Tp::AliasMap &aliases, DBusError *error)
{
    if (!mPriv->setAliasesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->setAliasesCB(aliases, error);
}

void BaseConnectionAliasingInterface::aliasesChanged(const Tp::AliasPairList &aliases)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "aliasesChanged", Q_ARG(Tp::AliasPairList, aliases)); //Can simply use emit in Qt5
}

}
