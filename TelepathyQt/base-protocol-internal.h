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

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class TP_QT_NO_EXPORT BaseProtocol::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList Interfaces READ interfaces)
    Q_PROPERTY(Tp::ParamSpecList Parameters READ parameters)
    Q_PROPERTY(QStringList ConnectionInterfaces READ connectionInterfaces)
    Q_PROPERTY(Tp::RequestableChannelClassList RequestableChannelClasses READ requestableChannelClasses)
    Q_PROPERTY(QString VCardField READ vcardField)
    Q_PROPERTY(QString EnglishName READ englishName)
    Q_PROPERTY(QString Icon READ iconName)
    Q_PROPERTY(QStringList AuthenticationTypes READ authenticationTypes)

public:
    Adaptee(const QDBusConnection &dbusConnection, BaseProtocol *protocol);
    ~Adaptee();

    QStringList interfaces() const;
    QStringList connectionInterfaces() const;
    ParamSpecList parameters() const;
    RequestableChannelClassList requestableChannelClasses() const;
    QString vcardField() const;
    QString englishName() const;
    QString iconName() const;
    QStringList authenticationTypes() const;

private Q_SLOTS:
    void identifyAccount(const QVariantMap &parameters,
            const Tp::Service::ProtocolAdaptor::IdentifyAccountContextPtr &context);
    void normalizeContact(const QString &contactID,
            const Tp::Service::ProtocolAdaptor::NormalizeContactContextPtr &context);

public:
    BaseProtocol *mProtocol;
    Service::ProtocolAdaptor *mAdaptor;
};

}
