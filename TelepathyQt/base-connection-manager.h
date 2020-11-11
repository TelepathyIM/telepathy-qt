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

#ifndef _TelepathyQt_base_connection_manager_h_HEADER_GUARD_
#define _TelepathyQt_base_connection_manager_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Callbacks>
#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>

#include <QDBusConnection>

namespace Tp
{

class TP_QT_EXPORT BaseConnectionManager : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionManager)

public:
    static BaseConnectionManagerPtr create(const QString &name)
    {
        return BaseConnectionManagerPtr(new BaseConnectionManager(
                    QDBusConnection::sessionBus(), name));
    }
    template<typename BaseConnectionManagerSubclass>
    static SharedPtr<BaseConnectionManagerSubclass> create(const QString &name)
    {
        return SharedPtr<BaseConnectionManagerSubclass>(new BaseConnectionManagerSubclass(
                    QDBusConnection::sessionBus(), name));
    }
    static BaseConnectionManagerPtr create(const QDBusConnection &dbusConnection,
            const QString &name)
    {
        return BaseConnectionManagerPtr(new BaseConnectionManager(dbusConnection, name));
    }
    template<typename BaseConnectionManagerSubclass>
    static SharedPtr<BaseConnectionManagerSubclass> create(const QDBusConnection &dbusConnection,
            const QString &name)
    {
        return SharedPtr<BaseConnectionManagerSubclass>(new BaseConnectionManagerSubclass(
                    dbusConnection, name));
    }

    ~BaseConnectionManager() override;

    QString name() const;

    QVariantMap immutableProperties() const override;

    QList<BaseProtocolPtr> protocols() const;
    BaseProtocolPtr protocol(const QString &protocolName) const;
    bool hasProtocol(const QString &protocolName) const;
    bool addProtocol(const BaseProtocolPtr &protocol);

    QList<AbstractConnectionManagerInterfacePtr> interfaces() const;
    AbstractConnectionManagerInterfacePtr interface(const QString  &interfaceName) const;
    bool plugInterface(const AbstractConnectionManagerInterfacePtr &interface);

    bool registerObject(DBusError *error = nullptr);

    QList<BaseConnectionPtr> connections() const;

Q_SIGNALS:
    void newConnection(const BaseConnectionPtr &connection);

protected:
    BaseConnectionManager(const QDBusConnection &dbusConnection, const QString &name);

    bool registerObject(const QString &busName, const QString &objectPath,
            DBusError *error) override;

private Q_SLOTS:
    TP_QT_NO_EXPORT void removeConnection();

private:
    TP_QT_NO_EXPORT void addConnection(const BaseConnectionPtr &connection);

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractConnectionManagerInterface : public AbstractDBusServiceInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractConnectionManagerInterface)

public:
    AbstractConnectionManagerInterface(const QString &interfaceName);
    ~AbstractConnectionManagerInterface() override;

protected:
    virtual void setBaseConnectionManager(BaseConnectionManager *manager);

private:
    friend class BaseConnectionManager;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseConnectionManagerAccountApiInterface : public AbstractConnectionManagerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionManagerAccountApiInterface)

public:
    static BaseConnectionManagerAccountApiInterfacePtr create()
    {
        return BaseConnectionManagerAccountApiInterfacePtr(new BaseConnectionManagerAccountApiInterface());
    }

    ~BaseConnectionManagerAccountApiInterface() override;

    using RequestApiContextPtr = MethodInvocationContextPtr<QString, QDBusObjectPath>;
    using RequestApiCallback = Callback1<void, RequestApiContextPtr>;
    void setRequestApiCallback(const RequestApiCallback &cb);

protected:
    BaseConnectionManagerAccountApiInterface();
//    void setBaseConnectionManager(BaseConnectionManager *manager) override;

private:
    void createAdaptor() override;

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}

#endif
