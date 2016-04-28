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
    Private(BaseConnection *connection, const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters)
        : connection(connection),
          cmName(cmName),
          protocolName(protocolName),
          parameters(parameters),
          selfHandle(0),
          status(Tp::ConnectionStatusDisconnected),
          adaptee(new BaseConnection::Adaptee(dbusConnection, connection))
    {
    }

    BaseConnection *connection;
    QString cmName;
    QString protocolName;
    QVariantMap parameters;
    QHash<QString, AbstractConnectionInterfacePtr> interfaces;
    QSet<BaseChannelPtr> channels;
    uint selfHandle;
    QString selfID;
    uint status;
    CreateChannelCallback createChannelCB;
    ConnectCallback connectCB;
    InspectHandlesCallback inspectHandlesCB;
    RequestHandlesCallback requestHandlesCB;
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

QStringList BaseConnection::Adaptee::interfaces() const
{
    QStringList ret;
    foreach(const AbstractConnectionInterfacePtr &iface, mConnection->interfaces()) {
        ret << iface->interfaceName();
    }
    return ret;
}

uint BaseConnection::Adaptee::selfHandle() const
{
    return mConnection->selfHandle();
}

QString BaseConnection::Adaptee::selfID() const
{
    return mConnection->selfID();
}

uint BaseConnection::Adaptee::status() const
{
    return mConnection->status();
}

