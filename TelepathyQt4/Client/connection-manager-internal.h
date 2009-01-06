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

#ifndef _TelepathyQt4_cli_connection_manager_internal_h_HEADER_GUARD_
#define _TelepathyQt4_cli_connection_manager_internal_h_HEADER_GUARD_

#include <TelepathyQt4/Client/PendingStringList>

#include <QDBusConnection>
#include <QLatin1String>
#include <QQueue>
#include <QString>

namespace Telepathy
{
namespace Client
{

class ConnectionManager;
class ConnectionManagerInterface;

struct ConnectionManager::Private
{
    ConnectionManager *parent;
    ConnectionManagerInterface* baseInterface;
    bool ready;
    QQueue<void (Private::*)()> introspectQueue;
    QQueue<QString> getParametersQueue;
    QQueue<QString> protocolQueue;
    QStringList interfaces;
    ProtocolInfoList protocols;

    Private(ConnectionManager *parent);
    ~Private();

    static QString makeBusName(const QString& name);
    static QString makeObjectPath(const QString& name);

    ProtocolInfo *protocol(const QString &protocolName);

    bool checkConfigFile();
    void callReadConfig();
    void callGetAll();
    void callGetParameters();
    void callListProtocols();

    class PendingReady : public PendingOperation
    {
        // ConnectionManager is a friend so it can call finished() etc.
        friend class ConnectionManager;
    public:
        PendingReady(ConnectionManager *parent);
    };

    PendingReady *pendingReady;
};

class ConnectionManagerPendingNames : public PendingStringList
{
    Q_OBJECT

public:
    ConnectionManagerPendingNames(const QDBusConnection &bus);
    ~ConnectionManagerPendingNames() {};

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *);
    void continueProcessing();

private:
    void invokeMethod(const QLatin1String &method);
    void parseResult(const QStringList &names);

    QQueue<QLatin1String> mMethodsQueue;
    QStringList mResult;
    QDBusConnection mBus;
};

} // Telepathy::Client
} // Telepathy

#endif
