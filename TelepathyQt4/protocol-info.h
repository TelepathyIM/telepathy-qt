/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_protocol_info_h_HEADER_GUARD_
#define _TelepathyQt4_protocol_info_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/AvatarSpec>
#include <TelepathyQt4/Global>
#include <TelepathyQt4/PresenceSpec>
#include <TelepathyQt4/ProtocolParameter>
#include <TelepathyQt4/Types>

#include <QSharedDataPointer>
#include <QString>
#include <QList>

namespace Tp
{

class ConnectionCapabilities;

class TELEPATHY_QT4_EXPORT ProtocolInfo
{
public:
    ProtocolInfo();
    ProtocolInfo(const ProtocolInfo &other);
    ~ProtocolInfo();

    bool isValid() const { return mPriv.constData() != 0; }

    ProtocolInfo &operator=(const ProtocolInfo &other);

    QString cmName() const;

    QString name() const;

    ProtocolParameterList parameters() const;
    bool hasParameter(const QString &name) const;

    bool canRegister() const;

    ConnectionCapabilities capabilities() const;

    QString vcardField() const;

    QString englishName() const;

    QString iconName() const;

    PresenceSpecList allowedPresenceStatuses() const;

    AvatarSpec avatarRequirements() const;

private:
    friend class ConnectionManager;

    TELEPATHY_QT4_NO_EXPORT ProtocolInfo(const QString &cmName, const QString &name);

    TELEPATHY_QT4_NO_EXPORT void addParameter(const ParamSpec &spec);
    TELEPATHY_QT4_NO_EXPORT void setVCardField(const QString &vcardField);
    TELEPATHY_QT4_NO_EXPORT void setEnglishName(const QString &englishName);
    TELEPATHY_QT4_NO_EXPORT void setIconName(const QString &iconName);
    TELEPATHY_QT4_NO_EXPORT void setRequestableChannelClasses(const RequestableChannelClassList &caps);
    TELEPATHY_QT4_NO_EXPORT void setAllowedPresenceStatuses(const PresenceSpecList &statuses);
    TELEPATHY_QT4_NO_EXPORT void setAvatarRequirements(const AvatarSpec &avatarRequirements);

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

typedef QList<ProtocolInfo> ProtocolInfoList;

} // Tp

Q_DECLARE_METATYPE(Tp::ProtocolInfo);
Q_DECLARE_METATYPE(Tp::ProtocolInfoList);

#endif
