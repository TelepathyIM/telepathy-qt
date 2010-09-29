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

#include "TelepathyQt4/_gen/profile-manager.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Profile>
#include <TelepathyQt4/ReadinessHelper>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ProfileManager::Private
{
    Private(ProfileManager *parent);

    static void introspectMain(Private *self);

    ProfileManager *parent;
    ReadinessHelper *readinessHelper;
    QHash<QString, ProfilePtr> profiles;
};

ProfileManager::Private::Private(ProfileManager *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper())
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
}

void ProfileManager::Private::introspectMain(ProfileManager::Private *self)
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

            if (self->profiles.contains(serviceName)) {
                debug() << "Profile for service" << serviceName << "already "
                    "exists. Ignoring profile file:" << fileName;
                continue;
            }

            ProfilePtr profile = Profile::createForFileName(fileName);
            if (!profile->isValid()) {
                continue;
            }

            if (profile->type() != QLatin1String("IM")) {
                debug() << "Ignoring profile for service" << serviceName <<
                    ": type != IM. Profile file:" << fileName;
                continue;
            }

            debug() << "Found profile for service" << serviceName <<
                "- profile file:" << fileName;
            self->profiles.insert(serviceName, profile);
        }
    }

    self->readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

/**
 * \class ProfileManager
 * \headerfile TelepathyQt4/profile-manager.h <TelepathyQt4/ProfileManager>
 *
 * \brief The ProfileManager class provides helper methods to retrieve Profile
 * objects.
 */

/**
 * Feature representing the core that needs to become ready to make the ProfileManager
 * object usable.
 *
 * Note that this feature must be enabled in order to use all ProfileManager methods.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature ProfileManager::FeatureCore = Feature(QLatin1String(ProfileManager::staticMetaObject.className()), 0, true);

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
    : QObject(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
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
