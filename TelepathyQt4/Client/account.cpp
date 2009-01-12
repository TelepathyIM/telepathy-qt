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

#include <TelepathyQt4/Client/Account>
#include "TelepathyQt4/Client/account-internal.h"

#include "TelepathyQt4/Client/_gen/account.moc.hpp"
#include "TelepathyQt4/Client/_gen/account-internal.moc.hpp"
#include "TelepathyQt4/_gen/cli-account.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-body.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingVoidMethodCall>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>

#include <QQueue>
#include <QRegExp>
#include <QTimer>

// TODO listen to avatarChanged signal

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

namespace Telepathy
{
namespace Client
{

Account::Private::PendingReady::PendingReady(Account::Features features, QObject *parent)
    : PendingOperation(parent),
      features(features)
{
}

Account::Private::Private(Account *parent)
    : QObject(parent),
      baseInterface(new AccountInterface(parent->dbusConnection(),
                        parent->busName(), parent->objectPath(), parent)),
      ready(false),
      features(0),
      pendingFeatures(0),
      valid(false),
      enabled(false),
      connectsAutomatically(false),
      cm(0),
      protocolInfo(0),
      connectionStatus(Telepathy::ConnectionStatusDisconnected),
      connectionStatusReason(Telepathy::ConnectionStatusReasonNoneSpecified)
{
    // FIXME: QRegExp probably isn't the most efficient possible way to parse
    //        this :-)
    QRegExp rx("^" TELEPATHY_ACCOUNT_OBJECT_PATH_BASE
               "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
               "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
               "/([_A-Za-z][_A-Za-z0-9]*)"  // account-specific part
               );

    if (rx.exactMatch(parent->objectPath())) {
        cmName = rx.cap(1);
        protocol = rx.cap(2);
    } else {
        warning() << "Not a valid Account object path:" <<
            parent->objectPath();
    }

    connect(baseInterface,
            SIGNAL(Removed()),
            SLOT(onRemoved()));
    connect(baseInterface,
            SIGNAL(AccountPropertyChanged(const QVariantMap &)),
            SLOT(onPropertyChanged(const QVariantMap &)));

    introspectQueue.enqueue(&Private::callGetAll);
    QTimer::singleShot(0, this, SLOT(continueIntrospection()));
}

Account::Private::~Private()
{
    delete baseInterface;
    delete cm;
}

void Account::Private::callGetAll()
{
    debug() << "Calling Properties::GetAll(Account)";
    Account *ac = static_cast<Account *>(parent());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            ac->propertiesInterface()->GetAll(
                TELEPATHY_INTERFACE_ACCOUNT), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onGetAllAccountReturn(QDBusPendingCallWatcher *)));
}

void Account::Private::callGetAvatar()
{
    Account *ac = static_cast<Account *>(parent());
    AccountInterfaceAvatarInterface *iface = ac->avatarInterface();
    if (!iface) {
        Q_FOREACH (PendingReady *operation, pendingOperations) {
            if (operation->features & FeatureAvatar) {
                operation->setFinishedWithError(TELEPATHY_ERROR_NOT_IMPLEMENTED,
                        "Unimplemented");
                pendingOperations.removeOne(operation);
            }
        }
        continueIntrospection();
        return;
    }

    debug() << "Calling GetAvatar(Account)";
    DBus::PropertiesInterface *propertiesIface =
        ac->interface<DBus::PropertiesInterface>(*iface);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            propertiesIface->Get(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,
                "Avatar"), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onGetAvatarReturn(QDBusPendingCallWatcher *)));
}

void Account::Private::callGetProtocolInfo()
{
    Account *ac = static_cast<Account *>(parent());
    cm = new ConnectionManager(
            ac->dbusConnection(),
            cmName, this);
    connect(cm->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onConnectionManagerReady(Telepathy::Client::PendingOperation *)));
}

