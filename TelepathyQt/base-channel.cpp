/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
 * @copyright Copyright 2013 Canonical Ltd.
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
#include <TelepathyQt/BaseCall>
#include "TelepathyQt/base-channel-internal.h"

#include "TelepathyQt/_gen/base-channel.moc.hpp"
#include "TelepathyQt/_gen/base-channel-internal.moc.hpp"
#include "TelepathyQt/future-internal.h"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>
#include <TelepathyQt/AbstractProtocolInterface>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseChannel::Private {
    Private(BaseChannel *parent, const QDBusConnection &dbusConnection, BaseConnection *connection,
            const QString &channelType, uint targetHandleType, uint targetHandle)
        : parent(parent),
          connection(connection),
          channelType(channelType),
          targetHandleType(targetHandleType),
          targetHandle(targetHandle),
          requested(true),
          initiatorHandle(0),
          adaptee(new BaseChannel::Adaptee(dbusConnection, parent)) {
        static uint s_channelIncrementalId = 0;

        QString baseName;
        static const QString s_channelTypePrefix = TP_QT_IFACE_CHANNEL + QLatin1String(".Type.");

        if ((channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) && (targetHandleType == Tp::HandleTypeRoom)) {
            baseName = QLatin1String("Muc");
        } else if (channelType.startsWith(s_channelTypePrefix)) {
            baseName = channelType.mid(s_channelTypePrefix.length());
        }

        uniqueName = baseName + QLatin1String("Channel") + QString::number(s_channelIncrementalId);
        ++s_channelIncrementalId;
    }

    BaseChannel *parent;
    BaseConnection* connection;
    QString channelType;
    QHash<QString, AbstractChannelInterfacePtr> interfaces;
    QString uniqueName;
    uint targetHandleType;
    uint targetHandle;
    QString targetID;
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
    mChannel->close();
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
                         BaseConnection *connection,
                         const QString &channelType, uint targetHandleType,
                         uint targetHandle)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, connection,
                        channelType, targetHandleType, targetHandle))
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
    return mPriv->uniqueName;
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

void BaseChannel::close()
{
    // Method is used in destructor, so (to be sure that adaptee is exists) we should use DirectConnection
    QMetaObject::invokeMethod(mPriv->adaptee, "closed", Qt::DirectConnection);
    emit closed();
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

// Chan.T.Text
BaseChannelTextType::Adaptee::Adaptee(BaseChannelTextType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelTextType::Adaptee::~Adaptee()
{
}

void BaseChannelTextType::Adaptee::acknowledgePendingMessages(const Tp::UIntList &IDs,
        const Tp::Service::ChannelTypeTextAdaptor::AcknowledgePendingMessagesContextPtr &context)
{
    qDebug() << "BaseConnectionContactsInterface::acknowledgePendingMessages " << IDs;
    DBusError error;
    mInterface->acknowledgePendingMessages(IDs, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

struct TP_QT_NO_EXPORT BaseChannelTextType::Private {
    Private(BaseChannelTextType *parent, BaseChannel* channel)
        : channel(channel),
          pendingMessagesId(0),
          adaptee(new BaseChannelTextType::Adaptee(parent)) {
    }

    BaseChannel* channel;
    /* maps pending-message-id to message part list */
    QMap<uint, Tp::MessagePartList> pendingMessages;
    /* increasing unique id of pending messages */
    uint pendingMessagesId;
    MessageAcknowledgedCallback messageAcknowledgedCB;
    BaseChannelTextType::Adaptee *adaptee;
};

/**
 * \class BaseChannelTextType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Type.Text
 *
 */

/**
 * Class constructor.
 */
BaseChannelTextType::BaseChannelTextType(BaseChannel* channel)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_TEXT),
      mPriv(new Private(this, channel))
{
}

/**
 * Class destructor.
 */
BaseChannelTextType::~BaseChannelTextType()
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
QVariantMap BaseChannelTextType::immutableProperties() const
{
    return QVariantMap();
}

void BaseChannelTextType::createAdaptor()
{
    (void) new Service::ChannelTypeTextAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelTextType::addReceivedMessage(const Tp::MessagePartList &msg)
{
    MessagePartList message = msg;
    if (msg.empty()) {
        warning() << "empty message: not sent";
        return;
    }
    MessagePart &header = message.front();

    if (header.count(QLatin1String("pending-message-id")))
        warning() << "pending-message-id will be overwritten";

    /* Add pending-message-id to header */
    uint pendingMessageId = mPriv->pendingMessagesId++;
    header[QLatin1String("pending-message-id")] = QDBusVariant(pendingMessageId);
    mPriv->pendingMessages[pendingMessageId] = message;

    uint timestamp = 0;
    if (header.count(QLatin1String("message-received")))
        timestamp = header[QLatin1String("message-received")].variant().toUInt();

    uint handle = 0;
    if (header.count(QLatin1String("message-sender")))
        handle = header[QLatin1String("message-sender")].variant().toUInt();

    uint type = ChannelTextMessageTypeNormal;
    if (header.count(QLatin1String("message-type")))
        type = header[QLatin1String("message-type")].variant().toUInt();

    //FIXME: flags are not parsed
    uint flags = 0;

    QString content;
    for (MessagePartList::Iterator i = message.begin() + 1; i != message.end(); ++i)
        if (i->count(QLatin1String("content-type"))
                && i->value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain")
                && i->count(QLatin1String("content"))) {
            content = i->value(QLatin1String("content")).variant().toString();
            break;
        }
    if (content.length() > 0)
        QMetaObject::invokeMethod(mPriv->adaptee, "received",
                                  Qt::QueuedConnection,
                                  Q_ARG(uint, pendingMessageId),
                                  Q_ARG(uint, timestamp),
                                  Q_ARG(uint, handle),
                                  Q_ARG(uint, type),
                                  Q_ARG(uint, flags),
                                  Q_ARG(QString, content));

    /* Signal on ChannelMessagesInterface */
    BaseChannelMessagesInterfacePtr messagesIface = BaseChannelMessagesInterfacePtr::dynamicCast(
                mPriv->channel->interface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES));
    if (messagesIface)
        QMetaObject::invokeMethod(messagesIface.data(), "messageReceived",
                                  Qt::QueuedConnection,
                                  Q_ARG(Tp::MessagePartList, message));
}

Tp::MessagePartListList BaseChannelTextType::pendingMessages()
{
    return mPriv->pendingMessages.values();
}

/*
 * Will be called with the value of the message-token field after a received message has been acknowledged,
 * if the message-token field existed in the header.
 */
void BaseChannelTextType::setMessageAcknowledgedCallback(const MessageAcknowledgedCallback &cb)
{
    mPriv->messageAcknowledgedCB = cb;
}

void BaseChannelTextType::acknowledgePendingMessages(const Tp::UIntList &IDs, DBusError* error)
{
    foreach(uint id, IDs) {
        QMap<uint, Tp::MessagePartList>::Iterator i = mPriv->pendingMessages.find(id);
        if (i == mPriv->pendingMessages.end()) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("id not found"));
            return;
        }

        MessagePart &header = i->front();
        if (header.count(QLatin1String("message-token")) && mPriv->messageAcknowledgedCB.isValid())
            mPriv->messageAcknowledgedCB(header[QLatin1String("message-token")].variant().toString());

        mPriv->pendingMessages.erase(i);
    }

    /* Signal on ChannelMessagesInterface */
    BaseChannelMessagesInterfacePtr messagesIface = BaseChannelMessagesInterfacePtr::dynamicCast(
                mPriv->channel->interface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES));
    if (messagesIface) //emit after return
        QMetaObject::invokeMethod(messagesIface.data(), "pendingMessagesRemoved",
                                  Qt::QueuedConnection,
                                  Q_ARG(Tp::UIntList, IDs));
}



void BaseChannelTextType::sent(uint timestamp, uint type, QString text)
{
    QMetaObject::invokeMethod(mPriv->adaptee,"sent",Q_ARG(uint, timestamp), Q_ARG(uint, type), Q_ARG(QString, text)); //Can simply use emit in Qt5
}


// Chan.I.Messages
BaseChannelMessagesInterface::Adaptee::Adaptee(BaseChannelMessagesInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelMessagesInterface::Adaptee::~Adaptee()
{
}

void BaseChannelMessagesInterface::Adaptee::sendMessage(const Tp::MessagePartList &message, uint flags,
        const Tp::Service::ChannelInterfaceMessagesAdaptor::SendMessageContextPtr &context)
{
    DBusError error;
    QString token = mInterface->sendMessage(message, flags, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(token);
}

struct TP_QT_NO_EXPORT BaseChannelMessagesInterface::Private {
    Private(BaseChannelMessagesInterface *parent,
            BaseChannelTextType* textTypeInterface,
            QStringList supportedContentTypes,
            Tp::UIntList messageTypes,
            uint messagePartSupportFlags,
            uint deliveryReportingSupport)
        : textTypeInterface(textTypeInterface),
          supportedContentTypes(supportedContentTypes),
          messageTypes(messageTypes),
          messagePartSupportFlags(messagePartSupportFlags),
          deliveryReportingSupport(deliveryReportingSupport),
          adaptee(new BaseChannelMessagesInterface::Adaptee(parent)) {
    }

    BaseChannelTextType* textTypeInterface;
    QStringList supportedContentTypes;
    Tp::UIntList messageTypes;
    uint messagePartSupportFlags;
    uint deliveryReportingSupport;
    SendMessageCallback sendMessageCB;
    BaseChannelMessagesInterface::Adaptee *adaptee;
};

/**
 * \class BaseChannelMessagesInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Messages
 *
 */

/**
 * Class constructor.
 */
BaseChannelMessagesInterface::BaseChannelMessagesInterface(BaseChannelTextType *textType,
        QStringList supportedContentTypes,
        UIntList messageTypes,
        uint messagePartSupportFlags,
        uint deliveryReportingSupport)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES),
      mPriv(new Private(this, textType, supportedContentTypes, messageTypes,
                        messagePartSupportFlags, deliveryReportingSupport))
{
}

/**
 * Class destructor.
 */
BaseChannelMessagesInterface::~BaseChannelMessagesInterface()
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
QVariantMap BaseChannelMessagesInterface::immutableProperties() const
{
    QVariantMap map;

    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".SupportedContentTypes"),
               QVariant::fromValue(mPriv->adaptee->supportedContentTypes()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".MessageTypes"),
               QVariant::fromValue(mPriv->adaptee->messageTypes()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".MessagePartSupportFlags"),
               QVariant::fromValue(mPriv->adaptee->messagePartSupportFlags()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES + QLatin1String(".DeliveryReportingSupport"),
               QVariant::fromValue(mPriv->adaptee->deliveryReportingSupport()));
    return map;
}

void BaseChannelMessagesInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceMessagesAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

QStringList BaseChannelMessagesInterface::supportedContentTypes()
{
    return mPriv->supportedContentTypes;
}

Tp::UIntList BaseChannelMessagesInterface::messageTypes()
{
    return mPriv->messageTypes;
}

uint BaseChannelMessagesInterface::messagePartSupportFlags()
{
    return mPriv->messagePartSupportFlags;
}

uint BaseChannelMessagesInterface::deliveryReportingSupport()
{
    return mPriv->deliveryReportingSupport;
}

Tp::MessagePartListList BaseChannelMessagesInterface::pendingMessages()
{
    return mPriv->textTypeInterface->pendingMessages();
}

void BaseChannelMessagesInterface::messageSent(const Tp::MessagePartList &content, uint flags, const QString &messageToken)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "messageSent", Q_ARG(Tp::MessagePartList, content), Q_ARG(uint, flags), Q_ARG(QString, messageToken)); //Can simply use emit in Qt5
}

void BaseChannelMessagesInterface::pendingMessagesRemoved(const Tp::UIntList &messageIDs)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "pendingMessagesRemoved", Q_ARG(Tp::UIntList, messageIDs)); //Can simply use emit in Qt5
}

void BaseChannelMessagesInterface::messageReceived(const Tp::MessagePartList &message)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "messageReceived", Q_ARG(Tp::MessagePartList, message)); //Can simply use emit in Qt5
}

void BaseChannelMessagesInterface::setSendMessageCallback(const SendMessageCallback &cb)
{
    mPriv->sendMessageCB = cb;
}

QString BaseChannelMessagesInterface::sendMessage(const Tp::MessagePartList &message, uint flags, DBusError* error)
{
    if (!mPriv->sendMessageCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    const QString token = mPriv->sendMessageCB(message, flags, error);

    Tp::MessagePartList fixedMessage = message;

    MessagePart header = fixedMessage.front();

    uint timestamp = 0;
    if (header.count(QLatin1String("message-sent"))) {
        timestamp = header[QLatin1String("message-sent")].variant().toUInt();
    } else {
        timestamp = QDateTime::currentMSecsSinceEpoch() / 1000;
        header[QLatin1String("message-sent")] = QDBusVariant(timestamp);
    }

    fixedMessage.replace(0, header);

    //emit after return
    QMetaObject::invokeMethod(mPriv->adaptee, "messageSent",
                              Qt::QueuedConnection,
                              Q_ARG(Tp::MessagePartList, fixedMessage),
                              Q_ARG(uint, flags),
                              Q_ARG(QString, token));

    if (message.empty()) {
        warning() << "Sending empty message";
        return token;
    }

    uint type = ChannelTextMessageTypeNormal;
    if (header.count(QLatin1String("message-type")))
        type = header[QLatin1String("message-type")].variant().toUInt();

    QString content;
    for (MessagePartList::const_iterator i = message.begin() + 1; i != message.end(); ++i)
        if (i->count(QLatin1String("content-type"))
                && i->value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain")
                && i->count(QLatin1String("content"))) {
            content = i->value(QLatin1String("content")).variant().toString();
            break;
        }
    //emit after return
    QMetaObject::invokeMethod(mPriv->textTypeInterface, "sent",
                              Qt::QueuedConnection,
                              Q_ARG(uint, timestamp),
                              Q_ARG(uint, type),
                              Q_ARG(QString, content));
    return token;
}

// Chan.T.RoomList
// The BaseChannelRoomListType code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseChannelRoomListType::Private {
    Private(BaseChannelRoomListType *parent,
            const QString &server)
        : server(server),
          listingRooms(false),
          adaptee(new BaseChannelRoomListType::Adaptee(parent))
    {
    }

    QString server;
    bool listingRooms;
    ListRoomsCallback listRoomsCB;
    StopListingCallback stopListingCB;
    BaseChannelRoomListType::Adaptee *adaptee;
};

