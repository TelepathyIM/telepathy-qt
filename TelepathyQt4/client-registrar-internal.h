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

class ClientObserverAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Observer")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Observer\" >\n"
"    <property name=\"ObserverChannelFilter\" type=\"aa{sv}\" access=\"read\" />\n"
"    <method name=\"ObserveChannels\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Connection\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Channels\" type=\"a(oa{sv})\" direction=\"in\" />\n"
"      <arg name=\"Dispatch_Operation\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Requests_Satisfied\" type=\"ao\" direction=\"in\" />\n"
"      <arg name=\"Observer_Info\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::ChannelClassList ObserverChannelFilter READ ObserverChannelFilter)

public:
    ClientObserverAdaptor(
            const QDBusConnection &bus,
            AbstractClientObserver *client,
            QObject *parent);
    virtual ~ClientObserverAdaptor();

public: // Properties
    inline Tp::ChannelClassList ObserverChannelFilter() const
    {
        return mClient->observerChannelFilter();
    }

public Q_SLOTS: // Methods
    void ObserveChannels(const QDBusObjectPath &account,
            const QDBusObjectPath &connection,
            const Tp::ChannelDetailsList &channels,
            const QDBusObjectPath &dispatchOperation,
            const Tp::ObjectPathList &requestsSatisfied,
            const QVariantMap &observerInfo,
            const QDBusMessage &message);

private:
    QDBusConnection mBus;
    AbstractClientObserver *mClient;
};

class ClientApproverAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Client.Approver")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Client.Approver\" >\n"
"    <property name=\"ApproverChannelFilter\" type=\"aa{sv}\" access=\"read\" />\n"
"    <method name=\"AddDispatchOperation\" >\n"
"      <arg name=\"Channels\" type=\"a(oa{sv})\" direction=\"in\" />\n"
"      <arg name=\"Dispatch_Operation\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::ChannelClassList ApproverChannelFilter READ ApproverChannelFilter)

public:
    ClientApproverAdaptor(
            const QDBusConnection &bus,
            AbstractClientApprover *client,
            QObject *parent);
    virtual ~ClientApproverAdaptor();

public: // Properties
    inline Tp::ChannelClassList ApproverChannelFilter() const
    {
        return mClient->approverChannelFilter();
    }

public Q_SLOTS: // Methods
    void AddDispatchOperation(const Tp::ChannelDetailsList &channels,
            const QDBusObjectPath &dispatchOperation,
            const QVariantMap &properties,
            const QDBusMessage &message);

private:
    QDBusConnection mBus;
    AbstractClientApprover *mClient;
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
    ClientHandlerAdaptor(
            const QDBusConnection &bus,
            AbstractClientHandler *client,
            QObject *parent);
    virtual ~ClientHandlerAdaptor();

public: // Properties
    inline Tp::ChannelClassList HandlerChannelFilter() const
    {
        return mClient->handlerChannelFilter();
    }

    inline bool BypassApproval() const
    {
        return mClient->bypassApproval();
    }

    inline Tp::ObjectPathList HandledChannels() const
    {
        // mAdaptorsForConnection includes this, so no need to start the set
        // with this->mHandledChannels
        QList<ClientHandlerAdaptor*> adaptors(mAdaptorsForConnection.value(
                    qMakePair(mBus.name(), mBus.baseService())));
        QSet<ChannelPtr> handledChannels;
        foreach (ClientHandlerAdaptor *handlerAdaptor, adaptors) {
            handledChannels.unite(handlerAdaptor->mHandledChannels);
        }

        Tp::ObjectPathList ret;
        foreach (const ChannelPtr &channel, handledChannels) {
            ret.append(QDBusObjectPath(channel->objectPath()));
        }
        return ret;
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
    void onChannelInvalidated(Tp::DBusProxy *proxy);

private:
    static void onContextFinished(const MethodInvocationContextPtr<> &context,
            const QList<ChannelPtr> &channels, ClientHandlerAdaptor *self);

    QDBusConnection mBus;
    AbstractClientHandler *mClient;
    QSet<ChannelPtr> mHandledChannels;

    static QHash<QPair<QString, QString>, QList<ClientHandlerAdaptor *> > mAdaptorsForConnection;
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
            AbstractClientHandler *client,
            QObject *parent);
    virtual ~ClientHandlerRequestsAdaptor();

public Q_SLOTS: // Methods
    void AddRequest(const QDBusObjectPath &request,
            const QVariantMap &requestProperties,
            const QDBusMessage &message);
    void RemoveRequest(const QDBusObjectPath &request,
            const QString &errorName, const QString &errorMessage,
            const QDBusMessage &message);

private:
    QDBusConnection mBus;
    AbstractClientHandler *mClient;
};

} // Tp

#endif