void Account::Private::updateProperties(const QVariantMap &props)
{
    if (props.contains("Interfaces")) {
        interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    }

    if (props.contains("DisplayName")) {
        displayName = qdbus_cast<QString>(props["DisplayName"]);
        Q_EMIT displayNameChanged(displayName);
    }

    if (props.contains("Icon")) {
        icon = qdbus_cast<QString>(props["Icon"]);
        Q_EMIT iconChanged(icon);
    }

    if (props.contains("Nickname")) {
        nickname = qdbus_cast<QString>(props["Nickname"]);
        Q_EMIT nicknameChanged(icon);
    }

    if (props.contains("NormalizedName")) {
        normalizedName = qdbus_cast<QString>(props["NormalizedName"]);
    }

    if (props.contains("Valid")) {
        valid = qdbus_cast<bool>(props["Valid"]);
        Q_EMIT validityChanged(valid);
    }

    if (props.contains("Enabled")) {
        enabled = qdbus_cast<bool>(props["Enabled"]);
        Q_EMIT stateChanged(enabled);
    }

    if (props.contains("ConnectAutomatically")) {
        connectsAutomatically =
                qdbus_cast<bool>(props["ConnectAutomatically"]);
    }

    if (props.contains("Parameters")) {
        parameters = qdbus_cast<QVariantMap>(props["Parameters"]);
        Q_EMIT parametersChanged(parameters);
    }

    if (props.contains("AutomaticPresence")) {
        automaticPresence = qdbus_cast<Telepathy::SimplePresence>(
                props["AutomaticPresence"]);
    }

    if (props.contains("CurrentPresence")) {
        currentPresence = qdbus_cast<Telepathy::SimplePresence>(
                props["CurrentPresence"]);
        Q_EMIT presenceChanged(currentPresence);
    }

    if (props.contains("RequestedPresence")) {
        requestedPresence = qdbus_cast<Telepathy::SimplePresence>(
                props["RequestedPresence"]);
    }

    if (props.contains("Connection")) {
        QString path = qdbus_cast<QDBusObjectPath>(props["Connection"]).path();
        if (path == QLatin1String("/")) {
            connectionObjectPath = QString();
        } else {
            connectionObjectPath = path;
        }
    }

    if (props.contains("ConnectionStatus") || props.contains("ConnectionStatusReason")) {
        if (props.contains("ConnectionStatus")) {
            connectionStatus = Telepathy::ConnectionStatus(
                    qdbus_cast<uint>(props["ConnectionStatus"]));
        }

        if (props.contains("ConnectionStatusReason")) {
            connectionStatusReason = Telepathy::ConnectionStatusReason(
                    qdbus_cast<uint>(props["ConnectionStatusReason"]));
        }
        Q_EMIT connectionStatusChanged(connectionStatus, connectionStatusReason);
    }
}

void Account::Private::onGetAllAccountReturn(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Account)";
        updateProperties(reply.value());
        debug() << "Account is ready";
        ready = true;
    } else {
        warning().nospace() <<
            "GetAll(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
}

void Account::Private::onGetAvatarReturn(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QByteArray, QString> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to GetAvatar(Account)";
        avatarData = reply.argumentAt<0>();
        avatarMimeType = reply.argumentAt<1>();

        features |= Account::FeatureAvatar;
        pendingFeatures ^= Account::FeatureAvatar;
    } else {
        // signal all pending operations that cares about avatar that
        // it failed
        Q_FOREACH (PendingReady *operation, pendingOperations) {
            if (operation->features & FeatureAvatar) {
                operation->setFinishedWithError(reply.error());
                pendingOperations.removeOne(operation);
            }
        }

        warning().nospace() <<
            "GetAvatar(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
}

void Account::Private::onConnectionManagerReady(PendingOperation *operation)
{
    bool error = operation->isError();
    if (!error) {
        Q_FOREACH (ProtocolInfo *info, cm->protocols()) {
            if (info->name() == protocol) {
                protocolInfo = info;
            }
        }

        error = (protocolInfo == 0);
    }

    if (error) {
        // signal all pending operations that cares about protocol info that
        // it failed
        Q_FOREACH (PendingReady *operation, pendingOperations) {
            if (operation->features & FeatureProtocolInfo) {
                operation->setFinishedWithError(operation->errorName(),
                        operation->errorMessage());
                pendingOperations.removeOne(operation);
            }
        }
    }

    continueIntrospection();
}

void Account::Private::onPropertyChanged(const QVariantMap &delta)
{
    updateProperties(delta);
}

void Account::Private::onRemoved()
{
    ready = false;
    valid = false;
    enabled = false;
    Q_EMIT removed();
}

void Account::Private::continueIntrospection()
{
    if (introspectQueue.isEmpty()) {
        Q_FOREACH (PendingReady *operation, pendingOperations) {
            if (operation->features == 0 && ready) {
                operation->setFinished();
            }
            else if (operation->features == Account::FeatureAvatar &&
                features & Account::FeatureAvatar) {
                operation->setFinished();
            }
            else if (operation->features == Account::FeatureProtocolInfo &&
                     features & Account::FeatureProtocolInfo) {
                operation->setFinished();
            }
            else if (operation->features == (Account::FeatureAvatar | Account::FeatureProtocolInfo) &&
                     features == (Account::FeatureAvatar | Account::FeatureProtocolInfo)) {
                operation->setFinished();
            }
            if (operation->isFinished()) {
                pendingOperations.removeOne(operation);
            }
        }
    }
    else {
        (this->*(introspectQueue.dequeue()))();
    }
}

