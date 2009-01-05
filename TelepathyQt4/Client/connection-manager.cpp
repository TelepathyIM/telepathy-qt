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

#define IN_TELEPATHY_QT4_HEADER
#include "connection-manager.h"
#include "connection-manager.moc.hpp"

#include <TelepathyQt4/_gen/cli-connection-manager-body.hpp>
#include <TelepathyQt4/_gen/cli-connection-manager.moc.hpp>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/ManagerFile>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/debug-internal.h>

#include <QQueue>
#include <QStringList>
#include <QTimer>

namespace Telepathy
{
namespace Client
{

ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QDBusSignature &dbusSignature,
                                     QVariant defaultValue,
                                     Telepathy::ConnMgrParamFlag flags)
    : mName(name),
      mDBusSignature(dbusSignature),
      mType(ManagerFile::variantFromDBusSignature("", dbusSignature.signature())),
      mDefaultValue(defaultValue),
      mFlags(flags)
{
}


ProtocolParameter::~ProtocolParameter()
{
}


bool ProtocolParameter::isRequired() const
{
    return mFlags & ConnMgrParamFlagRequired;
}


bool ProtocolParameter::isSecret() const
{
    return mFlags & ConnMgrParamFlagSecret;
}


bool ProtocolParameter::requiredForRegistration() const
{
    return mFlags & ConnMgrParamFlagRegister;
}


bool ProtocolParameter::operator==(const ProtocolParameter &other) const
{
    return (mName == other.name());
}


bool ProtocolParameter::operator==(const QString &name) const
{
    return (mName == name);
}


struct ProtocolInfo::Private
{
    ProtocolParameterList params;
};


ProtocolInfo::ProtocolInfo(const QString &cmName, const QString &name)
    : mPriv(new Private()),
      mCmName(cmName),
      mName(name)
{
}


ProtocolInfo::~ProtocolInfo()
{
    Q_FOREACH (ProtocolParameter *param, mPriv->params) {
        delete param;
    }
}


const ProtocolParameterList &ProtocolInfo::parameters() const
{
    return mPriv->params;
}


bool ProtocolInfo::hasParameter(const QString &name) const
{
    Q_FOREACH (ProtocolParameter *param, mPriv->params) {
        if (param->name() == name) {
            return true;
        }
    }
    return false;
}


bool ProtocolInfo::canRegister() const
{
    return hasParameter(QLatin1String("register"));
}


void ProtocolInfo::addParameter(const ParamSpec &spec)
{
    QVariant defaultValue;
    if (spec.flags & ConnMgrParamFlagHasDefault)
        defaultValue = spec.defaultValue.variant();

    uint flags = spec.flags;
    if (spec.name.endsWith("password"))
        flags |= Telepathy::ConnMgrParamFlagSecret;

    ProtocolParameter *param = new ProtocolParameter(spec.name,
            QDBusSignature(spec.signature),
            defaultValue,
            (Telepathy::ConnMgrParamFlag) flags);

    mPriv->params.append(param);
}


struct ConnectionManager::Private
{
    ConnectionManager *parent;
    ConnectionManagerInterface* baseInterface;
    bool ready;
    QQueue<void (Private::*)()> introspectQueue;
    QQueue<QString> getParametersQueue;
    QQueue<QString> protocolQueue;
    QStringList interfaces;
    ProtocolInfoList protocols;;

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
};


ConnectionManager::Private::Private(ConnectionManager *parent)
    : parent(parent),
      baseInterface(new ConnectionManagerInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      ready(false)
{
    debug() << "Creating new ConnectionManager:" << parent->busName();

    introspectQueue.enqueue(&Private::callReadConfig);
    QTimer::singleShot(0, parent, SLOT(continueIntrospection()));
}


ConnectionManager::Private::~Private()
{
    Q_FOREACH (ProtocolInfo* info, protocols) {
        delete info;
    }
}


QString ConnectionManager::Private::makeBusName(const QString& name)
{
    return QString::fromAscii(
            TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE).append(name);
}


QString ConnectionManager::Private::makeObjectPath(const QString& name)
{
    return QString::fromAscii(
            TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE).append(name);
}


ProtocolInfo *ConnectionManager::Private::protocol(const QString &protocolName)
{
    Q_FOREACH (ProtocolInfo *info, protocols) {
        if (info->name() == protocolName) {
            return info;
        }
    }
    return NULL;
}


bool ConnectionManager::Private::checkConfigFile()
{
    ManagerFile f(parent->name());
    if (!f.isValid()) {
        return false;
    }

    Q_FOREACH (QString protocol, f.protocols()) {
        ProtocolInfo *info = new ProtocolInfo(parent->name(),
                                              protocol);
        protocols.append(info);

        Q_FOREACH (ParamSpec spec, f.parameters(protocol)) {
            info->addParameter(spec);
        }
    }

#if 0
    Q_FOREACH (ProtocolInfo *info, protocols) {
        qDebug() << "protocol name   :" << info->name();
        qDebug() << "protocol cn name:" << info->cmName();
        Q_FOREACH (ProtocolParameter *param, info->parameters()) {
            qDebug() << "\tparam name:       " << param->name();
            qDebug() << "\tparam is required:" << param->isRequired();
            qDebug() << "\tparam is secret:  " << param->isSecret();
            qDebug() << "\tparam value:      " << param->defaultValue();
        }
    }
#endif

    return true;
}


void ConnectionManager::Private::callReadConfig()
{
    if (!checkConfigFile()) {
        introspectQueue.enqueue(&Private::callGetAll);
        introspectQueue.enqueue(&Private::callListProtocols);
    }

    parent->continueIntrospection();
}


void ConnectionManager::Private::callGetAll()
{
    debug() << "Calling Properties::GetAll(ConnectionManager)";
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
            parent->propertiesInterface()->GetAll(
                TELEPATHY_INTERFACE_CONNECTION_MANAGER), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onGetAllConnectionManagerReturn(QDBusPendingCallWatcher*)));
}


