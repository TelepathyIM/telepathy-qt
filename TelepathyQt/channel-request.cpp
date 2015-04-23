/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/ChannelRequest>

#include "TelepathyQt/_gen/cli-channel-request-body.hpp"
#include "TelepathyQt/_gen/cli-channel-request.moc.hpp"
#include "TelepathyQt/_gen/channel-request.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>

#include <QDateTime>
#include <QSharedData>

namespace Tp
{

struct TP_QT_NO_EXPORT ChannelRequest::Private
{
    Private(ChannelRequest *parent, const QVariantMap &immutableProperties,
            const AccountFactoryConstPtr &, const ConnectionFactoryConstPtr &,
            const ChannelFactoryConstPtr &, const ContactFactoryConstPtr &);
    ~Private();

    static void introspectMain(Private *self);

    // \param lastCall Is this the last call to extractMainProps ie. should actions that only must
    // be done once be done in this call
    void extractMainProps(const QVariantMap &props, bool lastCall);

    // Public object
    ChannelRequest *parent;

    // Context
    AccountFactoryConstPtr accFact;
    ConnectionFactoryConstPtr connFact;
    ChannelFactoryConstPtr chanFact;
    ContactFactoryConstPtr contactFact;

    // Instance of generated interface class
    Client::ChannelRequestInterface *baseInterface;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    QVariantMap immutableProperties;

    ReadinessHelper *readinessHelper;

    // Introspection
    AccountPtr account;
    QDateTime userActionTime;
    QString preferredHandler;
    QualifiedPropertyValueMapList requests;
    ChannelRequestHints hints;
    bool propertiesDone;

    bool gotSWC;
    ChannelPtr chan;
};

ChannelRequest::Private::Private(ChannelRequest *parent,
        const QVariantMap &immutableProperties,
        const AccountFactoryConstPtr &accFact,
        const ConnectionFactoryConstPtr &connFact,
        const ChannelFactoryConstPtr &chanFact,
        const ContactFactoryConstPtr &contactFact)
    : parent(parent),
      accFact(accFact),
      connFact(connFact),
      chanFact(chanFact),
      contactFact(contactFact),
      baseInterface(new Client::ChannelRequestInterface(parent)),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      immutableProperties(immutableProperties),
      readinessHelper(parent->readinessHelper()),
      propertiesDone(false),
      gotSWC(false)
{
    debug() << "Creating new ChannelRequest:" << parent->objectPath();

    parent->connect(baseInterface,
            SIGNAL(Failed(QString,QString)),
            SIGNAL(failed(QString,QString)));
    parent->connect(baseInterface,
            SIGNAL(Succeeded()),
            SLOT(onLegacySucceeded()));
    parent->connect(baseInterface,
            SIGNAL(SucceededWithChannel(QDBusObjectPath,QVariantMap,QDBusObjectPath,QVariantMap)),
            SLOT(onSucceededWithChannel(QDBusObjectPath,QVariantMap,QDBusObjectPath,QVariantMap)));

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

    // For early access to the immutable properties through the friendly getters - will be called
    // again with lastCall = true eventually, if/when becomeReady is called, though
    QVariantMap mainProps;
    foreach (QString key, immutableProperties.keys()) {
        // The key.count thing is so that we don't match "org.fdo.Tp.CR.OptionalInterface.Prop" too
        if (key.startsWith(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String("."))
                && key.count(QLatin1Char('.')) ==
                    QString(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".")).count(QLatin1Char('.'))) {
            QVariant value = immutableProperties.value(key);
            mainProps.insert(key.remove(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".")), value);
        }
    }
    extractMainProps(mainProps, false);
}

ChannelRequest::Private::~Private()
{
}

void ChannelRequest::Private::introspectMain(ChannelRequest::Private *self)
{
    QVariantMap props;
    QString key;
    bool needIntrospectMainProps = false;
    const char *propertiesNames[] = { "Account", "UserActionTime",
        "PreferredHandler", "Requests", "Interfaces",
        NULL };
    for (unsigned i = 0; propertiesNames[i] != NULL; ++i) {
        key = TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".");
        key += QLatin1String(propertiesNames[i]);
        if (!self->immutableProperties.contains(key)) {
            needIntrospectMainProps = true;
            break;
        }
        props.insert(QLatin1String(propertiesNames[i]),
                self->immutableProperties[key]);
    }

    if (needIntrospectMainProps) {
        debug() << "Calling Properties::GetAll(ChannelRequest)";
        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    self->properties->GetAll(TP_QT_IFACE_CHANNEL_REQUEST),
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
    } else {
        self->extractMainProps(props, true);
    }
}

