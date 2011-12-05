/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/Feature>

namespace Tp
{

struct TP_QT_NO_EXPORT Feature::Private : public QSharedData
{
    Private(bool critical) : critical(critical) {}

    bool critical;
};

/**
 * \class Feature
 * \ingroup utils
 * \headerfile TelepathyQt/feature.h <TelepathyQt/Feature>
 *
 * \brief The Feature class represents a feature that can be enabled
 * on demand.
 */

Feature::Feature()
    : QPair<QString, uint>()
{
}

Feature::Feature(const QString &className, uint id, bool critical)
    : QPair<QString, uint>(className, id),
      mPriv(new Private(critical))
{
}

Feature::Feature(const Feature &other)
    : QPair<QString, uint>(other.first, other.second),
      mPriv(other.mPriv)
{
}

Feature::~Feature()
{
}

Feature &Feature::operator=(const Feature &other)
{
    *this = other;
    this->mPriv = other.mPriv;
    return *this;
}

bool Feature::isCritical() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->critical;
}

/**
 * \class Features
 * \ingroup utils
 * \headerfile TelepathyQt/feature.h <TelepathyQt/Features>
 *
 * \brief The Features class represents a list of Feature.
 */

} // Tp
