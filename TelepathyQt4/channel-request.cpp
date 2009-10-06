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

#include <TelepathyQt4/ChannelRequest>

#include "TelepathyQt4/_gen/cli-channel-request-body.hpp"
#include "TelepathyQt4/_gen/cli-channel-request.moc.hpp"
#include "TelepathyQt4/_gen/channel-request.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingVoid>

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

struct TELEPATHY_QT4_NO_EXPORT ChannelRequest::Private
{
    Private(ChannelRequest *parent);
    ~Private();

    static void introspectMain(Private *self);

    void extractMainProps(const QVariantMap &props);

    // Public object
    ChannelRequest *parent;

    // Instance of generated interface class
    Client::ChannelRequestInterface *baseInterface;

    // Optional interface proxies
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
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

    parent->connect(baseInterface,
            SIGNAL(Failed(const QString &, const QString &)),
            SIGNAL(failed(const QString &, const QString &)));
    parent->connect(baseInterface,
            SIGNAL(Succeeded()),
            SIGNAL(succeeded()));

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

    debug() << "Calling Properties::GetAll(ChannelRequest)";
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(TELEPATHY_INTERFACE_CHANNEL_REQUEST),
                self->parent);
    // FIXME: This is a Qt bug fixed upstream, should be in the next Qt release.
    //        We should not need to check watcher->isFinished() here, remove the
    //        check when a fixed Qt version is released.
    if (watcher->isFinished()) {
        self->parent->gotMainProperties(watcher);
    } else {
        self->parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
    }
}

void ChannelRequest::Private::extractMainProps(const QVariantMap &props)
{
    parent->setInterfaces(qdbus_cast<QStringList>(props.value("Interfaces")));

    if (!account && props.contains("Account")) {
        QDBusObjectPath accountObjectPath =
            qdbus_cast<QDBusObjectPath>(props.value("Account"));
        account = Account::create(
                TELEPATHY_ACCOUNT_MANAGER_BUS_NAME,
                accountObjectPath.path());
    }

    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    uint stamp = (uint) qdbus_cast<qlonglong>(props.value("UserActionTime"));
    if (stamp != 0) {
        userActionTime = QDateTime::fromTime_t(stamp);
    }

    preferredHandler = qdbus_cast<QString>(props.value("PreferredHandler"));
    requests = qdbus_cast<QualifiedPropertyValueMapList>(props.value("Requests"));
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
    return ChannelRequestPtr(new ChannelRequest(QDBusConnection::sessionBus(),
                objectPath, immutableProperties));
}

ChannelRequestPtr ChannelRequest::create(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ChannelRequestPtr(new ChannelRequest(bus, objectPath,
                immutableProperties));
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
    mPriv->extractMainProps(immutableProperties);
}

/**
 * Class destructor.
 */
ChannelRequest::~ChannelRequest()
{
    delete mPriv;
}

AccountPtr ChannelRequest::account() const
{
    return mPriv->account;
}

QDateTime ChannelRequest::userActionTime() const
{
    return mPriv->userActionTime;
}

QString ChannelRequest::preferredHandler() const
{
    return mPriv->preferredHandler;
}

QualifiedPropertyValueMapList ChannelRequest::requests() const
{
    return mPriv->requests;
}

PendingOperation *ChannelRequest::cancel()
{
    return new PendingVoid(mPriv->baseInterface->Cancel(), this);
}

PendingOperation *ChannelRequest::proceed()
{
    return new PendingVoid(mPriv->baseInterface->Proceed(), this);
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

        mPriv->extractMainProps(props);

        if (mPriv->account) {
            connect(mPriv->account->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onAccountReady(Tp::PendingOperation *)));
        } else {
            warning() << "Properties.GetAll(ChannelRequest) is missing "
                "account property, ignoring";
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        }
    }
    else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        warning().nospace() << "Properties::GetAll(ChannelRequest) failed with "
            << reply.error().name() << ": " << reply.error().message();
    }
}

void ChannelRequest::onAccountReady(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Unable to make account ready";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                op->errorName(), op->errorMessage());
        return;
    }
    mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
}

} // Tp
