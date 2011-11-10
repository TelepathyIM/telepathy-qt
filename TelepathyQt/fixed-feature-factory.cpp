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

#include <TelepathyQt/FixedFeatureFactory>

#include "TelepathyQt/_gen/fixed-feature-factory.moc.hpp"

#include <TelepathyQt/Feature>

namespace Tp
{

struct TP_QT_NO_EXPORT FixedFeatureFactory::Private
{
    Features features;
};

/**
 * \class FixedFeatureFactory
 * \ingroup utils
 * \headerfile TelepathyQt/fixed-feature-factory.h <TelepathyQt/FixedFeatureFactory>
 *
 * \brief The FixedFeatureFactory class is a base class for all D-Bus proxy
 * factories which want the same set of features for all constructed proxies.
 */

/**
 * Class constructor.
 *
 * The intention for storing the bus here is that it generally doesn't make sense to construct
 * proxies for multiple buses in the same context. Allowing that would lead to more complex keying
 * needs in the cache, as well.
 *
 * \param bus The D-Bus bus connection for the objects constructed using this factory.
 */
FixedFeatureFactory::FixedFeatureFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus), mPriv(new Private)
{
}

/**
 * Class destructor.
 */
FixedFeatureFactory::~FixedFeatureFactory()
{
    delete mPriv;
}

/**
 * Gets the features this factory will make ready on constructed proxies.
 *
 * \return The set of features.
 */
Features FixedFeatureFactory::features() const
{
    return mPriv->features;
}

/**
 * Adds a single feature this factory will make ready on further constructed proxies.
 *
 * No feature removal is provided, to guard against uncooperative modules removing features other
 * modules have set and depend on.
 *
 * \param feature The feature to add.
 */
void FixedFeatureFactory::addFeature(const Feature &feature)
{
    addFeatures(Features(feature));
}

/**
 * Adds a set of features this factory will make ready on further constructed proxies.
 *
 * No feature removal is provided, to guard against uncooperative modules removing features other
 * modules have set and depend on.
 *
 * \param features The features to add.
 */
void FixedFeatureFactory::addFeatures(const Features &features)
{
    mPriv->features.unite(features);
}

/**
 * Fixed implementation of the per-proxy feature getter.
 *
 * \return features(), irrespective of the actual \a proxy.
 */
Features FixedFeatureFactory::featuresFor(const DBusProxyPtr &proxy) const
{
    Q_UNUSED(proxy);

    return features();
}

}