void ChannelRequest::Private::extractMainProps(const QVariantMap &props, bool lastCall)
{
    PendingReady *readyOp = 0;

    if (props.contains(QLatin1String("Account"))) {
        QDBusObjectPath accountObjectPath =
            qdbus_cast<QDBusObjectPath>(props.value(QLatin1String("Account")));

        if (!account.isNull()) {
            if (accountObjectPath.path() == account->objectPath()) {
                // Most often a no-op, but we want this to guarantee the old behavior in all cases
                readyOp = account->becomeReady();
            } else {
                warning() << "The account" << accountObjectPath.path() << "was not the expected"
                    << account->objectPath() << "for CR" << parent->objectPath();
                // Construct a new one instead
                account.reset();
            }
        }

        // We need to check again because we might have dropped the expected account just a sec ago
        if (account.isNull() && !accountObjectPath.path().isEmpty()) {
            if (!accFact.isNull()) {
                readyOp = accFact->proxy(
                        TP_QT_ACCOUNT_MANAGER_BUS_NAME, accountObjectPath.path(),
                        connFact, chanFact, contactFact);
                account = AccountPtr::qObjectCast(readyOp->proxy());
            } else {
                account = Account::create(
                        TP_QT_ACCOUNT_MANAGER_BUS_NAME,
                        accountObjectPath.path(), connFact, chanFact, contactFact);
                readyOp = account->becomeReady();
            }
        }
    }

    // FIXME See http://bugs.freedesktop.org/show_bug.cgi?id=21690
    uint stamp = (uint) qdbus_cast<qlonglong>(props.value(QLatin1String("UserActionTime")));
    if (stamp != 0) {
        userActionTime = QDateTime::fromTime_t(stamp);
    }

    preferredHandler = qdbus_cast<QString>(props.value(QLatin1String("PreferredHandler")));
    requests = qdbus_cast<QualifiedPropertyValueMapList>(props.value(QLatin1String("Requests")));

    parent->setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
    readinessHelper->setInterfaces(parent->interfaces());

    if (props.contains(QLatin1String("Hints"))) {
        hints = qdbus_cast<QVariantMap>(props.value(QLatin1String("Hints")));
    }

    if (lastCall) {
        propertiesDone = true;
    }

    if (account) {
        parent->connect(readyOp,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAccountReady(Tp::PendingOperation*)));
    } else if (lastCall) {
        warning() << "No account for ChannelRequest" << parent->objectPath();
        readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

/**
 * \class ChannelRequest
 * \ingroup clientchannelrequest
 * \headerfile TelepathyQt/channel-request.h <TelepathyQt/ChannelRequest>
 *
 * \brief The ChannelRequest class represents a Telepathy channel request.
 *
 * A channel request is an object in the channel dispatcher representing an
 * ongoing request for some channels to be created or found. There can be any
 * number of channel request objects at the same time.
 *
 * A channel request can be cancelled by any client (not just the one that
 * requested it). This means that the channel dispatcher will close the
 * resulting channel, or refrain from requesting it at all, rather than
 * dispatching it to a handler.
 *
 * See \ref async_model
 */

/**
 * Feature representing the core that needs to become ready to make the
 * ChannelRequest object usable.
 *
 * Note that this feature must be enabled in order to use most
 * ChannelRequest methods.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature ChannelRequest::FeatureCore = Feature(QLatin1String(ChannelRequest::staticMetaObject.className()), 0, true);

/**
 * Create a new channel request object using the given \a bus and the given factories.
 *
 * \param bus QDBusConnection to use.
 * \param objectPath The channel request object path.
 * \param immutableProperties The channel request immutable properties.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return A ChannelRequestPtr object pointing to the newly created ChannelRequest object.
 */
ChannelRequestPtr ChannelRequest::create(const QDBusConnection &bus, const QString &objectPath,
        const QVariantMap &immutableProperties,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return ChannelRequestPtr(new ChannelRequest(bus, objectPath, immutableProperties,
                accountFactory, connectionFactory, channelFactory, contactFactory));
}

/**
 * Create a new channel request object for the given \a account.
 *
 * The returned instance will use factories from the account.
 *
 * \param account The account that the request was made through.
 * \param objectPath The channel request object path.
 * \param immutableProperties The channel request immutable properties.
 * \return A ChannelRequestPtr object pointing to the newly created ChannelRequest object.
 */
ChannelRequestPtr ChannelRequest::create(const AccountPtr &account, const QString &objectPath,
        const QVariantMap &immutableProperties)
{
    return ChannelRequestPtr(new ChannelRequest(account, objectPath, immutableProperties));
}

/**
 * Construct a new channel request object using the given \a bus and the given factories.
 *
 * \param bus QDBusConnection to use.
 * \param objectPath The channel request object path.
 * \param accountFactory The account factory to use.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \param immutableProperties The immutable properties of the channel request.
 * \return A ChannelRequestPtr object pointing to the newly created ChannelRequest.
 */
ChannelRequest::ChannelRequest(const QDBusConnection &bus,
        const QString &objectPath, const QVariantMap &immutableProperties,
        const AccountFactoryConstPtr &accountFactory,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
    : StatefulDBusProxy(bus,
            TP_QT_IFACE_CHANNEL_DISPATCHER,
            objectPath, FeatureCore),
      OptionalInterfaceFactory<ChannelRequest>(this),
      mPriv(new Private(this, immutableProperties, accountFactory, connectionFactory,
                  channelFactory, contactFactory))
{
    if (accountFactory->dbusConnection().name() != bus.name()) {
        warning() << "  The D-Bus connection in the account factory is not the proxy connection";
    }

    if (connectionFactory->dbusConnection().name() != bus.name()) {
        warning() << "  The D-Bus connection in the connection factory is not the proxy connection";
    }

    if (channelFactory->dbusConnection().name() != bus.name()) {
        warning() << "  The D-Bus connection in the channel factory is not the proxy connection";
    }
}

/**
 * Construct a new channel request object using the given \a account.
 *
 * The constructed instance will use the factories from the account.
 *
 * \param account Account to use.
 * \param objectPath The channel request object path.
 * \param immutableProperties The immutable properties of the channel request.
 * \return A ChannelRequestPtr object pointing to the newly created ChannelRequest.
 */
ChannelRequest::ChannelRequest(const AccountPtr &account,
        const QString &objectPath, const QVariantMap &immutableProperties)
    : StatefulDBusProxy(account->dbusConnection(),
            TP_QT_IFACE_CHANNEL_DISPATCHER,
            objectPath, FeatureCore),
      OptionalInterfaceFactory<ChannelRequest>(this),
      mPriv(new Private(this, immutableProperties, AccountFactoryPtr(),
                  account->connectionFactory(),
                  account->channelFactory(),
                  account->contactFactory()))
{
    mPriv->account = account;
}

/**
 * Class destructor.
 */
ChannelRequest::~ChannelRequest()
{
    delete mPriv;
}

/**
 * Return the account on which this request was made.
 *
 * This method can be used even before the ChannelRequest is ready, in which case the account object
 * corresponding to the immutable properties is returned. In this case, the Account object is not
 * necessarily ready either. This is useful for eg. matching ChannelRequests from
 * ClientHandlerInterface::addRequest() with existing accounts in the application: either by object
 * path, or if account factories are in use, even by object identity.
 *
 * If the account is not provided in the immutable properties, this will only return a non-\c NULL
 * AccountPtr once ChannelRequest::FeatureCore is ready on this object.
 *
 * \return A pointer to the Account object.
 */
AccountPtr ChannelRequest::account() const
{
    return mPriv->account;
}

/**
 * Return the time at which the user action occurred, or 0 if this channel
 * request is for some reason not involving user action.
 *
 * Unix developers: this corresponds to the _NET_WM_USER_TIME property in EWMH.
 *
 * This property is set when the channel request is created, and can never
 * change.
 *
 * This method can be used even before the ChannelRequest is ready: in this case, the user action
 * time from the immutable properties, if any, is returned.
 *
 * \return The time at which the user action occurred as QDateTime.
 */
QDateTime ChannelRequest::userActionTime() const
{
    return mPriv->userActionTime;
}

/**
 * Return either the well-known bus name (starting with
 * org.freedesktop.Telepathy.Client.) of the preferred handler for this channel,
 * or an empty string to indicate that any handler would be acceptable.
 *
 * This property is set when the channel request is created, and can never
 * change.
 *
 * This method can be used even before the ChannelRequest is ready: in this case, the preferred
 * handler from the immutable properties, if any, is returned.
 *
 * \return The preferred handler, or an empty string if any handler would be
 *         acceptable.
 */
QString ChannelRequest::preferredHandler() const
{
    return mPriv->preferredHandler;
}

/**
 * Return the desirable properties for the channel or channels to be created, as specified when
 * placing the request in the first place.
 *
 * This property is set when the channel request is created, and can never
 * change.
 *
 * This method can be used even before the ChannelRequest is ready: in this case, the requested
 * channel properties from the immutable properties, if any, are returned. This is useful for e.g.
 * matching ChannelRequests from ClientHandlerInterface::addRequest() with existing requests in the
 * application (by the target ID or handle, most likely).
 *
 * \return The requested desirable channel properties as a list of
 *         QualifiedPropertyValueMap objects.
 */
QualifiedPropertyValueMapList ChannelRequest::requests() const
{
    return mPriv->requests;
}

/**
 * Return the dictionary of metadata provided by the channel requester when requesting the channel.
 *
 * This property is set when the channel request is created, and can never change.
 *
 * This method can be used even before the ChannelRequest is ready: in this case, the requested
 * channel properties from the immutable properties, if any, are returned. This is useful for e.g.
 * matching ChannelRequests from ClientHandlerInterface::addRequest() with existing requests in the
 * application (by the target ID or handle, most likely).
 *
 * \sa Account::supportsRequestHints()
 * \return The hints in the request as a ChannelRequestHints object, if any.
 */
ChannelRequestHints ChannelRequest::hints() const
{
    return mPriv->hints;
}

/**
 * Return all of the immutable properties passed to this object when created.
 *
 * This is useful for e.g. getting to domain-specific properties of channel requests.
 *
 * \return The immutable properties as QVariantMap.
 */
QVariantMap ChannelRequest::immutableProperties() const
{
    QVariantMap props = mPriv->immutableProperties;

    if (!account().isNull()) {
        props.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Account"),
            QVariant::fromValue(QDBusObjectPath(account()->objectPath())));
    }

    if (userActionTime().isValid()) {
        props.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".UserActionTime"),
            QVariant::fromValue(userActionTime().toTime_t()));
    }

    if (!preferredHandler().isNull()) {
        props.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".PreferredHandler"),
                preferredHandler());
    }

    if (!requests().isEmpty()) {
        props.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Requests"),
            QVariant::fromValue(requests()));
    }

    props.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Interfaces"),
            QVariant::fromValue(interfaces()));

    return props;
}

