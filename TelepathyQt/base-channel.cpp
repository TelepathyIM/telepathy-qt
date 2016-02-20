/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
 * @copyright Copyright (C) 2013 Canonical Ltd.
 * @copyright Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>
 * @copyright Copyright (C) 2016 Niels Ole Salscheider <niels_ole@salscheider-online.de>
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
#include "TelepathyQt/_gen/future-constants.h"
#include "TelepathyQt/_gen/future-types.h"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>
#include <TelepathyQt/AbstractProtocolInterface>

#include <QDateTime>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
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
 * \ingroup servicechannel
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

BaseConnection *BaseChannel::connection() const
{
    return mPriv->connection;
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
    foreach(const AbstractChannelInterfacePtr &iface, interfaces()) {
        iface->close();
    }

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
    interface->setBaseChannel(this);
    return true;
}

/**
 * \class AbstractChannelInterface
 * \ingroup servicechannel
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

void AbstractChannelInterface::close()
{
}

void AbstractChannelInterface::setBaseChannel(BaseChannel *channel)
{
    Q_UNUSED(channel)
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
    debug() << "BaseConnectionContactsInterface::acknowledgePendingMessages " << IDs;
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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

// Chan.T.FileTransfer
// The BaseChannelFileTransferType code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseChannelFileTransferType::Private {

    Private(BaseChannelFileTransferType *parent,
            const QVariantMap &request)
        : state(Tp::FileTransferStatePending),
          transferredBytes(0),
          initialOffset(0),
          deviceOffset(0),
          device(0),
          weOpenedDevice(false),
          serverSocket(0),
          clientSocket(0),
          adaptee(new BaseChannelFileTransferType::Adaptee(parent))
    {
        contentType = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentType")).toString();
        filename = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Filename")).toString();
        size = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Size")).toULongLong();
        contentHashType = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHashType")).toUInt();
        contentHash = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHash")).toString();
        description = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Description")).toString();
        qint64 dbusDataValue = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Date")).value<qint64>();
        if (dbusDataValue != 0) {
            date.setTime_t(dbusDataValue);
        }

        if (request.contains(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".URI"))) {
            uri = request.value(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".URI")).toString();
        }

        if (request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")).toBool()) {
            direction = BaseChannelFileTransferType::Outgoing;
        } else {
            direction = BaseChannelFileTransferType::Incoming;
        }
    }

    uint state;
    QString contentType;
    QString filename;
    qulonglong size;
    uint contentHashType;
    QString contentHash;
    QString description;
    QDateTime date;
    qulonglong transferredBytes;
    qulonglong initialOffset;
    qulonglong deviceOffset;
    QString uri;
    QString fileCollection;
    QIODevice *device; // A socket to read or write file to underlying connection manager
    bool weOpenedDevice;
    QTcpServer *serverSocket; // Server socket is an implementation detail.
    QIODevice *clientSocket; // A socket to communicate with a Telepathy client
    BaseChannelFileTransferType::Direction direction;
    BaseChannelFileTransferType::Adaptee *adaptee;

    friend class BaseChannelFileTransferType::Adaptee;

};

BaseChannelFileTransferType::Adaptee::Adaptee(BaseChannelFileTransferType *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelFileTransferType::Adaptee::~Adaptee()
{
}

uint BaseChannelFileTransferType::Adaptee::state() const
{
    return mInterface->state();
}

QString BaseChannelFileTransferType::Adaptee::contentType() const
{
    return mInterface->contentType();
}

QString BaseChannelFileTransferType::Adaptee::filename() const
{
    return mInterface->filename();
}

qulonglong BaseChannelFileTransferType::Adaptee::size() const
{
    return mInterface->size();
}

uint BaseChannelFileTransferType::Adaptee::contentHashType() const
{
    return mInterface->contentHashType();
}

QString BaseChannelFileTransferType::Adaptee::contentHash() const
{
    return mInterface->contentHash();
}

QString BaseChannelFileTransferType::Adaptee::description() const
{
    return mInterface->description();
}

qlonglong BaseChannelFileTransferType::Adaptee::date() const
{
    return mInterface->date().toTime_t();
}

Tp::SupportedSocketMap BaseChannelFileTransferType::Adaptee::availableSocketTypes() const
{
    return mInterface->availableSocketTypes();
}

qulonglong BaseChannelFileTransferType::Adaptee::transferredBytes() const
{
    return mInterface->transferredBytes();
}

qulonglong BaseChannelFileTransferType::Adaptee::initialOffset() const
{
    return mInterface->initialOffset();
}

QString BaseChannelFileTransferType::Adaptee::uri() const
{
    return mInterface->uri();
}

QString BaseChannelFileTransferType::Adaptee::fileCollection() const
{
    return mInterface->fileCollection();
}

void BaseChannelFileTransferType::Adaptee::setUri(const QString &uri)
{
    mInterface->setUri(uri);
}

void BaseChannelFileTransferType::Adaptee::acceptFile(uint addressType, uint accessControl, const QDBusVariant &accessControlParam, qulonglong offset,
        const Tp::Service::ChannelTypeFileTransferAdaptor::AcceptFileContextPtr &context)
{
    debug() << "BaseChannelFileTransferType::Adaptee::acceptFile";

    if (mInterface->mPriv->device) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE, QLatin1String("File transfer can only be started once in the same channel"));
        return;
    }

    DBusError error;
    mInterface->createSocket(addressType, accessControl, accessControlParam, &error);

    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }

    QDBusVariant address = mInterface->socketAddress();

    mInterface->setState(Tp::FileTransferStateAccepted, Tp::FileTransferStateChangeReasonNone);

    mInterface->mPriv->initialOffset = offset;
    QMetaObject::invokeMethod(this, "initialOffsetDefined", Q_ARG(qulonglong, offset));

    context->setFinished(address);
}

void BaseChannelFileTransferType::Adaptee::provideFile(uint addressType, uint accessControl, const QDBusVariant &accessControlParam,
        const Tp::Service::ChannelTypeFileTransferAdaptor::ProvideFileContextPtr &context)
{
    debug() << "BaseChannelFileTransferType::Adaptee::provideFile";

    DBusError error;
    mInterface->createSocket(addressType, accessControl, accessControlParam, &error);

    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }

    QDBusVariant address = mInterface->socketAddress();

    mInterface->tryToOpenAndTransfer();
    context->setFinished(address);
}

/**
 * \class BaseChannelFileTransferType
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class of Channel.Type.FileTransfer channel type.
 *
 * Default implementation currently support only IPv4 and IPv6 sockets with localhost access control.
 *
 * Usage:
 * -# Add FileTransfer to the list of the protocol requestable channel classes.
 * -# Add FileTransfer to the list of the connection requestable channel classes.
 * -# Setup ContactCapabilities interface and ensure that FileTransfer requestable channel class presence matches to
 *    actual local (!) and remote contacts capabilities.
 * -# Implement initial FileTransfer channel support in createChannel callback.
 *     -# The channel of interest are those with channelType TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER.
 *     -# Create BaseChannel and plug BaseChannelFileTransferType interface.
 *     -# If transferInterface->direction() is Outgoing, notify the remote side.
 * -# Implement incoming file request handler:
 *     -# Properly setup the request details, take care on TargetHandle and InitiatorHandle.
 *     -# Call BaseConnection::createChannel() with the details. Do not suppress handler!
 *     -# Use remoteProvideFile() to pass the input device and its offset.
 *     -# transferredBytes property will be updated automatically on bytes written to the client socket.
 * -# Implement "remote side accepted transfer" handler:
 *     -# Use remoteAcceptFile() to pass the requested initial offset and output device.
 *     -# Update transferredBytes property on bytes written to the remote side.
 *
 * Incoming transfer process:
 * -# Connection manager creates not requested channel with ChannelType = TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER and
 *     other properties, such as Filename, Size and ContentType.
 * -# The channel initial state is Pending.
 * -# At any time:
 *     -# Client calls AcceptFile method to configure the socket and request an initial offset. The implementation
 *        calls createSocket(), which should trigger (now or later) a call to setClientSocket() to setup the client
 *        socket. socketAddress() method used to return the socket address. This changes the state to Accepted.
 *     -# The connection manager calls remoteProvideFile() method to pass the input device and it's offset. The device
 *        offset is a number of bytes, already skipped by the device. The interface would skip remaining
 *        initialOffset - deviceOffset bytes.
 *     -# Client connects to the socket and triggers setClientSocket() call.
 * -# The channel state is Open now.
 * -# If the device is already ready to read, or emit readyRead() signal, the interface reads data from the device and
 *    write it to the clientSocket.
 * -# Client socket emit bytesWritten() signal, the interface updates transferredBytes count.
 * -# If transferredBytes == size, then the channel state changes to Completed.
 *    Otherwise the interface waits for further data from the device socket.
 *
 * Outgoing transfer process:
 * -# Client requests a channel with ChannelType = TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER and other properties, such as
 *    Filename, Size and ContentType.
 * -# Connection manager creates the requested channel with initial state Pending.
 * -# Connection manager asks remote contact to accept the transfer.
 * -# At any time:
 *     -# Remote contact accept file, connection manager calls remoteAcceptFile() method to pass the output device
 *        and an initial offset. This changes the state to Accepted.
 *     -# Client calls ProvideFile method to configure a socket. The implementation calls createSocket(), which should
 *        trigger (now or later) a call to setClientSocket() to setup the client socket. socketAddress() method used
 *        to return the socket address.
 *     -# Client connects to the socket and triggers setClientSocket() call.
 * -# The channel state is Open now.
 * -# Client writes data to the socket.
 * -# The clientSocket emits readyRead() signal, the interface reads the data from the clientSocket and write it to the
 *    io device.
 * -# Connection manager calls updates transferredBytes property on actual data write.
 * -# If transferredBytes == size, then the channel state changes to Completed.
 *    Otherwise the interface waits for further data from the client socket.
 *
 * Subclassing:
 * + Reimplement a public virtual method availableSocketTypes() to expose extra socket types.
 * + Overload protected createSocket() method to provide own socket address type, access control and its param
 *   implementation.
 * + Custom createSocket() implementation MUST be paired with custom socketAddress() method implementation.
 * + Use setClientSocket() method to pass the client socket.
 *
 */

