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

#include <TelepathyQt4/Client/ReadinessHelper>

#include "TelepathyQt4/Client/_gen/readiness-helper.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Constants>

#include <QTimer>

namespace Telepathy
{
namespace Client
{

struct ReadinessHelper::Private
{
    Private(ReadinessHelper *parent,
            DBusProxy *proxy,
            uint currentStatus,
            const QMap<uint, Introspectable> &introspectables);
    ~Private();

    void setCurrentStatus(uint newStatus);
    void introspectCore();
    void setIntrospectCompleted(uint feature, bool success);
    void iterateIntrospection();

    void abortOperations(const QString &errorName, const QString &errorMessage);

    ReadinessHelper *parent;
    DBusProxy *proxy;
    uint currentStatus;
    QStringList interfaces;
    QMap<uint, Introspectable> introspectables;
    QSet<uint> supportedStatuses;
    QSet<uint> supportedFeatures;
    QSet<uint> satisfiedFeatures;
    QSet<uint> requestedFeatures;
    QSet<uint> missingFeatures;
    QSet<uint> pendingFeatures;
    QSet<uint> inFlightFeatures;
    QList<PendingReady *> pendingOperations;

    bool pendingStatusChange;
    uint pendingStatus;
};

ReadinessHelper::Private::Private(
        ReadinessHelper *parent,
        DBusProxy *proxy,
        uint currentStatus,
        const QMap<uint, Introspectable> &introspectables)
    : parent(parent),
      proxy(proxy),
      currentStatus(currentStatus),
      introspectables(introspectables),
      pendingStatusChange(false),
      pendingStatus(-1)
{
    // we must have an introspectable for core
    Q_ASSERT(introspectables.contains(0));

    QMap<uint, Introspectable>::const_iterator i = introspectables.constBegin();
    while (i != introspectables.constEnd()) {
        uint feature = i.key();
        Introspectable introspectable = i.value();
        Q_ASSERT(introspectable.introspectFunc != 0);
        supportedStatuses += introspectable.makesSenseForStatuses;
        supportedFeatures += feature;
        ++i;
    }

    debug() << "ReadinessHelper: supportedStatuses =" << supportedStatuses;
    debug() << "ReadinessHelper: supportedFeatures =" << supportedFeatures;

    if (supportedStatuses.contains(currentStatus)) {
        introspectCore();
    }
}

ReadinessHelper::Private::~Private()
{
    abortOperations(TELEPATHY_ERROR_CANCELLED, "Destroyed");
}

void ReadinessHelper::Private::setCurrentStatus(uint newStatus)
{
    if (inFlightFeatures.isEmpty()) {
        currentStatus = newStatus;
        satisfiedFeatures.clear();
        missingFeatures.clear();

        // retrieve all features that were requested for the new status
        pendingFeatures = requestedFeatures;

        if (supportedStatuses.contains(currentStatus)) {
            introspectCore();
        } else {
            emit parent->statusReady(currentStatus);
        }
    } else {
        debug() << "status changed while introspection process was running";
        pendingStatusChange = true;
        pendingStatus = newStatus;
    }
}

void ReadinessHelper::Private::introspectCore()
{
    debug() << "Status changed to" << currentStatus << "- introspecting core";
    requestedFeatures += 0;
    pendingFeatures += 0;
    QTimer::singleShot(0, parent, SLOT(iterateIntrospection()));
}

void ReadinessHelper::Private::setIntrospectCompleted(uint feature, bool success)
{
    debug() << "ReadinessHelper::setIntrospectCompleted: feature:" << feature <<
        "- success:" << success;
    if (pendingStatusChange) {
        debug() << "ReadinessHelper::setIntrospectCompleted called while there is "
            "a pending status change - ignoring";

        inFlightFeatures.remove(feature);

        // ignore all introspection completed as the state changed
        if (!inFlightFeatures.isEmpty()) {
            return;
        }
        pendingStatusChange = false;
        setCurrentStatus(pendingStatus);
        return;
    }

    Q_ASSERT(pendingFeatures.contains(feature));
    Q_ASSERT(inFlightFeatures.contains(feature));

    if (success) {
        satisfiedFeatures.insert(feature);
    }
    else {
        missingFeatures.insert(feature);
    }

    pendingFeatures.remove(feature);
    inFlightFeatures.remove(feature);

    QTimer::singleShot(0, parent, SLOT(iterateIntrospection()));
}

void ReadinessHelper::Private::iterateIntrospection()
{
    if (!proxy->isValid()) {
        return;
    }

    if (!supportedStatuses.contains(currentStatus)) {
        debug() << "ignoring iterate introspection for status" << currentStatus;
        // don't do anything just now to avoid spurious becomeReady finishes
        return;
    }

    // take care to flag anything with dependencies in missing, and the
    // stuff depending on them, as missing
    QMap<uint, Introspectable>::const_iterator i = introspectables.constBegin();
    while (i != introspectables.constEnd()) {
        uint feature = i.key();
        Introspectable introspectable = i.value();
        QSet<uint> dependsOnFeatures = introspectable.dependsOnFeatures;
        if (!dependsOnFeatures.intersect(missingFeatures).isEmpty()) {
            missingFeatures += feature;
        }
        ++i;
    }

    // check if any pending operations for becomeReady should finish now
    // based on their requested features having nothing more than what
    // satisfiedFeatures + missingFeatures has
    foreach (PendingReady *operation, pendingOperations) {
        if ((operation->requestedFeatures() - (satisfiedFeatures + missingFeatures)).isEmpty()) {
            operation->setFinished();
            pendingOperations.removeOne(operation);
        }
    }

    if ((requestedFeatures - (satisfiedFeatures + missingFeatures)).isEmpty()) {
        // all requested features satisfied or missing
        emit parent->statusReady(currentStatus);
        return;
    }

    // update pendingFeatures with the difference of requested and
    // satisfied + missing
    pendingFeatures -= (satisfiedFeatures + missingFeatures);

    // find out which features don't have dependencies that are still pending
    QSet<uint> readyToIntrospect;
    foreach (uint feature, pendingFeatures) {
        // missing doesn't have to be considered here anymore
        if ((introspectables[feature].dependsOnFeatures - satisfiedFeatures).isEmpty()) {
            readyToIntrospect.insert(feature);
        }
    }

    // now readyToIntrospect should contain all the features which have
    // all their feature dependencies satisfied
    foreach (uint feature, readyToIntrospect) {
        if (inFlightFeatures.contains(feature)) {
            continue;
        }

        inFlightFeatures.insert(feature);

        Introspectable introspectable = introspectables[feature];

        if (!introspectable.makesSenseForStatuses.contains(currentStatus)) {
            // No-op satisfy features for which nothing has to be done in
            // the current state
            setIntrospectCompleted(feature, true);
            return; // will be called with a single-shot soon again
        }

        if (feature != 0) {
            foreach (const QString &interface, introspectable.dependsOnInterfaces) {
                if (!interfaces.contains(interface)) {
                    // Core is a dependency for everything, so interfaces are
                    // introspected - if not all of them are present, the feature can't
                    // possibly be satisfied
                    debug() << "feature" << feature << "depends on interfaces" <<
                        introspectable.dependsOnInterfaces << ", but interface" << interface <<
                        "is not present";
                    setIntrospectCompleted(feature, false);
                    return; // will be called with a single-shot soon again
                }
            }
        }

        // yes, with the dependency info, we can even parallelize
        // introspection of several features at once, reducing total round trip
        // time considerably with many independent features!
        (*(introspectable.introspectFunc))(introspectable.introspectFuncData);
    }
}

void ReadinessHelper::Private::abortOperations(const QString &errorName,
        const QString &errorMessage)
{
    foreach (PendingReady *operation, pendingOperations) {
        operation->setFinishedWithError(errorName, errorMessage);
    }
}

ReadinessHelper::ReadinessHelper(DBusProxy *proxy,
        uint currentStatus,
        const QMap<uint, Introspectable> &introspectables,
        QObject *parent)
    : QObject(parent),
      mPriv(new Private(this, proxy, currentStatus, introspectables))
{
    connect(proxy,
            SIGNAL(invalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)),
            SLOT(onProxyInvalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)));
}

