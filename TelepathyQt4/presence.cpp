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

#include <TelepathyQt4/Presence>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT Presence::Private : public QSharedData
{
    Private(const SimplePresence &sp)
        : sp(sp)
    {
    }

    Private(ConnectionPresenceType type, const QString &status, const QString &statusMessage)
    {
        sp.type = type;
        sp.status = status;
        sp.statusMessage = statusMessage;
    }

    SimplePresence sp;
};

Presence::Presence()
{
}

Presence::Presence(const SimplePresence &sp)
    : mPriv(new Private(sp))
{
}

Presence::Presence(ConnectionPresenceType type, const QString &status, const QString &statusMessage)
    : mPriv(new Private(type, status, statusMessage))
{
}

Presence::Presence(const Presence &other)
    : mPriv(other.mPriv)
{
}

Presence::~Presence()
{
}

Presence Presence::available(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeAvailable, QLatin1String("available"), statusMessage);
}

Presence Presence::away(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeAway, QLatin1String("away"), statusMessage);
}

Presence Presence::brb(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeAway, QLatin1String("brb"), statusMessage);
}

Presence Presence::busy(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeBusy, QLatin1String("busy"), statusMessage);
}

Presence Presence::xa(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeExtendedAway, QLatin1String("xa"), statusMessage);
}

Presence Presence::hidden(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeHidden, QLatin1String("hidden"), statusMessage);
}

Presence Presence::offline(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeOffline, QLatin1String("offline"), statusMessage);
}

Presence &Presence::operator=(const Presence &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool Presence::operator==(const Presence &other) const
{
    return mPriv->sp == other.mPriv->sp;
}

ConnectionPresenceType Presence::type() const
{
    if (!isValid()) {
        return ConnectionPresenceTypeUnknown;
    }

    return (ConnectionPresenceType) mPriv->sp.type;
}

QString Presence::status() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->sp.status;
}

QString Presence::statusMessage() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->sp.statusMessage;
}

void Presence::setStatus(const SimplePresence &value)
{
    if (!isValid()) {
        mPriv = new Private(value);
        return;
    }

    mPriv->sp = value;
}

void Presence::setStatus(ConnectionPresenceType type, const QString &status,
        const QString &statusMessage)
{
    if (!isValid()) {
        mPriv = new Private(type, status, statusMessage);
        return;
    }

    mPriv->sp.type = type;
    mPriv->sp.status = status;
    mPriv->sp.statusMessage = statusMessage;
}

} // Tp
