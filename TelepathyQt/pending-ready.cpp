/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#include <TelepathyQt/PendingReady>

#include "TelepathyQt/_gen/pending-ready.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/DBusProxy>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingReady::Private
{
   Private(const DBusProxyPtr &proxy,
           const Features &requestedFeatures)
        : proxy(proxy),
          requestedFeatures(requestedFeatures)
    {
    }

    DBusProxyPtr proxy;
    Features requestedFeatures;
};

/**
 * \class PendingReady
 * \ingroup utils
 * \headerfile TelepathyQt/pending-ready.h <TelepathyQt/PendingReady>
 *
 * \brief The PendingReady class represents the features requested and the reply
 * to a request for an object to become ready.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via ReadyObject::becomeReady() or a DBusProxyFactory subclass.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingReady object.
 *
 * \todo Actually make it do the prepare ops. Currently they aren't taken into account in any way.
 *
 * \param object The object that will become ready.
 * \param requestedFeatures Features to be made ready on the object.
 */
PendingReady::PendingReady(const SharedPtr<RefCounted> &object,
        const Features &requestedFeatures)
    : PendingOperation(object),
      mPriv(new Private(DBusProxyPtr(dynamic_cast<DBusProxy*>((DBusProxy*) object.data())),
                  requestedFeatures))
{
    // This is a PendingReady created by ReadinessHelper, and will be set ready by it - so should
    // not do anything ourselves here.
}

/**
 * Construct a new PendingReady object.
 *
 * \todo Actually make it do the prepare ops. Currently they aren't taken into account in any way.
 *
 * \param factory The factory the request was made with.
 * \param proxy The proxy that will become ready.
 * \param requestedFeatures Features to be made ready on the object.
 */
PendingReady::PendingReady(const SharedPtr<DBusProxyFactory> &factory,
        const DBusProxyPtr &proxy,
        const Features &requestedFeatures)
    : PendingOperation(factory),
      mPriv(new Private(proxy, requestedFeatures))
{
    if (requestedFeatures.isEmpty()) {
        setFinished();
        return;
    }

    connect(proxy->becomeReady(requestedFeatures),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onNestedFinished(Tp::PendingOperation*)));
}

/**
 * Class destructor.
 */
PendingReady::~PendingReady()
{
    delete mPriv;
}

/**
 * Return the proxy that should become ready.
 *
 * \return A pointer to the DBusProxy object if the operation was
 *         created by a proxy object or a DBusProxyFactory,
 *         otherwise a null DBusProxyPtr.
 */
DBusProxyPtr PendingReady::proxy() const
{
    return mPriv->proxy;
}

/**
 * Return the features that were requested to become ready on the
 * object.
 *
 * \return The requested features as a set of Feature objects.
 */
Features PendingReady::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

void PendingReady::onNestedFinished(Tp::PendingOperation *nested)
{
    Q_ASSERT(nested->isFinished());

    if (nested->isValid()) {
        setFinished();
    } else {
        warning() << "Nested PendingReady for" << object() << "failed with"
            << nested->errorName() << ":" << nested->errorMessage();
        setFinishedWithError(nested->errorName(), nested->errorMessage());
    }
}

} // Tp