PendingOperation *Account::Private::becomeReady(Account::Features requestedFeatures)
{
    debug() << "calling becomeReady with requested features:" << requestedFeatures;
    Q_FOREACH (PendingReady *operation, pendingOperations) {
        if (operation->features == requestedFeatures) {
            debug() << "returning cached pending operation";
            return operation;
        }
    }

    if ((requestedFeatures & FeatureAvatar) &&
        !(features & FeatureAvatar) &&
        !(pendingFeatures & FeatureAvatar)) {
        introspectQueue.enqueue(&Private::callGetAvatar);
    }

    if ((requestedFeatures & FeatureProtocolInfo) &&
        !(features & FeatureProtocolInfo) &&
        !(pendingFeatures & FeatureProtocolInfo)) {
        introspectQueue.enqueue(&Private::callGetProtocolInfo);
    }

    pendingFeatures |= features;

    QTimer::singleShot(0, this, SLOT(continueIntrospection()));

    debug() << "creating new pending operation";
    PendingReady *operation = new PendingReady(requestedFeatures, this);
    pendingOperations.append(operation);
    return operation;
}

/**
 * \class Account
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/Client/account.h> <TelepathyQt4/Client/Account>
 *
 * Object representing a Telepathy account.
 */

/**
 * Construct a new Account object.
 *
 * \param am Account manager.
 * \param objectPath Account object path.
 * \param parent Object parent.
 */
Account::Account(AccountManager *am, const QDBusObjectPath &objectPath,
        QObject *parent)
    : StatelessDBusProxy(am->dbusConnection(),
            am->busName(), objectPath.path(), parent),
      mPriv(new Private(this))
{
    connect(mPriv,
            SIGNAL(removed()),
            SIGNAL(removed()));
    connect(mPriv,
            SIGNAL(displayNameChanged(const QString &)),
            SIGNAL(displayNameChanged(const QString &)));
    connect(mPriv,
            SIGNAL(iconChanged(const QString &)),
            SIGNAL(iconChanged(const QString &)));
    connect(mPriv,
            SIGNAL(nicknameChanged(const QString &)),
            SIGNAL(nicknameChanged(const QString &)));
    connect(mPriv,
            SIGNAL(stateChanged(bool)),
            SIGNAL(stateChanged(bool)));
    connect(mPriv,
            SIGNAL(validityChanged(bool)),
            SIGNAL(validityChanged(bool)));
    connect(mPriv,
            SIGNAL(parametersChanged(const QVariantMap &)),
            SIGNAL(parametersChanged(const QVariantMap &)));
    connect(mPriv,
            SIGNAL(presenceChanged(const Telepathy::SimplePresence &)),
            SIGNAL(presenceChanged(const Telepathy::SimplePresence &)));
    connect(mPriv,
            SIGNAL(avatarChanged(const QByteArray &, const QString &)),
            SIGNAL(avatarChanged(const QByteArray &, const QString &)));
    connect(mPriv,
            SIGNAL(connectionStatusChanged(Telepathy::ConnectionStatus,
                                           Telepathy::ConnectionStatusReason)),
            SIGNAL(connectionStatusChanged(Telepathy::ConnectionStatus,
                                           Telepathy::ConnectionStatusReason)));
}

/**
 * Class destructor.
 */
Account::~Account()
{
    delete mPriv;
}

/**
 * Get the AccountManager from which this Account was created.
 *
 * \return A pointer to the AccountManager object that owns this Account.
 */
AccountManager *Account::manager() const
{
    return qobject_cast<AccountManager *>(parent());
}

/**
 * Return whether this is a valid account.
 *
 * \return \c true if the account is valid, \c false otherwise.
 */
bool Account::isValid() const
{
    return mPriv->valid;
}

/**
 * Return whether this account is enabled.
 *
 * \return \c true if the account is enabled, \c false otherwise.
 */
bool Account::isEnabled() const
{
    return mPriv->enabled;
}

PendingOperation *Account::setEnabled(bool value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "Enabled", QDBusVariant(value)));
}

QString Account::cmName() const
{
    return mPriv->cmName;
}

QString Account::protocol() const
{
    return mPriv->protocol;
}

QString Account::displayName() const
{
    return mPriv->displayName;
}

PendingOperation *Account::setDisplayName(const QString &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "DisplayName", QDBusVariant(value)));
}

