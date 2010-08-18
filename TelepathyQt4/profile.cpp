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

#include <TelepathyQt4/Profile>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT Profile::Private
{
};

/**
 * \class Profile
 * \headerfile TelepathyQt4/profile.h <TelepathyQt4/Profile>
 *
 * \brief The Profile class provides an easy way to read telepathy profile
 * files according to http://telepathy.freedesktop.org/spec.html.
 */

/**
 * Construct a new Profile object used to read .profiles compliant files.
 */
Profile::Profile(const QString &serviceName)
    : mPriv(new Private())
{
}

/**
 * Class destructor.
 */
Profile::~Profile()
{
    delete mPriv;
}

QString Profile::serviceName() const
{
    return QString();
}

bool Profile::isValid() const
{
    return false;
}

QString Profile::type() const
{
    return QString();
}

QString Profile::provider() const
{
    return QString();
}

QString Profile::name() const
{
    return QString();
}

QString Profile::iconName() const
{
    return QString();
}

QString Profile::cmName() const
{
    return QString();
}

QString Profile::protocolName() const
{
    return QString();
}

RequestableChannelClassList Profile::unsupportedChannelClasses() const
{
    return RequestableChannelClassList();
}

} // Tp