bool BaseConnection::Adaptee::hasImmortalHandles() const
{
    /* True if handles last for the whole lifetime of the Connection.
     * This SHOULD be the case in all connection managers, but connection managers
     * MUST interoperate with older clients (which reference-count handles). */
    return true;
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

void BaseConnection::Adaptee::disconnect(const Tp::Service::ConnectionAdaptor::DisconnectContextPtr &context)
{
    debug() << "BaseConnection::Adaptee::disconnect";

    foreach(const BaseChannelPtr &channel, mConnection->mPriv->channels) {
        /* BaseChannel::closed() signal triggers removeChannel() method call with proper cleanup */
        channel->close();
    }

    /* This signal will remove the connection from the connection manager
     * and destroy this object. */
    emit mConnection->disconnected();

    context->setFinished();
}

void BaseConnection::Adaptee::getInterfaces(const Service::ConnectionAdaptor::GetInterfacesContextPtr &context)
{
    context->setFinished(interfaces());
}

void BaseConnection::Adaptee::getProtocol(const Service::ConnectionAdaptor::GetProtocolContextPtr &context)
{
    context->setFinished(mConnection->protocolName());
}

void BaseConnection::Adaptee::getSelfHandle(const Service::ConnectionAdaptor::GetSelfHandleContextPtr &context)
{
    context->setFinished(mConnection->selfHandle());
}

void BaseConnection::Adaptee::getStatus(const Tp::Service::ConnectionAdaptor::GetStatusContextPtr &context)
{
    context->setFinished(mConnection->status());
}

void BaseConnection::Adaptee::holdHandles(uint handleType, const UIntList &handles, const Service::ConnectionAdaptor::HoldHandlesContextPtr &context)
{
    // This method no does anything since 0.21.6
    Q_UNUSED(handleType)
    Q_UNUSED(handles)
    context->setFinished();
}

void BaseConnection::Adaptee::inspectHandles(uint handleType,
                                             const Tp::UIntList &handles,
                                             const Tp::Service::ConnectionAdaptor::InspectHandlesContextPtr &context)
{
    DBusError error;
    QStringList identifiers = mConnection->inspectHandles(handleType, handles, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(identifiers);
}

void BaseConnection::Adaptee::listChannels(const Service::ConnectionAdaptor::ListChannelsContextPtr &context)
{
    context->setFinished(mConnection->channelsInfo());
}

void BaseConnection::Adaptee::requestChannel(const QString &type, uint handleType, uint handle, bool suppressHandler,
        const Tp::Service::ConnectionAdaptor::RequestChannelContextPtr &context)
{
    debug() << "BaseConnection::Adaptee::requestChannel (deprecated)";
    DBusError error;

    QVariantMap request;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = type;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")] = handleType;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = handle;

    bool yours;
    BaseChannelPtr channel = mConnection->ensureChannel(request, yours, suppressHandler, &error);
    if (error.isValid() || !channel) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(QDBusObjectPath(channel->objectPath()));
}

void BaseConnection::Adaptee::releaseHandles(uint handleType, const UIntList &handles, const Service::ConnectionAdaptor::ReleaseHandlesContextPtr &context)
{
    // This method no does anything since 0.21.6
    Q_UNUSED(handleType)
    Q_UNUSED(handles)
    context->setFinished();
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
    // There is no immutable properties.
    return QVariantMap();
}

uint BaseConnection::selfHandle() const
{
    return mPriv->selfHandle;
}

void BaseConnection::setSelfHandle(uint selfHandle)
{
    if (selfHandle == mPriv->selfHandle) {
        return;
    }

    mPriv->selfHandle = selfHandle;
    QMetaObject::invokeMethod(mPriv->adaptee, "selfHandleChanged", Q_ARG(uint, mPriv->selfHandle)); //Can simply use emit in Qt5
    QMetaObject::invokeMethod(mPriv->adaptee, "selfContactChanged", Q_ARG(uint, mPriv->selfHandle), Q_ARG(QString, mPriv->selfID)); //Can simply use emit in Qt5
}

QString BaseConnection::selfID() const
{
    return mPriv->selfID;
}

void BaseConnection::setSelfID(const QString &selfID)
{
    if (selfID == mPriv->selfID) {
        return;
    }

    mPriv->selfID = selfID;
    QMetaObject::invokeMethod(mPriv->adaptee, "selfContactChanged", Q_ARG(uint, mPriv->selfHandle), Q_ARG(QString, mPriv->selfID)); //Can simply use emit in Qt5
}

void BaseConnection::setSelfContact(uint selfHandle, const QString &selfID)
{
    if ((selfHandle == mPriv->selfHandle) && (selfID == mPriv->selfID)) {
        return;
    }

    if (selfHandle != mPriv->selfHandle) {
        QMetaObject::invokeMethod(mPriv->adaptee, "selfHandleChanged", Q_ARG(uint, mPriv->selfHandle)); //Can simply use emit in Qt5
        mPriv->selfHandle = selfHandle;
    }

    mPriv->selfID = selfID;
    QMetaObject::invokeMethod(mPriv->adaptee, "selfContactChanged", Q_ARG(uint, mPriv->selfHandle), Q_ARG(QString, mPriv->selfID)); //Can simply use emit in Qt5
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

Tp::BaseChannelPtr BaseConnection::createChannel(const QVariantMap &request, bool suppressHandler, DBusError *error)
{
    if (!mPriv->createChannelCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseChannelPtr();
    }
    if (!mPriv->inspectHandlesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseChannelPtr();
    }

    if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"))) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QString(QLatin1String("The %1.Requested property must not be presented in the request details.")).arg(TP_QT_IFACE_CHANNEL));
        return BaseChannelPtr();
    }

    QVariantMap requestDetails = request;
    requestDetails[TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")] = suppressHandler;

    BaseChannelPtr channel = mPriv->createChannelCB(requestDetails, error);
    if (error->isValid())
        return BaseChannelPtr();

    QString targetID = channel->targetID();
    if ((channel->targetHandle() != 0) && targetID.isEmpty()) {
        QStringList list = mPriv->inspectHandlesCB(channel->targetHandleType(),  UIntList() << channel->targetHandle(), error);
        if (error->isValid()) {
            debug() << "BaseConnection::createChannel: could not resolve handle " << channel->targetHandle();
            return BaseChannelPtr();
        } else {
            debug() << "BaseConnection::createChannel: found targetID " << *list.begin();
            targetID = *list.begin();
        }
        channel->setTargetID(targetID);
    }

    if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle"))) {
        channel->setInitiatorHandle(request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")).toUInt());
    }

    QString initiatorID = channel->initiatorID();
    if ((channel->initiatorHandle() != 0) && initiatorID.isEmpty()) {
        QStringList list = mPriv->inspectHandlesCB(HandleTypeContact, UIntList() << channel->initiatorHandle(), error);
        if (error->isValid()) {
            debug() << "BaseConnection::createChannel: could not resolve handle " << channel->initiatorHandle();
            return BaseChannelPtr();
        } else {
            debug() << "BaseConnection::createChannel: found initiatorID " << *list.begin();
            initiatorID = *list.begin();
        }
        channel->setInitiatorID(initiatorID);
    }
    channel->setRequested(suppressHandler);

    channel->registerObject(error);
    if (error->isValid())
        return BaseChannelPtr();

    addChannel(channel, suppressHandler);

    return channel;
}

void BaseConnection::setConnectCallback(const ConnectCallback &cb)
{
    mPriv->connectCB = cb;
}

void BaseConnection::setInspectHandlesCallback(const InspectHandlesCallback &cb)
{
    mPriv->inspectHandlesCB = cb;
}

QStringList BaseConnection::inspectHandles(uint handleType, const Tp::UIntList &handles, DBusError *error)
{
    if (!mPriv->inspectHandlesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QStringList();
    }
    return mPriv->inspectHandlesCB(handleType, handles, error);
}

void BaseConnection::setRequestHandlesCallback(const RequestHandlesCallback &cb)
{
    mPriv->requestHandlesCB = cb;
}

Tp::UIntList BaseConnection::requestHandles(uint handleType, const QStringList &identifiers, DBusError *error)
{
    if (!mPriv->requestHandlesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::UIntList();
    }
    return mPriv->requestHandlesCB(handleType, identifiers, error);
}

Tp::ChannelInfoList BaseConnection::channelsInfo()
{
    debug() << "BaseConnection::channelsInfo:";
    Tp::ChannelInfoList list;
    foreach(const BaseChannelPtr & c, mPriv->channels) {
        Tp::ChannelInfo info;
        info.channel = QDBusObjectPath(c->objectPath());
        info.channelType = c->channelType();
        info.handle = c->targetHandle();
        info.handleType = c->targetHandleType();
        debug() << "BaseConnection::channelsInfo " << info.channel.path();
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

/**
 * \fn Tp::BaseChannelPtr BaseConnection::ensureChannel(const QVariantMap &request, bool &yours, bool suppressHandler, DBusError *error)
 *
 * Return a new or exists channel, satisfying the given \a request.
 *
 * This method iterate over exist channels to find the one satisfying the \a request. If there is no
 * suitable channel, then new channel with given request details will be created.
 * This method uses the matchChannel() method to check whether there exists a channel which confirms with the \a request.
 *
 * If \a error is passed, any error that may occur will be stored there.
 *
 * \param request A dictionary containing the desirable properties.
 * \param yours A returning argument. \c true if returned channel is a new one and \c false otherwise.
 * \param suppressHandler An option to suppress handler in case of a new channel creation.
 * \param error A pointer to an empty DBusError where any possible error will be stored.
 * \return A pointer to a channel, satisfying the given \a request.
 * \sa matchChannel()
 */
Tp::BaseChannelPtr BaseConnection::ensureChannel(const QVariantMap &request, bool &yours, bool suppressHandler, DBusError *error)
{
    if (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Missing parameters"));
        return Tp::BaseChannelPtr();
    }

    const QString channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();

    foreach(const BaseChannelPtr &channel, mPriv->channels) {
        if (channel->channelType() != channelType) {
            continue;
        }

        bool match = matchChannel(channel, request, error);

        if (error->isValid()) {
            return BaseChannelPtr();
        }

        if (match) {
            yours = false;
            return channel;
        }
    }

    yours = true;
    return createChannel(request, suppressHandler, error);
}

void BaseConnection::addChannel(BaseChannelPtr channel, bool suppressHandler)
{
    if (mPriv->channels.contains(channel)) {
        warning() << "BaseConnection::addChannel: Channel already added.";
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
                              Q_ARG(bool, suppressHandler));

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
    interface->setBaseConnection(this);
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
 * Return a unique name for this connection.
 *
 * \return A unique name for this connection.
 */
QString BaseConnection::uniqueName() const
{
    return QString(QLatin1String("connection_%1")).arg((quintptr) this, 0, 16);
}

/**
 * Reimplemented from DBusService.
 */
bool BaseConnection::registerObject(const QString &busName,
                                    const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

/**
 * \fn bool BaseConnection::matchChannel(const BaseChannelPtr &channel, const QVariantMap &request, DBusError *error)
 *
 * Check \a channel on conformity with \a request.
 *
 * This virtual method is used to check if a \a channel satisfying the given request.
 * It is warranted, that the type of the channel meets the requested type.
 *
 * The default implementation compares TargetHandleType and TargetHandle/TargetID.
 * If \a error is passed, any error that may occur will be stored there.
 *
 * \param channel A pointer to a channel to be checked.
 * \param request A dictionary containing the desirable properties.
 * \param error A pointer to an empty DBusError where any
 * possible error will be stored.
 * \return \c true if channel match the request and \c false otherwise.
 * \sa ensureChannel()
 */
bool BaseConnection::matchChannel(const BaseChannelPtr &channel, const QVariantMap &request, DBusError *error)
{
    Q_UNUSED(error);

    if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"))) {
        uint targetHandleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
        if (channel->targetHandleType() != targetHandleType) {
            return false;
        }
        if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
            uint targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
            return channel->targetHandle() == targetHandle;
        } else  if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"))) {
            const QString targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
            return channel->targetID() == targetID;
        } else {
            // Request is not valid
            return false;
        }
    }

    // Unknown request
    return false;
}

/**
 * \fn void BaseConnection::disconnected()
 *
 * Emitted when this connection has been disconnected.
 */

/**
 * \class AbstractConnectionInterface
 * \ingroup serviceconn
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

void AbstractConnectionInterface::setBaseConnection(BaseConnection *connection)
{
    Q_UNUSED(connection)
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
 * \ingroup serviceconn
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
    BaseChannelPtr channel = mPriv->connection->ensureChannel(request, yours, /* suppressHandler */ true, error);

    if (error->isValid())
        return;

    objectPath = QDBusObjectPath(channel->objectPath());
    details = channel->details().properties;
}

void BaseConnectionRequestsInterface::createChannel(const QVariantMap &request,
        QDBusObjectPath &objectPath,
        QVariantMap &details, DBusError *error)
{
    if (!request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"))) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Missing parameters"));
        return;
    }

    BaseChannelPtr channel = mPriv->connection->createChannel(request, /* suppressHandler */ true, error);

    if (error->isValid())
        return;

    objectPath = QDBusObjectPath(channel->objectPath());
    details = channel->details().properties;
}