QString Account::icon() const
{
    return mPriv->icon;
}

PendingOperation *Account::setIcon(const QString &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "Icon", QDBusVariant(value)));
}

QString Account::nickname() const
{
    return mPriv->nickname;
}

PendingOperation *Account::setNickname(const QString &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "Nickname", QDBusVariant(value)));
}

QByteArray Account::avatarData() const
{
    if (!mPriv->features & FeatureAvatar) {
        warning() << "Trying to retrieve avatar data from account without"
                     "calling Account::becomeReady(FeatureAvatar)";
    }
    return mPriv->avatarData;
}

QString Account::avatarMimeType() const
{
    if (!mPriv->features & FeatureAvatar) {
        warning() << "Trying to retrieve avatar mimetype from account without"
                     "calling Account::becomeReady(FeatureAvatar)";
    }
    return mPriv->avatarMimeType;
}

PendingOperation *Account::setAvatar(const QByteArray &data,
        const QString &mimeType)
{
    AccountInterfaceAvatarInterface *iface = avatarInterface();
    if (!iface) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Unimplemented");
    }

    DBus::PropertiesInterface *propertiesIface =
        OptionalInterfaceFactory::interface<DBus::PropertiesInterface>(*iface);
    QDBusArgument arg;
    arg << data << mimeType;
    return new PendingVoidMethodCall(this,
            propertiesIface->Set(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,
                "Avatar", QDBusVariant(arg.asVariant())));
}

QVariantMap Account::parameters() const
{
    return mPriv->parameters;
}

PendingOperation *Account::updateParameters(const QVariantMap &set,
        const QStringList &unset)
{
    return new PendingVoidMethodCall(this,
            baseInterface()->UpdateParameters(set, unset));
}

ProtocolInfo *Account::protocolInfo() const
{
    if (!mPriv->features & FeatureProtocolInfo) {
        warning() << "Trying to retrieve protocol info from account without"
                     "calling Account::becomeReady(FeatureProtocolInfo)";
    }
    return mPriv->protocolInfo;
}

Telepathy::SimplePresence Account::automaticPresence() const
{
    return mPriv->automaticPresence;
}

PendingOperation *Account::setAutomaticPresence(
        const Telepathy::SimplePresence &value)
{
    QDBusArgument arg;
    arg << value;
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "AutomaticPresence", QDBusVariant(arg.asVariant())));
}

bool Account::connectsAutomatically() const
{
    return mPriv->connectsAutomatically;
}

PendingOperation *Account::setConnectsAutomatically(bool value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "ConnectAutomatically", QDBusVariant(value)));
}

Telepathy::ConnectionStatus Account::connectionStatus() const
{
    return mPriv->connectionStatus;
}

Telepathy::ConnectionStatusReason Account::connectionStatusReason() const
{
    return mPriv->connectionStatusReason;
}

// PendingConnection *Account::getConnection(Connection::Features features = 0) const
// {
//     return 0;
// }

Telepathy::SimplePresence Account::currentPresence() const
{
    return mPriv->currentPresence;
}

Telepathy::SimplePresence Account::requestedPresence() const
{
    return mPriv->requestedPresence;
}

PendingOperation *Account::setRequestedPresence(
        const Telepathy::SimplePresence &value)
{
    QDBusArgument arg;
    arg << value;
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "RequestedPresence", QDBusVariant(arg.asVariant())));
}

QString Account::uniqueIdentifier() const
{
    QString path = objectPath();
    // 25 = len("/org/freedesktop/Account/")
    return path.right(path.length() - 25);
}

QString Account::connectionObjectPath() const
{
    return mPriv->connectionObjectPath;
}

QString Account::normalizedName() const
{
    return mPriv->normalizedName;
}

PendingOperation *Account::remove()
{
    return new PendingVoidMethodCall(this, baseInterface()->Remove());
}

bool Account::isReady(Features features) const
{
    return mPriv->ready
        && ((mPriv->features & features) == features);
}

PendingOperation *Account::becomeReady(Features features)
{
    if (isReady(features)) {
        return new PendingSuccess(this);
    }

    return mPriv->becomeReady(features);
}

QStringList Account::interfaces() const
{
    return mPriv->interfaces;
}

/**
 * \fn DBus::propertiesInterface *Account::propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * Account interface relies on properties, so this interface is
 * always assumed to be present.
 */

/**
 * Get the AccountInterface for this Account. This
 * method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing AccountInterface for this
 *         Account.
 */
AccountInterface *Account::baseInterface() const
{
    return mPriv->baseInterface;
}

} // Telepathy::Client
} // Telepathy
