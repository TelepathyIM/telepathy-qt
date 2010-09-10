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

#include <TelepathyQt4/ReadinessHelper>

#include "TelepathyQt4/_gen/readiness-helper.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Constants>

#include <QDBusError>
#include <QSharedData>
#include <QTimer>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ReadinessHelper::Introspectable::Private : public QSharedData
{
    Private(const QSet<uint> &makesSenseForStatuses,
            const Features &dependsOnFeatures,
            const QStringList &dependsOnInterfaces,
            IntrospectFunc introspectFunc,
            void *introspectFuncData,
            bool critical)
        : makesSenseForStatuses(makesSenseForStatuses),
        dependsOnFeatures(dependsOnFeatures),
        dependsOnInterfaces(dependsOnInterfaces),
        introspectFunc(introspectFunc),
        introspectFuncData(introspectFuncData),
        critical(critical) {}

    QSet<uint> makesSenseForStatuses;
    Features dependsOnFeatures;
    QStringList dependsOnInterfaces;
    IntrospectFunc introspectFunc;
    void *introspectFuncData;
    bool critical;
};

ReadinessHelper::Introspectable::Introspectable()
    : mPriv(new Private(QSet<uint>(), Features(), QStringList(), 0, 0, false))
{
}

ReadinessHelper::Introspectable::Introspectable(const QSet<uint> &makesSenseForStatuses,
        const Features &dependsOnFeatures, const QStringList &dependsOnInterfaces,
        IntrospectFunc introspectFunc, void *introspectFuncData, bool critical)
    : mPriv(new Private(makesSenseForStatuses, dependsOnFeatures, dependsOnInterfaces,
                introspectFunc, introspectFuncData, critical))
{
}

ReadinessHelper::Introspectable::Introspectable(const Introspectable &other)
    : mPriv(other.mPriv)
{
}

ReadinessHelper::Introspectable::~Introspectable()
{
}

ReadinessHelper::Introspectable &ReadinessHelper::Introspectable::operator=(
        const Introspectable &other)
{
    mPriv = other.mPriv;
    return *this;
}

struct TELEPATHY_QT4_NO_EXPORT ReadinessHelper::Private
{
    Private(ReadinessHelper *parent,
            QObject *object,
            uint currentStatus,
            const Introspectables &introspectables);
    ~Private();

    void setCurrentStatus(uint newStatus);
    void setIntrospectCompleted(const Feature &feature, bool success,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    void iterateIntrospection();
    Features depsFor(const Feature &feature); // Recursive dependencies for a feature

    void abortOperations(const QString &errorName, const QString &errorMessage);

    ReadinessHelper *parent;
    QObject *object;
    DBusProxy *proxy;
    uint currentStatus;
    QStringList interfaces;
    Introspectables introspectables;
    QSet<uint> supportedStatuses;
    Features supportedFeatures;
    Features satisfiedFeatures;
    Features requestedFeatures;
    Features missingFeatures;
    Features pendingFeatures;
    Features inFlightFeatures;
    QHash<Feature, QPair<QString, QString> > missingFeaturesErrors;
    QList<PendingReady *> pendingOperations;

    bool pendingStatusChange;
    uint pendingStatus;
};

ReadinessHelper::Private::Private(
        ReadinessHelper *parent,
        QObject *object,
        uint currentStatus,
        const Introspectables &introspectables)
    : parent(parent),
      object(object),
      proxy(qobject_cast<DBusProxy*>(object)),
      currentStatus(currentStatus),
      introspectables(introspectables),
      pendingStatusChange(false),
      pendingStatus(-1)
{
    for (Introspectables::const_iterator i = introspectables.constBegin();
            i != introspectables.constEnd(); ++i) {
        Feature feature = i.key();
        Introspectable introspectable = i.value();
        Q_ASSERT(introspectable.mPriv->introspectFunc != 0);
        supportedStatuses += introspectable.mPriv->makesSenseForStatuses;
        supportedFeatures += feature;
    }

    if (proxy) {
        parent->connect(proxy,
                SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
                SLOT(onProxyInvalidated(Tp::DBusProxy *, const QString &, const QString &)));
    }

    debug() << "ReadinessHelper: supportedStatuses =" << supportedStatuses;
    debug() << "ReadinessHelper: supportedFeatures =" << supportedFeatures;
}

ReadinessHelper::Private::~Private()
{
    abortOperations(QLatin1String(TELEPATHY_ERROR_CANCELLED), QLatin1String("Destroyed"));
}

void ReadinessHelper::Private::setCurrentStatus(uint newStatus)
{
    if (currentStatus == newStatus) {
        return;
    }

    if (inFlightFeatures.isEmpty()) {
        currentStatus = newStatus;
        satisfiedFeatures.clear();
        missingFeatures.clear();

        // retrieve all features that were requested for the new status
        pendingFeatures = requestedFeatures;

        if (supportedStatuses.contains(currentStatus)) {
            QTimer::singleShot(0, parent, SLOT(iterateIntrospection()));
        } else {
            emit parent->statusReady(currentStatus);
        }
    } else {
        debug() << "status changed while introspection process was running";
        pendingStatusChange = true;
        pendingStatus = newStatus;
    }
}

void ReadinessHelper::Private::setIntrospectCompleted(const Feature &feature,
        bool success, const QString &errorName, const QString &errorMessage)
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
        missingFeaturesErrors.insert(feature,
                QPair<QString, QString>(errorName, errorMessage));
        if (errorName.isEmpty()) {
            warning() << "ReadinessHelper::setIntrospectCompleted: Feature" <<
                feature << "introspection failed but no error message was given";
        }
    }

    pendingFeatures.remove(feature);
    inFlightFeatures.remove(feature);

    QTimer::singleShot(0, parent, SLOT(iterateIntrospection()));
}