/**
 * Cancel the channel request.
 *
 * If failed() is emitted in response to this method, the error will be
 * #TP_QT_ERROR_CANCELLED.
 *
 * If the channel has already been dispatched to a handler, then it's too late
 * to call this method, and the channel request will no longer exist.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *ChannelRequest::cancel()
{
    return new PendingVoid(mPriv->baseInterface->Cancel(),
            ChannelRequestPtr(this));
}

/**
 * Return the Channel which this request succeeded with, if any.
 *
 * This will only ever be populated if Account::requestsSucceedWithChannel() is \c true, and
 * succeeded() has already been emitted on this ChannelRequest. Note that a PendingChannelRequest
 * being successfully finished already implies succeeded() has been emitted.
 *
 * \return A pointer to the Channel object, or a null ChannelPtr if there isn't any.
 */
ChannelPtr ChannelRequest::channel() const
{
    return mPriv->chan;
}

/**
 * Proceed with the channel request.
 *
 * The client that created this object calls this method when it has connected
 * signal handlers for succeeded() and failed(). Note that this is done
 * automatically when using PendingChannelRequest.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *ChannelRequest::proceed()
{
    return new PendingVoid(mPriv->baseInterface->Proceed(),
            ChannelRequestPtr(this));
}

/**
 * Return the ChannelRequestInterface for this ChannelRequest class. This method is
 * protected since the convenience methods provided by this class should
 * always be used instead of the interface by users of the class.
 *
 * \return A pointer to the existing Client::ChannelRequestInterface object for this
 *         ChannelRequest object.
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

        mPriv->extractMainProps(props, true);
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        warning().nospace() << "Properties::GetAll(ChannelRequest) failed with "
            << reply.error().name() << ": " << reply.error().message();
    }

    watcher->deleteLater();
}

void ChannelRequest::onAccountReady(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Unable to make ChannelRequest.Account ready";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                op->errorName(), op->errorMessage());
        return;
    }

    if (mPriv->propertiesDone && !isReady()) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

void ChannelRequest::onLegacySucceeded()
{
    if (mPriv->gotSWC) {
        return;
    }

    emit succeeded(ChannelPtr());
}

void ChannelRequest::onSucceededWithChannel(
        const QDBusObjectPath &connPath,
        const QVariantMap &connProps,
        const QDBusObjectPath &chanPath,
        const QVariantMap &chanProps)
{
    if (mPriv->gotSWC) {
        warning().nospace() << "Got SucceededWithChannel again for CR(" << objectPath() << ")!";
        return;
    }

    mPriv->gotSWC = true;

    QList<PendingOperation *> readyOps;

    QString connBusName = connPath.path().mid(1).replace(
            QLatin1String("/"), QLatin1String("."));
    PendingReady *connReady = mPriv->connFact->proxy(connBusName, connPath.path(),
            mPriv->chanFact, mPriv->contactFact);
    ConnectionPtr conn = ConnectionPtr::qObjectCast(connReady->proxy());
    readyOps.append(connReady);

    PendingReady *chanReady = mPriv->chanFact->proxy(conn, chanPath.path(), chanProps);
    mPriv->chan = ChannelPtr::qObjectCast(chanReady->proxy());
    readyOps.append(chanReady);

    connect(new PendingComposite(readyOps, ChannelRequestPtr(this)),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChanBuilt(Tp::PendingOperation*)));
}

void ChannelRequest::onChanBuilt(Tp::PendingOperation *op)
{
    if (op->isError()) {
        warning() << "Failed to build Channel which the ChannelRequest succeeded with,"
            << "succeeding with NULL channel:" << op->errorName() << ',' << op->errorMessage();
        mPriv->chan.reset();
    }

    emit succeeded(mPriv->chan);
}

/**
 * \fn void ChannelRequest::failed(const QString &errorName,
 *          const QString &errorMessage)
 *
 * Emitted when the channel request has failed. No further
 * methods must not be called on it.
 *
 * \param errorName The name of a D-Bus error.
 * \param errorMessage The error message.
 * \sa succeeded()
 */

