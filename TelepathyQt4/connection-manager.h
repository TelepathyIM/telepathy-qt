/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt4_connection_manager_h_HEADER_GUARD_
#define _TelepathyQt4_connection_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-connection-manager.h>

#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/SharedPtr>

#include <QSet>

namespace Tp
{

class ConnectionCapabilities;
class PendingConnection;
class PendingReady;
class PendingStringList;
class ProtocolParameter;
class ProtocolInfo;

typedef QList<ProtocolParameter*> ProtocolParameterList;
typedef QList<ProtocolInfo*> ProtocolInfoList;

class TELEPATHY_QT4_EXPORT ProtocolParameter
{
public:
    ProtocolParameter(const QString &name,
                      const QDBusSignature &dbusSignature,
                      QVariant defaultValue,
                      ConnMgrParamFlag flags);
    ~ProtocolParameter();

    QString name() const;
    QDBusSignature dbusSignature() const;
    QVariant::Type type() const;
    QVariant defaultValue() const;

    bool isRequired() const;
    bool isSecret() const;
    bool isRequiredForRegistration() const;

    bool operator==(const ProtocolParameter &other) const;
    bool operator==(const QString &name) const;

private:
    Q_DISABLE_COPY(ProtocolParameter);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT ProtocolInfo
{
public:
    ~ProtocolInfo();

    QString cmName() const;

    QString name() const;

    const ProtocolParameterList &parameters() const;
    bool hasParameter(const QString &name) const;

    bool canRegister() const;

    ConnectionCapabilities *capabilities() const;

    QString vcardField() const;

    QString englishName() const;

    QString iconName() const;

private:
    Q_DISABLE_COPY(ProtocolInfo);

    ProtocolInfo(const QString &cmName, const QString &name);

    void addParameter(const ParamSpec &spec);
    void setRequestableChannelClasses(const RequestableChannelClassList &caps);
    void setVCardField(const QString &vcardField);
    void setEnglishName(const QString &englishName);
    void setIconName(const QString &iconName);

    struct Private;
    friend struct Private;
    friend class ConnectionManager;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT ConnectionManager : public StatelessDBusProxy,
                          public OptionalInterfaceFactory<ConnectionManager>,
                          public ReadyObject,
                          public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ConnectionManager);
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QStringList supportedProtocols READ supportedProtocols)
    Q_PROPERTY(ProtocolInfoList protocols READ protocols)

public:
    static const Feature FeatureCore;

    // TODO (API/ABI break): Should we keep ConnectionManager::requestConnection. If so we
    //                       probably want to pass factories here and use them in requestConnection.
    static ConnectionManagerPtr create(const QString &name);
    static ConnectionManagerPtr create(const QDBusConnection &bus,
            const QString &name);

    virtual ~ConnectionManager();

    QString name() const;

    QStringList supportedProtocols() const;
    const ProtocolInfoList &protocols() const;
    bool hasProtocol(const QString &protocolName) const;
    ProtocolInfo *protocol(const QString &protocolName) const;

    // TODO (API/ABI break): Do we want to keep requestConnection as public API?
    PendingConnection *requestConnection(const QString &protocolName,
            const QVariantMap &parameters);

    static PendingStringList *listNames(
            const QDBusConnection &bus = QDBusConnection::sessionBus());

    TELEPATHY_QT4_DEPRECATED inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory<ConnectionManager>::interface<Client::DBus::PropertiesInterface>();
    }

protected:
    ConnectionManager(const QString &name);
    ConnectionManager(const QDBusConnection &bus, const QString &name);

    Client::ConnectionManagerInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);
    void gotProtocolsLegacy(QDBusPendingCallWatcher *);
    void gotParametersLegacy(QDBusPendingCallWatcher *);
    void onProtocolReady(Tp::PendingOperation *);

private:
    friend class PendingConnection;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