// Conn.I.Contacts
// The BaseConnectionContactsInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseConnectionContactsInterface::Private {
    Private(BaseConnectionContactsInterface *parent)
        : connection(0),
          adaptee(new BaseConnectionContactsInterface::Adaptee(parent))
    {
    }

    QStringList contactAttributeInterfaces;
    GetContactAttributesCallback getContactAttributesCB;
    BaseConnection *connection;
    BaseConnectionContactsInterface::Adaptee *adaptee;
};

BaseConnectionContactsInterface::Adaptee::Adaptee(BaseConnectionContactsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionContactsInterface::Adaptee::~Adaptee()
{
}

QStringList BaseConnectionContactsInterface::Adaptee::contactAttributeInterfaces() const
{
    return mInterface->contactAttributeInterfaces();
}

void BaseConnectionContactsInterface::Adaptee::getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, bool /* hold */,
        const Tp::Service::ConnectionInterfaceContactsAdaptor::GetContactAttributesContextPtr &context)
{
    DBusError error;
    Tp::ContactAttributesMap attributes = mInterface->getContactAttributes(handles, interfaces, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(attributes);
}

void BaseConnectionContactsInterface::Adaptee::getContactByID(const QString &identifier, const QStringList &interfaces,
        const Tp::Service::ConnectionInterfaceContactsAdaptor::GetContactByIDContextPtr &context)
{
    debug() << "BaseConnectionContactsInterface::Adaptee::getContactByID";
    DBusError error;
    uint handle;
    QVariantMap attributes;

    mInterface->getContactByID(identifier, interfaces, handle, attributes, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(handle, attributes);
}

/**
 * \class BaseConnectionContactsInterface
 * \ingroup serviceconn
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

void BaseConnectionContactsInterface::setBaseConnection(BaseConnection *connection)
{
    mPriv->connection = connection;
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

QStringList BaseConnectionContactsInterface::contactAttributeInterfaces() const
{
    return mPriv->contactAttributeInterfaces;
}

void BaseConnectionContactsInterface::setContactAttributeInterfaces(const QStringList &contactAttributeInterfaces)
{
    mPriv->contactAttributeInterfaces = contactAttributeInterfaces;
}

void BaseConnectionContactsInterface::createAdaptor()
{
    (void) new Tp::Service::ConnectionInterfaceContactsAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionContactsInterface::setGetContactAttributesCallback(const GetContactAttributesCallback &cb)
{
    mPriv->getContactAttributesCB = cb;
}

Tp::ContactAttributesMap BaseConnectionContactsInterface::getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, DBusError *error)
{
    if (!mPriv->getContactAttributesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ContactAttributesMap();
    }
    return mPriv->getContactAttributesCB(handles, interfaces, error);
}

void BaseConnectionContactsInterface::getContactByID(const QString &identifier, const QStringList &interfaces, uint &handle, QVariantMap &attributes, DBusError *error)
{
    const Tp::UIntList handles = mPriv->connection->requestHandles(Tp::HandleTypeContact, QStringList() << identifier, error);
    if (error->isValid() || handles.isEmpty()) {
        // The check for empty handles is paranoid, because the error must be set in such case.
        error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Could not process ID"));
        return;
    }

    const Tp::ContactAttributesMap result = getContactAttributes(handles, interfaces, error);

    if (error->isValid()) {
        return;
    }

    handle = handles.first();
    attributes = result.value(handle);
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
        : maximumStatusMessageLength(0),
          adaptee(new BaseConnectionSimplePresenceInterface::Adaptee(parent)) {
    }
    SetPresenceCallback setPresenceCB;
    SimpleStatusSpecMap statuses;
    uint maximumStatusMessageLength;
    /* The current presences */
    SimpleContactPresences presences;
    BaseConnectionSimplePresenceInterface::Adaptee *adaptee;
};

/**
 * \class BaseConnectionSimplePresenceInterface
 * \ingroup serviceconn
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
    Tp::SimpleContactPresences newPresences;

    foreach(uint handle, presences.keys()) {
        if (mPriv->presences.contains(handle) && mPriv->presences.value(handle) == presences.value(handle)) {
            continue;
        }
        mPriv->presences[handle] = presences[handle];
        newPresences[handle] = presences[handle];
    }

    if (!newPresences.isEmpty()) {
        QMetaObject::invokeMethod(mPriv->adaptee, "presencesChanged", Q_ARG(Tp::SimpleContactPresences, newPresences)); //Can simply use emit in Qt5
    }
}

void BaseConnectionSimplePresenceInterface::setSetPresenceCallback(const SetPresenceCallback &cb)
{
    mPriv->setPresenceCB = cb;
}

SimpleContactPresences BaseConnectionSimplePresenceInterface::getPresences(const UIntList &contacts)
{
    Tp::SimpleContactPresences presences;
    foreach(uint handle, contacts) {
        static const Tp::SimplePresence unknownPresence = { /* type */ ConnectionPresenceTypeUnknown, /* status */ QLatin1String("unknown") };
        presences[handle] = mPriv->presences.value(handle, unknownPresence);
    }

    return presences;
}

