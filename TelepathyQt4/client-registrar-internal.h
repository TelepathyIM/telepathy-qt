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
#include <TelepathyQt4/Types>

namespace Tp
{

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
    ClientHandlerAdaptor(AbstractClientHandler *client);
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
        return mClient->handledChannels();
    }

public Q_SLOTS: // Methods
    void HandleChannels(const QDBusObjectPath &account,
            const QDBusObjectPath &connection,
            const Tp::ChannelDetailsList &channels,
            const Tp::ObjectPathList &requestsSatisfied,
            qulonglong userActionTime,
            const QVariantMap &handlerInfo);

private:
    AbstractClientHandler *mClient;
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
    ClientHandlerRequestsAdaptor(AbstractClientHandler *client);
    virtual ~ClientHandlerRequestsAdaptor();

public Q_SLOTS: // Methods
    void AddRequest(const QDBusObjectPath &request,
            const QVariantMap &properties);
    void RemoveRequest(const QDBusObjectPath &request,
            const QString &error, const QString &message);

private:
    AbstractClientHandler *mClient;
};

} // Tp

#endif
