/**
 * This file is part of TelepathyQt
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

#ifndef _TelepathyQt_protocol_info_h_HEADER_GUARD_
#define _TelepathyQt_protocol_info_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/AvatarSpec>
#include <TelepathyQt/Global>
#include <TelepathyQt/PresenceSpec>
#include <TelepathyQt/ProtocolParameter>
#include <TelepathyQt/Types>

#include <QSharedDataPointer>
#include <QString>
#include <QList>

namespace Tp
{

class ConnectionCapabilities;
class PendingString;

class TP_QT_EXPORT ProtocolInfo
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

    QStringList addressableVCardFields() const;
    QStringList addressableUriSchemes() const;

    PendingString *normalizeVCardAddress(const QString &vcardField, const QString &vcardAddress);
    PendingString *normalizeContactUri(const QString &uri);

private:
    friend class ConnectionManager;

    TP_QT_NO_EXPORT ProtocolInfo(const ConnectionManagerPtr &cm, const QString &name);

    TP_QT_NO_EXPORT void addParameter(const ParamSpec &spec);
    TP_QT_NO_EXPORT void setVCardField(const QString &vcardField);
    TP_QT_NO_EXPORT void setEnglishName(const QString &englishName);
    TP_QT_NO_EXPORT void setIconName(const QString &iconName);
    TP_QT_NO_EXPORT void setRequestableChannelClasses(const RequestableChannelClassList &caps);
    TP_QT_NO_EXPORT void setAllowedPresenceStatuses(const PresenceSpecList &statuses);
    TP_QT_NO_EXPORT void setAvatarRequirements(const AvatarSpec &avatarRequirements);
    TP_QT_NO_EXPORT void setAddressableVCardFields(const QStringList &vcardFields);
    TP_QT_NO_EXPORT void setAddressableUriSchemes(const QStringList &uriSchemes);

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

typedef QList<ProtocolInfo> ProtocolInfoList;

} // Tp

Q_DECLARE_METATYPE(Tp::ProtocolInfo);
Q_DECLARE_METATYPE(Tp::ProtocolInfoList);

#endif
