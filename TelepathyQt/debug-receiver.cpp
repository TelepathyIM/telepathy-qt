/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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
#include <TelepathyQt/DebugReceiver>

#include "TelepathyQt/_gen/debug-receiver.moc.hpp"
#include "TelepathyQt/_gen/cli-debug-receiver-body.hpp"
#include "TelepathyQt/_gen/cli-debug-receiver.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/PendingDebugMessageList>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/ReadinessHelper>

namespace Tp
{

struct TP_QT_NO_EXPORT DebugReceiver::Private
{
    Private(DebugReceiver *parent);

    static void introspectCore(Private *self);

    DebugReceiver *parent;
    Client::DebugInterface *baseInterface;
};

DebugReceiver::Private::Private(DebugReceiver *parent)
    : parent(parent),
      baseInterface(new Client::DebugInterface(parent))
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
            QSet<uint>() << 0,                                                      // makesSenseForStatuses
            Features(),                                                             // dependsOnFeatures (core)
            QStringList(),                                                          // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &DebugReceiver::Private::introspectCore,
            this);
    introspectables[DebugReceiver::FeatureCore] = introspectableCore;

    parent->readinessHelper()->addIntrospectables(introspectables);
}


void DebugReceiver::Private::introspectCore(DebugReceiver::Private *self)
{
    //this is done only to verify that the object exists...
    PendingVariantMap *op = self->baseInterface->requestAllProperties();
    self->parent->connect(op,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onRequestAllPropertiesFinished(Tp::PendingOperation*)));
}

/**
 * \class DebugReceiver
 * \ingroup clientsideproxies
 * \headerfile TelepathyQt/debug-receiver.h <TelepathyQt/DebugReceiver>
 *
 * \brief The DebugReceiver class provides a D-Bus proxy for a Telepathy
 * Debug object.
 *
 * A Debug object provides debugging messages from services.
 */

/**
 * Feature representing the core that needs to become ready to make the DebugReceiver
 * object usable.
 *
 * Note that this feature must be enabled in order to use most DebugReceiver methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature DebugReceiver::FeatureCore = Feature(QLatin1String(DebugReceiver::staticMetaObject.className()), 0, true);

DebugReceiverPtr DebugReceiver::create(const QString &busName, const QDBusConnection &bus)
{
    return DebugReceiverPtr(new DebugReceiver(bus, busName));
}

DebugReceiver::DebugReceiver(const QDBusConnection &bus, const QString &busName)
    : StatefulDBusProxy(bus, busName, TP_QT_DEBUG_OBJECT_PATH, DebugReceiver::FeatureCore),
      mPriv(new Private(this))
{
}

DebugReceiver::~DebugReceiver()
{
    delete mPriv;
}

PendingDebugMessageList *DebugReceiver::fetchMessages() const
{
    return new PendingDebugMessageList(
            mPriv->baseInterface->GetMessages(),
            DebugReceiverPtr(const_cast<DebugReceiver*>(this)));
}

PendingOperation *DebugReceiver::setMonitoringEnabled(bool enabled)
{
    if (!isReady()) {
        warning() << "DebugReceiver::setMonitoringEnabled called without DebugReceiver being ready";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("FeatureCore is not ready"), DebugReceiverPtr(this));
    }

    return mPriv->baseInterface->setPropertyEnabled(enabled);
}

void DebugReceiver::onRequestAllPropertiesFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        readinessHelper()->setIntrospectCompleted(
            FeatureCore, false, op->errorName(), op->errorMessage());
    } else {
        connect(mPriv->baseInterface,
                SIGNAL(NewDebugMessage(double,QString,uint,QString)),
                SLOT(onNewDebugMessage(double,QString,uint,QString)));

        readinessHelper()->setIntrospectCompleted(FeatureCore, true);
    }
}

void DebugReceiver::onNewDebugMessage(double time, const QString &domain,
        uint level, const QString &message)
{
    DebugMessage msg;
    msg.timestamp = time;
    msg.domain = domain;
    msg.level = level;
    msg.message = message;

    emit newDebugMessage(msg);
}

} // Tp
