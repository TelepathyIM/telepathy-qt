/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_client_registrar_internal_h_HEADER_GUARD_
#define _TelepathyQt4_cli_client_registrar_internal_h_HEADER_GUARD_

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

#include <TelepathyQt4/AbstractClientHandler>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Types>

namespace Tp
{

class PendingClientOperation;
class PendingOperation;

class ClientAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList Interfaces READ Interfaces)

public:
    ClientAdaptor(const QStringList &interfaces, QObject *parent);
    virtual ~ClientAdaptor();

public: // Properties
    inline QStringList Interfaces() const
    {
        return mInterfaces;
    }

private:
    QStringList mInterfaces;
};

class ClientHandlerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Handler")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Handler\" >\n"
"    <property name=\"HandlerChannelFilter\" type=\"aa{sv}\" access=\"read\" />\n"
"    <property name=\"BypassApproval\" type=\"b\" access=\"read\" />\n"
"    <property name=\"HandledChannels\" type=\"ao\" access=\"read\" />\n"
"    <method name=\"HandleChannels\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Connection\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Channels\" type=\"a(oa{sv})\" direction=\"in\" />\n"
"      <arg name=\"Requests_Satisfied\" type=\"ao\" direction=\"in\" />\n"
"      <arg name=\"User_Action_Time\" type=\"t\" direction=\"in\" />\n"
"      <arg name=\"Handler_Info\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::ChannelClassList HandlerChannelFilter READ HandlerChannelFilter)
    Q_PROPERTY(bool BypassApproval READ BypassApproval)
    Q_PROPERTY(Tp::ObjectPathList HandledChannels READ HandledChannels)

public:
    ClientHandlerAdaptor(const QDBusConnection &bus,
            const AbstractClientHandlerPtr &client,
            QObject *parent);
    virtual ~ClientHandlerAdaptor();

public: // Properties
    inline Tp::ChannelClassList HandlerChannelFilter() const
    {
        return mClient->channelFilter();
    }

    inline bool BypassApproval() const
    {
        return mClient->bypassApproval();
    }

    inline Tp::ObjectPathList HandledChannels() const
    {
        Tp::ObjectPathList paths;
        foreach (const ChannelPtr &channel, mClient->handledChannels()) {
            paths.append(QDBusObjectPath(channel->objectPath()));
        }
        return paths;
    }

public Q_SLOTS: // Methods
    void HandleChannels(const QDBusObjectPath &account,
            const QDBusObjectPath &connection,
            const Tp::ChannelDetailsList &channels,
            const Tp::ObjectPathList &requestsSatisfied,
            qulonglong userActionTime,
            const QVariantMap &handlerInfo,
            const QDBusMessage &message);

private Q_SLOTS:
    void onHandleChannelsCallFinished();

private:
    void processHandleChannelsQueue();

    class HandleChannelsCall;

    QDBusConnection mBus;
    AbstractClientHandlerPtr mClient;
    QQueue<HandleChannelsCall*> mHandleChannelsQueue;
    bool mProcessingHandleChannels;
};

class ClientHandlerAdaptor::HandleChannelsCall : public QObject
{
    Q_OBJECT

public:
    HandleChannelsCall(const AbstractClientHandlerPtr &client,
            const QDBusObjectPath &account,
            const QDBusObjectPath &connection,
            const ChannelDetailsList &channels,
            const ObjectPathList &requestsSatisfied,
            qulonglong userActionTime,
            const QVariantMap &handlerInfo,
            const QDBusConnection &bus,
            const QDBusMessage &message,
            QObject *parent);
    virtual ~HandleChannelsCall();

    void process();

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onObjectReady(Tp::PendingOperation *op);
    void onConnectionReady(Tp::PendingOperation *op);

private:
    void checkFinished();
    void setFinishedWithError(const QString &errorName,
            const QString &errorMessage);

    AbstractClientHandlerPtr mClient;
    QDBusObjectPath mAccountPath;
    QDBusObjectPath mConnectionPath;
    ChannelDetailsList mChannelDetailsList;
    ObjectPathList mRequestsSatisfied;
    qulonglong mUserActionTime;
    QVariantMap mHandlerInfo;
    QDBusConnection mBus;
    QDBusMessage mMessage;
    PendingClientOperation *mOperation;
    AccountPtr mAccount;
    ConnectionPtr mConnection;
    QList<ChannelPtr> mChannels;
    QList<ChannelRequestPtr> mChannelRequests;
};

class ClientHandlerRequestsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Interface.Requests")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Interface.Requests\" >\n"
"    <method name=\"AddRequest\" >\n"
"      <arg name=\"Request\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"RemoveRequest\" >\n"
"      <arg name=\"Request\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Error\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Message\" type=\"s\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

public:
    ClientHandlerRequestsAdaptor(const QDBusConnection &bus,
            const AbstractClientHandlerPtr &client,
            QObject *parent);
    virtual ~ClientHandlerRequestsAdaptor();

public Q_SLOTS: // Methods
    void AddRequest(const QDBusObjectPath &request,
            const QVariantMap &requestProperties,
            const QDBusMessage &message);
    void RemoveRequest(const QDBusObjectPath &request,
            const QString &errorName, const QString &errorMessage,
            const QDBusMessage &message);

private Q_SLOTS:
    void onAddRequestCallFinished();

private:
    void processAddRequestQueue();

    class AddRequestCall;

    QDBusConnection mBus;
    AbstractClientHandlerPtr mClient;
    QQueue<AddRequestCall*> mAddRequestQueue;
    bool mProcessingAddRequest;
};

class ClientHandlerRequestsAdaptor::AddRequestCall : public QObject
{
    Q_OBJECT

public:
    AddRequestCall(const AbstractClientHandlerPtr &client,
            const QDBusObjectPath &request,
            const QVariantMap &requestProperties,
            const QDBusConnection &bus,
            QObject *parent);
    virtual ~AddRequestCall();

    void process();

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onChannelRequestReady(Tp::PendingOperation *op);

private:
    AbstractClientHandlerPtr mClient;
    QDBusObjectPath mRequestPath;
    QVariantMap mRequestProperties;
    QDBusConnection mBus;
    ChannelRequestPtr mRequest;
};

} // Tp

#endif
