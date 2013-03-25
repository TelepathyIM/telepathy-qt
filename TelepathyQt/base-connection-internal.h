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

#include "TelepathyQt/_gen/svc-connection.h"

#include <TelepathyQt/Global>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/Types>
#include "TelepathyQt/debug-internal.h"

namespace Tp
{

class TP_QT_NO_EXPORT BaseConnection::Adaptee : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList interfaces READ interfaces)
    Q_PROPERTY(uint selfHandle READ selfHandle)
    Q_PROPERTY(uint status READ status)
    Q_PROPERTY(bool hasImmortalHandles READ hasImmortalHandles)
public:
    Adaptee(const QDBusConnection &dbusConnection, BaseConnection *cm);
    ~Adaptee();

    QStringList interfaces() const;
    uint status() const {
        return mConnection->status();
    }
    uint selfHandle() const;
    bool hasImmortalHandles() const {
        return true;
    }

private Q_SLOTS:
    void getSelfHandle(const Tp::Service::ConnectionAdaptor::GetSelfHandleContextPtr &context);
    void getStatus(const Tp::Service::ConnectionAdaptor::GetStatusContextPtr &context);
    void connect(const Tp::Service::ConnectionAdaptor::ConnectContextPtr &context);
    void getInterfaces(const Tp::Service::ConnectionAdaptor::GetInterfacesContextPtr &context) {
        context->setFinished(interfaces());
    }

    void getProtocol(const Tp::Service::ConnectionAdaptor::GetProtocolContextPtr &context) {
        context->setFinished(QLatin1String("whosthere"));
    }

    void holdHandles(uint handleType, const Tp::UIntList &handles, const Tp::Service::ConnectionAdaptor::HoldHandlesContextPtr &context) {
        context->setFinished();
    }

    void inspectHandles(uint handleType, const Tp::UIntList &handles, const Tp::Service::ConnectionAdaptor::InspectHandlesContextPtr &context);

    void listChannels(const Tp::Service::ConnectionAdaptor::ListChannelsContextPtr &context) {
        context->setFinished(mConnection->channelsInfo());
    }

    void disconnect(const Tp::Service::ConnectionAdaptor::DisconnectContextPtr &context);

    //void releaseHandles(uint handleType, const Tp::UIntList &handles, const Tp::Service::ConnectionAdaptor::ReleaseHandlesContextPtr &context);
    void requestChannel(const QString &type, uint handleType, uint handle, bool suppressHandler, const Tp::Service::ConnectionAdaptor::RequestChannelContextPtr &context);
    void requestHandles(uint handleType, const QStringList &identifiers, const Tp::Service::ConnectionAdaptor::RequestHandlesContextPtr &context);
    //void addClientInterest(const QStringList &tokens, const Tp::Service::ConnectionAdaptor::AddClientInterestContextPtr &context);
    //void removeClientInterest(const QStringList &tokens, const Tp::Service::ConnectionAdaptor::RemoveClientInterestContextPtr &context);

public:
    BaseConnection *mConnection;
    Service::ConnectionAdaptor *mAdaptor;

Q_SIGNALS:
    void selfHandleChanged(uint selfHandle);
    void newChannel(const QDBusObjectPath &objectPath, const QString &channelType, uint handleType, uint handle, bool suppressHandler);
    void connectionError(const QString &error, const QVariantMap &details);
    void statusChanged(uint status, uint reason);
};

}