/**
 * Class constructor.
 */
BaseChannelFileTransferType::BaseChannelFileTransferType(const QVariantMap &request)
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER),
      mPriv(new Private(this, request))
{
}

bool BaseChannelFileTransferType::createSocket(uint addressType, uint accessControl, const QDBusVariant &accessControlParam, Tp::DBusError *error)
{
    Q_UNUSED(accessControlParam);

    if (accessControl != Tp::SocketAccessControlLocalhost) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Requested access control mechanism is not supported."));
        return false;
    }

    QHostAddress address;

    switch (addressType) {
    case Tp::SocketAddressTypeIPv4:
        address = QHostAddress(QHostAddress::LocalHost);
        break;
    case Tp::SocketAddressTypeIPv6:
        address = QHostAddress(QHostAddress::LocalHostIPv6);
        break;
    default:
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Requested address type is not supported."));
        return false;
    }

    if (mPriv->serverSocket) {
        error->set(TP_QT_ERROR_NOT_AVAILABLE, QLatin1String("File transfer can only be started once in the same channel"));
        return false;
    }

    mPriv->serverSocket = new QTcpServer(this);
    mPriv->serverSocket->setMaxPendingConnections(1);

    connect(mPriv->serverSocket, SIGNAL(newConnection()), this, SLOT(onSocketConnection()));

    bool result =  mPriv->serverSocket->listen(address);
    if (!result) {
        error->set(TP_QT_ERROR_NETWORK_ERROR, mPriv->serverSocket->errorString());
    }

    return result;
}

QDBusVariant BaseChannelFileTransferType::socketAddress() const
{
    if (!mPriv->serverSocket) {
        return QDBusVariant();
    }

    switch (mPriv->serverSocket->serverAddress().protocol()) {
    case QAbstractSocket::IPv4Protocol: {
        SocketAddressIPv4 a;
        a.address = mPriv->serverSocket->serverAddress().toString();
        a.port    = mPriv->serverSocket->serverPort();
        return QDBusVariant(QVariant::fromValue(a));
    }
    case QAbstractSocket::IPv6Protocol: {
        SocketAddressIPv6 a;
        a.address = mPriv->serverSocket->serverAddress().toString();
        a.port    = mPriv->serverSocket->serverPort();
        return QDBusVariant(QVariant::fromValue(a));
    }
    default:
        break;
    }

    return QDBusVariant();
}

void BaseChannelFileTransferType::setTransferredBytes(qulonglong count)
{
    if (mPriv->transferredBytes == count) {
        return;
    }

    mPriv->transferredBytes = count;
    QMetaObject::invokeMethod(mPriv->adaptee, "transferredBytesChanged", Q_ARG(qulonglong, count)); //Can simply use emit in Qt5

    if (transferredBytes() == size()) {
        mPriv->clientSocket->close();
        mPriv->serverSocket->close();
        setState(Tp::FileTransferStateCompleted, Tp::FileTransferStateChangeReasonNone);
    }
}

