/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
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

#include <TelepathyQt/BaseChannel>
#include "TelepathyQt/base-channel-internal.h"

#include "TelepathyQt/_gen/base-channel.moc.hpp"
#include "TelepathyQt/_gen/base-channel-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>
#include <TelepathyQt/AbstractProtocolInterface>
#include <QString>
#include <QVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseChannel::Private {
    Private(BaseChannel *parent, const QDBusConnection &dbusConnection, BaseConnection* connection,
            const QString &channelType, uint targetHandle, uint targetHandleType)
        : parent(parent),
          connection(connection),
          channelType(channelType),
          targetHandle(targetHandle),
          targetHandleType(targetHandleType),
          adaptee(new BaseChannel::Adaptee(dbusConnection, parent)) {
    }

    BaseChannel *parent;
    BaseConnection* connection;
    QString channelType;
    QHash<QString, AbstractChannelInterfacePtr> interfaces;
    uint targetHandle;
    QString targetID;
    uint targetHandleType;
    bool requested;
    uint initiatorHandle;
    QString initiatorID;
    BaseChannel::Adaptee *adaptee;
};


BaseChannel::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
                              BaseChannel *channel)
    : QObject(channel),
      mChannel(channel)
{
    debug() << "Creating service::channelAdaptor for " << channel->dbusObject();
    mAdaptor = new Service::ChannelAdaptor(dbusConnection, this, channel->dbusObject());
}

BaseChannel::Adaptee::~Adaptee()
{
}

QStringList BaseChannel::Adaptee::interfaces() const
{
    QStringList ret;
    foreach(const AbstractChannelInterfacePtr & iface, mChannel->interfaces()) {
        if (iface->interfaceName().contains(QLatin1String(".Type.")))
            continue; //Do not include "Type"
        ret << iface->interfaceName();
    }
    return ret;
}

void BaseChannel::Adaptee::close(const Tp::Service::ChannelAdaptor::CloseContextPtr &context)
{
    //emit after return
    QMetaObject::invokeMethod(this, "closed",
                              Qt::QueuedConnection);
    //emit after return
    QMetaObject::invokeMethod(mChannel, "closed",
                              Qt::QueuedConnection);

    context->setFinished();
}

/**
 * \class BaseChannel
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for channel implementations.
 *
 */

BaseChannel::BaseChannel(const QDBusConnection &dbusConnection,
                         BaseConnection* connection,
                         const QString &channelType, uint targetHandle,
                         uint targetHandleType)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, connection,
                        channelType, targetHandle, targetHandleType))
{
}

/**
 * Class destructor.
 */
BaseChannel::~BaseChannel()
{
    delete mPriv;
}

/**
 * Return a unique name for this channel.
 *
 * \return A unique name for this channel.
 */
QString BaseChannel::uniqueName() const
{
    return QString(QLatin1String("_%1")).arg((quintptr) this, 0, 16);
}

bool BaseChannel::registerObject(DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    QString name = uniqueName();
    QString busName = mPriv->connection->busName();
    //QString busName = QString(QLatin1String("%1.%2"))
    //        .arg(mPriv->connection->busName(),name);
    QString objectPath = QString(QLatin1String("%1/%2"))
                         .arg(mPriv->connection->objectPath(), name);
    debug() << "Registering channel: busName: " << busName << " objectName: " << objectPath;
    DBusError _error;

    debug() << "Channel: registering interfaces  at " << dbusObject();
    foreach(const AbstractChannelInterfacePtr & iface, mPriv->interfaces) {
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
bool BaseChannel::registerObject(const QString &busName,
                                 const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

QString BaseChannel::channelType() const
{
    return mPriv->channelType;
}
QList<AbstractChannelInterfacePtr> BaseChannel::interfaces() const
{
    return mPriv->interfaces.values();
}
uint BaseChannel::targetHandle() const
{
    return mPriv->targetHandle;
}
QString BaseChannel::targetID() const
{
    return mPriv->targetID;
}
uint BaseChannel::targetHandleType() const
{
    return mPriv->targetHandleType;
}
bool BaseChannel::requested() const
{
    return mPriv->requested;
}
uint BaseChannel::initiatorHandle() const
{
    return mPriv->initiatorHandle;
}
QString BaseChannel::initiatorID() const
{
    return mPriv->initiatorID;
}

void BaseChannel::setInitiatorHandle(uint initiatorHandle)
{
    mPriv->initiatorHandle = initiatorHandle;
}

void BaseChannel::setInitiatorID(const QString &initiatorID)
{
    mPriv->initiatorID = initiatorID;
}

void BaseChannel::setTargetID(const QString &targetID)
{
    mPriv->targetID = targetID;
}

void BaseChannel::setRequested(bool requested)
{
    mPriv->requested = requested;
}

/**
 * Return the immutable properties of this channel object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this channel object.
 */
QVariantMap BaseChannel::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
               QVariant::fromValue(mPriv->adaptee->channelType()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
               QVariant::fromValue(mPriv->adaptee->targetHandle()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".Interfaces"),
               QVariant::fromValue(mPriv->adaptee->interfaces()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
               QVariant::fromValue(mPriv->adaptee->targetID()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
               QVariant::fromValue(mPriv->adaptee->targetHandleType()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested"),
               QVariant::fromValue(mPriv->adaptee->requested()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle"),
               QVariant::fromValue(mPriv->adaptee->initiatorHandle()));
    map.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorID"),
               QVariant::fromValue(mPriv->adaptee->initiatorID()));
    return map;
}

Tp::ChannelDetails BaseChannel::details() const
{
    Tp::ChannelDetails details;
    details.channel = QDBusObjectPath(objectPath());
    details.properties.unite(immutableProperties());

    foreach(const AbstractChannelInterfacePtr & iface, mPriv->interfaces) {
        details.properties.unite(iface->immutableProperties());
    }

    return details;
}

/**
 * Return a pointer to the interface with the given name.
 *
 * \param interfaceName The D-Bus name of the interface,
 * ex. TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING.
 * \return A pointer to the AbstractProtocolInterface object that implements
 * the D-Bus interface with the given name, or a null pointer if such an interface
 * has not been plugged into this object.
 * \sa plugInterface(), interfaces()
 */
AbstractChannelInterfacePtr BaseChannel::interface(const QString &interfaceName) const
{
    return mPriv->interfaces.value(interfaceName);
}

/**
 * Plug a new interface into this Protocol D-Bus object.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param interface An AbstractProtocolInterface instance that implements
 * the interface that is to be plugged.
 * \return \c true on success or \c false otherwise
 * \sa interfaces(), interface()
 */
bool BaseChannel::plugInterface(const AbstractChannelInterfacePtr &interface)
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
 * \class AbstractChannelInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for all the Channel object interface implementations.
 */

AbstractChannelInterface::AbstractChannelInterface(const QString &interfaceName)
    : AbstractDBusServiceInterface(interfaceName)
{
}

AbstractChannelInterface::~AbstractChannelInterface()
{
}

}
