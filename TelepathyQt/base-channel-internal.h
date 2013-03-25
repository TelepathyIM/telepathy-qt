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

#include "TelepathyQt/_gen/svc-channel.h"

#include <TelepathyQt/Global>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/Types>
#include "TelepathyQt/debug-internal.h"


namespace Tp
{

class TP_QT_NO_EXPORT BaseChannel::Adaptee : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString channelType READ channelType)
    Q_PROPERTY(QStringList interfaces READ interfaces)
    Q_PROPERTY(uint targetHandle READ targetHandle)
    Q_PROPERTY(QString targetID READ targetID)
    Q_PROPERTY(uint targetHandleType READ targetHandleType)
    Q_PROPERTY(bool requested READ requested)
    Q_PROPERTY(uint initiatorHandle READ initiatorHandle)
    Q_PROPERTY(QString initiatorID READ initiatorID)
public:
    Adaptee(const QDBusConnection &dbusConnection, BaseChannel *cm);
    ~Adaptee();

    QString channelType() const {
        return mChannel->channelType();
    }
    QStringList interfaces() const;
    uint targetHandle() const {
        return mChannel->targetHandle();
    }
    QString targetID() const {
        return mChannel->targetID();
    }
    uint targetHandleType() const {
        return mChannel->targetHandleType();
    }
    bool requested() const {
        return mChannel->requested();
    }
    uint initiatorHandle() const {
        return mChannel->initiatorHandle();
    }
    QString initiatorID() const {
        return mChannel->initiatorID();
    }

private Q_SLOTS:
    void close(const Tp::Service::ChannelAdaptor::CloseContextPtr &context);
    void getChannelType(const Tp::Service::ChannelAdaptor::GetChannelTypeContextPtr &context) {
        context->setFinished(channelType());
    }

    void getHandle(const Tp::Service::ChannelAdaptor::GetHandleContextPtr &context) {
        context->setFinished(targetHandleType(), targetHandle());
    }

    void getInterfaces(const Tp::Service::ChannelAdaptor::GetInterfacesContextPtr &context) {
        context->setFinished(interfaces());
    }


public:
    BaseChannel *mChannel;
    Service::ChannelAdaptor *mAdaptor;

signals:
    void closed();
};

class TP_QT_NO_EXPORT BaseChannelTextType::Adaptee : public QObject
{
    Q_OBJECT
public:
    Adaptee(BaseChannelTextType *interface);
    ~Adaptee();

public slots:
    void acknowledgePendingMessages(const Tp::UIntList &IDs, const Tp::Service::ChannelTypeTextAdaptor::AcknowledgePendingMessagesContextPtr &context);
//deprecated:
    //void getMessageTypes(const Tp::Service::ChannelTypeTextAdaptor::GetMessageTypesContextPtr &context);
    //void listPendingMessages(bool clear, const Tp::Service::ChannelTypeTextAdaptor::ListPendingMessagesContextPtr &context);
    //void send(uint type, const QString &text, const Tp::Service::ChannelTypeTextAdaptor::SendContextPtr &context);
signals:
    void lostMessage();
    void received(uint ID, uint timestamp, uint sender, uint type, uint flags, const QString &text);
    void sendError(uint error, uint timestamp, uint type, const QString &text);
    void sent(uint timestamp, uint type, const QString &text);

public:
    BaseChannelTextType *mInterface;
};

class TP_QT_NO_EXPORT BaseChannelMessagesInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList supportedContentTypes READ supportedContentTypes)
    Q_PROPERTY(Tp::UIntList messageTypes READ messageTypes)
    Q_PROPERTY(uint messagePartSupportFlags READ messagePartSupportFlags)
    Q_PROPERTY(Tp::MessagePartListList pendingMessages READ pendingMessages)
    Q_PROPERTY(uint deliveryReportingSupport READ deliveryReportingSupport)
public:
    Adaptee(BaseChannelMessagesInterface *interface);
    ~Adaptee();

    QStringList supportedContentTypes() {
        return mInterface->supportedContentTypes();
    }
    Tp::UIntList messageTypes() {
        return mInterface->messageTypes();
    }
    uint messagePartSupportFlags() {
        return mInterface->messagePartSupportFlags();
    }
    uint deliveryReportingSupport() {
        return mInterface->deliveryReportingSupport();
    }
    Tp::MessagePartListList pendingMessages() {
        return mInterface->pendingMessages();
    }

public slots:
    void sendMessage(const Tp::MessagePartList &message, uint flags, const Tp::Service::ChannelInterfaceMessagesAdaptor::SendMessageContextPtr &context);
//deprecated, never implemented:
    //void getPendingMessageContent(uint messageID, const Tp::UIntList &parts, const Tp::Service::ChannelInterfaceMessagesAdaptor::GetPendingMessageContentContextPtr &context);
signals:
    void messageSent(const Tp::MessagePartList &content, uint flags, const QString &messageToken);
    void pendingMessagesRemoved(const Tp::UIntList &messageIDs);
    void messageReceived(const Tp::MessagePartList &message);

public:
    BaseChannelMessagesInterface *mInterface;
};

}
