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
    Q_PROPERTY(QStringList interfaces READ interfaces)
    Q_PROPERTY(Tp::ParamSpecList parameters READ parameters)
    Q_PROPERTY(QStringList connectionInterfaces READ connectionInterfaces)
    Q_PROPERTY(Tp::RequestableChannelClassList requestableChannelClasses READ requestableChannelClasses)
    Q_PROPERTY(QString vcardField READ vcardField)
    Q_PROPERTY(QString englishName READ englishName)
    Q_PROPERTY(QString icon READ icon)
    Q_PROPERTY(QStringList authenticationTypes READ authenticationTypes)

public:
    Adaptee(const QDBusConnection &dbusConnection, BaseProtocol *protocol);
    ~Adaptee();

    QStringList interfaces() const;
    QStringList connectionInterfaces() const;
    ParamSpecList parameters() const;
    RequestableChannelClassList requestableChannelClasses() const;
    QString vcardField() const;
    QString englishName() const;
    QString icon() const;
    QStringList authenticationTypes() const;

private Q_SLOTS:
    void identifyAccount(const QVariantMap &parameters,
            const Tp::Service::ProtocolAdaptor::IdentifyAccountContextPtr &context);
    void normalizeContact(const QString &contactId,
            const Tp::Service::ProtocolAdaptor::NormalizeContactContextPtr &context);

public:
    BaseProtocol *mProtocol;
};

class TP_QT_NO_EXPORT BaseProtocolAddressingInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList addressableVCardFields READ addressableVCardFields)
    Q_PROPERTY(QStringList addressableURISchemes READ addressableURISchemes)

public:
    Adaptee(BaseProtocolAddressingInterface *interface);
    ~Adaptee();

    QStringList addressableVCardFields() const;
    QStringList addressableURISchemes() const;

private Q_SLOTS:
    void normalizeVCardAddress(const QString &vcardField, const QString &vcardAddress,
            const Tp::Service::ProtocolInterfaceAddressingAdaptor::NormalizeVCardAddressContextPtr &context);
    void normalizeContactURI(const QString &uri,
            const Tp::Service::ProtocolInterfaceAddressingAdaptor::NormalizeContactURIContextPtr &context);

public:
    BaseProtocolAddressingInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseProtocolAvatarsInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList supportedAvatarMIMETypes READ supportedAvatarMIMETypes)
    Q_PROPERTY(uint minimumAvatarHeight READ minimumAvatarHeight)
    Q_PROPERTY(uint minimumAvatarWidth READ minimumAvatarWidth)
    Q_PROPERTY(uint recommendedAvatarHeight READ recommendedAvatarHeight)
    Q_PROPERTY(uint recommendedAvatarWidth READ recommendedAvatarWidth)
    Q_PROPERTY(uint maximumAvatarHeight READ maximumAvatarHeight)
    Q_PROPERTY(uint maximumAvatarWidth READ maximumAvatarWidth)
    Q_PROPERTY(uint maximumAvatarBytes READ maximumAvatarBytes)

public:
    Adaptee(BaseProtocolAvatarsInterface *interface);
    ~Adaptee();

    QStringList supportedAvatarMIMETypes() const;
    uint minimumAvatarHeight() const;
    uint minimumAvatarWidth() const;
    uint recommendedAvatarHeight() const;
    uint recommendedAvatarWidth() const;
    uint maximumAvatarHeight() const;
    uint maximumAvatarWidth() const;
    uint maximumAvatarBytes() const;

public:
    BaseProtocolAvatarsInterface *mInterface;
};

class TP_QT_NO_EXPORT BaseProtocolPresenceInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Tp::SimpleStatusSpecMap statuses READ statuses)

public:
    Adaptee(BaseProtocolPresenceInterface *interface);
    ~Adaptee();

    SimpleStatusSpecMap statuses() const;

public:
    BaseProtocolPresenceInterface *mInterface;
};

}
