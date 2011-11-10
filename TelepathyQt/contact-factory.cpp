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

#include <TelepathyQt/ContactFactory>

namespace Tp
{

struct TP_QT_NO_EXPORT ContactFactory::Private
{
    Features features;
};

/**
 * \class ContactFactory
 * \ingroup utils
 * \headerfile TelepathyQt/contact-factory.h <TelepathyQt/ContactFactory>
 *
 * \brief The ContactFactory class is responsible for constructing Contact
 * objects according to application-defined settings.
 */

/**
 * Creates a new ContactFactory.
 *
 * \param features The features to make ready on constructed contacts.
 * \returns A pointer to the created factory.
 */
ContactFactoryPtr ContactFactory::create(const Features &features)
{
    return ContactFactoryPtr(new ContactFactory(features));
}

/**
 * Class constructor.
 *
 * \param features The features to make ready on constructed contacts.
 */
ContactFactory::ContactFactory(const Features &features)
    : mPriv(new Private)
{
    addFeatures(features);
}

/**
 * Class destructor.
 */
ContactFactory::~ContactFactory()
{
    delete mPriv;
}

/**
 * Gets the features this factory will make ready on constructed contacts.
 *
 * \return The set of features.
 */
Features ContactFactory::features() const
{
    Features features = mPriv->features;
    // FeatureAvatarData depends on FeatureAvatarToken
    if (features.contains(Contact::FeatureAvatarData) &&
        !features.contains(Contact::FeatureAvatarToken)) {
        features.insert(Contact::FeatureAvatarToken);
    }

    return features;
}

/**
 * Adds a single feature this factory will make ready on further constructed contacts.
 *
 * No feature removal is provided, to guard against uncooperative modules removing features other
 * modules have set and depend on.
 *
 * \param feature The feature to add.
 */
void ContactFactory::addFeature(const Feature &feature)
{
    addFeatures(Features(feature));
}

/**
 * Adds a set of features this factory will make ready on further constructed contacts.
 *
 * No feature removal is provided, to guard against uncooperative modules removing features other
 * modules have set and depend on.
 *
 * \param features The features to add.
 */
void ContactFactory::addFeatures(const Features &features)
{
    mPriv->features.unite(features);
}

/**
 * Can be used by subclasses to override the Contact subclass constructed by the factory.
 *
 * The default implementation constructs Tp::Contact objects.
 *
 * \param manager The contact manager this contact belongs.
 * \param handle The contact handle.
 * \param features The desired contact features.
 * \param attributes The desired contact attributes.
 * \return A pointer to the constructed contact.
 */
ContactPtr ContactFactory::construct(Tp::ContactManager *manager, const ReferencedHandles &handle,
        const Features &features, const QVariantMap &attributes) const
{
    ContactPtr contact = ContactPtr(new Contact(manager, handle, features, attributes));
    return contact;
}

/**
 * Can be used by subclasses to do arbitrary manipulation on constructed Contact objects.
 *
 * The default implementation does nothing.
 *
 * \param contact The contact to be prepared.
 * \return A PendingOperation used to prepare the contact or NULL if there is nothing to prepare.
 */
PendingOperation *ContactFactory::prepare(const ContactPtr &contact) const
{
    return NULL;
}

}
