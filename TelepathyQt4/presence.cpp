/**
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/Presence>

#include "TelepathyQt4/debug-internal.h"

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

/**
 * \class Presence
 * \ingroup wrappers
 * \headerfile TelepathyQt4/presence.h <TelepathyQt4/Presence>
 *
 * \brief The Presence class represents a Telepathy simple presence.
 */

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
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return true;
        }
        return false;
    }

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

SimplePresence Presence::barePresence() const
{
    if (!isValid()) {
        return SimplePresence();
    }

    return mPriv->sp;
}

struct TELEPATHY_QT4_NO_EXPORT PresenceSpec::Private : public QSharedData
{
    Private(const QString &status, const SimpleStatusSpec &spec)
        : status(status),
          spec(spec)
    {
    }

    QString status;
    SimpleStatusSpec spec;
};

/**
 * \class PresenceSpec
 * \ingroup wrappers
 * \headerfile TelepathyQt4/presence.h <TelepathyQt4/PresenceSpec>
 *
 * \brief The PresenceSpec class represents a Telepathy presence information
 * supported by a protocol.
 */

PresenceSpec::PresenceSpec()
{
}

PresenceSpec::PresenceSpec(const QString &status, const SimpleStatusSpec &spec)
    : mPriv(new Private(status, spec))
{
}

PresenceSpec::PresenceSpec(const PresenceSpec &other)
    : mPriv(other.mPriv)
{
}

PresenceSpec::~PresenceSpec()
{
}

PresenceSpec &PresenceSpec::operator=(const PresenceSpec &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool PresenceSpec::operator==(const PresenceSpec &other) const
{
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return true;
        }
        return false;
    }

    return (mPriv->status == other.mPriv->status) &&
           (mPriv->spec == other.mPriv->spec);
}

bool PresenceSpec::operator<(const PresenceSpec &other) const
{
    if (!isValid()) {
        return false;
    }

    if (!other.isValid()) {
        return true;
    }

    return (mPriv->status < other.mPriv->status);
}

Presence PresenceSpec::presence(const QString &statusMessage) const
{
    if (!isValid()) {
        return Presence();
    }

    if (!canHaveStatusMessage() && !statusMessage.isEmpty()) {
        warning() << "Passing a status message to PresenceSpec with "
            "canHaveStatusMessage() being false";
    }

    return Presence((ConnectionPresenceType) mPriv->spec.type, mPriv->status, statusMessage);
}

bool PresenceSpec::maySetOnSelf() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->spec.maySetOnSelf;
}

bool PresenceSpec::canHaveStatusMessage() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->spec.canHaveMessage;
}

SimpleStatusSpec PresenceSpec::bareSpec() const
{
    if (!isValid()) {
        return SimpleStatusSpec();
    }

    return mPriv->spec;
}

/**
 * \class PresenceSpecList
 * \ingroup wrappers
 * \headerfile TelepathyQt4/presence.h <TelepathyQt4/PresenceSpecList>
 *
 * \brief The PresenceSpecList class represents a list of PresenceSpec.
 */

} // Tp
