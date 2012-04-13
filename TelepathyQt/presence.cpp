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

#include <TelepathyQt/Presence>

#include "TelepathyQt/debug-internal.h"

namespace Tp
{

struct TP_QT_NO_EXPORT Presence::Private : public QSharedData
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
 * \headerfile TelepathyQt/presence.h <TelepathyQt/Presence>
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

Presence Presence::chat(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeAvailable, QLatin1String("chat"), statusMessage);
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

Presence Presence::dnd(const QString &statusMessage)
{
    return Presence(ConnectionPresenceTypeBusy, QLatin1String("dnd"), statusMessage);
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

bool Presence::operator!=(const Presence &other) const
{
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return false;
        }
        return true;
    }

    return mPriv->sp != other.mPriv->sp;
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

// Sets all fields
void Presence::setStatus(const SimplePresence &value)
{
    if (!isValid()) {
        mPriv = new Private(value);
        return;
    }

    mPriv->sp = value;
}

// TODO: explain in proper docs that we don't have setStatusType and setStatus(QString status)
// separately, because:
// 1) type and status are tightly related with each other
// 2) all statuses can't have status message so changing the status alone might make the presence
// illegal if a message was left around
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

void Presence::setStatusMessage(const QString &statusMessage)
{
    if (!isValid()) {
        return;
    }

    mPriv->sp.statusMessage = statusMessage;
}

SimplePresence Presence::barePresence() const
{
    if (!isValid()) {
        return SimplePresence();
    }

    return mPriv->sp;
}

struct TP_QT_NO_EXPORT PresenceSpec::Private : public QSharedData
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
 * \headerfile TelepathyQt/presence.h <TelepathyQt/PresenceSpec>
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

PresenceSpec PresenceSpec::available(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeAvailable;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("available"), spec);
}

PresenceSpec PresenceSpec::chat(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeAvailable;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("chat"), spec);
}

PresenceSpec PresenceSpec::pstn(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeAvailable;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("pstn"), spec);
}

PresenceSpec PresenceSpec::away(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeAway;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("away"), spec);
}

PresenceSpec PresenceSpec::brb(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeAway;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("brb"), spec);
}

PresenceSpec PresenceSpec::busy(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeBusy;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("busy"), spec);
}

PresenceSpec PresenceSpec::dnd(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeBusy;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("dnd"), spec);
}

PresenceSpec PresenceSpec::xa(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeExtendedAway;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("xa"), spec);
}

PresenceSpec PresenceSpec::hidden(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeHidden;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("hidden"), spec);
}

PresenceSpec PresenceSpec::offline(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeOffline;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("offline"), spec);
}

PresenceSpec PresenceSpec::unknown(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeUnknown;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("unknown"), spec);
}

PresenceSpec PresenceSpec::error(PresenceSpec::SimpleStatusFlags flags)
{
    SimpleStatusSpec spec;
    spec.type = ConnectionPresenceTypeError;
    spec.maySetOnSelf = flags & MaySetOnSelf;
    spec.canHaveMessage = flags & CanHaveStatusMessage;
    return PresenceSpec(QLatin1String("error"), spec);
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

bool PresenceSpec::operator!=(const PresenceSpec &other) const
{
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return false;
        }
        return true;
    }

    return (mPriv->status != other.mPriv->status) &&
           (mPriv->spec != other.mPriv->spec);
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
 * \headerfile TelepathyQt/presence.h <TelepathyQt/PresenceSpecList>
 *
 * \brief The PresenceSpecList class represents a list of PresenceSpec.
 */

} // Tp