void ConnectionManager::Private::callGetParameters()
{
    QString protocol = getParametersQueue.dequeue();
    protocolQueue.enqueue(protocol);
    debug() << "Calling ConnectionManager::GetParameters(" <<
        protocol << ")";
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
            baseInterface->GetParameters(protocol), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onGetParametersReturn(QDBusPendingCallWatcher*)));
}


void ConnectionManager::Private::callListProtocols()
{
    debug() << "Calling ConnectionManager::ListProtocols";
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
            baseInterface->ListProtocols(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onListProtocolsReturn(QDBusPendingCallWatcher*)));
}


ConnectionManager::ConnectionManager(const QString& name, QObject* parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            Private::makeBusName(name), Private::makeObjectPath(name),
            parent),
      mPriv(new Private(this)),
      mName(name)
{
}


ConnectionManager::ConnectionManager(const QDBusConnection& bus,
        const QString& name, QObject* parent)
    : StatelessDBusProxy(bus, Private::makeBusName(name),
            Private::makeObjectPath(name), parent),
      mPriv(new Private(this)),
      mName(name)
{
}


ConnectionManager::~ConnectionManager()
{
    delete mPriv;
}


QStringList ConnectionManager::interfaces() const
{
    return mPriv->interfaces;
}


QStringList ConnectionManager::supportedProtocols() const
{
    QStringList protocols;
    Q_FOREACH (const ProtocolInfo *info, mPriv->protocols) {
        protocols.append(info->name());
    }
    return protocols;
}


const ProtocolInfoList &ConnectionManager::protocols() const
{
    return mPriv->protocols;
}


bool ConnectionManager::isReady() const
{
    return mPriv->ready;
}


ConnectionManagerInterface* ConnectionManager::baseInterface() const
{
    return mPriv->baseInterface;
}


void ConnectionManager::onGetAllConnectionManagerReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(ConnectionManager)";
        props = reply.value();
    } else {
        warning().nospace() <<
            "Properties.GetAll(ConnectionManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    // If Interfaces is not supported, the spec says to assume it's
    // empty, so keep the empty list mPriv was initialized with
    if (props.contains("Interfaces")) {
        mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    }
    continueIntrospection();
}


void ConnectionManager::onListProtocolsReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;
    QStringList protocols;

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.ListProtocols";
        protocols = reply.value();
    } else {
        warning().nospace() <<
            "ConnectionManager.ListProtocols failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    Q_FOREACH (const QString &protocolName, protocols) {
        mPriv->protocols.append(new ProtocolInfo(mName,
                                                 protocolName));

        mPriv->getParametersQueue.enqueue(protocolName);
        mPriv->introspectQueue.enqueue(&Private::callGetParameters);
    }
    continueIntrospection();
}


void ConnectionManager::onGetParametersReturn(
        QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<ParamSpecList> reply = *watcher;
    ParamSpecList parameters;
    QString protocolName = mPriv->protocolQueue.dequeue();
    ProtocolInfo *info = mPriv->protocol(protocolName);

    if (!reply.isError()) {
        debug() << "Got reply to ConnectionManager.GetParameters";
        parameters = reply.value();
    } else {
        warning().nospace() <<
            "ConnectionManager.GetParameters failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    Q_FOREACH (const ParamSpec& spec, parameters) {
        debug() << "Parameter" << spec.name << "has flags" << spec.flags
            << "and signature" << spec.signature;

        info->addParameter(spec);
    }
    continueIntrospection();
}

void ConnectionManager::continueIntrospection()
{
    if (!mPriv->ready) {
        if (mPriv->introspectQueue.isEmpty()) {
            debug() << "ConnectionManager is ready";
            mPriv->ready = true;
            Q_EMIT ready(this);
        } else {
            (mPriv->*(mPriv->introspectQueue.dequeue()))();
        }
    }
}

} // Telepathy::Client
} // Telepathy