BaseChannelRoomListType::Adaptee::Adaptee(BaseChannelRoomListType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelRoomListType::Adaptee::~Adaptee()
{
}

QString BaseChannelRoomListType::Adaptee::server() const
{
    return mInterface->server();
}

void BaseChannelRoomListType::Adaptee::getListingRooms(
        const Tp::Service::ChannelTypeRoomListAdaptor::GetListingRoomsContextPtr &context)
{
    context->setFinished(mInterface->getListingRooms());
}

void BaseChannelRoomListType::Adaptee::listRooms(
        const Tp::Service::ChannelTypeRoomListAdaptor::ListRoomsContextPtr &context)
{
    qDebug() << "BaseChannelRoomListType::Adaptee::listRooms";
    DBusError error;
    mInterface->listRooms(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelRoomListType::Adaptee::stopListing(
        const Tp::Service::ChannelTypeRoomListAdaptor::StopListingContextPtr &context)
{
    qDebug() << "BaseChannelRoomListType::Adaptee::stopListing";
    DBusError error;
    mInterface->stopListing(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelRoomListType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Type.RoomList
 */

/**
 * Class constructor.
 */
BaseChannelRoomListType::BaseChannelRoomListType(const QString &server)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_ROOM_LIST),
      mPriv(new Private(this, server))
{
}

/**
 * Class destructor.
 */
BaseChannelRoomListType::~BaseChannelRoomListType()
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
QVariantMap BaseChannelRoomListType::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_ROOM_LIST + QLatin1String(".Server"),
               QVariant::fromValue(mPriv->adaptee->server()));
    return map;
}

QString BaseChannelRoomListType::server() const
{
    return mPriv->server;
}

void BaseChannelRoomListType::createAdaptor()
{
    (void) new Tp::Service::ChannelTypeRoomListAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

bool BaseChannelRoomListType::getListingRooms()
{
    return mPriv->listingRooms;
}

void BaseChannelRoomListType::setListingRooms(bool listing)
{
    if (mPriv->listingRooms == listing) {
        return;
    }

    mPriv->listingRooms = listing;
    QMetaObject::invokeMethod(mPriv->adaptee, "listingRooms", Q_ARG(bool, listing)); //Can simply use emit in Qt5
}

void BaseChannelRoomListType::setListRoomsCallback(const ListRoomsCallback &cb)
{
    mPriv->listRoomsCB = cb;
}

void BaseChannelRoomListType::listRooms(DBusError *error)
{
    if (!mPriv->listRoomsCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->listRoomsCB(error);
}

void BaseChannelRoomListType::setStopListingCallback(const StopListingCallback &cb)
{
    mPriv->stopListingCB = cb;
}

void BaseChannelRoomListType::stopListing(DBusError *error)
{
    if (!mPriv->stopListingCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->stopListingCB(error);
}

void BaseChannelRoomListType::gotRooms(const Tp::RoomInfoList &rooms)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "gotRooms", Q_ARG(Tp::RoomInfoList, rooms)); //Can simply use emit in Qt5
}

//Chan.T.ServerAuthentication
BaseChannelServerAuthenticationType::Adaptee::Adaptee(BaseChannelServerAuthenticationType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelServerAuthenticationType::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelServerAuthenticationType::Private {
    Private(BaseChannelServerAuthenticationType *parent, const QString& authenticationMethod)
        : authenticationMethod(authenticationMethod),
          adaptee(new BaseChannelServerAuthenticationType::Adaptee(parent)) {
    }
    QString authenticationMethod;
    BaseChannelServerAuthenticationType::Adaptee *adaptee;
};

QString BaseChannelServerAuthenticationType::Adaptee::authenticationMethod() const
{
    return mInterface->mPriv->authenticationMethod;
}

/**
 * \class BaseChannelServerAuthenticationType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Type.ServerAuthentifcation
 *
 */

/**
 * Class constructor.
 */
BaseChannelServerAuthenticationType::BaseChannelServerAuthenticationType(const QString& authenticationMethod)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION),
      mPriv(new Private(this, authenticationMethod))
{
}

/**
 * Class destructor.
 */
BaseChannelServerAuthenticationType::~BaseChannelServerAuthenticationType()
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
QVariantMap BaseChannelServerAuthenticationType::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION + QLatin1String(".AuthenticationMethod"),
               QVariant::fromValue(mPriv->adaptee->authenticationMethod()));
    return map;
}

void BaseChannelServerAuthenticationType::createAdaptor()
{
    (void) new Service::ChannelTypeServerAuthenticationAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

//Chan.I.CaptchaAuthentication
BaseChannelCaptchaAuthenticationInterface::Adaptee::Adaptee(BaseChannelCaptchaAuthenticationInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelCaptchaAuthenticationInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelCaptchaAuthenticationInterface::Private {
    Private(BaseChannelCaptchaAuthenticationInterface *parent, bool canRetryCaptcha)
        : canRetryCaptcha(canRetryCaptcha),
          captchaStatus(CaptchaStatusLocalPending),
          adaptee(new BaseChannelCaptchaAuthenticationInterface::Adaptee(parent)) {
    }
    bool canRetryCaptcha;
    bool captchaStatus;
    QString captchaError;
    QVariantMap captchaErrorDetails;
    GetCaptchasCallback getCaptchasCB;
    GetCaptchaDataCallback getCaptchaDataCB;
    AnswerCaptchasCallback answerCaptchasCB;
    CancelCaptchaCallback cancelCaptchaCB;
    BaseChannelCaptchaAuthenticationInterface::Adaptee *adaptee;
};

bool BaseChannelCaptchaAuthenticationInterface::Adaptee::canRetryCaptcha() const
{
    return mInterface->mPriv->canRetryCaptcha;
}

uint BaseChannelCaptchaAuthenticationInterface::Adaptee::captchaStatus() const
{
    return mInterface->mPriv->captchaStatus;
}

QString BaseChannelCaptchaAuthenticationInterface::Adaptee::captchaError() const
{
    return mInterface->mPriv->captchaError;
}

QVariantMap BaseChannelCaptchaAuthenticationInterface::Adaptee::captchaErrorDetails() const
{
    return mInterface->mPriv->captchaErrorDetails;
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchas(const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::GetCaptchasContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchas";
    DBusError error;
    Tp::CaptchaInfoList captchaInfo;
    uint numberRequired;
    QString language;
    mInterface->mPriv->getCaptchasCB(captchaInfo, numberRequired, language, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(captchaInfo, numberRequired, language);
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchaData(uint ID, const QString& mimeType, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::GetCaptchaDataContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchaData " << ID << mimeType;
    DBusError error;
    QByteArray captchaData = mInterface->mPriv->getCaptchaDataCB(ID, mimeType, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(captchaData);
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::answerCaptchas(const Tp::CaptchaAnswers& answers, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::AnswerCaptchasContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::answerCaptchas";
    DBusError error;
    mInterface->mPriv->answerCaptchasCB(answers, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelCaptchaAuthenticationInterface::Adaptee::cancelCaptcha(uint reason, const QString& debugMessage, const Tp::Service::ChannelInterfaceCaptchaAuthenticationAdaptor::CancelCaptchaContextPtr &context)
{
    qDebug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::cancelCaptcha "
             << reason << " " << debugMessage;
    DBusError error;
    mInterface->mPriv->cancelCaptchaCB(reason, debugMessage, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelCaptchaAuthenticationInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.CaptchaAuthentication
 *
 */

/**
 * Class constructor.
 */
BaseChannelCaptchaAuthenticationInterface::BaseChannelCaptchaAuthenticationInterface(bool canRetryCaptcha)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION),
      mPriv(new Private(this, canRetryCaptcha))
{
}

/**
 * Class destructor.
 */
BaseChannelCaptchaAuthenticationInterface::~BaseChannelCaptchaAuthenticationInterface()
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
QVariantMap BaseChannelCaptchaAuthenticationInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION + QLatin1String(".CanRetryCaptcha"),
               QVariant::fromValue(mPriv->adaptee->canRetryCaptcha()));
    return map;
}

void BaseChannelCaptchaAuthenticationInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceCaptchaAuthenticationAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelCaptchaAuthenticationInterface::setGetCaptchasCallback(const GetCaptchasCallback &cb)
{
    mPriv->getCaptchasCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setGetCaptchaDataCallback(const GetCaptchaDataCallback &cb)
{
    mPriv->getCaptchaDataCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setAnswerCaptchasCallback(const AnswerCaptchasCallback &cb)
{
    mPriv->answerCaptchasCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setCancelCaptchaCallback(const CancelCaptchaCallback &cb)
{
    mPriv->cancelCaptchaCB = cb;
}

void BaseChannelCaptchaAuthenticationInterface::setCaptchaStatus(uint status)
{
    mPriv->captchaStatus = status;
    notifyPropertyChanged(QLatin1String("CaptchaStatus"), QVariant::fromValue(status));
}

void BaseChannelCaptchaAuthenticationInterface::setCaptchaError(const QString& busName)
{
    mPriv->captchaError = busName;
    notifyPropertyChanged(QLatin1String("CaptchaError"), QVariant::fromValue(busName));
}

void BaseChannelCaptchaAuthenticationInterface::setCaptchaErrorDetails(const QVariantMap& error)
{
    mPriv->captchaErrorDetails = error;
    notifyPropertyChanged(QLatin1String("CaptchaErrorDetails"), QVariant::fromValue(error));
}

// Chan.I.SASLAuthentication
// The BaseChannelSASLAuthenticationInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseChannelSASLAuthenticationInterface::Private {
    Private(BaseChannelSASLAuthenticationInterface *parent,
            const QStringList &availableMechanisms,
            bool hasInitialData,
            bool canTryAgain,
            const QString &authorizationIdentity,
            const QString &defaultUsername,
            const QString &defaultRealm,
            bool maySaveResponse)
        : availableMechanisms(availableMechanisms),
          hasInitialData(hasInitialData),
          canTryAgain(canTryAgain),
          saslStatus(0),
          authorizationIdentity(authorizationIdentity),
          defaultUsername(defaultUsername),
          defaultRealm(defaultRealm),
          maySaveResponse(maySaveResponse),
          adaptee(new BaseChannelSASLAuthenticationInterface::Adaptee(parent))
    {
    }

    QStringList availableMechanisms;
    bool hasInitialData;
    bool canTryAgain;
    uint saslStatus;
    QString saslError;
    QVariantMap saslErrorDetails;
    QString authorizationIdentity;
    QString defaultUsername;
    QString defaultRealm;
    bool maySaveResponse;
    StartMechanismCallback startMechanismCB;
    StartMechanismWithDataCallback startMechanismWithDataCB;
    RespondCallback respondCB;
    AcceptSASLCallback acceptSaslCB;
    AbortSASLCallback abortSaslCB;
    BaseChannelSASLAuthenticationInterface::Adaptee *adaptee;
};

BaseChannelSASLAuthenticationInterface::Adaptee::Adaptee(BaseChannelSASLAuthenticationInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelSASLAuthenticationInterface::Adaptee::~Adaptee()
{
}

QStringList BaseChannelSASLAuthenticationInterface::Adaptee::availableMechanisms() const
{
    return mInterface->availableMechanisms();
}

bool BaseChannelSASLAuthenticationInterface::Adaptee::hasInitialData() const
{
    return mInterface->hasInitialData();
}

bool BaseChannelSASLAuthenticationInterface::Adaptee::canTryAgain() const
{
    return mInterface->canTryAgain();
}

uint BaseChannelSASLAuthenticationInterface::Adaptee::saslStatus() const
{
    return mInterface->saslStatus();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::saslError() const
{
    return mInterface->saslError();
}

QVariantMap BaseChannelSASLAuthenticationInterface::Adaptee::saslErrorDetails() const
{
    return mInterface->saslErrorDetails();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::authorizationIdentity() const
{
    return mInterface->authorizationIdentity();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::defaultUsername() const
{
    return mInterface->defaultUsername();
}

QString BaseChannelSASLAuthenticationInterface::Adaptee::defaultRealm() const
{
    return mInterface->defaultRealm();
}

bool BaseChannelSASLAuthenticationInterface::Adaptee::maySaveResponse() const
{
    return mInterface->maySaveResponse();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::startMechanism(const QString &mechanism,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::StartMechanismContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::startMechanism";
    DBusError error;
    mInterface->startMechanism(mechanism, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::startMechanismWithData(const QString &mechanism, const QByteArray &initialData,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::StartMechanismWithDataContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::startMechanismWithData";
    DBusError error;
    mInterface->startMechanismWithData(mechanism, initialData, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::respond(const QByteArray &responseData,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::RespondContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::respond";
    DBusError error;
    mInterface->respond(responseData, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::acceptSasl(
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::AcceptSASLContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::acceptSasl";
    DBusError error;
    mInterface->acceptSasl(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelSASLAuthenticationInterface::Adaptee::abortSasl(uint reason, const QString &debugMessage,
        const Tp::Service::ChannelInterfaceSASLAuthenticationAdaptor::AbortSASLContextPtr &context)
{
    qDebug() << "BaseChannelSASLAuthenticationInterface::Adaptee::abortSasl";
    DBusError error;
    mInterface->abortSasl(reason, debugMessage, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelSASLAuthenticationInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.SASLAuthentication
 */

/**
 * Class constructor.
 */
BaseChannelSASLAuthenticationInterface::BaseChannelSASLAuthenticationInterface(const QStringList &availableMechanisms,
                                                                               bool hasInitialData,
                                                                               bool canTryAgain,
                                                                               const QString &authorizationIdentity,
                                                                               const QString &defaultUsername,
                                                                               const QString &defaultRealm,
                                                                               bool maySaveResponse)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION),
      mPriv(new Private(this, availableMechanisms, hasInitialData, canTryAgain, authorizationIdentity, defaultUsername, defaultRealm, maySaveResponse))
{
}

/**
 * Class destructor.
 */
BaseChannelSASLAuthenticationInterface::~BaseChannelSASLAuthenticationInterface()
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
QVariantMap BaseChannelSASLAuthenticationInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".AvailableMechanisms"),
               QVariant::fromValue(mPriv->adaptee->availableMechanisms()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".HasInitialData"),
               QVariant::fromValue(mPriv->adaptee->hasInitialData()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".CanTryAgain"),
               QVariant::fromValue(mPriv->adaptee->canTryAgain()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".AuthorizationIdentity"),
               QVariant::fromValue(mPriv->adaptee->authorizationIdentity()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".DefaultUsername"),
               QVariant::fromValue(mPriv->adaptee->defaultUsername()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".DefaultRealm"),
               QVariant::fromValue(mPriv->adaptee->defaultRealm()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION + QLatin1String(".MaySaveResponse"),
               QVariant::fromValue(mPriv->adaptee->maySaveResponse()));
    return map;
}

QStringList BaseChannelSASLAuthenticationInterface::availableMechanisms() const
{
    return mPriv->availableMechanisms;
}

bool BaseChannelSASLAuthenticationInterface::hasInitialData() const
{
    return mPriv->hasInitialData;
}

bool BaseChannelSASLAuthenticationInterface::canTryAgain() const
{
    return mPriv->canTryAgain;
}

uint BaseChannelSASLAuthenticationInterface::saslStatus() const
{
    return mPriv->saslStatus;
}

void BaseChannelSASLAuthenticationInterface::setSaslStatus(uint status, const QString &reason, const QVariantMap &details)
{
    mPriv->saslStatus = status;
    mPriv->saslError = reason;
    mPriv->saslErrorDetails = details;
    QMetaObject::invokeMethod(mPriv->adaptee, "saslStatusChanged", Q_ARG(uint, status), Q_ARG(QString, reason), Q_ARG(QVariantMap, details)); //Can simply use emit in Qt5
}

QString BaseChannelSASLAuthenticationInterface::saslError() const
{
    return mPriv->saslError;
}

void BaseChannelSASLAuthenticationInterface::setSaslError(const QString &saslError)
{
    mPriv->saslError = saslError;
}

QVariantMap BaseChannelSASLAuthenticationInterface::saslErrorDetails() const
{
    return mPriv->saslErrorDetails;
}

void BaseChannelSASLAuthenticationInterface::setSaslErrorDetails(const QVariantMap &saslErrorDetails)
{
    mPriv->saslErrorDetails = saslErrorDetails;
}

QString BaseChannelSASLAuthenticationInterface::authorizationIdentity() const
{
    return mPriv->authorizationIdentity;
}

QString BaseChannelSASLAuthenticationInterface::defaultUsername() const
{
    return mPriv->defaultUsername;
}

QString BaseChannelSASLAuthenticationInterface::defaultRealm() const
{
    return mPriv->defaultRealm;
}

bool BaseChannelSASLAuthenticationInterface::maySaveResponse() const
{
    return mPriv->maySaveResponse;
}

void BaseChannelSASLAuthenticationInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceSASLAuthenticationAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelSASLAuthenticationInterface::setStartMechanismCallback(const BaseChannelSASLAuthenticationInterface::StartMechanismCallback &cb)
{
    mPriv->startMechanismCB = cb;
}

void BaseChannelSASLAuthenticationInterface::startMechanism(const QString &mechanism, DBusError *error)
{
    if (!mPriv->startMechanismCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->startMechanismCB(mechanism, error);
}

void BaseChannelSASLAuthenticationInterface::setStartMechanismWithDataCallback(const BaseChannelSASLAuthenticationInterface::StartMechanismWithDataCallback &cb)
{
    mPriv->startMechanismWithDataCB = cb;
}

void BaseChannelSASLAuthenticationInterface::startMechanismWithData(const QString &mechanism, const QByteArray &initialData, DBusError *error)
{
    if (!mPriv->startMechanismWithDataCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->startMechanismWithDataCB(mechanism, initialData, error);
}

void BaseChannelSASLAuthenticationInterface::setRespondCallback(const BaseChannelSASLAuthenticationInterface::RespondCallback &cb)
{
    mPriv->respondCB = cb;
}

void BaseChannelSASLAuthenticationInterface::respond(const QByteArray &responseData, DBusError *error)
{
    if (!mPriv->respondCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->respondCB(responseData, error);
}

void BaseChannelSASLAuthenticationInterface::setAcceptSaslCallback(const BaseChannelSASLAuthenticationInterface::AcceptSASLCallback &cb)
{
    mPriv->acceptSaslCB = cb;
}

void BaseChannelSASLAuthenticationInterface::acceptSasl(DBusError *error)
{
    if (!mPriv->acceptSaslCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->acceptSaslCB(error);
}

void BaseChannelSASLAuthenticationInterface::setAbortSaslCallback(const BaseChannelSASLAuthenticationInterface::AbortSASLCallback &cb)
{
    mPriv->abortSaslCB = cb;
}

void BaseChannelSASLAuthenticationInterface::abortSasl(uint reason, const QString &debugMessage, DBusError *error)
{
    if (!mPriv->abortSaslCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->abortSaslCB(reason, debugMessage, error);
}

void BaseChannelSASLAuthenticationInterface::newChallenge(const QByteArray &challengeData)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "newChallenge", Q_ARG(QByteArray, challengeData)); //Can simply use emit in Qt5
}

// Chan.I.Securable
struct TP_QT_NO_EXPORT BaseChannelSecurableInterface::Private {
    Private(BaseChannelSecurableInterface *parent)
        : encrypted(false),
          verified(false),
          adaptee(new BaseChannelSecurableInterface::Adaptee(parent))
    {
    }

    bool encrypted;
    bool verified;
    BaseChannelSecurableInterface::Adaptee *adaptee;
};

BaseChannelSecurableInterface::Adaptee::Adaptee(BaseChannelSecurableInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelSecurableInterface::Adaptee::~Adaptee()
{
}

bool BaseChannelSecurableInterface::Adaptee::encrypted() const
{
    return mInterface->encrypted();
}

bool BaseChannelSecurableInterface::Adaptee::verified() const
{
    return mInterface->verified();
}

/**
 * \class BaseChannelSecurableInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Securable
 */

/**
 * Class constructor.
 */
BaseChannelSecurableInterface::BaseChannelSecurableInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_SECURABLE),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseChannelSecurableInterface::~BaseChannelSecurableInterface()
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
QVariantMap BaseChannelSecurableInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

bool BaseChannelSecurableInterface::encrypted() const
{
    return mPriv->encrypted;
}

void BaseChannelSecurableInterface::setEncrypted(bool encrypted)
{
    mPriv->encrypted = encrypted;
}

bool BaseChannelSecurableInterface::verified() const
{
    return mPriv->verified;
}

void BaseChannelSecurableInterface::setVerified(bool verified)
{
    mPriv->verified = verified;
}

void BaseChannelSecurableInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceSecurableAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

// Chan.I.ChatState
struct TP_QT_NO_EXPORT BaseChannelChatStateInterface::Private {
    Private(BaseChannelChatStateInterface *parent)
        : adaptee(new BaseChannelChatStateInterface::Adaptee(parent))
    {
    }

    Tp::ChatStateMap chatStates;
    SetChatStateCallback setChatStateCB;
    BaseChannelChatStateInterface::Adaptee *adaptee;
};

BaseChannelChatStateInterface::Adaptee::Adaptee(BaseChannelChatStateInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelChatStateInterface::Adaptee::~Adaptee()
{
}

Tp::ChatStateMap BaseChannelChatStateInterface::Adaptee::chatStates() const
{
    return mInterface->chatStates();
}

void BaseChannelChatStateInterface::Adaptee::setChatState(uint state,
        const Tp::Service::ChannelInterfaceChatStateAdaptor::SetChatStateContextPtr &context)
{
    qDebug() << "BaseChannelChatStateInterface::Adaptee::setChatState";
    DBusError error;
    mInterface->setChatState(state, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelChatStateInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Chat.State
 */

/**
 * Class constructor.
 */
BaseChannelChatStateInterface::BaseChannelChatStateInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CHAT_STATE),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseChannelChatStateInterface::~BaseChannelChatStateInterface()
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
QVariantMap BaseChannelChatStateInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

Tp::ChatStateMap BaseChannelChatStateInterface::chatStates() const
{
    return mPriv->chatStates;
}

void BaseChannelChatStateInterface::setChatStates(const Tp::ChatStateMap &chatStates)
{
    mPriv->chatStates = chatStates;
}

void BaseChannelChatStateInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceChatStateAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelChatStateInterface::setSetChatStateCallback(const BaseChannelChatStateInterface::SetChatStateCallback &cb)
{
    mPriv->setChatStateCB = cb;
}

void BaseChannelChatStateInterface::setChatState(uint state, DBusError *error)
{
    if (!mPriv->setChatStateCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->setChatStateCB(state, error);
}

void BaseChannelChatStateInterface::chatStateChanged(uint contact, uint state)
{
    QMetaObject::invokeMethod(mPriv->adaptee, "chatStateChanged", Q_ARG(uint, contact), Q_ARG(uint, state)); //Can simply use emit in Qt5
}

//Chan.I.Group
BaseChannelGroupInterface::Adaptee::Adaptee(BaseChannelGroupInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelGroupInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelGroupInterface::Private {
    Private(BaseChannelGroupInterface *parent, ChannelGroupFlags initialFlags, uint selfHandle)
        : flags(initialFlags),
          selfHandle(selfHandle),
          adaptee(new BaseChannelGroupInterface::Adaptee(parent)) {
    }
    ChannelGroupFlags flags;
    Tp::HandleOwnerMap handleOwners;
    Tp::LocalPendingInfoList localPendingMembers;
    Tp::UIntList members;
    Tp::UIntList remotePendingMembers;
    uint selfHandle;
    Tp::HandleIdentifierMap memberIdentifiers;
    RemoveMembersCallback removeMembersCB;
    AddMembersCallback addMembersCB;
    Tp::UIntList getLocalPendingMembers() const {
        Tp::UIntList ret;
        foreach(const LocalPendingInfo & info, localPendingMembers)
        ret << info.toBeAdded;
        return ret;
    }
    BaseChannelGroupInterface::Adaptee *adaptee;
};

uint BaseChannelGroupInterface::Adaptee::groupFlags() const
{
    return mInterface->mPriv->flags;
}

Tp::HandleOwnerMap BaseChannelGroupInterface::Adaptee::handleOwners() const
{
    return mInterface->mPriv->handleOwners;
}

Tp::LocalPendingInfoList BaseChannelGroupInterface::Adaptee::localPendingMembers() const
{
    return mInterface->mPriv->localPendingMembers;
}

Tp::UIntList BaseChannelGroupInterface::Adaptee::members() const
{
    return mInterface->mPriv->members;
}

Tp::UIntList BaseChannelGroupInterface::Adaptee::remotePendingMembers() const
{
    return mInterface->mPriv->remotePendingMembers;
}

uint BaseChannelGroupInterface::Adaptee::selfHandle() const
{
    return mInterface->mPriv->selfHandle;
}

Tp::HandleIdentifierMap BaseChannelGroupInterface::Adaptee::memberIdentifiers() const
{
    return mInterface->mPriv->memberIdentifiers;
}

void BaseChannelGroupInterface::Adaptee::addMembers(const Tp::UIntList& contacts,
        const QString& message,
        const Tp::Service::ChannelInterfaceGroupAdaptor::AddMembersContextPtr &context)
{
    debug() << "BaseChannelGroupInterface::Adaptee::addMembers";
    if (!mInterface->mPriv->addMembersCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->addMembersCB(contacts, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelGroupInterface::Adaptee::removeMembers(const Tp::UIntList& contacts, const QString& message,
        const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersContextPtr &context)
{
    debug() << "BaseChannelGroupInterface::Adaptee::removeMembers";
    if (!mInterface->mPriv->removeMembersCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->removeMembersCB(contacts, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelGroupInterface::Adaptee::removeMembersWithReason(const Tp::UIntList& contacts,
        const QString& message,
        uint reason,
        const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersWithReasonContextPtr &context)
{
    debug() << "BaseChannelGroupInterface::Adaptee::removeMembersWithReason";
    removeMembers(contacts, message, context);
}

void BaseChannelGroupInterface::Adaptee::getAllMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetAllMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->members, mInterface->mPriv->getLocalPendingMembers(), mInterface->mPriv->remotePendingMembers);
}

void BaseChannelGroupInterface::Adaptee::getGroupFlags(const Tp::Service::ChannelInterfaceGroupAdaptor::GetGroupFlagsContextPtr &context)
{
    context->setFinished(mInterface->mPriv->flags);
}

void BaseChannelGroupInterface::Adaptee::getHandleOwners(const Tp::UIntList& handles, const Tp::Service::ChannelInterfaceGroupAdaptor::GetHandleOwnersContextPtr &context)
{
    Tp::UIntList ret;
    foreach(uint handle, handles)
    ret.append(mInterface->mPriv->handleOwners.contains(handle) ? mInterface->mPriv->handleOwners[handle] : 0);
    context->setFinished(ret);
}

void BaseChannelGroupInterface::Adaptee::getLocalPendingMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetLocalPendingMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->getLocalPendingMembers());
}

void BaseChannelGroupInterface::Adaptee::getLocalPendingMembersWithInfo(const Tp::Service::ChannelInterfaceGroupAdaptor::GetLocalPendingMembersWithInfoContextPtr &context)
{
    context->setFinished(mInterface->mPriv->localPendingMembers);
}

void BaseChannelGroupInterface::Adaptee::getMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->members);
}

void BaseChannelGroupInterface::Adaptee::getRemotePendingMembers(const Tp::Service::ChannelInterfaceGroupAdaptor::GetRemotePendingMembersContextPtr &context)
{
    context->setFinished(mInterface->mPriv->remotePendingMembers);
}

void BaseChannelGroupInterface::Adaptee::getSelfHandle(const Tp::Service::ChannelInterfaceGroupAdaptor::GetSelfHandleContextPtr &context)
{
    context->setFinished(mInterface->mPriv->selfHandle);
}

/**
 * \class BaseChannelGroupInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Group
 *
 */

/**
 * Class constructor.
 */
BaseChannelGroupInterface::BaseChannelGroupInterface(ChannelGroupFlags initialFlags, uint selfHandle)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP),
      mPriv(new Private(this, initialFlags, selfHandle))
{
}

/**
 * Class destructor.
 */
BaseChannelGroupInterface::~BaseChannelGroupInterface()
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
QVariantMap BaseChannelGroupInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelGroupInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceGroupAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelGroupInterface::setRemoveMembersCallback(const RemoveMembersCallback &cb)
{
    mPriv->removeMembersCB = cb;
}

void BaseChannelGroupInterface::setAddMembersCallback(const AddMembersCallback &cb)
{
    mPriv->addMembersCB = cb;
}

void BaseChannelGroupInterface::addMembers(const Tp::UIntList& handles, const QStringList& identifiers)
{
    if (handles.size() != identifiers.size()) {
        debug() << "BaseChannelGroupInterface::addMembers: handles.size() != identifiers.size()";
        return;
    }
    Tp::UIntList added;
    for (int i = 0; i < handles.size(); ++i) {
        uint handle = handles[i];
        if (mPriv->members.contains(handle))
            continue;

        mPriv->memberIdentifiers[handle] = identifiers[i];
        mPriv->members.append(handle);
        added.append(handle);
    }
    if (!added.isEmpty())
        QMetaObject::invokeMethod(mPriv->adaptee,"membersChanged",Q_ARG(QString, QString()), Q_ARG(Tp::UIntList, added), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(uint, 0), Q_ARG(uint, ChannelGroupChangeReasonNone)); //Can simply use emit in Qt5
}

void BaseChannelGroupInterface::removeMembers(const Tp::UIntList& handles)
{
    Tp::UIntList removed;
    foreach(uint handle, handles) {
        if (mPriv->members.contains(handle))
            continue;

        mPriv->memberIdentifiers.remove(handle);
        mPriv->members.removeAll(handle);
        removed.append(handle);
    }
    if (!removed.isEmpty())
        QMetaObject::invokeMethod(mPriv->adaptee,"membersChanged",Q_ARG(QString, QString()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, removed), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(Tp::UIntList, Tp::UIntList()), Q_ARG(uint, 0), Q_ARG(uint,ChannelGroupChangeReasonNone)); //Can simply use emit in Qt5 //Can simply use emit in Qt5
}

// Chan.I.Room2
// The BaseChannelRoomInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseChannelRoomInterface::Private {
    Private(BaseChannelRoomInterface *parent,
            const QString &roomName,
            const QString &server,
            const QString &creator,
            uint creatorHandle,
            const QDateTime &creationTimestamp)
        : roomName(roomName),
          server(server),
          creator(creator),
          creatorHandle(creatorHandle),
          creationTimestamp(creationTimestamp),
          adaptee(new BaseChannelRoomInterface::Adaptee(parent))
    {
    }

    QString roomName;
    QString server;
    QString creator;
    uint creatorHandle;
    QDateTime creationTimestamp;
    BaseChannelRoomInterface::Adaptee *adaptee;
};

BaseChannelRoomInterface::Adaptee::Adaptee(BaseChannelRoomInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelRoomInterface::Adaptee::~Adaptee()
{
}

QString BaseChannelRoomInterface::Adaptee::roomName() const
{
    return mInterface->roomName();
}

QString BaseChannelRoomInterface::Adaptee::server() const
{
    return mInterface->server();
}

QString BaseChannelRoomInterface::Adaptee::creator() const
{
    return mInterface->creator();
}

uint BaseChannelRoomInterface::Adaptee::creatorHandle() const
{
    return mInterface->creatorHandle();
}

QDateTime BaseChannelRoomInterface::Adaptee::creationTimestamp() const
{
    return mInterface->creationTimestamp();
}

/**
 * \class BaseChannelRoomInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Room2
 */

/**
 * Class constructor.
 */
BaseChannelRoomInterface::BaseChannelRoomInterface(const QString &roomName,
                                                   const QString &server,
                                                   const QString &creator,
                                                   uint creatorHandle,
                                                   const QDateTime &creationTimestamp)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM),
      mPriv(new Private(this, roomName, server, creator, creatorHandle, creationTimestamp))
{
}

/**
 * Class destructor.
 */
BaseChannelRoomInterface::~BaseChannelRoomInterface()
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
QVariantMap BaseChannelRoomInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM + QLatin1String(".RoomName"),
               QVariant::fromValue(mPriv->adaptee->roomName()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM + QLatin1String(".Server"),
               QVariant::fromValue(mPriv->adaptee->server()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM + QLatin1String(".Creator"),
               QVariant::fromValue(mPriv->adaptee->creator()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM + QLatin1String(".CreatorHandle"),
               QVariant::fromValue(mPriv->adaptee->creatorHandle()));
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM + QLatin1String(".CreationTimestamp"),
               QVariant::fromValue(mPriv->adaptee->creationTimestamp()));
    return map;
}

QString BaseChannelRoomInterface::roomName() const
{
    return mPriv->roomName;
}

QString BaseChannelRoomInterface::server() const
{
    return mPriv->server;
}

QString BaseChannelRoomInterface::creator() const
{
    return mPriv->creator;
}

uint BaseChannelRoomInterface::creatorHandle() const
{
    return mPriv->creatorHandle;
}

QDateTime BaseChannelRoomInterface::creationTimestamp() const
{
    return mPriv->creationTimestamp;
}

void BaseChannelRoomInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceRoomAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

// Chan.I.RoomConfig1
// The BaseChannelRoomConfigInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseChannelRoomConfigInterface::Private {
    Private(BaseChannelRoomConfigInterface *parent)
        : anonymous(false),
          inviteOnly(false),
          limit(0),
          moderated(false),
          persistent(false),
          isPrivate(false),
          passwordProtected(false),
          canUpdateConfiguration(false),
          configurationRetrieved(false),
          adaptee(new BaseChannelRoomConfigInterface::Adaptee(parent))
    {
    }

    bool anonymous;
    bool inviteOnly;
    uint limit;
    bool moderated;
    QString title;
    QString description;
    bool persistent;
    bool isPrivate;
    bool passwordProtected;
    QString password;
    QString passwordHint;
    bool canUpdateConfiguration;
    QStringList mutableProperties;
    bool configurationRetrieved;
    UpdateConfigurationCallback updateConfigurationCB;
    BaseChannelRoomConfigInterface::Adaptee *adaptee;
};

BaseChannelRoomConfigInterface::Adaptee::Adaptee(BaseChannelRoomConfigInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelRoomConfigInterface::Adaptee::~Adaptee()
{
}

bool BaseChannelRoomConfigInterface::Adaptee::anonymous() const
{
    return mInterface->anonymous();
}

bool BaseChannelRoomConfigInterface::Adaptee::inviteOnly() const
{
    return mInterface->inviteOnly();
}

uint BaseChannelRoomConfigInterface::Adaptee::limit() const
{
    return mInterface->limit();
}

bool BaseChannelRoomConfigInterface::Adaptee::moderated() const
{
    return mInterface->moderated();
}

QString BaseChannelRoomConfigInterface::Adaptee::title() const
{
    return mInterface->title();
}

QString BaseChannelRoomConfigInterface::Adaptee::description() const
{
    return mInterface->description();
}

bool BaseChannelRoomConfigInterface::Adaptee::persistent() const
{
    return mInterface->persistent();
}

bool BaseChannelRoomConfigInterface::Adaptee::isPrivate() const
{
    return mInterface->isPrivate();
}

bool BaseChannelRoomConfigInterface::Adaptee::passwordProtected() const
{
    return mInterface->passwordProtected();
}

QString BaseChannelRoomConfigInterface::Adaptee::password() const
{
    return mInterface->password();
}

QString BaseChannelRoomConfigInterface::Adaptee::passwordHint() const
{
    return mInterface->passwordHint();
}

bool BaseChannelRoomConfigInterface::Adaptee::canUpdateConfiguration() const
{
    return mInterface->canUpdateConfiguration();
}

QStringList BaseChannelRoomConfigInterface::Adaptee::mutableProperties() const
{
    return mInterface->mutableProperties();
}

bool BaseChannelRoomConfigInterface::Adaptee::configurationRetrieved() const
{
    return mInterface->configurationRetrieved();
}

void BaseChannelRoomConfigInterface::Adaptee::updateConfiguration(const QVariantMap &properties,
        const Tp::Service::ChannelInterfaceRoomConfigAdaptor::UpdateConfigurationContextPtr &context)
{
    qDebug() << "BaseChannelRoomConfigInterface::Adaptee::updateConfiguration";
    DBusError error;
    mInterface->updateConfiguration(properties, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelRoomConfigInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.RoomConfig1
 */

/**
 * Class constructor.
 */
BaseChannelRoomConfigInterface::BaseChannelRoomConfigInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM_CONFIG),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseChannelRoomConfigInterface::~BaseChannelRoomConfigInterface()
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
QVariantMap BaseChannelRoomConfigInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

bool BaseChannelRoomConfigInterface::anonymous() const
{
    return mPriv->anonymous;
}

void BaseChannelRoomConfigInterface::setAnonymous(bool anonymous)
{
    mPriv->anonymous = anonymous;
    notifyPropertyChanged(QLatin1String("Anonymous"), QVariant::fromValue(anonymous));
}

bool BaseChannelRoomConfigInterface::inviteOnly() const
{
    return mPriv->inviteOnly;
}

void BaseChannelRoomConfigInterface::setInviteOnly(bool inviteOnly)
{
    mPriv->inviteOnly = inviteOnly;
    notifyPropertyChanged(QLatin1String("InviteOnly"), QVariant::fromValue(inviteOnly));
}

uint BaseChannelRoomConfigInterface::limit() const
{
    return mPriv->limit;
}

void BaseChannelRoomConfigInterface::setLimit(uint limit)
{
    mPriv->limit = limit;
    notifyPropertyChanged(QLatin1String("Limit"), QVariant::fromValue(limit));
}

bool BaseChannelRoomConfigInterface::moderated() const
{
    return mPriv->moderated;
}

void BaseChannelRoomConfigInterface::setModerated(bool moderated)
{
    mPriv->moderated = moderated;
    notifyPropertyChanged(QLatin1String("Moderated"), QVariant::fromValue(moderated));
}

QString BaseChannelRoomConfigInterface::title() const
{
    return mPriv->title;
}

void BaseChannelRoomConfigInterface::setTitle(const QString &title)
{
    mPriv->title = title;
    notifyPropertyChanged(QLatin1String("Title"), QVariant::fromValue(title));
}

QString BaseChannelRoomConfigInterface::description() const
{
    return mPriv->description;
}

void BaseChannelRoomConfigInterface::setDescription(const QString &description)
{
    mPriv->description = description;
    notifyPropertyChanged(QLatin1String("Description"), QVariant::fromValue(description));
}

bool BaseChannelRoomConfigInterface::persistent() const
{
    return mPriv->persistent;
}

void BaseChannelRoomConfigInterface::setPersistent(bool persistent)
{
    mPriv->persistent = persistent;
    notifyPropertyChanged(QLatin1String("Persistent"), QVariant::fromValue(persistent));
}

bool BaseChannelRoomConfigInterface::isPrivate() const
{
    return mPriv->isPrivate;
}

void BaseChannelRoomConfigInterface::setPrivate(bool newPrivate)
{
    mPriv->isPrivate = newPrivate;
    notifyPropertyChanged(QLatin1String("Private"), QVariant::fromValue(newPrivate));
}

bool BaseChannelRoomConfigInterface::passwordProtected() const
{
    return mPriv->passwordProtected;
}

void BaseChannelRoomConfigInterface::setPasswordProtected(bool passwordProtected)
{
    mPriv->passwordProtected = passwordProtected;
    notifyPropertyChanged(QLatin1String("PasswordProtected"), QVariant::fromValue(passwordProtected));
}

QString BaseChannelRoomConfigInterface::password() const
{
    return mPriv->password;
}

void BaseChannelRoomConfigInterface::setPassword(const QString &password)
{
    mPriv->password = password;
    notifyPropertyChanged(QLatin1String("Password"), QVariant::fromValue(password));
}

QString BaseChannelRoomConfigInterface::passwordHint() const
{
    return mPriv->passwordHint;
}

void BaseChannelRoomConfigInterface::setPasswordHint(const QString &passwordHint)
{
    mPriv->passwordHint = passwordHint;
    notifyPropertyChanged(QLatin1String("PasswordHint"), QVariant::fromValue(passwordHint));
}

bool BaseChannelRoomConfigInterface::canUpdateConfiguration() const
{
    return mPriv->canUpdateConfiguration;
}

void BaseChannelRoomConfigInterface::setCanUpdateConfiguration(bool canUpdateConfiguration)
{
    mPriv->canUpdateConfiguration = canUpdateConfiguration;
    notifyPropertyChanged(QLatin1String("CanUpdateConfiguration"), QVariant::fromValue(canUpdateConfiguration));
}

QStringList BaseChannelRoomConfigInterface::mutableProperties() const
{
    return mPriv->mutableProperties;
}

void BaseChannelRoomConfigInterface::setMutableProperties(const QStringList &mutableProperties)
{
    mPriv->mutableProperties = mutableProperties;
    notifyPropertyChanged(QLatin1String("MutableProperties"), QVariant::fromValue(mutableProperties));
}

bool BaseChannelRoomConfigInterface::configurationRetrieved() const
{
    return mPriv->configurationRetrieved;
}

void BaseChannelRoomConfigInterface::setConfigurationRetrieved(bool configurationRetrieved)
{
    mPriv->configurationRetrieved = configurationRetrieved;
    notifyPropertyChanged(QLatin1String("ConfigurationRetrieved"), QVariant::fromValue(configurationRetrieved));
}

void BaseChannelRoomConfigInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceRoomConfigAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelRoomConfigInterface::setUpdateConfigurationCallback(const UpdateConfigurationCallback &cb)
{
    mPriv->updateConfigurationCB = cb;
}

void BaseChannelRoomConfigInterface::updateConfiguration(const QVariantMap &properties, DBusError *error)
{
    if (!mPriv->updateConfigurationCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->updateConfigurationCB(properties, error);
}

// Chan.T.Call
BaseChannelCallType::Adaptee::Adaptee(BaseChannelCallType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelCallType::Adaptee::~Adaptee()
{
}


struct TP_QT_NO_EXPORT BaseChannelCallType::Private {
    Private(BaseChannelCallType *parent, BaseChannel* channel, bool hardwareStreaming,
            uint initialTransport,
            bool initialAudio,
            bool initialVideo,
            QString initialAudioName,
            QString initialVideoName,
            bool mutableContents)
        : hardwareStreaming(hardwareStreaming),
          initialTransport(initialTransport),
          initialAudio(initialAudio),
          initialVideo(initialVideo),
          initialAudioName(initialAudioName),
          initialVideoName(initialVideoName),
          mutableContents(mutableContents),
          channel(channel),
          adaptee(new BaseChannelCallType::Adaptee(parent)) {
    }

    Tp::ObjectPathList contents;
    QVariantMap callStateDetails;
    uint callState;
    uint callFlags;
    Tp::CallStateReason callStateReason;
    bool hardwareStreaming;
    Tp::CallMemberMap callMembers;
    Tp::HandleIdentifierMap memberIdentifiers;
    uint initialTransport;
    bool initialAudio;
    bool initialVideo;
    QString initialAudioName;
    QString initialVideoName;
    bool mutableContents;

    QList<Tp::BaseCallContentPtr> mCallContents;
    AcceptCallback acceptCB;
    HangupCallback hangupCB;
    SetQueuedCallback setQueuedCB;
    SetRingingCallback setRingingCB;
    AddContentCallback addContentCB;

    BaseChannel *channel;
    BaseChannelCallType::Adaptee *adaptee;
};

void BaseChannelCallType::Adaptee::setRinging(const Tp::Service::ChannelTypeCallAdaptor::SetRingingContextPtr &context)
{
    if (!mInterface->mPriv->setRingingCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->setRingingCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelCallType::Adaptee::setQueued(const Tp::Service::ChannelTypeCallAdaptor::SetQueuedContextPtr &context)
{
    if (!mInterface->mPriv->setQueuedCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->setQueuedCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelCallType::Adaptee::accept(const Tp::Service::ChannelTypeCallAdaptor::AcceptContextPtr &context)
{
    if (!mInterface->mPriv->acceptCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->acceptCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelCallType::Adaptee::hangup(uint reason, const QString &detailedHangupReason, const QString &message, const Tp::Service::ChannelTypeCallAdaptor::HangupContextPtr &context)
{
    if (!mInterface->mPriv->hangupCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    DBusError error;
    mInterface->mPriv->hangupCB(reason, detailedHangupReason, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelCallType::Adaptee::addContent(const QString &contentName, const Tp::MediaStreamType &contentType, const Tp::MediaStreamDirection &initialDirection, const Tp::Service::ChannelTypeCallAdaptor::AddContentContextPtr &context)
{
    if (!mInterface->mPriv->addContentCB.isValid()) {
        Tp::BaseCallContentPtr ptr = mInterface->addContent(contentName, contentType, initialDirection);
        QDBusObjectPath objPath;
        objPath.setPath(ptr->objectPath());
        context->setFinished(objPath);
        return;
    }

    DBusError error;
    QDBusObjectPath objPath = mInterface->mPriv->addContentCB(contentName, contentType, initialDirection, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(objPath);
}

/**
 * \class BaseChannelCallType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Type.Call
 *
 */

/**
 * Class constructor.
 */
BaseChannelCallType::BaseChannelCallType(BaseChannel* channel, bool hardwareStreaming,
                                         uint initialTransport,
                                         bool initialAudio,
                                         bool initialVideo,
                                         QString initialAudioName,
                                         QString initialVideoName,
                                         bool mutableContents)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_CALL),
      mPriv(new Private(this, channel,
                        hardwareStreaming,
                        initialTransport,
                        initialAudio,
                        initialVideo,
                        initialAudioName,
                        initialVideoName,
                        mutableContents))
{
}

Tp::ObjectPathList BaseChannelCallType::contents() {
    return mPriv->contents;
}

QVariantMap BaseChannelCallType::callStateDetails() {
    return mPriv->callStateDetails;
}

uint BaseChannelCallType::callState() {
    return mPriv->callState;
}

uint BaseChannelCallType::callFlags() {
    return mPriv->callFlags;
}

Tp::CallStateReason BaseChannelCallType::callStateReason() {
    return mPriv->callStateReason;
}

bool BaseChannelCallType::hardwareStreaming() {
    return mPriv->hardwareStreaming;
}

Tp::CallMemberMap BaseChannelCallType::callMembers() {
    return mPriv->callMembers;
}

Tp::HandleIdentifierMap BaseChannelCallType::memberIdentifiers() {
    return mPriv->memberIdentifiers;
}

uint BaseChannelCallType::initialTransport() {
    return mPriv->initialTransport;
}

bool BaseChannelCallType::initialAudio() {
    return mPriv->initialAudio;
}

bool BaseChannelCallType::initialVideo() {
    return mPriv->initialVideo;
}

QString BaseChannelCallType::initialVideoName() {
    return mPriv->initialVideoName;
}

QString BaseChannelCallType::initialAudioName() {
    return mPriv->initialAudioName;
}

bool BaseChannelCallType::mutableContents() {
    return mPriv->mutableContents;
}

/**
 * Class destructor.
 */
BaseChannelCallType::~BaseChannelCallType()
{
    delete mPriv;
}

QVariantMap BaseChannelCallType::immutableProperties() const
{
    QVariantMap map;

    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".HardwareStreaming"),
               QVariant::fromValue(mPriv->adaptee->hardwareStreaming()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialTransport"),
               QVariant::fromValue(mPriv->adaptee->initialTransport()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
               QVariant::fromValue(mPriv->adaptee->initialAudio()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
               QVariant::fromValue(mPriv->adaptee->initialVideo()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"),
               QVariant::fromValue(mPriv->adaptee->initialAudioName()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"),
               QVariant::fromValue(mPriv->adaptee->initialVideoName()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".MutableContents"),
               QVariant::fromValue(mPriv->adaptee->mutableContents()));
    return map;
}

void BaseChannelCallType::createAdaptor()
{
    (void) new Service::ChannelTypeCallAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

void BaseChannelCallType::setCallState(const Tp::CallState &state, uint flags, const Tp::CallStateReason &stateReason, const QVariantMap &callStateDetails)
{
    mPriv->callState = state;
    mPriv->callFlags = flags;
    mPriv->callStateReason = stateReason;
    mPriv->callStateDetails = callStateDetails;
    QMetaObject::invokeMethod(mPriv->adaptee, "callStateChanged", Q_ARG(uint, state), Q_ARG(uint, flags), Q_ARG(Tp::CallStateReason, stateReason), Q_ARG(QVariantMap, callStateDetails));
}

void BaseChannelCallType::setAcceptCallback(const AcceptCallback &cb)
{
    mPriv->acceptCB = cb;
}

void BaseChannelCallType::setHangupCallback(const HangupCallback &cb)
{
    mPriv->hangupCB = cb;
}

void BaseChannelCallType::setSetRingingCallback(const SetRingingCallback &cb)
{
    mPriv->setRingingCB = cb;
}

void BaseChannelCallType::setSetQueuedCallback(const SetQueuedCallback &cb)
{
    mPriv->setQueuedCB = cb;
}

void BaseChannelCallType::setAddContentCallback(const AddContentCallback &cb)
{
    mPriv->addContentCB = cb;
}

void BaseChannelCallType::setMembersFlags(const Tp::CallMemberMap &flagsChanged, const Tp::HandleIdentifierMap &identifiers, const Tp::UIntList &removed, const Tp::CallStateReason &reason)
{
    mPriv->callMembers = flagsChanged;
    mPriv->memberIdentifiers = identifiers;
    QMetaObject::invokeMethod(mPriv->adaptee, "callMembersChanged", Q_ARG(Tp::CallMemberMap, flagsChanged), Q_ARG(Tp::HandleIdentifierMap, identifiers), Q_ARG(Tp::UIntList, removed), Q_ARG(Tp::CallStateReason, reason));
}

BaseCallContentPtr BaseChannelCallType::addContent(const QString &name, const Tp::MediaStreamType &type, const Tp::MediaStreamDirection &direction)
{
    BaseCallContentPtr ptr = BaseCallContent::create(mPriv->channel->dbusConnection(), mPriv->channel, name, type, direction);
    DBusError error;
    ptr->registerObject(&error);
    QDBusObjectPath objpath;
    objpath.setPath(ptr->objectPath());
    mPriv->contents.append(objpath);
    QMetaObject::invokeMethod(mPriv->adaptee, "contentAdded", Q_ARG(QDBusObjectPath, objpath));

    return ptr;
}

void BaseChannelCallType::addContent(BaseCallContentPtr content)
{
    DBusError error;
    content->registerObject(&error);
    QDBusObjectPath objpath;
    objpath.setPath(content->objectPath());
    mPriv->contents.append(objpath);
    QMetaObject::invokeMethod(mPriv->adaptee, "contentAdded", Q_ARG(QDBusObjectPath, objpath));
}

// Chan.I.Hold
BaseChannelHoldInterface::Adaptee::Adaptee(BaseChannelHoldInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelHoldInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelHoldInterface::Private {
    Private(BaseChannelHoldInterface *parent, Tp::LocalHoldState state)
        : state(state),
          reason(Tp::LocalHoldStateReasonNone),
          adaptee(new BaseChannelHoldInterface::Adaptee(parent)) {
    }

    SetHoldStateCallback setHoldStateCB;
    Tp::LocalHoldState state;
    Tp::LocalHoldStateReason reason;
    BaseChannelHoldInterface::Adaptee *adaptee;
};

void BaseChannelHoldInterface::Adaptee::getHoldState(const Tp::Service::ChannelInterfaceHoldAdaptor::GetHoldStateContextPtr &context)
{
    context->setFinished(mInterface->getHoldState(), mInterface->getHoldReason());
}

void BaseChannelHoldInterface::Adaptee::requestHold(bool hold, const Tp::Service::ChannelInterfaceHoldAdaptor::RequestHoldContextPtr &context)
{
    if (!mInterface->mPriv->setHoldStateCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    Tp::LocalHoldState state = hold ? Tp::LocalHoldStateHeld : Tp::LocalHoldStateUnheld;

    DBusError error;
    mInterface->mPriv->setHoldStateCB(state, Tp::LocalHoldStateReasonRequested, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelHoldInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Hold
 *
 */

/**
 * Class constructor.
 */
BaseChannelHoldInterface::BaseChannelHoldInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD),
      mPriv(new Private(this, Tp::LocalHoldStateUnheld))
{
}

Tp::LocalHoldState BaseChannelHoldInterface::getHoldState() const
{
    return mPriv->state;
}

Tp::LocalHoldStateReason BaseChannelHoldInterface::getHoldReason() const
{
    return mPriv->reason;
}

void BaseChannelHoldInterface::setSetHoldStateCallback(const SetHoldStateCallback &cb)
{
    mPriv->setHoldStateCB = cb;
}

void BaseChannelHoldInterface::setHoldState(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason)
{
    if (mPriv->state != state) {
        mPriv->state = state;
        mPriv->reason = reason;
        QMetaObject::invokeMethod(mPriv->adaptee, "holdStateChanged", Q_ARG(uint, state), Q_ARG(uint, reason));
    }
}

/**
 * Class destructor.
 */
BaseChannelHoldInterface::~BaseChannelHoldInterface()
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
QVariantMap BaseChannelHoldInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelHoldInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceHoldAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


// Chan.I.MergeableConference
BaseChannelMergeableConferenceInterface::Adaptee::Adaptee(BaseChannelMergeableConferenceInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelMergeableConferenceInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelMergeableConferenceInterface::Private {
    Private(BaseChannelMergeableConferenceInterface *parent)
        :adaptee(new BaseChannelMergeableConferenceInterface::Adaptee(parent)) {
    }

    MergeCallback mergeCB;
    BaseChannelMergeableConferenceInterface::Adaptee *adaptee;
};

void BaseChannelMergeableConferenceInterface::Adaptee::merge(const QDBusObjectPath &channelPath, const Tp::Service::ChannelInterfaceMergeableConferenceAdaptor::MergeContextPtr &context)
{
    if (!mInterface->mPriv->mergeCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    DBusError error;
    mInterface->mPriv->mergeCB(channelPath, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelMergeableConferenceInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.MergeableConference
 *
 */

/**
 * Class constructor.
 */
BaseChannelMergeableConferenceInterface::BaseChannelMergeableConferenceInterface()
    : AbstractChannelInterface(TP_QT_FUTURE_IFACE_CHANNEL_INTERFACE_MERGEABLE_CONFERENCE),
      mPriv(new Private(this))
{
}

void BaseChannelMergeableConferenceInterface::setMergeCallback(const MergeCallback &cb)
{
    mPriv->mergeCB = cb;
}

/**
 * Class destructor.
 */
BaseChannelMergeableConferenceInterface::~BaseChannelMergeableConferenceInterface()
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
QVariantMap BaseChannelMergeableConferenceInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelMergeableConferenceInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceMergeableConferenceAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


// Chan.I.Splittable
BaseChannelSplittableInterface::Adaptee::Adaptee(BaseChannelSplittableInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelSplittableInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelSplittableInterface::Private {
    Private(BaseChannelSplittableInterface *parent)
        :adaptee(new BaseChannelSplittableInterface::Adaptee(parent)) {
    }

    SplitCallback splitCB;
    BaseChannelSplittableInterface::Adaptee *adaptee;
};

void BaseChannelSplittableInterface::Adaptee::split(const Tp::Service::ChannelInterfaceSplittableAdaptor::SplitContextPtr &context)
{
    if (!mInterface->mPriv->splitCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    DBusError error;
    mInterface->mPriv->splitCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseChannelSplittableInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Splittable
 *
 */

/**
 * Class constructor.
 */
BaseChannelSplittableInterface::BaseChannelSplittableInterface()
    : AbstractChannelInterface(TP_QT_FUTURE_IFACE_CHANNEL_INTERFACE_SPLITTABLE),
      mPriv(new Private(this))
{
}

void BaseChannelSplittableInterface::setSplitCallback(const SplitCallback &cb)
{
    mPriv->splitCB = cb;
}

/**
 * Class destructor.
 */
BaseChannelSplittableInterface::~BaseChannelSplittableInterface()
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
QVariantMap BaseChannelSplittableInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelSplittableInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceSplittableAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


// Chan.I.Conference
BaseChannelConferenceInterface::Adaptee::Adaptee(BaseChannelConferenceInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelConferenceInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelConferenceInterface::Private {
    Private(BaseChannelConferenceInterface *parent,
            Tp::ObjectPathList initialChannels,
            Tp::UIntList initialInviteeHandles,
            QStringList initialInviteeIDs,
            QString invitationMessage,
            ChannelOriginatorMap originalChannels) :
        channels(initialChannels),
        initialChannels(initialChannels),
        initialInviteeHandles(initialInviteeHandles),
        initialInviteeIDs(initialInviteeIDs),
        invitationMessage(invitationMessage),
        originalChannels(originalChannels),
        adaptee(new BaseChannelConferenceInterface::Adaptee(parent)) {
    }
    Tp::ObjectPathList channels;
    Tp::ObjectPathList initialChannels;
    Tp::UIntList initialInviteeHandles;
    QStringList initialInviteeIDs;
    QString invitationMessage;
    ChannelOriginatorMap originalChannels;

    BaseChannelConferenceInterface::Adaptee *adaptee;
};

/**
 * \class BaseChannelConferenceInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Conference
 *
 */

/**
 * Class constructor.
 */
BaseChannelConferenceInterface::BaseChannelConferenceInterface(Tp::ObjectPathList initialChannels,
    Tp::UIntList initialInviteeHandles,
    QStringList initialInviteeIDs,
    QString invitationMessage,
    ChannelOriginatorMap originalChannels)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE),
      mPriv(new Private(this, initialChannels, initialInviteeHandles, initialInviteeIDs, invitationMessage, originalChannels))
{
}

Tp::ObjectPathList BaseChannelConferenceInterface::channels() const
{
    return mPriv->channels;
}

Tp::ObjectPathList BaseChannelConferenceInterface::initialChannels() const
{
    return mPriv->initialChannels;
}

Tp::UIntList BaseChannelConferenceInterface::initialInviteeHandles() const
{
    return mPriv->initialInviteeHandles;
}

QStringList BaseChannelConferenceInterface::initialInviteeIDs() const
{
    return mPriv->initialInviteeIDs;
}

QString BaseChannelConferenceInterface::invitationMessage() const
{
    return mPriv->invitationMessage;
}

void BaseChannelConferenceInterface::mergeChannel(const QDBusObjectPath &channel, uint channelHandle, const QVariantMap &properties)
{
    mPriv->channels.append(channel);
    if (channelHandle != 0) {
        mPriv->originalChannels[channelHandle] = channel;
    }
    QMetaObject::invokeMethod(mPriv->adaptee, "channelMerged", Q_ARG(QDBusObjectPath, channel), Q_ARG(uint, channelHandle), Q_ARG(QVariantMap, properties));
}


void BaseChannelConferenceInterface::removeChannel(const QDBusObjectPath &channel, const QVariantMap& details)
{
    mPriv->channels.removeAll(channel);
    if (mPriv->originalChannels.values().contains(channel)) {
        mPriv->originalChannels.remove(mPriv->originalChannels.key(channel));
    }
    QMetaObject::invokeMethod(mPriv->adaptee, "channelRemoved", Q_ARG(QDBusObjectPath, channel), Q_ARG(QVariantMap, details));
}

ChannelOriginatorMap BaseChannelConferenceInterface::originalChannels() const
{
    return mPriv->originalChannels;
}

/**
 * Class destructor.
 */
BaseChannelConferenceInterface::~BaseChannelConferenceInterface()
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
QVariantMap BaseChannelConferenceInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelConferenceInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceConferenceAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

// Chan.I.SMS
BaseChannelSMSInterface::Adaptee::Adaptee(BaseChannelSMSInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelSMSInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseChannelSMSInterface::Private {
    Private(BaseChannelSMSInterface *parent, bool flash, bool smsChannel)
        : flash(flash),
        smsChannel(smsChannel),
        adaptee(new BaseChannelSMSInterface::Adaptee(parent))
    {
    }

    bool flash;
    bool smsChannel;
    GetSMSLengthCallback getSMSLengthCB;
    BaseChannelSMSInterface::Adaptee *adaptee;
};

void BaseChannelSMSInterface::Adaptee::getSMSLength(const Tp::MessagePartList & messages, const Tp::Service::ChannelInterfaceSMSAdaptor::GetSMSLengthContextPtr &context)
{
    if (!mInterface->mPriv->getSMSLengthCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    DBusError error;
    mInterface->mPriv->getSMSLengthCB(messages, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    // TODO: implement
    context->setFinished(0,0,0);
}

/**
 * \class BaseChannelSMSInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.SMS
 *
 */

/**
 * Class constructor.
 */
BaseChannelSMSInterface::BaseChannelSMSInterface(bool flash, bool smsChannel)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_SMS),
      mPriv(new Private(this, flash, smsChannel))
{
}

void BaseChannelSMSInterface::setGetSMSLengthCallback(const GetSMSLengthCallback &cb)
{
    mPriv->getSMSLengthCB = cb;
}

/**
 * Class destructor.
 */
BaseChannelSMSInterface::~BaseChannelSMSInterface()
{
    delete mPriv;
}

bool BaseChannelSMSInterface::flash() const
{
    return mPriv->flash;
}

bool BaseChannelSMSInterface::smsChannel() const
{
    return mPriv->smsChannel;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelSMSInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_INTERFACE_SMS + QLatin1String(".Flash"),
               QVariant::fromValue(mPriv->adaptee->flash()));
    return map;
}

void BaseChannelSMSInterface::createAdaptor()
{
    (void) new Service::ChannelInterfaceSMSAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

}
