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

#include <TelepathyQt4/RequestableChannelClassSpec>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT RequestableChannelClassSpec::Private : public QSharedData
{
    Private(const RequestableChannelClass &rcc)
        : rcc(rcc) {}

    RequestableChannelClass rcc;
};

RequestableChannelClassSpec::RequestableChannelClassSpec(const RequestableChannelClass &rcc)
    : mPriv(new Private(rcc))
{
}

RequestableChannelClassSpec::RequestableChannelClassSpec()
{
}

RequestableChannelClassSpec::RequestableChannelClassSpec(const RequestableChannelClassSpec &other)
    : mPriv(other.mPriv)
{
}

RequestableChannelClassSpec::~RequestableChannelClassSpec()
{
}

RequestableChannelClassSpec &RequestableChannelClassSpec::operator=(const RequestableChannelClassSpec &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool RequestableChannelClassSpec::operator==(const RequestableChannelClassSpec &other) const
{
    return mPriv->rcc == other.mPriv->rcc;
}

bool RequestableChannelClassSpec::supports(const RequestableChannelClassSpec &other) const
{
    if (mPriv->rcc.fixedProperties == other.mPriv->rcc.fixedProperties) {
        foreach (const QString &prop, other.mPriv->rcc.allowedProperties) {
            if (!mPriv->rcc.allowedProperties.contains(prop)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

QString RequestableChannelClassSpec::channelType() const
{
    if (!isValid()) {
        return QString();
    }
    return qdbus_cast<QString>(mPriv->rcc.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
}

bool RequestableChannelClassSpec::hasTargetHandleType() const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"));
}

uint RequestableChannelClassSpec::targetHandleType() const
{
    if (!hasTargetHandleType()) {
        return (uint) -1;
    }
    return qdbus_cast<uint>(mPriv->rcc.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
}

bool RequestableChannelClassSpec::hasFixedProperty(const QString &name) const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(name);
}

QVariant RequestableChannelClassSpec::fixedProperty(const QString &name) const
{
    if (!isValid()) {
        return QVariant();
    }
    return mPriv->rcc.fixedProperties.value(name);
}

QVariantMap RequestableChannelClassSpec::fixedProperties() const
{
    if (!isValid()) {
        return QVariantMap();
    }
    return mPriv->rcc.fixedProperties;
}

bool RequestableChannelClassSpec::allowsProperty(const QString &name) const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.allowedProperties.contains(name);
}

QStringList RequestableChannelClassSpec::allowedProperties() const
{
    if (!isValid()) {
        return QStringList();
    }
    return mPriv->rcc.allowedProperties;
}

RequestableChannelClass RequestableChannelClassSpec::bareClass() const
{
    return isValid() ? mPriv->rcc : RequestableChannelClass();
}

} // Tp