/**
 * \fn void ChannelRequest::succeeded(const Tp::ChannelPtr &channel)
 *
 * Emitted when the channel request has succeeded. No further
 * methods must not be called on it.
 *
 * The \a channel parameter can be used to observe the channel resulting from the request (e.g. for
 * it getting closed). The pointer may be NULL if the Channel Dispatcher implementation is too old.
 * Whether a non-NULL channel can be expected can be checked with
 * Account::requestsSucceedWithChannel().
 *
 * If there is a channel, it will be of the subclass determined by and made ready (or not) according
 * to the settings of the ChannelFactory on the Account the request was made through.
 *
 * \param channel Pointer to a proxy for the resulting channel, if the Channel Dispatcher reported it.
 * \sa failed()
 */

/**
 * \class ChannelRequestHints
 * \ingroup clientchannelrequest
 * \headerfile TelepathyQt/channel-request.h <TelepathyQt/ChannelRequestHints>
 *
 * \brief The ChannelRequestHints class represents a dictionary of metadata
 * provided by the channel requester when requesting a channel.
 */

struct TP_QT_NO_EXPORT ChannelRequestHints::Private : public QSharedData
{
    Private() {}
    Private(const QVariantMap &hints) : hints(hints) {}

    QVariantMap hints;
};

ChannelRequestHints::ChannelRequestHints()
{
}

