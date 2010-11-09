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

#include <TelepathyQt4/Object>

#include "TelepathyQt4/_gen/object.moc.hpp"

namespace Tp
{

/**
 * \class Object
 * \ingroup clientobject
 * \headerfile TelepathyQt4/object.h <TelepathyQt4/Object>
 *
 * \brief The Object class provides an object with property notification.
 */

/**
 * Construct a new Object object.
 */
Object::Object()
    : QObject()
{
}

/**
 * Class destructor.
 */
Object::~Object()
{
}

/**
 * Notify that a property named \a propertyName changed.
 *
 * This method will emit propertyChanged() for \a propertyName.
 */
void Object::notify(const char *propertyName)
{
    emit propertyChanged(QLatin1String(propertyName));
}

} // Tp
