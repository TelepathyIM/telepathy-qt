/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt_connection_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt_connection_manager_internal_h_HEADER_GUARD_

#include <TelepathyQt/PendingStringList>

#include <QDBusConnection>
#include <QLatin1String>
#include <QQueue>
#include <QSet>
#include <QString>

namespace Tp
{

class ConnectionManager;
class ReadinessHelper;

struct TP_QT_NO_EXPORT ConnectionManager::Private
{
    Private(ConnectionManager *parent, const QString &name,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory);
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
    ConnectionManagerLowlevelPtr lowlevel;

    QString name;

    // Instance of generated interface class
    Client::ConnectionManagerInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Introspection
    QQueue<QString> parametersQueue;
    ProtocolInfoList protocols;
    QSet<SharedPtr<ProtocolWrapper> > wrappers;
};

struct TP_QT_NO_EXPORT ConnectionManagerLowlevel::Private
{
    Private(ConnectionManager *cm)
        : cm(cm)
    {
    }

    WeakPtr<ConnectionManager> cm;
};

class TP_QT_NO_EXPORT ConnectionManager::Private::PendingNames : public PendingStringList
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

class TP_QT_NO_EXPORT ConnectionManager::Private::ProtocolWrapper :
                public StatelessDBusProxy, public OptionalInterfaceFactory<ProtocolWrapper>
{
    Q_OBJECT

public:
    static const Feature FeatureCore;

    ProtocolWrapper(const ConnectionManagerPtr &cm,
            const QString &objectPath,
            const QString &name, const QVariantMap &props);
    ~ProtocolWrapper();

    ProtocolInfo info() const { return mInfo; }

    inline Client::ProtocolInterface *baseInterface() const
    {
        return interface<Client::ProtocolInterface>();
    }

    inline Client::ProtocolInterfaceAvatarsInterface *avatarsInterface() const
    {
        return interface<Client::ProtocolInterfaceAvatarsInterface>();
    }

    inline Client::ProtocolInterfacePresenceInterface *presenceInterface() const
    {
        return interface<Client::ProtocolInterfacePresenceInterface>();
    }

    inline Client::ProtocolInterfaceAddressingInterface *addressingInterface() const
    {
        return interface<Client::ProtocolInterfaceAddressingInterface>();
    }

private Q_SLOTS:
    void gotMainProperties(Tp::PendingOperation *op);
    void gotAvatarsProperties(Tp::PendingOperation *op);
    void gotPresenceProperties(Tp::PendingOperation *op);
    void gotAddressingProperties(Tp::PendingOperation *op);

private:
    static void introspectMain(ProtocolWrapper *self);
    void introspectMainProperties();
    void introspectInterfaces();
    void introspectAvatars();
    void introspectPresence();
    void introspectAddressing();

    void continueIntrospection();

    QVariantMap qualifyProperties(const QString &ifaceName,
            const QVariantMap &unqualifiedProps);
    void fillRCCs();
    bool extractImmutableProperties();
    void extractMainProperties(const QVariantMap &props);
    void extractAvatarsProperties(const QVariantMap &props);
    void extractPresenceProperties(const QVariantMap &props);
    void extractAddressingProperties(const QVariantMap &props);

    ReadinessHelper *mReadinessHelper;
    ProtocolInfo mInfo;
    QVariantMap mImmutableProps;
    bool mHasMainProps;
    bool mHasAvatarsProps;
    bool mHasPresenceProps;
    bool mHasAddressingProps;
    QQueue<void (ProtocolWrapper::*)()> introspectQueue;
};

} // Tp

#endif
