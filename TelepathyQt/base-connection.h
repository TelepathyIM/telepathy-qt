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

#ifndef _TelepathyQt_base_connection_h_HEADER_GUARD_
#define _TelepathyQt_base_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>
#include <TelepathyQt/Callbacks>

#include <QDBusConnection>

class QString;

namespace Tp
{

class TP_QT_EXPORT BaseConnection : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnection)

public:
    static BaseConnectionPtr create(const QString &cmName, const QString &protocolName,
                                    const QVariantMap &parameters) {
        return BaseConnectionPtr(new BaseConnection(
                                     QDBusConnection::sessionBus(), cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QString &cmName,
            const QString &protocolName, const QVariantMap &parameters) {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    QDBusConnection::sessionBus(), cmName, protocolName, parameters));
    }
    static BaseConnectionPtr create(const QDBusConnection &dbusConnection,
                                    const QString &cmName, const QString &protocolName,
                                    const QVariantMap &parameters) {
        return BaseConnectionPtr(new BaseConnection(
                                     dbusConnection, cmName, protocolName, parameters));
    }
    template<typename BaseConnectionSubclass>
    static SharedPtr<BaseConnectionSubclass> create(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters) {
        return SharedPtr<BaseConnectionSubclass>(new BaseConnectionSubclass(
                    dbusConnection, cmName, protocolName, parameters));
    }

    virtual ~BaseConnection();

    QString cmName() const;
    QString protocolName() const;
    QVariantMap parameters() const;
    uint status() const;
    QVariantMap immutableProperties() const;

    void setStatus(uint newStatus, uint reason);

    typedef Callback4<BaseChannelPtr, const QString&, uint, uint, DBusError*> CreateChannelCallback;
    void setCreateChannelCallback(const CreateChannelCallback &cb);
    Tp::BaseChannelPtr createChannel(const QString &channelType, uint targetHandleType, uint targetHandle, uint initiatorHandle, bool suppressHandler, DBusError *error);

    typedef Callback3<UIntList, uint, const QStringList&, DBusError*> RequestHandlesCallback;
    void setRequestHandlesCallback(const RequestHandlesCallback &cb);
    UIntList requestHandles(uint handleType, const QStringList &identifiers, DBusError* error);

    //typedef Callback3<uint, const QString&, const QString&, DBusError*> SetPresenceCallback;
    //void setSetPresenceCallback(const SetPresenceCallback &cb);

    void setSelfHandle(uint selfHandle);
    uint selfHandle() const;

    typedef Callback1<void, DBusError*> ConnectCallback;
    void setConnectCallback(const ConnectCallback &cb);

    typedef Callback3<QStringList, uint, const Tp::UIntList&, DBusError*> InspectHandlesCallback;
    void setInspectHandlesCallback(const InspectHandlesCallback &cb);

    Tp::ChannelInfoList channelsInfo();
    Tp::ChannelDetailsList channelsDetails();

    BaseChannelPtr ensureChannel(const QString &channelType, uint targetHandleType,
                                 uint targetHandle, bool &yours, uint initiatorHandle, bool suppressHandler, DBusError *error);
    void addChannel(BaseChannelPtr channel);

    QList<AbstractConnectionInterfacePtr> interfaces() const;
    AbstractConnectionInterfacePtr interface(const QString  &interfaceName) const;
    bool plugInterface(const AbstractConnectionInterfacePtr &interface);

    virtual QString uniqueName() const;
    bool registerObject(DBusError *error = NULL);

Q_SIGNALS:
    void disconnected();

private Q_SLOTS:
    TP_QT_NO_EXPORT void removeChannel();

protected:
    BaseConnection(const QDBusConnection &dbusConnection,
                   const QString &cmName, const QString &protocolName,
                   const QVariantMap &parameters);

    virtual bool registerObject(const QString &busName, const QString &objectPath,
                                DBusError *error);

private:
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractConnectionInterface : public AbstractDBusServiceInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractConnectionInterface)

public:
    AbstractConnectionInterface(const QString &interfaceName);
    virtual ~AbstractConnectionInterface();

private:
    friend class BaseConnection;

    class Private;
    friend class Private;
    Private *mPriv;
};

}

#endif