void ReadinessHelper::Private::iterateIntrospection()
{
    if (proxy && !proxy->isValid()) {
        return;
    }

    if (!supportedStatuses.contains(currentStatus)) {
        debug() << "ignoring iterate introspection for status" << currentStatus;
        // don't do anything just now to avoid spurious becomeReady finishes
        return;
    }

    // take care to flag anything with dependencies in missing, and the
    // stuff depending on them, as missing
    for (Introspectables::const_iterator i = introspectables.constBegin();
            i != introspectables.constEnd(); ++i) {
        Feature feature = i.key();
        Introspectable introspectable = i.value();
        if (!depsFor(feature).intersect(missingFeatures).isEmpty()) {
            missingFeatures.insert(feature);
            missingFeaturesErrors.insert(feature,
                    QPair<QString, QString>(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                        QLatin1String("Feature depends on other features that are not available")));
        }
    }

    // check if any pending operations for becomeReady should finish now
    // based on their requested features having nothing more than what
    // satisfiedFeatures + missingFeatures has
    QString errorName;
    QString errorMessage;
    foreach (PendingReady *operation, pendingOperations) {
        if ((operation->requestedFeatures() - (satisfiedFeatures + missingFeatures)).isEmpty()) {
            if (parent->isReady(operation->requestedFeatures(), &errorName, &errorMessage)) {
                operation->setFinished();
            } else {
                operation->setFinishedWithError(errorName, errorMessage);
            }
        }
    }

    if ((requestedFeatures - (satisfiedFeatures + missingFeatures)).isEmpty()) {
        // all requested features satisfied or missing
        emit parent->statusReady(currentStatus);
        return;
    }

    // Update pendingFeatures to contain the (recursive) dependencies of what's in it currently
    Features deps;
    foreach (Feature feature, pendingFeatures) {
        deps.unite(depsFor(feature));
    }
    pendingFeatures.unite(deps);

    // update pendingFeatures with the difference of requested and
    // satisfied + missing
    pendingFeatures -= (satisfiedFeatures + missingFeatures);

    // find out which features don't have dependencies that are still pending
    Features readyToIntrospect;
    foreach (const Feature &feature, pendingFeatures) {
        // missing doesn't have to be considered here anymore
        if ((introspectables[feature].mPriv->dependsOnFeatures - satisfiedFeatures).isEmpty()) {
            readyToIntrospect.insert(feature);
        }
    }

    // now readyToIntrospect should contain all the features which have
    // all their feature dependencies satisfied
    foreach (const Feature &feature, readyToIntrospect) {
        if (inFlightFeatures.contains(feature)) {
            continue;
        }

        inFlightFeatures.insert(feature);

        Introspectable introspectable = introspectables[feature];

        if (!introspectable.mPriv->makesSenseForStatuses.contains(currentStatus)) {
            // No-op satisfy features for which nothing has to be done in
            // the current state
            setIntrospectCompleted(feature, true);
            return; // will be called with a single-shot soon again
        }

        foreach (const QString &interface, introspectable.mPriv->dependsOnInterfaces) {
            if (!interfaces.contains(interface)) {
                // If a feature is ready to introspect and depends on a interface
                // that is not present the feature can't possibly be satisfied
                debug() << "feature" << feature << "depends on interfaces" <<
                    introspectable.mPriv->dependsOnInterfaces << ", but interface" << interface <<
                    "is not present";
                setIntrospectCompleted(feature, false,
                        QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                        QLatin1String("Feature depend on interfaces that are not available"));
                return; // will be called with a single-shot soon again
            }
        }

        // yes, with the dependency info, we can even parallelize
        // introspection of several features at once, reducing total round trip
        // time considerably with many independent features!
        (*(introspectable.mPriv->introspectFunc))(introspectable.mPriv->introspectFuncData);
    }
}

