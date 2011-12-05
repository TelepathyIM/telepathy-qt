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

#include <TelepathyQt/LocationInfo>

#include <QDBusArgument>

namespace Tp
{

struct TP_QT_NO_EXPORT LocationInfo::Private : public QSharedData
{
    QVariantMap location;
};

/**
 * \class LocationInfo
 * \ingroup clientconn
 * \headerfile TelepathyQt/location-info.h <TelepathyQt/LocationInfo>
 *
 * \brief The LocationInfo class represents the location of a
 * Telepathy Contact.
 */

/**
 * Construct a new LocationInfo object.
 */
LocationInfo::LocationInfo()
    : mPriv(new Private)
{
}

LocationInfo::LocationInfo(const QVariantMap &location)
    : mPriv(new Private)
{
    mPriv->location = location;
}

LocationInfo::LocationInfo(const LocationInfo &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
LocationInfo::~LocationInfo()
{
}

LocationInfo &LocationInfo::operator=(const LocationInfo &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

QString LocationInfo::countryCode() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("countrycode")));
}

QString LocationInfo::country() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("country")));
}

QString LocationInfo::region() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("region")));
}

QString LocationInfo::locality() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("locality")));
}

QString LocationInfo::area() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("area")));
}

QString LocationInfo::postalCode() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("postalcode")));
}

QString LocationInfo::street() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("street")));
}

QString LocationInfo::building() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("building")));
}

QString LocationInfo::floor() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("floor")));
}

QString LocationInfo::room() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("room")));
}

QString LocationInfo::text() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("text")));
}

QString LocationInfo::description() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("description")));
}

QString LocationInfo::uri() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("uri")));
}

QString LocationInfo::language() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("language")));
}

double LocationInfo::latitude() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("lat")));
}

double LocationInfo::longitude() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("lon")));
}

double LocationInfo::altitude() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("alt")));
}

double LocationInfo::accuracy() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("accuracy")));
}

double LocationInfo::speed() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("speed")));
}

double LocationInfo::bearing() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("bearing")));
}

QDateTime LocationInfo::timestamp() const
{
    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    qlonglong t = qdbus_cast<qlonglong>(mPriv->location.value(
                QLatin1String("timestamp")));
    if (t != 0) {
        return QDateTime::fromTime_t((uint) t);
    }
    return QDateTime();
}

QVariantMap LocationInfo::allDetails() const
{
    return mPriv->location;
}

void LocationInfo::updateData(const QVariantMap &location)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->location = location;
}

} // Tp
