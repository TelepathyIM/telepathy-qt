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

#include <TelepathyQt4/FixedFeatureFactory>

#include <TelepathyQt4/Feature>

namespace Tp
{

struct FixedFeatureFactory::Private
{
    Features features;
};

FixedFeatureFactory::FixedFeatureFactory(const QDBusConnection &bus)
    : DBusProxyFactory(bus), mPriv(new Private)
{
}

FixedFeatureFactory::~FixedFeatureFactory()
{
    delete mPriv;
}

Features FixedFeatureFactory::features() const
{
    return mPriv->features;
}

void FixedFeatureFactory::addFeature(const Feature &feature)
{
    addFeatures(Features(feature));
}

void FixedFeatureFactory::addFeatures(const Features &features)
{
    mPriv->features.unite(features);
}

Features FixedFeatureFactory::featuresFor(const SharedPtr<RefCounted> &proxy) const
{
    Q_UNUSED(proxy);

    return features();
}

}