ReadinessHelper::~ReadinessHelper()
{
    delete mPriv;
}

uint ReadinessHelper::currentStatus() const
{
    return mPriv->currentStatus;
}

void ReadinessHelper::setCurrentStatus(uint currentStatus)
{
    mPriv->setCurrentStatus(currentStatus);
}

QStringList ReadinessHelper::interfaces() const
{
    return mPriv->interfaces;
}

void ReadinessHelper::setInterfaces(const QStringList &interfaces)
{
    mPriv->interfaces = interfaces;
}

QSet<uint> ReadinessHelper::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

QSet<uint> ReadinessHelper::actualFeatures() const
{
    return mPriv->satisfiedFeatures;
}

QSet<uint> ReadinessHelper::missingFeatures() const
{
    return mPriv->missingFeatures;
}

bool ReadinessHelper::isReady(QSet<uint> features) const
{
    if (!mPriv->proxy->isValid()) {
        return false;
    }

    if (features.isEmpty()) {
        features << 0; // it is empty, consider core
    }

    // if we ask if core is ready, core should be on satisfiedFeatures
    if (features.contains(0)) {
        return (features - mPriv->satisfiedFeatures).isEmpty();
    } else {
        return (features - (mPriv->satisfiedFeatures + mPriv->missingFeatures)).isEmpty();
    }
}

