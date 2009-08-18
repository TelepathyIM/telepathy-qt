/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/PendingReady>

#include "TelepathyQt4/_gen/pending-ready.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingReady::Private
{
    Private(const Features &requestedFeatures, QObject *object) :
        requestedFeatures(requestedFeatures),
        object(object)
    {
    }

    Features requestedFeatures;
    QObject *object;
};

/**
 * \class PendingReady
 * \headerfile <TelepathyQt4/pending-ready.h> <TelepathyQt4/PendingReady>
 *
 * Class containing the features requested and the reply to a request
 * for an object to become ready. Instances of this class cannot be
 * constructed directly; the only way to get one is via Object::becomeReady().
 */

/**
 * Construct a PendingReady object.
 *
 * \param object The object that will become ready.
 */
PendingReady::PendingReady(const Features &requestedFeatures,
        QObject *object, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(requestedFeatures, object))
{
}

/**
 * Class destructor.
 */
PendingReady::~PendingReady()
{
    delete mPriv;
}

/**
 * Return the object through which the request was made.
 *
 * \return QObject representing the object through which the request was made.
 */
QObject *PendingReady::object() const
{
    return mPriv->object;
}

/**
 * Return the Features that were requested to become ready on the
 * object.
 *
 * \return Features.
 */
Features PendingReady::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

} // Tp