ChannelRequestHints::ChannelRequestHints(const QVariantMap &hints)
    : mPriv(new Private(hints))
{
}

ChannelRequestHints::ChannelRequestHints(const ChannelRequestHints &crh)
    : mPriv(crh.mPriv)
{
}

ChannelRequestHints::~ChannelRequestHints()
{
}

ChannelRequestHints &ChannelRequestHints::operator=(const ChannelRequestHints &other)
{
    if (this == &other) {
        return *this;
    }

    this->mPriv = other.mPriv;
    return *this;
}

bool ChannelRequestHints::isValid() const
{
    return mPriv.constData() != 0;
}

bool ChannelRequestHints::hasHint(const QString &reversedDomain, const QString &localName) const
{
    if (!isValid()) {
        return false;
    }

    const QString qualifiedName = reversedDomain + QLatin1Char('.') + localName;
    return mPriv->hints.contains(qualifiedName);
}

QVariant ChannelRequestHints::hint(const QString &reversedDomain, const QString &localName) const
{
    if (!isValid()) {
        return QVariant();
    }

    const QString qualifiedName = reversedDomain + QLatin1Char('.') + localName;
    return mPriv->hints.value(qualifiedName);
}

void ChannelRequestHints::setHint(const QString &reversedDomain, const QString &localName, const QVariant &value)
{
    const QString qualifiedName = reversedDomain + QLatin1Char('.') + localName;

    if (!isValid()) {
        mPriv = new Private();
    }

    mPriv->hints.insert(qualifiedName, value);
}

QVariantMap ChannelRequestHints::allHints() const
{
    return isValid() ? mPriv->hints : QVariantMap();
}

} // Tp
