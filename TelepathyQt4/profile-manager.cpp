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

#include <TelepathyQt4/ProfileManager>

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Profile>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ProfileManager::Private
{
    Private();

    void init();

    QHash<QString, ProfilePtr> profiles;
};

ProfileManager::Private::Private()
{
    init();
}

void ProfileManager::Private::init()
{
    QStringList searchDirs = Profile::searchDirs();

    foreach (const QString searchDir, searchDirs) {
        QDir dir(searchDir);
        dir.setFilter(QDir::Files);

        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fi = list.at(i);

            if (fi.completeSuffix() !=  QLatin1String("profile")) {
                continue;
            }

            QString fileName = fi.absoluteFilePath();
            QString serviceName = fi.baseName();

            if (profiles.contains(serviceName)) {
                debug() << "Profile for service" << serviceName << "already "
                    "exists. Ignoring profile file" << fileName;
                continue;
            }

            ProfilePtr profile = ProfilePtr(new Profile());
            profile->setFileName(fileName);
            debug() << "parsing profile file" << fileName;
            if (!profile->isValid()) {
                warning() << "error parsing profile file" << fileName;
                continue;
            }

            profiles.insert(serviceName, profile);
        }
    }
}

/**
 * \class ProfileManager
 * \headerfile TelepathyQt4/profile-manager.h <TelepathyQt4/ProfileManager>
 *
 * \brief The ProfileManager class provides helper methods to retrieve Profile
 * objects.
 */

/**
 * Create a new ProfileManager object.
 */
ProfileManagerPtr ProfileManager::create()
{
    return ProfileManagerPtr(new ProfileManager());
}

/**
 * Construct a new ProfileManager object.
 */
ProfileManager::ProfileManager()
    : mPriv(new Private())
{
}

/**
 * Class destructor.
 */
ProfileManager::~ProfileManager()
{
    delete mPriv;
}

/**
 * Return a list of all available profiles.
 *
 * \return A list of all available profiles.
 */
QList<ProfilePtr> ProfileManager::profiles() const
{
    return mPriv->profiles.values();
}

/**
 * Return a list of all available profiles for a given connection manager.
 *
 * \param cmName Connection manager name.
 * \return A list of all available profiles for a given connection manager.
 */
QList<ProfilePtr> ProfileManager::profilesForCM(const QString &cmName) const
{
    QList<ProfilePtr> ret;
    foreach (const ProfilePtr &profile, mPriv->profiles) {
        if (profile->cmName() == cmName) {
            ret << profile;
        }
    }
    return ret;
}

/**
 * Return a list of all available profiles for a given \a protocol.
 *
 * \param protocolName Protocol name.
 * \return A list of all available profiles for a given \a protocol.
 */
QList<ProfilePtr> ProfileManager::profilesForProtocol(
        const QString &protocolName) const
{
    QList<ProfilePtr> ret;
    foreach (const ProfilePtr &profile, mPriv->profiles) {
        if (profile->protocolName() == protocolName) {
            ret << profile;
        }
    }
    return ret;
}

/**
 * Return the profile for a given \a service.
 *
 * \param serviceName Service name.
 * \return The profile for \a service.
 */
ProfilePtr ProfileManager::profileForService(const QString &serviceName) const
{
    return mPriv->profiles.value(serviceName);
}

} // Tp
