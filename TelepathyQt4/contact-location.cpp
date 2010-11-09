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

#include <TelepathyQt4/ContactLocation>

#include <QDBusArgument>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ContactLocation::Private : public QSharedData
{
    QVariantMap location;
};

/**
 * \class ContactLocation
 * \ingroup clientconn
 * \headerfile TelepathyQt4/contact-location.h <TelepathyQt4/ContactLocation>
 *
 * \brief The ContactLocation class provides an object representing the
 * location of a Contact.
 */

/**
 * Construct a new ContactLocation object.
 */
ContactLocation::ContactLocation()
    : mPriv(new Private)
{
}

ContactLocation::ContactLocation(const QVariantMap &location)
    : mPriv(new Private)
{
    mPriv->location = location;
}

ContactLocation::ContactLocation(const ContactLocation &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
ContactLocation::~ContactLocation()
{
}

ContactLocation &ContactLocation::operator=(const ContactLocation &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

QString ContactLocation::countryCode() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("countrycode")));
}

QString ContactLocation::country() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("country")));
}

QString ContactLocation::region() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("region")));
}

QString ContactLocation::locality() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("locality")));
}

QString ContactLocation::area() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("area")));
}

QString ContactLocation::postalCode() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("postalcode")));
}

QString ContactLocation::street() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("street")));
}

QString ContactLocation::building() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("building")));
}

QString ContactLocation::floor() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("floor")));
}

QString ContactLocation::room() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("room")));
}

QString ContactLocation::text() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("text")));
}

QString ContactLocation::description() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("description")));
}

QString ContactLocation::uri() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("uri")));
}

QString ContactLocation::language() const
{
    return qdbus_cast<QString>(mPriv->location.value(
                QLatin1String("language")));
}

double ContactLocation::latitude() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("lat")));
}

double ContactLocation::longitude() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("lon")));
}

double ContactLocation::altitude() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("alt")));
}

double ContactLocation::accuracy() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("accuracy")));
}

double ContactLocation::speed() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("speed")));
}

double ContactLocation::bearing() const
{
    return qdbus_cast<double>(mPriv->location.value(
                QLatin1String("bearing")));
}

QDateTime ContactLocation::timestamp() const
{
    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    qlonglong t = qdbus_cast<qlonglong>(mPriv->location.value(
                QLatin1String("timestamp")));
    if (t != 0) {
        return QDateTime::fromTime_t((uint) t);
    }
    return QDateTime();
}

QVariantMap ContactLocation::allDetails() const
{
    return mPriv->location;
}

void ContactLocation::updateData(const QVariantMap &location)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->location = location;
}

} // Tp
