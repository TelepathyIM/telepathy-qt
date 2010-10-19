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

bool RequestableChannelClassSpec::hasChannelType() const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"));
}

QString RequestableChannelClassSpec::channelType() const
{
    if (!hasChannelType()) {
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

HandleType RequestableChannelClassSpec::targetHandleType() const
{
    if (!hasTargetHandleType()) {
        return (HandleType) -1;
    }
    return (HandleType) qdbus_cast<uint>(mPriv->rcc.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
}

bool RequestableChannelClassSpec::hasFixedProperty(const char *name) const
{
    return hasFixedProperty(QLatin1String(name));
}

bool RequestableChannelClassSpec::hasFixedProperty(const QString &name) const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.fixedProperties.contains(name);
}

QVariant RequestableChannelClassSpec::fixedProperty(const char *name) const
{
    return fixedProperty(QLatin1String(name));
}

QVariant RequestableChannelClassSpec::fixedProperty(const QString &name) const
{
    if (!isValid()) {
        return QVariant();
    }
    return mPriv->rcc.fixedProperties.value(name);
}

bool RequestableChannelClassSpec::hasAllowedProperty(const char *name) const
{
    return hasAllowedProperty(QLatin1String(name));
}

bool RequestableChannelClassSpec::hasAllowedProperty(const QString &name) const
{
    if (!isValid()) {
        return false;
    }
    return mPriv->rcc.allowedProperties.contains(name);
}

RequestableChannelClass RequestableChannelClassSpec::requestableChannelClass() const
{
    return isValid() ? mPriv->rcc : RequestableChannelClass();
}

} // Tp
