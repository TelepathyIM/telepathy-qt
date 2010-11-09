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

#include <TelepathyQt4/DBusProxy>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingReady::Private
{
    Private(const SharedPtr<const DBusProxyFactory> &factory, const Features &requestedFeatures,
            QObject *object, const DBusProxyPtr &proxy) :
        factory(factory),
        requestedFeatures(requestedFeatures),
        object(object),
        proxy(proxy)
    {
    }

    SharedPtr<const DBusProxyFactory> factory;
    Features requestedFeatures;
    QObject *object;
    DBusProxyPtr proxy;
};

/**
 * \class PendingReady
 * \headerfile TelepathyQt4/pending-ready.h <TelepathyQt4/PendingReady>
 *
 * \brief Class containing the features requested and the reply to a request
 * for an object to become ready.
 *
 * Instances of this class cannot be constructed directly; the only way to get one is via
 * Object::becomeReady() or a DBusProxyFactory subclass.
 */

/**
 * Construct a PendingReady object, which will wait for arbitrary manipulation on the proxy to
 * finish as appropriate for \a factory, specified by DBusProxyFactory::initialPrepare() and
 * DBusProxyFactory::readyPrepare().
 *
 * \todo Actually make it do the prepare ops. Currently they aren't taken into account in any way.
 *
 * \param factory The factory the request was made with.
 * \param requestedFeatures Features to be made ready on the object.
 * \param proxy The proxy in question.
 * \param parent QObject parent for the operation. Should not be the same as \a proxy to avoid
 * circular destruction.
 */
PendingReady::PendingReady(const SharedPtr<const DBusProxyFactory> &factory,
        const Features &requestedFeatures, const DBusProxyPtr &proxy, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(factory, requestedFeatures, 0, proxy))
{
    Q_ASSERT(!proxy.isNull());

    if (requestedFeatures.isEmpty()) {
        setFinished();
        return;
    }

    connect(proxy->becomeReady(requestedFeatures),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onNestedFinished(Tp::PendingOperation*)));
}

/**
 * Construct a PendingReady object.
 *
 * \param requestedFeatures Features to be made ready on the object.
 * \param object The object that will become ready.
 * \param parent QObject parent for the operation.
 */
PendingReady::PendingReady(const Features &requestedFeatures,
        QObject *object, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(SharedPtr<DBusProxyFactory>(), requestedFeatures, object,
                  DBusProxyPtr()))
{
    // This is a PendingReady created by ReadinessHelper, and will be set ready by it - so should
    // not do anything ourselves here.
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
 * This is only applicable for PendingReady objects from ReadyObject::becomeReady(). For others,
 * \c NULL is returned.
 *
 * \todo API/ABI break TODO: after shuffling the object hierarchy around, drop this and have just
 * ReadyObjectPtr PendingReady::object() const for all PendingReadys no matter the source
 *
 * \return The object through which the request was made.
 */
QObject *PendingReady::object() const
{
    return mPriv->object;
}

/**
 * Return the proxy constructed by the factory which is being made ready.
 *
 * This is only applicable for PendingReady objects from a DBusProxyFactory subclass. For others,
 * a \c NULL SharedPtr is returned.
 *
 * \return The proxy which is being made ready.
 */
DBusProxyPtr PendingReady::proxy() const
{
    return mPriv->proxy;
}

/**
 * Return the Features that were requested to become ready on the
 * object.
 *
 * \return Features The requested features
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