Features ReadinessHelper::Private::depsFor(const Feature &feature)
{
    Features deps;

    foreach (Feature dep, introspectables[feature].mPriv->dependsOnFeatures) {
        deps += dep;
        deps += depsFor(dep);
    }

    return deps;
}

void ReadinessHelper::Private::abortOperations(const QString &errorName,
        const QString &errorMessage)
{
    foreach (PendingReady *operation, pendingOperations) {
        parent->disconnect(operation,
                SIGNAL(finished(Tp::PendingOperation*)),
                parent,
                SLOT(onOperationFinished(Tp::PendingOperation*)));
        parent->disconnect(operation,
                SIGNAL(destroyed(QObject*)),
                parent,
                SLOT(onOperationDestroyed(QObject*)));
        operation->setFinishedWithError(errorName, errorMessage);
    }
    pendingOperations.clear();
}

ReadinessHelper::ReadinessHelper(QObject *object,
        uint currentStatus,
        const Introspectables &introspectables,
        QObject *parent)
    : QObject(parent),
      mPriv(new Private(this, object, currentStatus, introspectables))
{
}

ReadinessHelper::~ReadinessHelper()
{
    delete mPriv;
}

void ReadinessHelper::addIntrospectables(const Introspectables &introspectables)
{
    // QMap::unite will create multiple items if the key is already in the map
    // so let's make sure we don't duplicate keys
    for (Introspectables::const_iterator i = introspectables.constBegin();
            i != introspectables.constEnd(); ++i) {
        Feature feature = i.key();
        if (mPriv->introspectables.contains(feature)) {
            warning() << "ReadinessHelper::addIntrospectables: trying to add an "
                "introspectable for feature" << feature << "but introspectable "
                "for this feature already exists";
        } else {
            Introspectable introspectable = i.value();
            mPriv->introspectables.insert(feature, introspectable);
            mPriv->supportedStatuses += introspectable.mPriv->makesSenseForStatuses;
            mPriv->supportedFeatures += feature;
        }
    }

    debug() << "ReadinessHelper: new supportedStatuses =" << mPriv->supportedStatuses;
    debug() << "ReadinessHelper: new supportedFeatures =" << mPriv->supportedFeatures;
}

uint ReadinessHelper::currentStatus() const
{
    return mPriv->currentStatus;
}

void ReadinessHelper::setCurrentStatus(uint currentStatus)
{
    mPriv->setCurrentStatus(currentStatus);
}

/**
 * Force the current internal status to \a currentStatus.
 *
 * Note that this method will not start a new introspection or restart the
 * current one in case one is running.
 *
 * This is useful for example when the status is unknown initially but it will
 * become known in the first introspection run and there is no need to re-run
 * the introspection.
 *
 * \param currentStatus The status to set.
 */
void ReadinessHelper::forceCurrentStatus(uint currentStatus)
{
    mPriv->currentStatus = currentStatus;
}

QStringList ReadinessHelper::interfaces() const
{
    return mPriv->interfaces;
}

void ReadinessHelper::setInterfaces(const QStringList &interfaces)
{
    mPriv->interfaces = interfaces;
}

Features ReadinessHelper::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

Features ReadinessHelper::actualFeatures() const
{
    return mPriv->satisfiedFeatures;
}

Features ReadinessHelper::missingFeatures() const
{
    return mPriv->missingFeatures;
}