void BaseChannelFileTransferType::setClientSocket(QIODevice *socket)
{
    mPriv->clientSocket = socket;

    if (!socket) {
        warning() << "BaseChannelFileTransferType::setClientSocket() called with a null socket.";
        return;
    }

    switch (mPriv->direction) {
    case BaseChannelFileTransferType::Outgoing:
        connect(mPriv->clientSocket, SIGNAL(readyRead()), this, SLOT(doTransfer()));
        break;
    case BaseChannelFileTransferType::Incoming:
        connect(mPriv->clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));
        break;
    default:
        // Should not be ever possible
        Q_ASSERT(0);
        break;
    }

    tryToOpenAndTransfer();
}

void BaseChannelFileTransferType::onSocketConnection()
{
    setClientSocket(mPriv->serverSocket->nextPendingConnection());
}

void BaseChannelFileTransferType::doTransfer()
{
    if (!mPriv->clientSocket || !mPriv->device) {
        return;
    }

    QIODevice *input = 0;
    QIODevice *output = 0;

    switch (mPriv->direction) {
    case BaseChannelFileTransferType::Outgoing:
        input = mPriv->clientSocket;
        output = mPriv->device;
        break;
    case BaseChannelFileTransferType::Incoming:
        input = mPriv->device;
        output = mPriv->clientSocket;
        break;
    default:
        // Should not be ever possible
        Q_ASSERT(0);
        break;
    }

    static const int c_blockSize = 16 * 1024;
    char buffer[c_blockSize];
    char *inputPointer = buffer;

    qint64 length = input->read(buffer, sizeof(buffer));

    if (length) {
        // deviceOffset is the number of already skipped bytes
        if (mPriv->deviceOffset + length > initialOffset()) {
            if (mPriv->deviceOffset < initialOffset()) {
                qint64 diff = initialOffset() - mPriv->deviceOffset;
                length -= diff;
                inputPointer += diff;
                mPriv->deviceOffset += diff;
            }
            output->write(inputPointer, length);
        }
        mPriv->deviceOffset += length;
    }

    if (input->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "doTransfer", Qt::QueuedConnection);
    }
}

void BaseChannelFileTransferType::onBytesWritten(qint64 count)
{
    setTransferredBytes(transferredBytes() + count);
}

/**
 * Class destructor.
 */
