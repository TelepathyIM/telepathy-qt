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

#include <TelepathyQt/ProfileManager>

#include "TelepathyQt/_gen/profile-manager.moc.hpp"
#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/Profile>
#include <TelepathyQt/ReadinessHelper>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace Tp
{

struct TP_QT_NO_EXPORT ProfileManager::Private
{
    Private(ProfileManager *parent, const QDBusConnection &bus);

    static void introspectMain(Private *self);
    static void introspectFakeProfiles(Private *self);

    ProfileManager *parent;
    ReadinessHelper *readinessHelper;
    QDBusConnection bus;
    QHash<QString, ProfilePtr> profiles;
    QList<ConnectionManagerPtr> cms;
};

ProfileManager::Private::Private(ProfileManager *parent, const QDBusConnection &bus)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      bus(bus)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableFakeProfiles(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features() << FeatureCore,                                   // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectFakeProfiles,
        this);
    introspectables[FeatureFakeProfiles] = introspectableFakeProfiles;

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

void ProfileManager::Private::introspectFakeProfiles(ProfileManager::Private *self)
{
    PendingStringList *pendingCmNames = ConnectionManager::listNames(self->bus);
    self->parent->connect(pendingCmNames,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onCmNamesRetrieved(Tp::PendingOperation *)));
}

/**
 * \class ProfileManager
 * \headerfile TelepathyQt/profile-manager.h <TelepathyQt/ProfileManager>
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
 * Enabling this feature will make ProfileManager create fake Profile objects to all protocols
 * supported on the installed connection managers, even if they don't have .profile files installed
 * making use of them.
 *
 * Fake profiles are identified by Profile::isFake() returning \c true.
 *
 * The fake profile will contain the following info:
 *  - Profile::type() will return "IM"
 *  - Profile::provider() will return an empty string
 *  - Profile::serviceName() will return cmName-protocolName
 *  - Profile::name() and Profile::protocolName() will return protocolName
 *  - Profile::iconName() will return "im-protocolName"
 *  - Profile::cmName() will return cmName
 *  - Profile::parameters() will return a list matching CM default parameters for protocol with name
 *    protocolName.
 *  - Profile::presences() will return an empty list and
 *    Profile::allowOtherPresences() will return \c true, meaning that CM
 *    presences should be used
 *  - Profile::unsupportedChannelClassSpecs() will return an empty list
 *
 * Where cmName and protocolName are the name of the connection manager and the name of the protocol
 * for which this fake Profile is created, respectively.
 */
const Feature ProfileManager::FeatureFakeProfiles = Feature(QLatin1String(ProfileManager::staticMetaObject.className()), 1);

/**
 * Create a new ProfileManager object.
 */
ProfileManagerPtr ProfileManager::create(const QDBusConnection &bus)
{
    return ProfileManagerPtr(new ProfileManager(bus));
}

/**
 * Construct a new ProfileManager object.
 */
ProfileManager::ProfileManager(const QDBusConnection &bus)
    : Object(),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, bus))
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

void ProfileManager::onCmNamesRetrieved(Tp::PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "Getting available CMs failed with " <<
            op->errorName() << ":" << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureFakeProfiles, false,
                op->errorName(), op->errorMessage());
        return;
    }

    PendingStringList *pendingCmNames = qobject_cast<PendingStringList *>(op);
    QStringList cmNames(pendingCmNames->result());
    if (cmNames.isEmpty()) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureFakeProfiles, true);
        return;
    }

    QList<PendingOperation *> ops;
    foreach (const QString &cmName, cmNames) {
        ConnectionManagerPtr cm = ConnectionManager::create(mPriv->bus, cmName);
        mPriv->cms.append(cm);
        ops.append(cm->becomeReady());
    }

    PendingComposite *pc = new PendingComposite(ops, false, ProfileManagerPtr(this));
    connect(pc,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onCMsReady(Tp::PendingOperation *)));
}

void ProfileManager::onCMsReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Failed introspecting all CMs, trying to create fake profiles anyway";
    }

    ProfilePtr profile;
    foreach (const ConnectionManagerPtr &cm, mPriv->cms) {
        if (!cm->isReady()) {
            continue;
        }

        foreach (const QString &protocolName, cm->supportedProtocols()) {
            /* check if there is a profile whose service name is protocolName, and if found,
             * check if the profile is for cm, if not check if there is a profile whose service
             * name is cm-protocolName, and if not found create one named cm-protocolName. */
            profile = profileForService(protocolName);
            if (profile && profile->cmName() == cm->name()) {
                continue;
            }

            QString serviceName = QString(QLatin1String("%1-%2")).arg(cm->name()).arg(protocolName);
            profile = profileForService(serviceName);
            if (profile) {
                continue;
            }

            profile = ProfilePtr(new Profile(
                        serviceName,
                        cm->name(),
                        protocolName,
                        cm->protocol(protocolName)));
            mPriv->profiles.insert(serviceName, profile);
        }
    }

    mPriv->readinessHelper->setIntrospectCompleted(FeatureFakeProfiles, true);
}

} // Tp
