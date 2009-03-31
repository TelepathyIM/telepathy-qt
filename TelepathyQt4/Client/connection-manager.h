/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_connection_manager_h_HEADER_GUARD_
#define _TelepathyQt4_cli_connection_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-connection-manager.h>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/DBusProxy>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>
#include <TelepathyQt4/Client/ReadinessHelper>
#include <TelepathyQt4/Client/ReadyObject>
#include <TelepathyQt4/Client/Types>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/SharedPtr>

#include <QSet>

namespace Telepathy
{
namespace Client
{

class PendingConnection;
class PendingReady;
class PendingStringList;
class ProtocolParameter;
class ProtocolInfo;

typedef QList<ProtocolParameter*> ProtocolParameterList;
typedef QList<ProtocolInfo*> ProtocolInfoList;

class ProtocolParameter
{
public:
    ProtocolParameter(const QString &name,
                      const QDBusSignature &dbusSignature,
                      QVariant defaultValue,
                      Telepathy::ConnMgrParamFlag flags);
    ~ProtocolParameter();

    QString name() const { return mName; }
    QDBusSignature dbusSignature() const { return mDBusSignature; }
    QVariant::Type type() const { return mType; }
    QVariant defaultValue() const { return mDefaultValue; }

    bool isRequired() const;
    bool isSecret() const;
    bool requiredForRegistration() const;

    bool operator==(const ProtocolParameter &other) const;
    bool operator==(const QString &name) const;

private:
    Q_DISABLE_COPY(ProtocolParameter);

    struct Private;
    friend struct Private;
    Private *mPriv;
    QString mName;
    QDBusSignature mDBusSignature;
    QVariant::Type mType;
    QVariant mDefaultValue;
    Telepathy::ConnMgrParamFlag mFlags;
};


class ProtocolInfo
{
public:
    ~ProtocolInfo();

    QString cmName() const { return mCmName; }

    QString name() const { return mName; }

    const ProtocolParameterList &parameters() const;
    bool hasParameter(const QString &name) const;

    bool canRegister() const;

private:
    Q_DISABLE_COPY(ProtocolInfo);

    ProtocolInfo(const QString &cmName, const QString &name);

    void addParameter(const ParamSpec &spec);

    class Private;
    friend class Private;
    friend class ConnectionManager;
    Private *mPriv;
    QString mCmName;
    QString mName;
};


class ConnectionManager : public StatelessDBusProxy,
                          private OptionalInterfaceFactory<ConnectionManager>,
                          public ReadyObject,
                          public SharedData
{
    Q_OBJECT

public:
    static const Feature FeatureCore;

    ConnectionManager(const QString &name, QObject *parent = 0);
    ConnectionManager(const QDBusConnection &bus,
            const QString &name, QObject *parent = 0);

    virtual ~ConnectionManager();

    QString name() const;

    QStringList interfaces() const;

    QStringList supportedProtocols() const;
    const ProtocolInfoList &protocols() const;

    PendingConnection *requestConnection(const QString &protocol,
            const QVariantMap &parameters);

    inline DBus::PropertiesInterface *propertiesInterface() const
    {
        return OptionalInterfaceFactory<ConnectionManager>::interface<DBus::PropertiesInterface>();
    }

    static PendingStringList *listNames(const QDBusConnection &bus = QDBusConnection::sessionBus());

protected:
    ConnectionManagerInterface *baseInterface() const;

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);
    void gotProtocols(QDBusPendingCallWatcher *);
    void gotParameters(QDBusPendingCallWatcher *);

private:
    Q_DISABLE_COPY(ConnectionManager);

    struct Private;
    friend struct Private;
    friend class PendingConnection;
    Private *mPriv;
};

}
}

#endif
