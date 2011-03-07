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

#ifndef _TelepathyQt4_profile_manager_h_HEADER_GUARD_
#define _TelepathyQt4_profile_manager_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Object>
#include <TelepathyQt4/Profile>
#include <TelepathyQt4/ReadyObject>
#include <TelepathyQt4/Types>

#include <QDBusConnection>
#include <QObject>

namespace Tp
{

class PendingOperation;

class TELEPATHY_QT4_EXPORT ProfileManager : public Object, public ReadyObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProfileManager);

public:
    static const Feature FeatureCore;
    static const Feature FeatureFakeProfiles;

    static ProfileManagerPtr create(const QDBusConnection &bus = QDBusConnection::sessionBus());

    ~ProfileManager();

    QList<ProfilePtr> profiles() const;
    QList<ProfilePtr> profilesForCM(const QString &cmName) const;
    QList<ProfilePtr> profilesForProtocol(const QString &protocolName) const;
    ProfilePtr profileForService(const QString &serviceName) const;

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onCmNamesRetrieved(Tp::PendingOperation *op);
    TELEPATHY_QT4_NO_EXPORT void onCMsReady(Tp::PendingOperation *op);

private:
    ProfileManager(const QDBusConnection &bus);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
