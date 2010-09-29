/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_profile_manager_h_HEADER_GUARD_
#define _TelepathyQt4_profile_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QtCore/QObject>

#include <TelepathyQt4/Profile>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>

namespace Tp
{

class TELEPATHY_QT4_EXPORT ProfileManager : public QObject,
                public ReadyObject,
                public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ProfileManager);

public:
    static const Feature FeatureCore;

    static ProfileManagerPtr create();

    ~ProfileManager();

    QList<ProfilePtr> profiles() const;
    QList<ProfilePtr> profilesForCM(const QString &cmName) const;
    QList<ProfilePtr> profilesForProtocol(const QString &protocolName) const;
    ProfilePtr profileForService(const QString &serviceName) const;

private:
    ProfileManager();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
