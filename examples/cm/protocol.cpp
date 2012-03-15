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

#include "protocol.h"
#include "_gen/protocol.moc.hpp"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>

#include <QLatin1String>
#include <QVariantMap>

using namespace Tp;

Protocol::Protocol(const QDBusConnection &dbusConnection, const QString &name)
    : BaseProtocol(dbusConnection, name)
{
    setParameters(ProtocolParameterList() <<
            ProtocolParameter(QLatin1String("example-param"),
                QLatin1String("s"), ConnMgrParamFlagRequired));
    setRequestableChannelClasses(
            RequestableChannelClassSpecList() << RequestableChannelClassSpec::textChat());
    setEnglishName(QLatin1String("ExampleProto"));
    setIconName(QLatin1String("example-icon"));
    setVCardField(QLatin1String("x-example"));

    // callbacks
    setCreateConnectionCallback(memFun(this, &Protocol::createConnection));
    setIdentifyAccountCallback(memFun(this, &Protocol::identifyAccount));
    setNormalizeContactCallback(memFun(this, &Protocol::normalizeContact));
}

Protocol::~Protocol()
{
}

BaseConnectionPtr Protocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (error) {
        error->set(QLatin1String("CreateConnection.Error.Test"), QLatin1String(""));
    }
    return BaseConnectionPtr();
}

QString Protocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (error) {
        error->set(QLatin1String("IdentifyAccount.Error.Test"), QLatin1String(""));
    }
    return QString();
}

QString Protocol::normalizeContact(const QString &contactId, Tp::DBusError *error)
{
    if (error) {
        error->set(QLatin1String("NormalizeContact.Error.Test"), QLatin1String(""));
    }
    return QString();
}
