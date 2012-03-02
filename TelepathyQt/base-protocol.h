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

#ifndef _TelepathyQt_base_protocol_h_HEADER_GUARD_
#define _TelepathyQt_base_protocol_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/ProtocolParameterList>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>

#include <QDBusConnection>

class QString;
class QStringList;

namespace Tp
{

class TP_QT_EXPORT BaseProtocol : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseProtocol)

public:
    template<typename BaseProtocolSubclass>
    static BaseProtocolPtr create(const QString &name)
    {
        return BaseProtocolPtr(new BaseProtocolSubclass(QDBusConnection::sessionBus(), name));
    }
    template<typename BaseProtocolSubclass>
    static BaseProtocolPtr create(const QDBusConnection &dbusConnection, const QString &name)
    {
        return BaseProtocolPtr(new BaseProtocolSubclass(dbusConnection, name));
    }

    virtual ~BaseProtocol();

    QString name() const;

    // Proto
    QStringList connectionInterfaces() const;
    void setConnectionInterfaces(const QStringList &connInterfaces);

    ProtocolParameterList parameters() const;
    void setParameters(const ProtocolParameterList &parameters);

    RequestableChannelClassSpecList requestableChannelClasses() const;
    void setRequestableChannelClasses(const RequestableChannelClassSpecList &rccSpecs);

    QString vcardField() const;
    void setVCardField(const QString &vcardField);

    QString englishName() const;
    void setEnglishName(const QString &englishName);

    QString iconName() const;
    void setIconName(const QString &iconName);

    QStringList authenticationTypes() const;
    void setAuthenticationTypes(const QStringList &authenticationTypes);

protected:
    BaseProtocol(const QDBusConnection &dbusConnection, const QString &name);

protected Q_SLOTS:
    // identifyAccount
    // normalizeContact

private:
    friend class BaseConnectionManager;
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

}

#endif