BaseChannelFileTransferType::~BaseChannelFileTransferType()
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
QVariantMap BaseChannelFileTransferType::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentType"),
               QVariant::fromValue(contentType()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Filename"),
               QVariant::fromValue(filename()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Size"),
               QVariant::fromValue(size()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHashType"),
               QVariant::fromValue(contentHashType()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHash"),
               QVariant::fromValue(contentHash()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Description"),
               QVariant::fromValue(description()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Date"),
               QVariant::fromValue(date().toTime_t()));
    map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".AvailableSocketTypes"),
               QVariant::fromValue(availableSocketTypes()));

    if (mPriv->direction == Outgoing) {
        map.insert(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".URI"), QVariant::fromValue(uri()));
    }

    return map;
}

BaseChannelFileTransferType::Direction BaseChannelFileTransferType::direction() const
{
    return mPriv->direction;
}

uint BaseChannelFileTransferType::state() const
{
    return mPriv->state;
}

void BaseChannelFileTransferType::setState(uint state, uint reason)
{
    if (mPriv->state == state) {
        return;
    }

    mPriv->state = state;
    QMetaObject::invokeMethod(mPriv->adaptee, "fileTransferStateChanged", Q_ARG(uint, state), Q_ARG(uint, reason)); //Can simply use emit in Qt5
    emit stateChanged(state, reason);
}

QString BaseChannelFileTransferType::contentType() const
{
    return mPriv->contentType;
}

QString BaseChannelFileTransferType::filename() const
{
    return mPriv->filename;
}

qulonglong BaseChannelFileTransferType::size() const
{
    return mPriv->size;
}

uint BaseChannelFileTransferType::contentHashType() const
{
    return mPriv->contentHashType;
}

QString BaseChannelFileTransferType::contentHash() const
{
    return mPriv->contentHash;
}

QString BaseChannelFileTransferType::description() const
{
    return mPriv->description;
}

QDateTime BaseChannelFileTransferType::date() const
{
    return mPriv->date;
}

Tp::SupportedSocketMap BaseChannelFileTransferType::availableSocketTypes() const
{
    Tp::SupportedSocketMap types;
    types.insert(Tp::SocketAddressTypeIPv4, Tp::UIntList() << Tp::SocketAccessControlLocalhost);

    return types;
}

qulonglong BaseChannelFileTransferType::transferredBytes() const
{
    return mPriv->transferredBytes;
}

qulonglong BaseChannelFileTransferType::initialOffset() const
{
    return mPriv->initialOffset;
}

QString BaseChannelFileTransferType::uri() const
{
    return mPriv->uri;
}

void BaseChannelFileTransferType::setUri(const QString &uri)
{
    if (mPriv->direction == Outgoing) {
        warning() << "BaseChannelFileTransferType::setUri(): Failed to set URI property for outgoing transfer.";
        return;
    }

    // The property can be written only before AcceptFile.
    if (state() != FileTransferStatePending) {
        warning() << "BaseChannelFileTransferType::setUri(): Failed to set URI property after AcceptFile call.";
        return;
    }

    mPriv->uri = uri;
    QMetaObject::invokeMethod(mPriv->adaptee, "uriDefined", Q_ARG(QString, uri)); //Can simply use emit in Qt5
    emit uriDefined(uri);
}
QString BaseChannelFileTransferType::fileCollection() const
{
    return mPriv->fileCollection;
}

void BaseChannelFileTransferType::setFileCollection(const QString &fileCollection)
{
    mPriv->fileCollection = fileCollection;
}

void BaseChannelFileTransferType::createAdaptor()
{
    (void) new Tp::Service::ChannelTypeFileTransferAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

bool BaseChannelFileTransferType::remoteAcceptFile(QIODevice *output, qulonglong offset)
{
    QString errorText;
    bool deviceIsAlreadynOpened = output && output->isOpen();

    if (!output) {
        errorText = QLatin1String("The device must not be null.");
        goto errorLabel;
    }

    if (mPriv->state != Tp::FileTransferStatePending) {
        errorText = QLatin1String("The state should be Pending.");
        goto errorLabel;
    }

    if (mPriv->direction != Outgoing) {
        errorText = QLatin1String("The direction should be Outgoing.");
        goto errorLabel;
    }

    if (offset > size()) {
        errorText = QLatin1String("The offset should be less than the size.");
        goto errorLabel;
    }

    if (mPriv->device) {
        errorText = QLatin1String("The device is already set.");
        goto errorLabel;
    }

    if (!deviceIsAlreadynOpened) {
        if (!output->open(QIODevice::WriteOnly)) {
            errorText = QLatin1String("Unable to open the device .");
            goto errorLabel;
        }

        if (!output->isSequential()) {
            if (!output->seek(offset)) {
                errorText = QLatin1String("Unable to seek the device to the offset.");
                goto errorLabel;
            }
        }
    }

    if (!output->isWritable()) {
        errorText = QLatin1String("The device is not writable.");
        goto errorLabel;
    }

    if (!errorText.isEmpty()) {
        errorLabel:
        warning() << "BaseChannelFileTransferType::remoteAcceptFile(): Invalid call:" << errorText;
        setState(Tp::FileTransferStateCancelled, Tp::FileTransferStateChangeReasonLocalError);

        return false;
    }

    mPriv->device = output;
    mPriv->deviceOffset = offset;
    mPriv->weOpenedDevice = !deviceIsAlreadynOpened;
    mPriv->initialOffset = offset;

    QMetaObject::invokeMethod(mPriv->adaptee, "initialOffsetDefined", Q_ARG(qulonglong, offset)); //Can simply use emit in Qt5
    setState(Tp::FileTransferStateAccepted, Tp::FileTransferStateChangeReasonNone);

    return true;
}

/*!
 *
 * Connection manager should call this method to pass the input device and its offset.
 * The interface would skip remaining initialOffset - deviceOffset bytes.
 *
 * \param input The input device
 * \param deviceOffset The number of bytes, already skipped by the device.
 *
 * \return True if success, false otherwise.
 */
bool BaseChannelFileTransferType::remoteProvideFile(QIODevice *input, qulonglong deviceOffset)
{
    QString errorText;
    bool deviceIsAlreadyOpened = input && input->isOpen();

    if (!input) {
        errorText = QLatin1String("The device must not be null.");
        goto errorLabel;
    }

    switch (mPriv->state) {
    case Tp::FileTransferStatePending:
    case Tp::FileTransferStateAccepted:
        break;
    default:
        errorText = QLatin1String("The state should be Pending or Accepted.");
        goto errorLabel;
        break;
    }

    if (mPriv->direction != Incoming) {
        errorText = QLatin1String("The direction should be Incoming.");
        goto errorLabel;
    }

    if (deviceOffset > initialOffset()) {
        errorText = QLatin1String("The deviceOffset should be less or equal to the initialOffset.");
        goto errorLabel;
    }

    if (mPriv->device) {
        errorText = QLatin1String("The device is already set.");
        goto errorLabel;
    }

    if (!deviceIsAlreadyOpened) {
        if (!input->open(QIODevice::ReadOnly)) {
            errorText = QLatin1String("Unable to open the device .");
            goto errorLabel;
        }

        if (!input->isSequential()) {
            if (!input->seek(initialOffset())) {
                errorText = QLatin1String("Unable to seek the device to the initial offset.");
                goto errorLabel;
            }
            deviceOffset = initialOffset();
        }
    }

    if (!input->isReadable()) {
        errorText = QLatin1String("The device is not readable.");
        goto errorLabel;
    }

    if (!errorText.isEmpty()) {
        errorLabel:
        warning() << "BaseChannelFileTransferType::remoteProvideFile(): Invalid call:" << errorText;
        setState(Tp::FileTransferStateCancelled, Tp::FileTransferStateChangeReasonLocalError);

        return false;
    }

    mPriv->deviceOffset = deviceOffset;

    mPriv->device = input;
    mPriv->weOpenedDevice = !deviceIsAlreadyOpened;

    connect(mPriv->device, SIGNAL(readyRead()), this, SLOT(doTransfer()));

    tryToOpenAndTransfer();

    return true;
}

void BaseChannelFileTransferType::tryToOpenAndTransfer()
{
    if (state() == Tp::FileTransferStateAccepted) {
        setState(Tp::FileTransferStateOpen, Tp::FileTransferStateChangeReasonNone);
        setTransferredBytes(initialOffset());
    }

    if (state() == Tp::FileTransferStateOpen) {
        if (mPriv->clientSocket && mPriv->device) {
            QMetaObject::invokeMethod(this, "doTransfer", Qt::QueuedConnection);

        }
    }
}

void BaseChannelFileTransferType::close()
{
    uint transferState = state();
    if (transferState == FileTransferStatePending ||
        transferState == FileTransferStateAccepted ||
        transferState == FileTransferStateOpen) {
        // The file transfer was cancelled
        setState(Tp::FileTransferStateCancelled, Tp::FileTransferStateChangeReasonLocalStopped);
    }
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
    debug() << "BaseChannelRoomListType::Adaptee::listRooms";
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
    debug() << "BaseChannelRoomListType::Adaptee::stopListing";
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
    debug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchas";
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
    debug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::getCaptchaData " << ID << mimeType;
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
    debug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::answerCaptchas";
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
    debug() << "BaseChannelCaptchaAuthenticationInterface::Adaptee::cancelCaptcha "
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
 * \ingroup servicechannel
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
    debug() << "BaseChannelSASLAuthenticationInterface::Adaptee::startMechanism";
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
    debug() << "BaseChannelSASLAuthenticationInterface::Adaptee::startMechanismWithData";
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
    debug() << "BaseChannelSASLAuthenticationInterface::Adaptee::respond";
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
    debug() << "BaseChannelSASLAuthenticationInterface::Adaptee::acceptSasl";
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
    debug() << "BaseChannelSASLAuthenticationInterface::Adaptee::abortSasl";
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
    debug() << "BaseChannelChatStateInterface::Adaptee::setChatState";
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
 * \ingroup servicechannel
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

// Chan.I.Group
// The BaseChannelGroupInterface code is fully or partially generated by the TelepathyQt-Generator.
struct TP_QT_NO_EXPORT BaseChannelGroupInterface::Private {
    Private(BaseChannelGroupInterface *parent)
        : connection(0),
          selfHandle(0),
          adaptee(new BaseChannelGroupInterface::Adaptee(parent))
    {
    }

    Tp::UIntList getLocalPendingList() const;
    bool updateMemberIdentifiers();
    void emitMembersChangedSignal(const Tp::UIntList &added, const Tp::UIntList &removed, const Tp::UIntList &localPending, const Tp::UIntList &remotePending, QVariantMap details) const;

    BaseConnection *connection;
    Tp::ChannelGroupFlags groupFlags;
    Tp::HandleOwnerMap handleOwners;
    Tp::LocalPendingInfoList localPendingMembers;
    Tp::UIntList members;
    Tp::UIntList remotePendingMembers;
    uint selfHandle;
    Tp::HandleIdentifierMap memberIdentifiers;
    AddMembersCallback addMembersCB;
    RemoveMembersCallback removeMembersCB;
    BaseChannelGroupInterface::Adaptee *adaptee;
};

BaseChannelGroupInterface::Adaptee::Adaptee(BaseChannelGroupInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseChannelGroupInterface::Adaptee::~Adaptee()
{
}

uint BaseChannelGroupInterface::Adaptee::groupFlags() const
{
    return mInterface->groupFlags();
}

Tp::HandleOwnerMap BaseChannelGroupInterface::Adaptee::handleOwners() const
{
    return mInterface->handleOwners();
}

Tp::LocalPendingInfoList BaseChannelGroupInterface::Adaptee::localPendingMembers() const
{
    return mInterface->localPendingMembers();
}

Tp::UIntList BaseChannelGroupInterface::Adaptee::members() const
{
    return mInterface->members();
}

Tp::UIntList BaseChannelGroupInterface::Adaptee::remotePendingMembers() const
{
    return mInterface->remotePendingMembers();
}

uint BaseChannelGroupInterface::Adaptee::selfHandle() const
{
    return mInterface->selfHandle();
}

Tp::HandleIdentifierMap BaseChannelGroupInterface::Adaptee::memberIdentifiers() const
{
    return mInterface->memberIdentifiers();
}

void BaseChannelGroupInterface::Adaptee::addMembers(const Tp::UIntList &contacts, const QString &message,
        const Tp::Service::ChannelInterfaceGroupAdaptor::AddMembersContextPtr &context)
{
    qDebug() << "BaseChannelGroupInterface::Adaptee::addMembers";
    DBusError error;
    mInterface->addMembers(contacts, message, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelGroupInterface::Adaptee::removeMembers(const Tp::UIntList &contacts, const QString &message,
        const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersContextPtr &context)
{
    qDebug() << "BaseChannelGroupInterface::Adaptee::removeMembers";
    DBusError error;
    mInterface->removeMembers(contacts, message, Tp::ChannelGroupChangeReasonNone, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseChannelGroupInterface::Adaptee::removeMembersWithReason(const Tp::UIntList &contacts, const QString &message, uint reason,
        const Tp::Service::ChannelInterfaceGroupAdaptor::RemoveMembersWithReasonContextPtr &context)
{
    qDebug() << "BaseChannelGroupInterface::Adaptee::removeMembersWithReason";
    DBusError error;
    mInterface->removeMembers(contacts, message, reason, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

UIntList BaseChannelGroupInterface::Private::getLocalPendingList() const
{
    Tp::UIntList localPending;

    foreach (const Tp::LocalPendingInfo &info, localPendingMembers) {
        localPending << info.toBeAdded;
    }

    return localPending;
}

bool BaseChannelGroupInterface::Private::updateMemberIdentifiers()
{
    Tp::UIntList handles = members + remotePendingMembers + handleOwners.values();
    handles << selfHandle;

    foreach (const Tp::LocalPendingInfo &info, localPendingMembers) {
        handles << info.toBeAdded;
        if (info.actor && !handles.contains(info.actor)) {
            handles << info.actor;
        }
    }

    Tp::DBusError error;
    const QStringList identifiers = connection->inspectHandles(Tp::HandleTypeContact, handles, &error);

    if (error.isValid() || (handles.count() != identifiers.count())) {
        return false;
    }

    memberIdentifiers.clear();

    for (int i = 0; i < identifiers.count(); ++i) {
        memberIdentifiers[handles.at(i)] = identifiers.at(i);
    }
    return true;
}

void BaseChannelGroupInterface::Private::emitMembersChangedSignal(const UIntList &added, const UIntList &removed, const UIntList &localPending, const UIntList &remotePending, QVariantMap details) const
{
    const uint actor = details.value(QLatin1String("actor"), 0).toUInt();
    const uint reason = details.value(QLatin1String("change-reason"), Tp::ChannelGroupChangeReasonNone).toUInt();
    const QString message = details.value(QLatin1String("message")).toString();

    QMetaObject::invokeMethod(adaptee, "membersChanged",
                              Q_ARG(QString, message),
                              Q_ARG(Tp::UIntList, added),
                              Q_ARG(Tp::UIntList, removed),
                              Q_ARG(Tp::UIntList, localPending),
                              Q_ARG(Tp::UIntList, remotePending),
                              Q_ARG(uint, actor), Q_ARG(uint, reason)); //Can simply use emit in Qt5

    if (!details.contains(QLatin1String("contact-ids"))) {
        HandleIdentifierMap contactIds;
        foreach (uint handle, added + localPending + remotePending) {
            contactIds[handle] = memberIdentifiers[handle];
        }
        details.insert(QLatin1String("contact-ids"), QVariant::fromValue(contactIds));
    }

    QMetaObject::invokeMethod(adaptee, "membersChangedDetailed",
                              Q_ARG(Tp::UIntList, added),
                              Q_ARG(Tp::UIntList, removed),
                              Q_ARG(Tp::UIntList, localPending),
                              Q_ARG(Tp::UIntList, remotePending),
                              Q_ARG(QVariantMap, details));
}

/**
 * \class BaseChannelGroupInterface
 * \ingroup servicechannel
 * \headerfile TelepathyQt/base-channel.h <TelepathyQt/BaseChannel>
 *
 * \brief Base class for implementations of Channel.Interface.Group.
 *
 * Interface for channels which have multiple members, and where the members of the channel can
 * change during its lifetime. Your presence in the channel cannot be presumed by the channel's
 * existence (for example, a channel you may request membership of but your request may not be
 * granted).
 *
 * This interface implements three lists: a list of current members, and two lists of local
 * pending and remote pending members. Contacts on the remote pending list have been invited to
 * the channel, but the remote user has not accepted the invitation. Contacts on the local pending
 * list have requested membership of the channel, but the local user of the framework must accept
 * their request before they may join. A single contact should never appear on more than one of
 * the three lists. The lists are empty when the channel is created, and the MembersChanged signal
 * (and, if the channel's GroupFlags contains Tp::ChannelGroupFlagMembersChangedDetailed, the
 * MembersChangedDetailed signal) should be emitted when information is retrieved from the server,
 * or changes occur.
 *
 * Addition of members to the channel may be requested by using AddMembers. If remote
 * acknowledgement is required, use of the AddMembers method will cause users to appear on the
 * remote pending list. If no acknowledgement is required, AddMembers will add contacts to the
 * member list directly. If a contact is awaiting authorisation on the local pending list,
 * AddMembers will grant their membership request.
 *
 * Removal of contacts from the channel may be requested by using RemoveMembers. If a contact is
 * awaiting authorisation on the local pending list, RemoveMembers will refuse their membership
 * request. If a contact is on the remote pending list but has not yet accepted the invitation,
 * RemoveMembers will rescind the request if possible.
 *
 * It should not be presumed that the requester of a channel implementing this interface is
 * immediately granted membership, or indeed that they are a member at all, unless they appear
 * in the list. They may, for instance, be placed into the remote pending list until a connection
 * has been established or the request acknowledged remotely.
 *
 * If the local user joins a Group channel whose members or other state cannot be discovered until
 * the user joins (e.g. many chat room implementations), the connection manager should ensure that
 * the channel is, as far as possible, in a consistent state before adding the local contact to the
 * members set; until this happens, the local contact should be in the remote-pending set. For
 * instance, if the connection manager queries the server to find out the initial members list for
 * the channel, it should leave the local contact in the remote-pending set until it has finished
 * receiving the initial members list.
 *
 * If the protocol provides no reliable way to tell whether the complete initial members list has
 * been received yet, the connection manager should make a best-effort attempt to wait for the full
 * list (in the worst case, waiting for a suitable arbitrary timeout) rather than requiring user
 * interfaces to do so on its behalf.
 *
 * Minimal implementation of the interface should setup group flags (setGroupFlags()) and have
 * a setMembers() call. If the selfHandle is present in the group, then the setSelfHandle() should
 * be used to correctly setup the interface. Regardless of the group flags, the connection manager
 * implementation should setup removeMembers callback in order to let client leave the group
 * gracefully. If doing so fails with Tp::ChannelGroupChangeReasonPermissionDenied, this is
 * considered to a bug in the connection manager, but clients MUST recover by falling back to
 * closing the channel with the Close method.
 *
 * Depending on the protocol capabilities, addMembers() and removeMembers() callbacks can be setup
 * to support group members addition, invitation and removal.
 *
 * Note, that the interface automatically update the MemberIdentifiers property on members changes.
 *
 * \sa setGroupFlags(), setSelfHandle(), setMembers(), setAddMembersCallback(),
 * setRemoveMembersCallback(), setHandleOwners(),
 * setLocalPendingMembers(), setRemotePendingMembers()
 */

/**
 * Class constructor.
 */
BaseChannelGroupInterface::BaseChannelGroupInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseChannelGroupInterface::~BaseChannelGroupInterface()
{
    delete mPriv;
}

void BaseChannelGroupInterface::setBaseChannel(BaseChannel *channel)
{
    mPriv->connection = channel->connection();
}

/**
 * Return the immutable properties of this interface.
 *
 * There is no immutable properties presented on the interface.
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseChannelGroupInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

/**
 * Return the flags on this channel.
 *
 * The user interface can use this property to present information about which operations
 * are currently valid.
 *
 * \return An the flags on this channel.
 *
 * \sa setGroupFlags()
 * \sa Tp::ChannelGroupFlag
 */
Tp::ChannelGroupFlags BaseChannelGroupInterface::groupFlags() const
{
    return mPriv->groupFlags | Tp::ChannelGroupFlagProperties | Tp::ChannelGroupFlagMembersChangedDetailed;
}

/**
 * Set the group flags for this channel.
 *
 * The user interface can use this to present information about which operations are currently valid.
 * Take a note, that Tp::ChannelGroupFlagProperties and Tp::ChannelGroupFlagMembersChangedDetailed flags setted up
 * unconditionally. This way we always provide modern properties (ChannelGroupFlagProperties) and automatically
 * emit signal MembersChangedDetailed. There is no reason to behave differently and this improve compatibility with
 * future Telepathy specs.
 *
 * \param flags The flags on this channel.
 *
 * \sa groupFlags()
 * \sa Tp::ChannelGroupFlag
 */
void BaseChannelGroupInterface::setGroupFlags(const Tp::ChannelGroupFlags &flags)
{
    const Tp::ChannelGroupFlags keptFlags = mPriv->groupFlags & flags;
    const Tp::ChannelGroupFlags added     = flags & ~keptFlags;
    const Tp::ChannelGroupFlags removed   = mPriv->groupFlags & ~keptFlags;

    mPriv->groupFlags = flags;
    QMetaObject::invokeMethod(mPriv->adaptee, "groupFlagsChanged", Q_ARG(uint, added), Q_ARG(uint, removed)); //Can simply use emit in Qt5
}

/**
 * Return a list of this channel members.
 *
 * The members of this channel.
 *
 * \return A list of this channel members.
 */
Tp::UIntList BaseChannelGroupInterface::members() const
{
    return mPriv->members;
}

/**
 * Set the list of current members of the channel.
 *
 * Set the list of current members. Added members would be automatically removed from the local
 * and remote pending lists.
 *
 * \param members The actual list of members of the channel.
 * \param details The map with an information about the change.
 *
 * \sa members()
 * \sa setMembers(const UIntList &, const Tp::LocalPendingInfoList &, const Tp::UIntList &, const QVariantMap &)
 * \sa localPendingMembers()
 * \sa remotePendingMembers()
 */
void BaseChannelGroupInterface::setMembers(const UIntList &members, const QVariantMap &details)
{
    Tp::UIntList localPendingList = mPriv->getLocalPendingList();

    Tp::UIntList added;
    foreach (uint handle, members) {
        if (!mPriv->members.contains(handle)) {
            added << handle;

            // Remove added member from the local pending list
            int indexInLocalPending = localPendingList.indexOf(handle);

            if (indexInLocalPending >= 0) {
                localPendingList.removeAt(indexInLocalPending);
                mPriv->localPendingMembers.removeAt(indexInLocalPending);
            }

            // Remove added member from the remote pending list
            int indexInRemotePending = mPriv->remotePendingMembers.indexOf(handle);

            if (indexInRemotePending >= 0) {
                mPriv->remotePendingMembers.removeAt(indexInRemotePending);
            }
        }
    }

    Tp::UIntList removed;
    foreach (uint handle, mPriv->members) {
        if (!members.contains(handle)) {
            removed << handle;
        }
    }

    mPriv->members = members;

    mPriv->updateMemberIdentifiers();
    mPriv->emitMembersChangedSignal(added, removed, localPendingList, mPriv->remotePendingMembers, details);
}

/**
 * Set the list of members of the channel.
 *
 * Set the list of current members and update local and remote pending lists at the same time.
 *
 * \param members The actual list of members of the channel.
 * \param localPending The actual list of local pending members of the channel.
 * \param remotePending The actual list of remote pending members of the channel.
 * \param details The map with an information about the change.
 *
 * \sa members()
 * \sa setMembers(const UIntList &, const QVariantMap &)
 * \sa localPendingMembers()
 * \sa remotePendingMembers()
 */
void BaseChannelGroupInterface::setMembers(const Tp::UIntList &members, const Tp::LocalPendingInfoList &localPending, const Tp::UIntList &remotePending, const QVariantMap &details)
{
    Tp::UIntList added;
    foreach (uint handle, members) {
        if (!mPriv->members.contains(handle)) {
            added << handle;
        }
    }

    Tp::UIntList removed;
    foreach (uint handle, mPriv->members) {
        if (!members.contains(handle)) {
            removed << handle;
        }
    }

    // Do not use the setters here to avoid signal duplication
    mPriv->localPendingMembers = localPending;
    mPriv->remotePendingMembers = remotePending;
    mPriv->members = members;

    mPriv->updateMemberIdentifiers();
    mPriv->emitMembersChangedSignal(added, removed, mPriv->getLocalPendingList(), remotePending, details);
}

/**
 * Return a map from channel-specific handles to their owners.
 *
 * An integer representing the bitwise-OR of flags on this channel. The user interface
 * can use this property to present information about which operations are currently valid.
 *
 * \return A map from channel-specific handles to their owners.
 */
Tp::HandleOwnerMap BaseChannelGroupInterface::handleOwners() const
{
    return mPriv->handleOwners;
}

/**
 * Set a map from channel-specific handles to their owners.
 *
 * A map from channel-specific handles to their owners, including at least all of the
 * channel-specific handles in this channel's members, local-pending or remote-pending
 * sets as keys. Any handle not in the keys of this mapping is not channel-specific in
 * this channel. Handles which are channel-specific, but for which the owner is unknown,
 * MUST appear in this mapping with 0 as owner.
 *
 * \param handleOwners The new (actual) handle owners map.
 *
 * \sa handleOwners(), members(), localPendingMembers(), remotePendingMembers()
 */
void BaseChannelGroupInterface::setHandleOwners(const Tp::HandleOwnerMap &handleOwners)
{
    Tp::HandleOwnerMap added;
    Tp::UIntList removed;

    foreach (uint ownerHandle, mPriv->handleOwners.keys()) {
        if (!handleOwners.contains(ownerHandle)) {
            removed << ownerHandle;
        }
    }

    foreach (uint ownerHandle, handleOwners.keys()) {
        if (!mPriv->handleOwners.contains(ownerHandle)) {
            added[ownerHandle] = handleOwners.value(ownerHandle);
        }
    }

    mPriv->handleOwners = handleOwners;
    mPriv->updateMemberIdentifiers();

    Tp::HandleIdentifierMap identifiers;

    foreach (uint ownerHandle, added) {
        identifiers[ownerHandle] = mPriv->memberIdentifiers.value(ownerHandle);
    }

    QMetaObject::invokeMethod(mPriv->adaptee, "handleOwnersChanged", Q_ARG(Tp::HandleOwnerMap, added), Q_ARG(Tp::UIntList, removed)); //Can simply use emit in Qt5
    QMetaObject::invokeMethod(mPriv->adaptee, "handleOwnersChangedDetailed", Q_ARG(Tp::HandleOwnerMap, added), Q_ARG(Tp::UIntList, removed), Q_ARG(Tp::HandleIdentifierMap, identifiers)); //Can simply use emit in Qt5
}

/**
 * Return an array of contacts requesting channel membership
 *
 * An array of structs containing handles representing contacts requesting channel
 * membership and awaiting local approval with AddMembers call.
 *
 * \return An array of contacts requesting channel membership
 */
Tp::LocalPendingInfoList BaseChannelGroupInterface::localPendingMembers() const
{
    return mPriv->localPendingMembers;
}

/**
 * Set local pending members information list.
 *
 * This method is recommended to use for the local pending members list changes.
 * If the change affect the list and members list, use setMembers() instead.
 *
 * \param localPendingMembers
 *
 * \sa localPendingMembers(), setRemotePendingMembers(), setMembers()
 */
void BaseChannelGroupInterface::setLocalPendingMembers(const Tp::LocalPendingInfoList &localPendingMembers)
{
    mPriv->localPendingMembers = localPendingMembers;
    mPriv->updateMemberIdentifiers();

    uint actor = 0;
    uint reason = Tp::ChannelGroupChangeReasonNone;
    QString message;
    Tp::UIntList localPending;

    Tp::HandleIdentifierMap contactIds;

    if (!localPendingMembers.isEmpty()) {
        actor = localPendingMembers.first().actor;
        reason = localPendingMembers.first().reason;
        message = localPendingMembers.first().message;

        foreach (const Tp::LocalPendingInfo &info, localPendingMembers) {
            localPending << info.toBeAdded;

            if (actor != info.actor) {
                actor = 0;
            }

            if (reason != info.reason) {
                reason = 0;
            }

            if (message != info.message) {
                message.clear();
            }

            contactIds[info.toBeAdded] = mPriv->memberIdentifiers.value(info.toBeAdded);
        }
    }

    QVariantMap details;
    details.insert(QLatin1String("actor"), QVariant::fromValue(actor));
    details.insert(QLatin1String("change-reason"), QVariant::fromValue((uint)reason));
    details.insert(QLatin1String("contact-ids"), QVariant::fromValue(contactIds));
    details.insert(QLatin1String("message"), QVariant::fromValue(message));

    mPriv->emitMembersChangedSignal(/* addedMembers */ Tp::UIntList(), /* removedMembers */ Tp::UIntList(), localPending, mPriv->remotePendingMembers, details);
}

/**
 * Return an array of contacts requesting channel membership
 *
 * An array of handles representing contacts who have been invited
 * to the channel and are awaiting remote approval.
 *
 * \return An array of contacts requesting channel membership
 *
 * \sa setRemotePendingMembers()
 */
Tp::UIntList BaseChannelGroupInterface::remotePendingMembers() const
{
    return mPriv->remotePendingMembers;
}

/**
 * Set an array of contacts requesting channel membership
 *
 * An array of handles representing contacts who have been invited
 * to the channel and are awaiting remote approval.
 *
 * This method is recommended to use for the remote pending members list changes.
 * If the change affect the list and members list, use setMembers() instead.
 *
 * \param remotePendingMembers An array of contacts requesting channel membership
 *
 * \sa remotePendingMembers(), setLocalPendingMembers(), setMembers()
 */
void BaseChannelGroupInterface::setRemotePendingMembers(const Tp::UIntList &remotePendingMembers)
{
    mPriv->remotePendingMembers = remotePendingMembers;

    mPriv->updateMemberIdentifiers();
    mPriv->emitMembersChangedSignal(/* addedMembers */ Tp::UIntList(), /* removedMembers */ Tp::UIntList(), mPriv->getLocalPendingList(), mPriv->remotePendingMembers, /* details */ QVariantMap());
}

/**
 * Return the handle of the user on this channel.
 *
 * See setSelfHandle() for details.
 *
 * \return The handle of the user on this channel.
 */
uint BaseChannelGroupInterface::selfHandle() const
{
    return mPriv->selfHandle;
}

/**
 * Set the handle for the user on this channel.
 *
 * Set the handle for the user on this channel (which can also be a local or remote pending member),
 * or 0 if the user is not a member at all (which is likely to be the case, for instance, on
 * ContactList channels). Note that this is different from the result of Tp::Connection::selfHandle()
 * on some protocols, so the value of this handle should always be used with the methods of this interface.
 *
 * \sa selfHandle()
 */
void BaseChannelGroupInterface::setSelfHandle(uint selfHandle)
{
    mPriv->selfHandle = selfHandle;

    // selfHandleChanged is deprecated since 0.23.4.
    QMetaObject::invokeMethod(mPriv->adaptee, "selfHandleChanged", Q_ARG(uint, selfHandle)); //Can simply use emit in Qt5

    if (mPriv->connection) {
        DBusError error;
        QStringList selfID = mPriv->connection->inspectHandles(Tp::HandleTypeContact, Tp::UIntList() << selfHandle, &error);

        if (!selfID.isEmpty()) {
            QMetaObject::invokeMethod(mPriv->adaptee, "selfContactChanged", Q_ARG(uint, selfHandle), Q_ARG(QString, selfID.first())); //Can simply use emit in Qt5
        }
    }
}

/**
 * Return the string identifiers for handles mentioned in this channel.
 *
 * The string identifiers for handles mentioned in this channel, to give clients
 * the minimal information necessary to create contacts without waiting for round-trips.
 *
 * The property provided by the interface itself and based on selfHandle(), members(),
 * localPendingMembers(), remotePendingMembers() and handleOwners() values.
 *
 * \return The string identifiers for handles mentioned in this channel.
 */
Tp::HandleIdentifierMap BaseChannelGroupInterface::memberIdentifiers() const
{
    return mPriv->memberIdentifiers;
}

void BaseChannelGroupInterface::createAdaptor()
{
    (void) new Tp::Service::ChannelInterfaceGroupAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

/**
 * Set a callback that will be called to add members to the group.
 *
 * Invite all the given contacts into the channel, or accept requests for channel membership
 * for contacts on the pending local list.
 *
 * A message may be provided along with the request, which will be sent to the server if supported.
 * See the Tp::ChannelGroupFlag to find out in which cases the message should be provided.
 *
 * Attempting to add contacts who are already members is allowed; connection managers must silently
 * accept this, without error.
 *
 * \param cb The callback to set.
 * \sa addMembers()
 * \sa setRemoveMembersCallback()
 */
void BaseChannelGroupInterface::setAddMembersCallback(const AddMembersCallback &cb)
{
    mPriv->addMembersCB = cb;
}

/**
 * Call the AddMembers callback with passed arguments.
 *
 * \param contacts An array of contact handles to invite to the channel.
 * \param message A string message, which can be blank if desired.
 * \param error A pointer to Tp::DBusError object for error information.
 *
 * \sa setAddMembersCallback()
 */
void BaseChannelGroupInterface::addMembers(const Tp::UIntList &contacts, const QString &message, DBusError *error)
{
    if (!mPriv->addMembersCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->addMembersCB(contacts, message, error);
}

/**
 * Set a callback that will be called to remove members from the group with a reason.
 *
 * %Connection manager should setup this callback to support requests for: the removal contacts from
 * the channel, reject their request for channel membership on the pending local list, or rescind
 * their invitation on the pending remote list.
 *
 * If the SelfHandle is in the group, it can be removed via this method, in order to leave the
 * group gracefully. This is the recommended way to leave a chatroom, close or reject a call, and
 * so on.
 *
 * Accordingly, connection managers SHOULD support doing this, regardless of the value of
 * GroupFlags. If doing so fails with PermissionDenied, this is considered to a bug in the
 * connection manager, but clients MUST recover by falling back to closing the channel with the
 * Close method.
 *
 * Removing any contact from the local pending list is always allowed. Removing contacts other than
 * the SelfHandle from the channel's members is allowed if and only if Tp::ChannelGroupFlagCanRemove
 * is in the groupFlags(), while removing contacts other than the SelfHandle from the remote pending
 * list is allowed if and only if Tp::ChannelGroupFlagCanRescind is in the groupFlags().
 *
 * A message may be provided along with the request, which will be sent to the server if supported.
 * See the Tp::ChannelGroupFlag to find out in which cases the message should be provided.
 *
 * The reason code may be ignored if the underlying protocol is unable to represent the given reason.
 *
 * \param cb The callback to set.
 *
 * \sa removeMembers()
 * \sa setRemoveMembersCallback()
 */
void BaseChannelGroupInterface::setRemoveMembersCallback(const RemoveMembersCallback &cb)
{
    mPriv->removeMembersCB = cb;
}

/**
 * Call the RemoveMembers callback with passed arguments.
 *
 * \param contacts An array of contact handles to remove from the channel.
 * \param message A string message, which can be blank if desired.
 * \param reason A reason for the change.
 * \param error A pointer to Tp::DBusError object for error information.
 *
 * \sa setRemoveMembersCallback()
 */
void BaseChannelGroupInterface::removeMembers(const Tp::UIntList &contacts, const QString &message, uint reason, DBusError *error)
{
    if (!mPriv->removeMembersCB.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    return mPriv->removeMembersCB(contacts, message, reason, error);
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

qlonglong BaseChannelRoomInterface::Adaptee::creationTimestamp() const
{
    return mInterface->creationTimestamp().toTime_t();
}

/**
 * \class BaseChannelRoomInterface
 * \ingroup servicechannel
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
    debug() << "BaseChannelRoomConfigInterface::Adaptee::updateConfiguration";
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
 * \ingroup servicechannel
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
