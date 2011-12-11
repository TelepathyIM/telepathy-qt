/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/debug-internal.h>
#include <TelepathyQt/ReadinessHelper>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/PendingDebugMessageList>

namespace Tp
{

struct TP_QT_NO_EXPORT DebugReceiver::Private
{
    Private(DebugReceiver *parent);

    static void introspectCore(Private *self);
    static void introspectMonitor(Private *self);

    DebugReceiver *parent;
    Client::DebugInterface *baseInterface;
    ReadinessHelper *readinessHelper;

    bool newDebugMessageEnabled;
    bool enabledByUs;
};

DebugReceiver::Private::Private(DebugReceiver *parent)
    : parent(parent),
      baseInterface(new Client::DebugInterface(parent)),
      readinessHelper(parent->readinessHelper()),
      newDebugMessageEnabled(false),
      enabledByUs(false)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
            QSet<uint>() << 0,                                                      // makesSenseForStatuses
            Features(),                                                             // dependsOnFeatures (core)
            QStringList(),                                                          // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &DebugReceiver::Private::introspectCore,
            this);
    introspectables[DebugReceiver::FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableMonitor(
            QSet<uint>() << 0,                                                      // makesSenseForStatuses
            Features() << DebugReceiver::FeatureCore,                               // dependsOnFeatures (core)
            QStringList(),                                                          // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &DebugReceiver::Private::introspectMonitor,
            this);
    introspectables[DebugReceiver::FeatureMonitor] = introspectableMonitor;

    readinessHelper->addIntrospectables(introspectables);
}


void DebugReceiver::Private::introspectCore(DebugReceiver::Private *self)
{
    //this is done mostly to verify that the object exists...
    PendingVariantMap *op = self->baseInterface->requestAllProperties();
    self->parent->connect(op,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onRequestAllPropertiesFinished(Tp::PendingOperation*)));
}

void DebugReceiver::onRequestAllPropertiesFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        mPriv->readinessHelper->setIntrospectCompleted(
            FeatureCore, false, op->errorName(), op->errorMessage());
    } else {
        PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);
        mPriv->newDebugMessageEnabled = pvm->result()[QLatin1String("Enabled")].toBool();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}


void DebugReceiver::Private::introspectMonitor(DebugReceiver::Private *self)
{
    if (!self->newDebugMessageEnabled) {
        PendingOperation *op = self->baseInterface->setPropertyEnabled(true);
        self->parent->connect(op,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onSetPropertyEnabledFinished(Tp::PendingOperation*)));
    } else {
        self->parent->connect(self->baseInterface,
                SIGNAL(NewDebugMessage(double,QString,uint,QString)),
                SLOT(onNewDebugMessage(double,QString,uint,QString)));

        self->readinessHelper->setIntrospectCompleted(FeatureMonitor, true);
    }
}

void DebugReceiver::onSetPropertyEnabledFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        mPriv->readinessHelper->setIntrospectCompleted(
                FeatureMonitor, false, op->errorName(), op->errorMessage());
    } else {
        debug() << "DebugReceiver: Enabled emission of NewDebugMessage";
        mPriv->newDebugMessageEnabled = true;
        mPriv->enabledByUs = true;
        Private::introspectMonitor(mPriv);
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

    Q_EMIT newDebugMessage(msg);
}


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

const Feature DebugReceiver::FeatureMonitor = Feature(QLatin1String(DebugReceiver::staticMetaObject.className()), 1);


DebugReceiverPtr DebugReceiver::create(const QString &busName, const QDBusConnection &bus)
{
    return DebugReceiverPtr(new DebugReceiver(bus, busName,
                QLatin1String("/org/freedesktop/Telepathy/debug"),
                DebugReceiver::FeatureCore));
}

DebugReceiver::DebugReceiver(const QDBusConnection &bus,
        const QString &busName,
        const QString &objectPath,
        const Feature &featureCore)
    : StatefulDBusProxy(bus, busName, objectPath, featureCore),
      mPriv(new Private(this))
{
}

DebugReceiver::~DebugReceiver()
{
//TODO somehow we have to deal with this shitty spec....
//     if (isValid() && mPriv->enabledByUs) {
//         debug() << "DebugReceiver: Disabling emission of NewDebugMessage";
//         (void) mPriv->baseInterface->setPropertyEnabled(false);
//     }
    delete mPriv;
}

PendingDebugMessageList *DebugReceiver::fetchMessages() const
{
    return new PendingDebugMessageList(
            mPriv->baseInterface->GetMessages(),
            DebugReceiverPtr(const_cast<DebugReceiver*>(this)));
}

}