bool ReadinessHelper::isReady(const Feature &feature,
        QString *errorName, QString *errorMessage) const
{
    if (mPriv->proxy && !mPriv->proxy->isValid()) {
        if (errorName) {
            *errorName = mPriv->proxy->invalidationReason();
        }
        if (errorMessage) {
            *errorMessage = mPriv->proxy->invalidationMessage();
        }
        return false;
    }

    if (!mPriv->supportedFeatures.contains(feature)) {
        if (errorName) {
            *errorName = QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT);
        }
        if (errorMessage) {
            *errorMessage = QLatin1String("Unsupported feature");
        }
        return false;
    }

    bool ret = true;

    if (feature.isCritical()) {
        if (!mPriv->satisfiedFeatures.contains(feature)) {
            ret = false;
        }
    } else {
        if (!mPriv->satisfiedFeatures.contains(feature) &&
            !mPriv->missingFeatures.contains(feature)) {
            ret = false;
        }
    }

    if (!ret) {
        QPair<QString, QString> error = mPriv->missingFeaturesErrors[feature];
        if (errorName) {
            *errorName = error.first;
        }
        if (errorMessage) {
            *errorMessage = error.second;
        }
    }

    return ret;
}

bool ReadinessHelper::isReady(const Features &features, QString *errorName, QString *errorMessage) const
{
    if (mPriv->proxy && !mPriv->proxy->isValid()) {
        if (errorName) {
            *errorName = mPriv->proxy->invalidationReason();
        }
        if (errorMessage) {
            *errorMessage = mPriv->proxy->invalidationMessage();
        }
        return false;
    }

    Q_ASSERT(!features.isEmpty());

    foreach (const Feature &feature, features) {
        if (!isReady(feature, errorName, errorMessage)) {
            return false;
        }
    }
    return true;
}

PendingReady *ReadinessHelper::becomeReady(const Features &requestedFeatures)
{
    Q_ASSERT(!requestedFeatures.isEmpty());

    Features supportedFeatures = mPriv->supportedFeatures;
    if (supportedFeatures.intersect(requestedFeatures) != requestedFeatures) {
        warning() << "ReadinessHelper::becomeReady called with invalid features: requestedFeatures =" <<
            requestedFeatures << "- supportedFeatures =" << mPriv->supportedFeatures;
        PendingReady *operation =
            new PendingReady(requestedFeatures, mPriv->object, this);
        operation->setFinishedWithError(
                QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("Requested features contains unsupported feature"));
        return operation;
    }

    if (mPriv->proxy && !mPriv->proxy->isValid()) {
        PendingReady *operation =
            new PendingReady(requestedFeatures, mPriv->object, this);
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

    operation = new PendingReady(requestedFeatures, mPriv->object, this);
    connect(operation,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onOperationFinished(Tp::PendingOperation*)));
    connect(operation,
            SIGNAL(destroyed(QObject*)),
            SLOT(onOperationDestroyed(QObject*)));
    mPriv->pendingOperations.append(operation);

    QTimer::singleShot(0, this, SLOT(iterateIntrospection()));

    return operation;
}

void ReadinessHelper::setIntrospectCompleted(const Feature &feature, bool success,
        const QString &errorName, const QString &errorMessage)
{
    if (mPriv->proxy && !mPriv->proxy->isValid()) {
        // proxy became invalid, ignore here
        return;
    }
    mPriv->setIntrospectCompleted(feature, success, errorName, errorMessage);
}

void ReadinessHelper::setIntrospectCompleted(const Feature &feature, bool success,
        const QDBusError &error)
{
    setIntrospectCompleted(feature, success, error.name(), error.message());
}

void ReadinessHelper::iterateIntrospection()
{
    mPriv->iterateIntrospection();
}

void ReadinessHelper::onProxyInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    // clear satisfied and missing features as we have public methods to get them
    mPriv->satisfiedFeatures.clear();
    mPriv->missingFeatures.clear();

    mPriv->abortOperations(errorName, errorMessage);
}

void ReadinessHelper::onOperationFinished(PendingOperation *op)
{
    disconnect(op,
               SIGNAL(destroyed(QObject*)),
               this,
               SLOT(onOperationDestroyed(QObject*)));
    mPriv->pendingOperations.removeOne(qobject_cast<PendingReady*>(op));
}

void ReadinessHelper::onOperationDestroyed(QObject *obj)
{
    mPriv->pendingOperations.removeOne(qobject_cast<PendingReady*>(obj));
}

} // Tp