PendingReady *ReadinessHelper::becomeReady(QSet<uint> requestedFeatures)
{
    if (requestedFeatures.isEmpty()) {
        requestedFeatures << 0; // it is empty, consider core
    }

    QSet<uint> supportedFeatures = mPriv->supportedFeatures;
    if (supportedFeatures.intersect(requestedFeatures) != requestedFeatures) {
        debug() << "ReadinessHelper::becomeReady called with invalid features: requestedFeatures =" <<
            requestedFeatures << "- supportedFeatures =" << mPriv->supportedFeatures;
        PendingReady *operation =
            new PendingReady(requestedFeatures, mPriv->proxy);
        operation->setFinishedWithError(TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Requested features contains invalid feature");
        return operation;
    }

    if (!mPriv->proxy->isValid()) {
        PendingReady *operation =
            new PendingReady(requestedFeatures, mPriv->proxy);
        operation->setFinishedWithError(mPriv->proxy->invalidationReason(),
                mPriv->proxy->invalidationMessage());
        return operation;
    }

    PendingReady *operation;
    foreach (operation, mPriv->pendingOperations) {
        if (operation->requestedFeatures() == requestedFeatures) {
            return operation;
        }
    }

    mPriv->requestedFeatures += requestedFeatures;
    // it will be updated on iterateIntrospection
    mPriv->pendingFeatures += requestedFeatures;

    operation = new PendingReady(requestedFeatures, mPriv->proxy);
    mPriv->pendingOperations.append(operation);

    QTimer::singleShot(0, this, SLOT(iterateIntrospection()));

    return operation;
}

void ReadinessHelper::setIntrospectCompleted(uint feature, bool success)
{
    if (!mPriv->proxy->isValid()) {
        // proxy became invalid, ignore here
        return;
    }
    mPriv->setIntrospectCompleted(feature, success);
}

void ReadinessHelper::iterateIntrospection()
{
    mPriv->iterateIntrospection();
}

void ReadinessHelper::onProxyInvalidated(Telepathy::Client::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    mPriv->abortOperations(errorName, errorMessage);
}

}
}
