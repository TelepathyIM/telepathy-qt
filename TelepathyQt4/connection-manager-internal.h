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

#include <TelepathyQt4/PendingStringList>

#include <QDBusConnection>
#include <QLatin1String>
#include <QQueue>
#include <QString>

namespace Tp
{

class ConnectionManager;
class ReadinessHelper;

struct ConnectionManager::Private
{
    Private(ConnectionManager *parent, const QString &name);
    ~Private();

    bool parseConfigFile();

    static void introspectMain(Private *self);
    void introspectProtocols();
    void introspectParameters();

    static QString makeBusName(const QString &name);
    static QString makeObjectPath(const QString &name);

    ProtocolInfo *protocol(const QString &protocolName);

    class PendingNames;

    // Public object
    ConnectionManager *parent;

    QString name;

    // Instance of generated interface class
    Client::ConnectionManagerInterface *baseInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    QStringList interfaces;
    QQueue<QString> parametersQueue;
    ProtocolInfoList protocols;
};

class ConnectionManager::Private::PendingNames : public PendingStringList
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
    QStringList mResult;
    QDBusConnection mBus;
};

} // Tp

#endif