Tp::SimpleStatusSpecMap BaseConnectionSimplePresenceInterface::statuses() const
{
    return mPriv->statuses;
}

void BaseConnectionSimplePresenceInterface::setStatuses(const SimpleStatusSpecMap &statuses)
{
    mPriv->statuses = statuses;
}

uint BaseConnectionSimplePresenceInterface::maximumStatusMessageLength() const
{
    return mPriv->maximumStatusMessageLength;
}

void BaseConnectionSimplePresenceInterface::setMaximumStatusMessageLength(uint maximumStatusMessageLength)
{
    mPriv->maximumStatusMessageLength = maximumStatusMessageLength;
}

Tp::SimpleStatusSpecMap BaseConnectionSimplePresenceInterface::Adaptee::statuses() const
{
    return mInterface->mPriv->statuses;
}

int BaseConnectionSimplePresenceInterface::Adaptee::maximumStatusMessageLength() const
{
    return mInterface->mPriv->maximumStatusMessageLength;
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
    if ((uint)statusMessage.length() > mInterface->mPriv->maximumStatusMessageLength) {
        debug() << "BaseConnectionSimplePresenceInterface::Adaptee::setPresence: "
                << "truncating status to " << mInterface->mPriv->maximumStatusMessageLength;
        statusMessage = statusMessage.left(mInterface->mPriv->maximumStatusMessageLength);
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
    context->setFinished(mInterface->getPresences(contacts));
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
    debug() << "BaseConnectionContactListInterface::Adaptee::getContactListAttributes";
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
    debug() << "BaseConnectionContactListInterface::Adaptee::requestSubscription";
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
    debug() << "BaseConnectionContactListInterface::Adaptee::authorizePublication";
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
    debug() << "BaseConnectionContactListInterface::Adaptee::removeContacts";
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
    debug() << "BaseConnectionContactListInterface::Adaptee::unsubscribe";
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
    debug() << "BaseConnectionContactListInterface::Adaptee::unpublish";
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
    debug() << "BaseConnectionContactListInterface::Adaptee::download";
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
 * \ingroup serviceconn
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
    debug() << "BaseConnectionContactInfoInterface::Adaptee::getContactInfo";
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
    debug() << "BaseConnectionContactInfoInterface::Adaptee::refreshContactInfo";
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
    debug() << "BaseConnectionContactInfoInterface::Adaptee::requestContactInfo";
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
    debug() << "BaseConnectionContactInfoInterface::Adaptee::setContactInfo";
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
 * \ingroup serviceconn
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
 * \ingroup serviceconn
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
    debug() << "BaseConnectionAliasingInterface::Adaptee::getAliasFlags";
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
    debug() << "BaseConnectionAliasingInterface::Adaptee::requestAliases";
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
    debug() << "BaseConnectionAliasingInterface::Adaptee::getAliases";
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
    debug() << "BaseConnectionAliasingInterface::Adaptee::setAliases";
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
 * \ingroup serviceconn
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

// Conn.I.Avatars
struct TP_QT_NO_EXPORT BaseConnectionAvatarsInterface::Private {
    Private(BaseConnectionAvatarsInterface *parent)
        : adaptee(new BaseConnectionAvatarsInterface::Adaptee(parent))
    {
    }

    AvatarSpec avatarDetails;
    GetKnownAvatarTokensCallback getKnownAvatarTokensCB;
    RequestAvatarsCallback requestAvatarsCB;
    SetAvatarCallback setAvatarCB;
    ClearAvatarCallback clearAvatarCB;
    BaseConnectionAvatarsInterface::Adaptee *adaptee;

    friend class BaseConnectionAvatarsInterface::Adaptee;
};

BaseConnectionAvatarsInterface::Adaptee::Adaptee(BaseConnectionAvatarsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionAvatarsInterface::Adaptee::~Adaptee()
{
}

QStringList BaseConnectionAvatarsInterface::Adaptee::supportedAvatarMimeTypes() const
{
    return mInterface->mPriv->avatarDetails.supportedMimeTypes();
}

uint BaseConnectionAvatarsInterface::Adaptee::minimumAvatarHeight() const
{
    return mInterface->mPriv->avatarDetails.minimumHeight();
}

uint BaseConnectionAvatarsInterface::Adaptee::minimumAvatarWidth() const
{
    return mInterface->mPriv->avatarDetails.minimumWidth();
}

uint BaseConnectionAvatarsInterface::Adaptee::recommendedAvatarHeight() const
{
    return mInterface->mPriv->avatarDetails.recommendedHeight();
}

uint BaseConnectionAvatarsInterface::Adaptee::recommendedAvatarWidth() const
{
    return mInterface->mPriv->avatarDetails.recommendedWidth();
}

uint BaseConnectionAvatarsInterface::Adaptee::maximumAvatarHeight() const
{
    return mInterface->mPriv->avatarDetails.maximumHeight();
}

uint BaseConnectionAvatarsInterface::Adaptee::maximumAvatarWidth() const
{
    return mInterface->mPriv->avatarDetails.maximumWidth();
}

uint BaseConnectionAvatarsInterface::Adaptee::maximumAvatarBytes() const
{
    return mInterface->mPriv->avatarDetails.maximumBytes();
}

void BaseConnectionAvatarsInterface::Adaptee::getKnownAvatarTokens(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceAvatarsAdaptor::GetKnownAvatarTokensContextPtr &context)
{
    debug() << "BaseConnectionAvatarsInterface::Adaptee::getKnownAvatarTokens";
    DBusError error;
    Tp::AvatarTokenMap tokens = mInterface->getKnownAvatarTokens(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(tokens);
}

void BaseConnectionAvatarsInterface::Adaptee::requestAvatars(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceAvatarsAdaptor::RequestAvatarsContextPtr &context)
{
    debug() << "BaseConnectionAvatarsInterface::Adaptee::requestAvatars";
    DBusError error;
    mInterface->requestAvatars(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionAvatarsInterface::Adaptee::setAvatar(const QByteArray &avatar, const QString &mimeType,
        const Tp::Service::ConnectionInterfaceAvatarsAdaptor::SetAvatarContextPtr &context)
{
    debug() << "BaseConnectionAvatarsInterface::Adaptee::setAvatar";
    DBusError error;
    QString token = mInterface->setAvatar(avatar, mimeType, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(token);
}

void BaseConnectionAvatarsInterface::Adaptee::clearAvatar(
        const Tp::Service::ConnectionInterfaceAvatarsAdaptor::ClearAvatarContextPtr &context)
{
    debug() << "BaseConnectionAvatarsInterface::Adaptee::clearAvatar";
    DBusError error;
    mInterface->clearAvatar(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseConnectionAvatarsInterface
 * \ingroup serviceconn
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.Avatars
 */

/**
 * Class constructor.
 */
BaseConnectionAvatarsInterface::BaseConnectionAvatarsInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionAvatarsInterface::~BaseConnectionAvatarsInterface()
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
QVariantMap BaseConnectionAvatarsInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

AvatarSpec BaseConnectionAvatarsInterface::avatarDetails() const
{
    return mPriv->avatarDetails;
}

void BaseConnectionAvatarsInterface::setAvatarDetails(const AvatarSpec &spec)
{
    mPriv->avatarDetails = spec;
}

void BaseConnectionAvatarsInterface::createAdaptor()
{
    (void) new Service::ConnectionInterfaceAvatarsAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionAvatarsInterface::setGetKnownAvatarTokensCallback(const BaseConnectionAvatarsInterface::GetKnownAvatarTokensCallback &cb)
{
    mPriv->getKnownAvatarTokensCB = cb;
}

Tp::AvatarTokenMap BaseConnectionAvatarsInterface::getKnownAvatarTokens(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->getKnownAvatarTokensCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::AvatarTokenMap();
    }
    return mPriv->getKnownAvatarTokensCB(contacts, error);
}

void BaseConnectionAvatarsInterface::setRequestAvatarsCallback(const BaseConnectionAvatarsInterface::RequestAvatarsCallback &cb)
{
    mPriv->requestAvatarsCB = cb;
}

void BaseConnectionAvatarsInterface::requestAvatars(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->requestAvatarsCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->requestAvatarsCB(contacts, error);
}

void BaseConnectionAvatarsInterface::setSetAvatarCallback(const BaseConnectionAvatarsInterface::SetAvatarCallback &cb)
{
    mPriv->setAvatarCB = cb;
}

QString BaseConnectionAvatarsInterface::setAvatar(const QByteArray &avatar, const QString &mimeType, DBusError *error)
{
    if (!mPriv->setAvatarCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->setAvatarCB(avatar, mimeType, error);
}

void BaseConnectionAvatarsInterface::setClearAvatarCallback(const BaseConnectionAvatarsInterface::ClearAvatarCallback &cb)
{
    mPriv->clearAvatarCB = cb;
}

void BaseConnectionAvatarsInterface::clearAvatar(DBusError *error)
{
    if (!mPriv->clearAvatarCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->clearAvatarCB(error);
}

void BaseConnectionAvatarsInterface::avatarUpdated(uint contact, const QString &newAvatarToken)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "avatarUpdated", Q_ARG(uint, contact), Q_ARG(QString, newAvatarToken)); //Can simply use emit in Qt5
}

void BaseConnectionAvatarsInterface::avatarRetrieved(uint contact, const QString &token, const QByteArray &avatar, const QString &type)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "avatarRetrieved", Q_ARG(uint, contact), Q_ARG(QString, token), Q_ARG(QByteArray, avatar), Q_ARG(QString, type)); //Can simply use emit in Qt5
}

// Conn.I.ClientTypes
// The BaseConnectionClientTypesInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseConnectionClientTypesInterface::Private {
    Private(BaseConnectionClientTypesInterface *parent)
        : adaptee(new BaseConnectionClientTypesInterface::Adaptee(parent))
    {
    }

    GetClientTypesCallback getClientTypesCB;
    RequestClientTypesCallback requestClientTypesCB;
    BaseConnectionClientTypesInterface::Adaptee *adaptee;
};

BaseConnectionClientTypesInterface::Adaptee::Adaptee(BaseConnectionClientTypesInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionClientTypesInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionClientTypesInterface::Adaptee::getClientTypes(const Tp::UIntList &contacts,
        const Tp::Service::ConnectionInterfaceClientTypesAdaptor::GetClientTypesContextPtr &context)
{
    debug() << "BaseConnectionClientTypesInterface::Adaptee::getClientTypes";
    DBusError error;
    Tp::ContactClientTypes clientTypes = mInterface->getClientTypes(contacts, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(clientTypes);
}

void BaseConnectionClientTypesInterface::Adaptee::requestClientTypes(uint contact,
        const Tp::Service::ConnectionInterfaceClientTypesAdaptor::RequestClientTypesContextPtr &context)
{
    debug() << "BaseConnectionClientTypesInterface::Adaptee::requestClientTypes";
    DBusError error;
    QStringList clientTypes = mInterface->requestClientTypes(contact, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(clientTypes);
}

/**
 * \class BaseConnectionClientTypesInterface
 * \ingroup serviceconn
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.ClientTypes
 */

/**
 * Class constructor.
 */
BaseConnectionClientTypesInterface::BaseConnectionClientTypesInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CLIENT_TYPES),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionClientTypesInterface::~BaseConnectionClientTypesInterface()
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
QVariantMap BaseConnectionClientTypesInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionClientTypesInterface::createAdaptor()
{
    (void) new Tp::Service::ConnectionInterfaceClientTypesAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionClientTypesInterface::setGetClientTypesCallback(const GetClientTypesCallback &cb)
{
    mPriv->getClientTypesCB = cb;
}

Tp::ContactClientTypes BaseConnectionClientTypesInterface::getClientTypes(const Tp::UIntList &contacts, DBusError *error)
{
    if (!mPriv->getClientTypesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ContactClientTypes();
    }
    return mPriv->getClientTypesCB(contacts, error);
}

void BaseConnectionClientTypesInterface::setRequestClientTypesCallback(const RequestClientTypesCallback &cb)
{
    mPriv->requestClientTypesCB = cb;
}

QStringList BaseConnectionClientTypesInterface::requestClientTypes(uint contact, DBusError *error)
{
    if (!mPriv->requestClientTypesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QStringList();
    }
    return mPriv->requestClientTypesCB(contact, error);
}

void BaseConnectionClientTypesInterface::clientTypesUpdated(uint contact, const QStringList &clientTypes)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "clientTypesUpdated", Q_ARG(uint, contact), Q_ARG(QStringList, clientTypes)); //Can simply use emit in Qt5
}

// Conn.I.ContactCapabilities
// The BaseConnectionContactCapabilitiesInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseConnectionContactCapabilitiesInterface::Private {
    Private(BaseConnectionContactCapabilitiesInterface *parent)
        : adaptee(new BaseConnectionContactCapabilitiesInterface::Adaptee(parent))
    {
    }

    UpdateCapabilitiesCallback updateCapabilitiesCB;
    GetContactCapabilitiesCallback getContactCapabilitiesCB;
    BaseConnectionContactCapabilitiesInterface::Adaptee *adaptee;
};

BaseConnectionContactCapabilitiesInterface::Adaptee::Adaptee(BaseConnectionContactCapabilitiesInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseConnectionContactCapabilitiesInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionContactCapabilitiesInterface::Adaptee::updateCapabilities(const Tp::HandlerCapabilitiesList &handlerCapabilities,
        const Tp::Service::ConnectionInterfaceContactCapabilitiesAdaptor::UpdateCapabilitiesContextPtr &context)
{
    debug() << "BaseConnectionContactCapabilitiesInterface::Adaptee::updateCapabilities";
    DBusError error;
    mInterface->updateCapabilities(handlerCapabilities, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionContactCapabilitiesInterface::Adaptee::getContactCapabilities(const Tp::UIntList &handles,
        const Tp::Service::ConnectionInterfaceContactCapabilitiesAdaptor::GetContactCapabilitiesContextPtr &context)
{
    debug() << "BaseConnectionContactCapabilitiesInterface::Adaptee::getContactCapabilities";
    DBusError error;
    Tp::ContactCapabilitiesMap contactCapabilities = mInterface->getContactCapabilities(handles, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(contactCapabilities);
}

/**
 * \class BaseConnectionContactCapabilitiesInterface
 * \ingroup serviceconn
 * \headerfile TelepathyQt/base-connection.h <TelepathyQt/BaseConnection>
 *
 * \brief Base class for implementations of Connection.Interface.ContactCapabilities
 */

/**
 * Class constructor.
 */
BaseConnectionContactCapabilitiesInterface::BaseConnectionContactCapabilitiesInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseConnectionContactCapabilitiesInterface::~BaseConnectionContactCapabilitiesInterface()
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
QVariantMap BaseConnectionContactCapabilitiesInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionContactCapabilitiesInterface::createAdaptor()
{
    (void) new Tp::Service::ConnectionInterfaceContactCapabilitiesAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseConnectionContactCapabilitiesInterface::setUpdateCapabilitiesCallback(const UpdateCapabilitiesCallback &cb)
{
    mPriv->updateCapabilitiesCB = cb;
}

void BaseConnectionContactCapabilitiesInterface::updateCapabilities(const Tp::HandlerCapabilitiesList &handlerCapabilities, DBusError *error)
{
    if (!mPriv->updateCapabilitiesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->updateCapabilitiesCB(handlerCapabilities, error);
}

void BaseConnectionContactCapabilitiesInterface::setGetContactCapabilitiesCallback(const GetContactCapabilitiesCallback &cb)
{
    mPriv->getContactCapabilitiesCB = cb;
}

Tp::ContactCapabilitiesMap BaseConnectionContactCapabilitiesInterface::getContactCapabilities(const Tp::UIntList &handles, DBusError *error)
{
    if (!mPriv->getContactCapabilitiesCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return Tp::ContactCapabilitiesMap();
    }
    return mPriv->getContactCapabilitiesCB(handles, error);
}

void BaseConnectionContactCapabilitiesInterface::contactCapabilitiesChanged(const Tp::ContactCapabilitiesMap &caps)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "contactCapabilitiesChanged", Q_ARG(Tp::ContactCapabilitiesMap, caps)); //Can simply use emit in Qt5
}

}
