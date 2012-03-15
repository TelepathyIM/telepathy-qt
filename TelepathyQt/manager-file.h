/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt_manager_file_h_HEADER_GUARD_
#define _TelepathyQt_manager_file_h_HEADER_GUARD_

#include <TelepathyQt/AvatarSpec>
#include <TelepathyQt/PresenceSpec>
#include <TelepathyQt/Types>

#include <QMetaType>
#include <QVariant>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace Tp
{

class TP_QT_NO_EXPORT ManagerFile
{
public:
    ManagerFile();
    ManagerFile(const ManagerFile &other);
    ManagerFile(const QString &cmName);
    ~ManagerFile();

    ManagerFile &operator=(const ManagerFile &other);

    QString cmName() const;

    bool isValid() const;
    QStringList protocols() const;
    ParamSpecList parameters(const QString &protocol) const;
    QString vcardField(const QString &protocol) const;
    QString englishName(const QString &protocol) const;
    QString iconName(const QString &protocol) const;
    RequestableChannelClassList requestableChannelClasses(
            const QString &protocol) const;
    PresenceSpecList allowedPresenceStatuses(const QString &protocol) const;
    AvatarSpec avatarRequirements(const QString &protocol) const;
    QStringList addressableVCardFields(const QString &protocol) const;
    QStringList addressableUriSchemes(const QString &protocol) const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}

Q_DECLARE_METATYPE(Tp::ManagerFile);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
