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

#include "TelepathyQt/_gen/svc-connection-manager.h"

#include <TelepathyQt/Global>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/Types>

#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class TP_QT_NO_EXPORT BaseConnectionManager::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList interfaces READ interfaces)
    Q_PROPERTY(Tp::ProtocolPropertiesMap protocols READ protocols)

public:
    Adaptee(const QDBusConnection &dbusConnection, BaseConnectionManager *cm);
    ~Adaptee();

    QStringList interfaces() const;
    Tp::ProtocolPropertiesMap protocols() const;

Q_SIGNALS:
    void newConnection(const QString &busName, const QDBusObjectPath &objectPath, const QString &protocolName);

private Q_SLOTS:
    void getParameters(const QString &protocolName,
            const Tp::Service::ConnectionManagerAdaptor::GetParametersContextPtr &context);
    void listProtocols(
            const Tp::Service::ConnectionManagerAdaptor::ListProtocolsContextPtr &context);
    void requestConnection(const QString &protocolName, const QVariantMap &parameters,
            const Tp::Service::ConnectionManagerAdaptor::RequestConnectionContextPtr &context);

public:
    BaseConnectionManager *mCM;
    Service::ConnectionManagerAdaptor *mAdaptor;
};

}
