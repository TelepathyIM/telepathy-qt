/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#include <TelepathyQt/BaseProtocol>
#include "TelepathyQt/base-protocol-internal.h"

#include "TelepathyQt/_gen/base-protocol.moc.hpp"
#include "TelepathyQt/_gen/base-protocol-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Utils>

#include <QDBusObjectPath>
#include <QString>
#include <QStringList>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseProtocol::Private
{
    Private(BaseProtocol *parent, const QDBusConnection &dbusConnection,
            const QString &name)
        : parent(parent),
          name(name),
          adaptee(new BaseProtocol::Adaptee(dbusConnection, parent))
    {
    }

    BaseProtocol *parent;
    QString name;

    BaseProtocol::Adaptee *adaptee;

    QStringList connInterfaces;
    ProtocolParameterList parameters;
    RequestableChannelClassSpecList rccSpecs;
    QString vCardField;
    QString englishName;
    QString iconName;
    QStringList authTypes;
    CreateConnectionCallback createConnectionCb;
    IdentifyAccountCallback identifyAccountCb;
    NormalizeContactCallback normalizeContactCb;
};

BaseProtocol::Adaptee::Adaptee(const QDBusConnection &dbusConnection, BaseProtocol *protocol)
    : QObject(protocol),
      mProtocol(protocol)
{
    mAdaptor = new Service::ProtocolAdaptor(dbusConnection, this, protocol->dbusObject());
}

BaseProtocol::Adaptee::~Adaptee()
{
}

QVariantMap BaseProtocol::immutableProperties() const
{
    // FIXME
    // Ps.: also include optional interfaces immutable properties
    return QVariantMap();
}

QStringList BaseProtocol::Adaptee::interfaces() const
{
    // FIXME
    qDebug() << __FUNCTION__ << "called";
    return QStringList();
}

QStringList BaseProtocol::Adaptee::connectionInterfaces() const
{
    return mProtocol->connectionInterfaces();
}

ParamSpecList BaseProtocol::Adaptee::parameters() const
{
    ParamSpecList ret;
    foreach (const ProtocolParameter &param, mProtocol->parameters()) {
         ParamSpec paramSpec = param.bareParameter();
         if (!(paramSpec.flags & ConnMgrParamFlagHasDefault)) {
             // we cannot pass QVariant::Invalid over D-Bus, lets build a dummy value
             // that should be ignored according to the spec
             paramSpec.defaultValue = QDBusVariant(
                     parseValueWithDBusSignature(QString(), paramSpec.signature));
         }
         ret << paramSpec;
    }
    return ret;
}

RequestableChannelClassList BaseProtocol::Adaptee::requestableChannelClasses() const
{
    return mProtocol->requestableChannelClasses().bareClasses();
}

QString BaseProtocol::Adaptee::vCardField() const
{
    return mProtocol->vCardField();
}

QString BaseProtocol::Adaptee::englishName() const
{
    return mProtocol->englishName();
}

QString BaseProtocol::Adaptee::icon() const
{
    return mProtocol->iconName();
}

QStringList BaseProtocol::Adaptee::authenticationTypes() const
{
    return mProtocol->authenticationTypes();
}

void BaseProtocol::Adaptee::identifyAccount(const QVariantMap &parameters,
        const Tp::Service::ProtocolAdaptor::IdentifyAccountContextPtr &context)
{
    DBusError error;
    QString accountId;
    accountId = mProtocol->identifyAccount(parameters, &error);
    if (accountId.isEmpty()) {
        context->setFinishedWithError(error);
        return;
    }
    context->setFinished(accountId);
}

void BaseProtocol::Adaptee::normalizeContact(const QString &contactId,
        const Tp::Service::ProtocolAdaptor::NormalizeContactContextPtr &context)
{
    DBusError error;
    QString normalizedContactId;
    normalizedContactId = mProtocol->normalizeContact(contactId, &error);
    if (normalizedContactId.isEmpty()) {
        context->setFinishedWithError(error);
        return;
    }
    context->setFinished(normalizedContactId);
}

BaseProtocol::BaseProtocol(const QDBusConnection &dbusConnection, const QString &name)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, name))
{
}

BaseProtocol::~BaseProtocol()
{
    delete mPriv;
}

QString BaseProtocol::name() const
{
    return mPriv->name;
}

QStringList BaseProtocol::connectionInterfaces() const
{
    return mPriv->connInterfaces;
}

void BaseProtocol::setConnectionInterfaces(const QStringList &connInterfaces)
{
    mPriv->connInterfaces = connInterfaces;
}

ProtocolParameterList BaseProtocol::parameters() const
{
    return mPriv->parameters;
}

void BaseProtocol::setParameters(const ProtocolParameterList &parameters)
{
    mPriv->parameters = parameters;
}

RequestableChannelClassSpecList BaseProtocol::requestableChannelClasses() const
{
    return mPriv->rccSpecs;
}

void BaseProtocol::setRequestableChannelClasses(const RequestableChannelClassSpecList &rccSpecs)
{
    mPriv->rccSpecs = rccSpecs;
}

QString BaseProtocol::vCardField() const
{
    return mPriv->vCardField;
}

void BaseProtocol::setVCardField(const QString &vCardField)
{
    mPriv->vCardField = vCardField;
}

QString BaseProtocol::englishName() const
{
    return mPriv->englishName;
}

void BaseProtocol::setEnglishName(const QString &englishName)
{
    mPriv->englishName = englishName;
}

QString BaseProtocol::iconName() const
{
    return mPriv->iconName;
}

void BaseProtocol::setIconName(const QString &iconName)
{
     mPriv->iconName = iconName;
}

QStringList BaseProtocol::authenticationTypes() const
{
    return mPriv->authTypes;
}

void BaseProtocol::setAuthenticationTypes(const QStringList &authenticationTypes)
{
    mPriv->authTypes = authenticationTypes;
}

void BaseProtocol::setCreateConnectionCallback(const CreateConnectionCallback &cb)
{
    mPriv->createConnectionCb = cb;
}

BaseConnectionPtr BaseProtocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (!mPriv->createConnectionCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseConnectionPtr();
    }
    return mPriv->createConnectionCb(parameters, error);
}

void BaseProtocol::setIdentifyAccountCallback(const IdentifyAccountCallback &cb)
{
    mPriv->identifyAccountCb = cb;
}

QString BaseProtocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (!mPriv->identifyAccountCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->identifyAccountCb(parameters, error);
}

void BaseProtocol::setNormalizeContactCallback(const NormalizeContactCallback &cb)
{
    mPriv->normalizeContactCb = cb;
}

QString BaseProtocol::normalizeContact(const QString &contactId, Tp::DBusError *error)
{
    if (!mPriv->normalizeContactCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->normalizeContactCb(contactId, error);
}

bool BaseProtocol::registerObject(const QString &busName, const QString &objectPath,
        DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

}
