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

#ifndef _TelepathyQt_base_protocol_h_HEADER_GUARD_
#define _TelepathyQt_base_protocol_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/AvatarSpec>
#include <TelepathyQt/Callbacks>
#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/PresenceSpecList>
#include <TelepathyQt/ProtocolParameterList>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>

#include <QDBusConnection>

class QString;
class QStringList;

namespace Tp
{

class TP_QT_EXPORT BaseProtocol : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseProtocol)

public:
    static BaseProtocolPtr create(const QString &name)
    {
        return BaseProtocolPtr(new BaseProtocol(QDBusConnection::sessionBus(), name));
    }
    template<typename BaseProtocolSubclass>
    static BaseProtocolPtr create(const QString &name)
    {
        return BaseProtocolPtr(new BaseProtocolSubclass(QDBusConnection::sessionBus(), name));
    }
    static BaseProtocolPtr create(const QDBusConnection &dbusConnection, const QString &name)
    {
        return BaseProtocolPtr(new BaseProtocol(dbusConnection, name));
    }
    template<typename BaseProtocolSubclass>
    static BaseProtocolPtr create(const QDBusConnection &dbusConnection, const QString &name)
    {
        return BaseProtocolPtr(new BaseProtocolSubclass(dbusConnection, name));
    }

    virtual ~BaseProtocol();

    QString name() const;

    QVariantMap immutableProperties() const;

    // Proto
    QStringList connectionInterfaces() const;
    void setConnectionInterfaces(const QStringList &connInterfaces);

    ProtocolParameterList parameters() const;
    void setParameters(const ProtocolParameterList &parameters);

    RequestableChannelClassSpecList requestableChannelClasses() const;
    void setRequestableChannelClasses(const RequestableChannelClassSpecList &rccSpecs);

    QString vCardField() const;
    void setVCardField(const QString &vCardField);

    QString englishName() const;
    void setEnglishName(const QString &englishName);

    QString iconName() const;
    void setIconName(const QString &iconName);

    QStringList authenticationTypes() const;
    void setAuthenticationTypes(const QStringList &authenticationTypes);

    typedef Callback2<BaseConnectionPtr, const QVariantMap &, DBusError*> CreateConnectionCallback;
    void setCreateConnectionCallback(const CreateConnectionCallback &cb);
    Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, DBusError *error);

    typedef Callback2<QString, const QVariantMap &, DBusError*> IdentifyAccountCallback;
    void setIdentifyAccountCallback(const IdentifyAccountCallback &cb);
    QString identifyAccount(const QVariantMap &parameters, DBusError *error);

    typedef Callback2<QString, const QString &, DBusError*> NormalizeContactCallback;
    void setNormalizeContactCallback(const NormalizeContactCallback &cb);
    QString normalizeContact(const QString &contactId, DBusError *error);

    QSet<AbstractProtocolInterfacePtr> interfaces() const;
    bool plugInterface(const AbstractProtocolInterfacePtr &interface);

protected:
    BaseProtocol(const QDBusConnection &dbusConnection, const QString &name);

    virtual bool registerObject(const QString &busName, const QString &objectPath,
            DBusError *error);

private:
    friend class BaseConnectionManager;
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractProtocolInterface : public AbstractDBusServiceInterface
{
    Q_DISABLE_COPY(AbstractProtocolInterface)

public:
    AbstractProtocolInterface(const QLatin1String &interfaceName);
    virtual ~AbstractProtocolInterface();

protected:
    friend class BaseProtocol;

    void createAdaptor(const QDBusConnection &dbusConnection, QObject *dbusObject) = 0;

    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseProtocolAddressingInterface : public AbstractProtocolInterface
{
    Q_DISABLE_COPY(BaseProtocolAddressingInterface)

public:
    static BaseProtocolAddressingInterfacePtr create()
    {
        return BaseProtocolAddressingInterfacePtr(new BaseProtocolAddressingInterface());
    }
    template<typename BaseProtocolAddressingInterfaceSubclass>
    static BaseProtocolAddressingInterfacePtr create()
    {
        return BaseProtocolAddressingInterfacePtr(new BaseProtocolAddressingInterfaceSubclass());
    }

    virtual ~BaseProtocolAddressingInterface();

    QStringList addressableVCardFields() const;
    void setAddressableVCardFields(const QStringList &vcardFields);

    QStringList addressableUriSchemes() const;
    void setAddressableUriSchemes(const QStringList &uriSchemes);

    typedef Callback3<QString, const QString &, const QString &, DBusError*> NormalizeVCardAddressCallback;
    void setNormalizeVCardAddressCallback(const NormalizeVCardAddressCallback &cb);
    QString normalizeVCardAddress(const QString &vCardField, const QString &vCardAddress, DBusError *error);

    typedef Callback2<QString, const QString &, DBusError*> NormalizeContactUriCallback;
    void setNormalizeContactUriCallback(const NormalizeContactUriCallback &cb);
    QString normalizeContactUri(const QString &uri, DBusError *error);

protected:
    BaseProtocolAddressingInterface();

private:
    void createAdaptor(const QDBusConnection &dbusConnection, QObject *dbusObject);

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

/*
class TP_QT_EXPORT BaseProtocolAvatarsInterface : public QObject
{
    Q_OBJECT

public:
    BaseProtocolAvatarsInterface(const AvatarSpec &details);
    virtual ~BaseProtocolAvatarsInterface();

    AvatarSpec details() const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseProtocolPresenceInterface : public QObject
{
    Q_OBJECT

public:
    BaseProtocolPresenceInterface(const PresenceSpecList &statuses);
    virtual ~BaseProtocolPresenceInterface();

    PresenceSpecList statuses() const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};
*/

}

#endif
