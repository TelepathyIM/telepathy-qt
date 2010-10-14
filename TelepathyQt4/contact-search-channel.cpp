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

#include <TelepathyQt4/ContactSearchChannel>

#include "TelepathyQt4/_gen/contact-search-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Types>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ContactSearchChannel::Private
{
    Private(ContactSearchChannel *parent);
    ~Private();

    static void introspectMain(Private *self);

    // Public object
    ContactSearchChannel *parent;

    Client::ChannelTypeContactSearchInterface *contactSearchInterface;

    ReadinessHelper *readinessHelper;
};

ContactSearchChannel::Private::Private(ContactSearchChannel *parent)
    : parent(parent),
      contactSearchInterface(parent->contactSearchInterface(BypassInterfaceCheck)),
      readinessHelper(parent->readinessHelper())
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
}

ContactSearchChannel::Private::~Private()
{
}

void ContactSearchChannel::Private::introspectMain(ContactSearchChannel::Private *self)
{
    self->readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

/**
 * \class ContactSearchChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/contact-search-channel.h <TelepathyQt4/ContactSearchChannel>
 *
 * \brief The ContactSearchChannel class provides an object representing a
 * Telepathy channel of type ContactSearch.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * ContactSearchChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * ContactSearchChannel methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature ContactSearchChannel::FeatureCore = Feature(QLatin1String(ContactSearchChannel::staticMetaObject.className()), 0);

/**
 * Create a new ContactSearchChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A ContactSearchChannelPtr object pointing to the newly created
 *         ContactSearchChannel object.
 */
ContactSearchChannelPtr ContactSearchChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ContactSearchChannelPtr(new ContactSearchChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new contact search channel associated with the given \a objectPath
 * on the same service as the given \a connection.
 *
 * \param connection Connection owning this channel, and specifying the service.
 * \param objectPath Path to the object on the service.
 * \param immutableProperties The immutable properties of the channel.
 */
ContactSearchChannel::ContactSearchChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : Channel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
ContactSearchChannel::~ContactSearchChannel()
{
    delete mPriv;
}

} // Tp
