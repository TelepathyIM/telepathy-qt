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

#ifndef _TelepathyQt4_connection_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt4_connection_manager_internal_h_HEADER_GUARD_

#include <TelepathyQt4/PendingStringList>

#include <QDBusConnection>
#include <QLatin1String>
#include <QQueue>
#include <QSet>
#include <QString>

namespace Tp
{

class ConnectionManager;
class ReadinessHelper;

struct TELEPATHY_QT4_NO_EXPORT ConnectionManager::Private
{
    Private(ConnectionManager *parent, const QString &name);
    ~Private();

    bool parseConfigFile();

    static void introspectMain(Private *self);
    void introspectProtocolsLegacy();
    void introspectParametersLegacy();

    static QString makeBusName(const QString &name);
    static QString makeObjectPath(const QString &name);

    class PendingNames;
    class ProtocolWrapper;

    // Public object
    ConnectionManager *parent;

    QString name;

    // Instance of generated interface class
    Client::ConnectionManagerInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    QQueue<QString> parametersQueue;
    ProtocolInfoList protocols;
    QSet<SharedPtr<ProtocolWrapper> > wrappers;
};

class TELEPATHY_QT4_NO_EXPORT ConnectionManager::Private::PendingNames : public PendingStringList
{
    Q_OBJECT

public:
    PendingNames(const QDBusConnection &bus);
    ~PendingNames() {};

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *);
    void continueProcessing();

private:
    void invokeMethod(const QLatin1String &method);
    void parseResult(const QStringList &names);

    QQueue<QLatin1String> mMethodsQueue;
    QSet<QString> mResult;
    QDBusConnection mBus;
};

class TELEPATHY_QT4_NO_EXPORT ConnectionManager::Private::ProtocolWrapper :
                public StatelessDBusProxy, public OptionalInterfaceFactory<ProtocolWrapper>
{
    Q_OBJECT

public:
    static const Feature FeatureCore;

    ProtocolWrapper(const QDBusConnection &bus, const QString &busName,
            const QString &objectPath,
            const QString &cmName, const QString &name, const QVariantMap &props);
    ~ProtocolWrapper();

    ProtocolInfo info() const { return mInfo; }

    inline Client::DBus::PropertiesInterface *propertiesInterface() const
    {
        return optionalInterface<Client::DBus::PropertiesInterface>(BypassInterfaceCheck);
    }

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *);

private:
    static void introspectMain(ProtocolWrapper *self);

    void fillRCCs();
    bool receiveProperties(const QVariantMap &props);

    ReadinessHelper *mReadinessHelper;
    ProtocolInfo mInfo;
    QVariantMap mImmutableProps;
};

} // Tp

#endif
