/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt4/ChannelRequest>

#include "TelepathyQt4/_gen/cli-channel-request-body.hpp"
#include "TelepathyQt4/_gen/cli-channel-request.moc.hpp"
#include "TelepathyQt4/_gen/channel-request.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingVoidMethodCall>

#include <QDateTime>

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

/**
 * \defgroup clientchannelrequest ChannelRequest proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Channel Requests and their
 * optional interfaces.
 */

namespace Tp
{

struct ChannelRequest::Private
{
    Private(ChannelRequest *parent);
    ~Private();

    static void introspectMain(Private *self);

    // Public object
    ChannelRequest *parent;

    // Instance of generated interface class
    Client::ChannelRequestInterface *baseInterface;

    // Optional interface proxies
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    QStringList interfaces;
    AccountPtr account;
    QDateTime userActionTime;
    QString preferredHandler;
    QualifiedPropertyValueMapList requests;
};

ChannelRequest::Private::Private(ChannelRequest *parent)
    : parent(parent),
      baseInterface(new Client::ChannelRequestInterface(
                    parent->dbusConnection(), parent->busName(),
                    parent->objectPath(), parent)),
      properties(0),
      readinessHelper(parent->readinessHelper())
{
    debug() << "Creating new ChannelRequest:" << parent->objectPath();

    ReadinessHelper::Introspectables introspectables;

    // As ChannelRequest does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(Features() << FeatureCore);
}

ChannelRequest::Private::~Private()
{
}

void ChannelRequest::Private::introspectMain(ChannelRequest::Private *self)
{
    if (!self->properties) {
        self->properties = self->parent->propertiesInterface();
        Q_ASSERT(self->properties != 0);
    }

    connect(self->baseInterface,
            SIGNAL(Failed(const QString &, const QString &)),
            self->parent,
            SIGNAL(failed(const QString &, const QString &)));
    connect(self->baseInterface,
            SIGNAL(Succeeded()),
            self->parent,
            SIGNAL(succeeded()));

    debug() << "Calling Properties::GetAll(ChannelRequest)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(TELEPATHY_INTERFACE_CHANNEL_REQUEST),
                self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

/**
 * \class ChannelRequest
 * \ingroup clientchannelrequest
 * \headerfile <TelepathyQt4/channel-request.h> <TelepathyQt4/ChannelRequest>
 *
 * High-level proxy object for accessing remote Telepathy ChannelRequest objects.
 */

const Feature ChannelRequest::FeatureCore = Feature(ChannelRequest::staticMetaObject.className(), 0, true);

ChannelRequestPtr ChannelRequest::create(const QString &objectPath,
        const QVariantMap &immutableProperties)
{
    return ChannelRequestPtr(new ChannelRequest(objectPath,
                immutableProperties));
}

ChannelRequestPtr ChannelRequest::create(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ChannelRequestPtr(new ChannelRequest(bus, objectPath,
                immutableProperties));
}

ChannelRequest::ChannelRequest(const QString &objectPath,
        const QVariantMap &immutableProperties)
    : StatefulDBusProxy(QDBusConnection::sessionBus(),
            "org.freedesktop.Telepathy.ChannelDispatcher",
            objectPath),
      OptionalInterfaceFactory<ChannelRequest>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
    // FIXME: remember the immutableProperties, and use them to reduce the
    // number of D-Bus calls we need to make (but we should make at least
    // one, to check that the channel request does in fact exist)
}

ChannelRequest::ChannelRequest(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties)
    : StatefulDBusProxy(bus,
            "org.freedesktop.Telepathy.ChannelDispatcher",
            objectPath),
      OptionalInterfaceFactory<ChannelRequest>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
    // FIXME: remember the immutableProperties, and use them to reduce the
    // number of D-Bus calls we need to make (but we should make at least
    // one, to check that the channel request does in fact exist)
}

/**
 * Class destructor.
 */
ChannelRequest::~ChannelRequest()
{
    delete mPriv;
}

QStringList ChannelRequest::interfaces() const
{
    if (!isReady()) {
        warning() << "ChannelRequest::interfaces() used channel request not ready";
    }
    return mPriv->interfaces;
}

AccountPtr ChannelRequest::account() const
{
    if (!isReady()) {
        warning() << "ChannelRequest::account() used channel request not ready";
    }
    return mPriv->account;
}

QDateTime ChannelRequest::userActionTime() const
{
    if (!isReady()) {
        warning() << "ChannelRequest::userActionTime() used channel request not ready";
    }
    return mPriv->userActionTime;
}

QString ChannelRequest::preferredHandler() const
{
    if (!isReady()) {
        warning() << "ChannelRequest::preferredHandler() used channel request not ready";
    }
    return mPriv->preferredHandler;
}

QualifiedPropertyValueMapList ChannelRequest::requests() const
{
    if (!isReady()) {
        warning() << "ChannelRequest::requests() used channel request not ready";
    }
    return mPriv->requests;
}

PendingOperation *ChannelRequest::cancel()
{
    if (!isReady()) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_AVAILABLE,
                "ChannelRequest not ready");
    }

    return new PendingVoidMethodCall(this, mPriv->baseInterface->Cancel());
}

/**
 * Get the ChannelRequestInterface for this ChannelRequest class. This method is
 * protected since the convenience methods provided by this class should
 * always be used instead of the interface by users of the class.
 *
 * \return A pointer to the existing ChannelRequestInterface for this
 *         ChannelRequest
 */
Client::ChannelRequestInterface *ChannelRequest::baseInterface() const
{
    return mPriv->baseInterface;
}

void ChannelRequest::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties::GetAll(ChannelRequest)";
        props = reply.value();

        mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);

        QString accountObjectPath = qdbus_cast<QString>(props["Account"]);
        mPriv->account = Account::create(
                TELEPATHY_ACCOUNT_MANAGER_BUS_NAME,
                accountObjectPath);
        connect(mPriv->account->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onAccountReady(Tp::PendingOperation *)));

        // FIXME: Telepathy supports 64-bit time_t, but Qt only does so on
        // ILP64 systems (e.g. sparc64, but not x86_64). If QDateTime
        // gains a fromTimestamp64 method, we should use it instead.
        uint stamp = (uint) qdbus_cast<qlonglong>(props["UserActionTime"]);
        if (stamp != 0) {
            mPriv->userActionTime = QDateTime::fromTime_t(stamp);
        }

        mPriv->preferredHandler = qdbus_cast<QString>(props["PreferredHandler"]);
        mPriv->requests = qdbus_cast<QualifiedPropertyValueMapList>(props["Requests"]);
    }
    else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());
        warning().nospace() << "Properties::GetAll(ChannelRequest) failed with "
            << reply.error().name() << ": " << reply.error().message();
    }
}

void ChannelRequest::onAccountReady(PendingOperation *op)
{
    if (op->isError()) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                op->errorName(), op->errorMessage());
        return;
    }
    mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

} // Tp
