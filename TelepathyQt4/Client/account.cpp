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
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingVoidMethodCall>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>

#include <QQueue>
#include <QRegExp>
#include <QTimer>

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
      missingFeatures(0),
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

void Account::Private::checkForAvatarInterface()
{
    Account *ac = static_cast<Account *>(parent());
    AccountInterfaceAvatarInterface *iface = ac->avatarInterface();
    if (!iface) {
        debug() << "Avatar interface is not support for account" << ac->objectPath();
        // add it to missing features so we don't try to retrieve the avatar
        missingFeatures |= Account::FeatureAvatar;
    }
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
    debug() << "Calling GetAvatar(Account)";
    Account *ac = static_cast<Account *>(parent());
    // we already checked if avatar interface exists, so bypass avatar interface
    // checking
    AccountInterfaceAvatarInterface *iface =
        ac->avatarInterface(BypassInterfaceCheck);

    // If we are here it means the user cares about avatar, so
    // connect to avatar changed signal, so we update the avatar
    // when it changes.
    connect(iface,
            SIGNAL(AvatarChanged()),
            SLOT(onAvatarChanged()));

    retrieveAvatar();
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

        checkForAvatarInterface();
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

void Account::Private::retrieveAvatar()
{
    Account *ac = static_cast<Account *>(parent());
    // we already checked if avatar interface exists, so bypass avatar interface
    // checking
    AccountInterfaceAvatarInterface *iface =
        ac->avatarInterface(BypassInterfaceCheck);

    DBus::PropertiesInterface *propertiesIface =
        ac->interface<DBus::PropertiesInterface>(*iface);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            propertiesIface->Get(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,
                "Avatar"), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onGetAvatarReturn(QDBusPendingCallWatcher *)));
}

void Account::Private::onGetAllAccountReturn(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Account)";
        updateProperties(reply.value());
        debug() << "Account basic functionality is ready";
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
    QDBusPendingReply<QVariant> reply = *watcher;

    pendingFeatures ^= Account::FeatureAvatar;

    if (!reply.isError()) {
        features |= Account::FeatureAvatar;

        debug() << "Got reply to GetAvatar(Account)";
        avatar = qdbus_cast<Telepathy::Avatar>(reply);

        Q_EMIT avatarChanged(avatar);
    } else {
        // add it to missing features so we don't try to retrieve the avatar
        // again
        missingFeatures |= Account::FeatureAvatar;

        warning().nospace() <<
            "GetAvatar(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    continueIntrospection();

    watcher->deleteLater();
}

void Account::Private::onAvatarChanged()
{
    debug() << "Avatar changed, retrieving it";
    retrieveAvatar();
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

    pendingFeatures ^= Account::FeatureProtocolInfo;

    if (!error) {
        features |= Account::FeatureProtocolInfo;
    }
    else {
        missingFeatures |= Account::FeatureProtocolInfo;

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
                (features & Account::FeatureAvatar ||
                 missingFeatures & Account::FeatureAvatar)) {
                // if we don't have avatar we will just finish silently
                operation->setFinished();
            }
            else if (operation->features == Account::FeatureProtocolInfo &&
                     features & Account::FeatureProtocolInfo) {
                operation->setFinished();
            }
            else if (operation->features == (Account::FeatureAvatar | Account::FeatureProtocolInfo) &&
                     ((features == (Account::FeatureAvatar | Account::FeatureProtocolInfo)) ||
                      (features & Account::FeatureProtocolInfo && missingFeatures & Account::FeatureAvatar))) {
                // if we don't have avatar we will just finish silently
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

    if (requestedFeatures & FeatureAvatar) {
        // if the only feature requested is avatar and avatar is know to not be
        // supported, just finish silently
        if (requestedFeatures == FeatureAvatar &&
            missingFeatures & FeatureAvatar) {
            return new PendingSuccess(this);
        }

        // if we know that avatar is not supported, no need to
        // queue the call to get avatar
        if (!(missingFeatures & FeatureAvatar) &&
            !(features & FeatureAvatar) &&
            !(pendingFeatures & FeatureAvatar)) {
            introspectQueue.enqueue(&Private::callGetAvatar);
        }
    }

    if (requestedFeatures & FeatureProtocolInfo) {
        // the user asked for protocol info
        // but we already know that protocol info is not supported, so
        // fail directly
        if (missingFeatures & FeatureProtocolInfo) {
            return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                    QString("ProtocolInfo not found for protocol %1 on CM %2")
                        .arg(protocol).arg(cmName));
        }

        if (!(features & FeatureProtocolInfo) &&
            !(pendingFeatures & FeatureProtocolInfo)) {
            introspectQueue.enqueue(&Private::callGetProtocolInfo);
        }
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

/*! \enum Account::InterfaceSupportedChecking
 *
 * Specifies if the interface being supported by the remote object should be
 * checked by optionalInterface() and the convenience functions for it.
 *
 * \value CheckInterfaceSupported Don't return an interface instance unless it
 *                                can be guaranteed that the remote object
 *                                actually implements the interface.
 * \value BypassInterfaceCheck Return an interface instance even if it can't
 *                             be verified that the remote object supports the
 *                             interface.
 * \sa optionalInterface()
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
            SIGNAL(avatarChanged(const Telepathy::Avatar &)),
            SIGNAL(avatarChanged(const Telepathy::Avatar &)));
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
 * Get whether this is a valid account.
 *
 * \return \c true if the account is valid, \c false otherwise.
 */
bool Account::isValid() const
{
    return mPriv->valid;
}

/**
 * Get whether this account is enabled.
 *
 * \return \c true if the account is enabled, \c false otherwise.
 */
bool Account::isEnabled() const
{
    return mPriv->enabled;
}

/**
 * Set whether this account is enabled.
 *
 * \param value Whether this account should be enabled or disabled.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setEnabled(bool value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "Enabled", QDBusVariant(value)));
}

/**
 * Get this account connection manager name.
 *
 * \return Account connection manager name.
 */
QString Account::cmName() const
{
    return mPriv->cmName;
}

/**
 * Get this account protocol name.
 *
 * \return Account protocol name.
 */
QString Account::protocol() const
{
    return mPriv->protocol;
}

/**
 * Get this account display name.
 *
 * \return Account display name.
 */
QString Account::displayName() const
{
    return mPriv->displayName;
}

/**
 * Set this account display name.
 *
 * \param value Account display name.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setDisplayName(const QString &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "DisplayName", QDBusVariant(value)));
}

/**
 * Get this account icon name.
 *
 * \return Account icon name.
 */
QString Account::icon() const
{
    return mPriv->icon;
}

/**
 * Set this account icon.
 *
 * \param value Account icon name.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setIcon(const QString &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "Icon", QDBusVariant(value)));
}

/**
 * Get this account nickname.
 *
 * \return Account nickname.
 */
QString Account::nickname() const
{
    return mPriv->nickname;
}

/**
 * Set the account nickname.
 *
 * \param value Account nickname.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setNickname(const QString &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "Nickname", QDBusVariant(value)));
}

/**
 * Get this account avatar.
 *
 * Note that in order to make this method works you should call
 * Account::becomeReady(FeatureAvatar) and wait for it to finish
 * successfully.
 *
 * \return Account avatar.
 */
const Telepathy::Avatar &Account::avatar() const
{
    if (mPriv->missingFeatures & FeatureAvatar) {
        warning() << "Trying to retrieve avatar from account, but "
                     "avatar is not supported";
    }
    else if (!(mPriv->features & FeatureAvatar)) {
        warning() << "Trying to retrieve avatar from account without "
                     "calling Account::becomeReady(FeatureAvatar)";
    }
    return mPriv->avatar;
}

/**
 * Set this account avatar.
 *
 * \param data Avatar data.
 * \param mimeType Avatar mimetype.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setAvatar(const Telepathy::Avatar &avatar)
{
    AccountInterfaceAvatarInterface *iface = avatarInterface();
    if (!iface) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Unimplemented");
    }

    DBus::PropertiesInterface *propertiesIface =
        OptionalInterfaceFactory::interface<DBus::PropertiesInterface>(*iface);
    return new PendingVoidMethodCall(this,
            propertiesIface->Set(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,
                "Avatar", QDBusVariant(QVariant::fromValue(avatar))));
}

/**
 * Get this account parameters.
 *
 * \return Account parameters.
 */
QVariantMap Account::parameters() const
{
    return mPriv->parameters;
}

/**
 * Update this account parameters.
 *
 * \param set Parameters to set.
 * \param unset Parameters to unset.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::updateParameters(const QVariantMap &set,
        const QStringList &unset)
{
    return new PendingVoidMethodCall(this,
            baseInterface()->UpdateParameters(set, unset));
}

/**
 * Get the protocol info for this account protocol.
 *
 * Note that in order to make this method works you should call
 * Account::becomeReady(FeatureProtocolInfo) and wait for it to finish
 * successfully.
 *
 * \return ProtocolInfo for this account protocol.
 */
ProtocolInfo *Account::protocolInfo() const
{
    if (!mPriv->features & FeatureProtocolInfo) {
        warning() << "Trying to retrieve protocol info from account without"
                     "calling Account::becomeReady(FeatureProtocolInfo)";
    }
    return mPriv->protocolInfo;
}

/**
 * Get whether this account should be put online automatically whenever possible.
 *
 * \return \c true if it should try to connect automatically, \c false otherwise.
 */
bool Account::connectsAutomatically() const
{
    return mPriv->connectsAutomatically;
}

/**
 * Set whether this account should be put online automatically whenever possible.
 *
 * \param value Value indicating if this account should be put online whenever
 *              possible.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setConnectsAutomatically(bool value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "ConnectAutomatically", QDBusVariant(value)));
}

/**
 * Get the connection status of this account.
 *
 * \return Value indication the status of this account conneciton.
 */
Telepathy::ConnectionStatus Account::connectionStatus() const
{
    return mPriv->connectionStatus;
}

/**
 * Get the connection status reason of this account.
 *
 * \return Value indication the status reason of this account conneciton.
 */
Telepathy::ConnectionStatusReason Account::connectionStatusReason() const
{
    return mPriv->connectionStatusReason;
}

/**
 * Get the Connection object for this account.
 *
 * Note that the Connection object won't be cached by account, and
 * should be done by the application itself.
 *
 * Remember to call Connection::becomeReady on the new connection, to
 * make sure it is ready before using it.
 *
 * \return Connection object, or 0 if an error occurred.
 */
Connection *Account::getConnection() const
{
    if (mPriv->connectionObjectPath.isEmpty()) {
        return 0;
    }
    QString objectPath = mPriv->connectionObjectPath;
    QString serviceName = objectPath.replace('/', '.');
    return new Connection(dbusConnection(), serviceName, objectPath);
}

/**
 * Set the presence status that this account should have if it is brought
 * online.
 *
 * \return Presence status that will be set when this account is put online.
 */
Telepathy::SimplePresence Account::automaticPresence() const
{
    return mPriv->automaticPresence;
}

/**
 * Set the presence status that this account should have if it is brought
 * online.
 *
 * \param value Presence status to set when this account is put online.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa setRequestedPresence()
 */
PendingOperation *Account::setAutomaticPresence(
        const Telepathy::SimplePresence &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "AutomaticPresence", QDBusVariant(QVariant::fromValue(value))));
}

/**
 * Get the actual presence of this account.
 *
 * \return The actual presence of this account.
 * \sa requestedPresence(), automaticPresence()
 */
Telepathy::SimplePresence Account::currentPresence() const
{
    return mPriv->currentPresence;
}

/**
 * Get the requested presence of this account.
 *
 * When this is changed, the account manager should attempt to manipulate the
 * connection manager to make CurrentPresence match RequestedPresence as closely
 * as possible.
 *
 * \return The requested presence of this account.
 * \sa currentPresence(), automaticPresence()
 */
Telepathy::SimplePresence Account::requestedPresence() const
{
    return mPriv->requestedPresence;
}

/**
 * Set the requested presence.
 *
 * \param value The requested presence.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa setAutomaticPresence()
 */
PendingOperation *Account::setRequestedPresence(
        const Telepathy::SimplePresence &value)
{
    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(TELEPATHY_INTERFACE_ACCOUNT,
                "RequestedPresence", QDBusVariant(QVariant::fromValue(value))));
}

/**
 * Get the unique identifier for this account.
 *
 * This identifier should be unique per AccountManager implementation,
 * i.e. at least per QDBusConnection.
 *
 * \return Account unique identifier.
 */
QString Account::uniqueIdentifier() const
{
    QString path = objectPath();
    // 25 = len("/org/freedesktop/Account/")
    return path.right(path.length() - 25);
}

/**
 * Get the connection object path of this account.
 *
 * \return Account connection object path.
 */
QString Account::connectionObjectPath() const
{
    return mPriv->connectionObjectPath;
}

QString Account::normalizedName() const
{
    return mPriv->normalizedName;
}

/**
 * Delete this account.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::remove()
{
    return new PendingVoidMethodCall(this, baseInterface()->Remove());
}

/**
 * Return whether this object has finished its initial setup.
 *
 * This is mostly useful as a sanity check, in code that shouldn't be run
 * until the object is ready. To wait for the object to be ready, call
 * becomeReady() and connect to the finished signal on the result.
 *
 * \param features Which features should be tested.
 * \return \c true if the object has finished initial setup.
 */
bool Account::isReady(Features features) const
{
    return mPriv->ready
        && ((mPriv->features & features) == features);
}

/**
 * Return a pending operation which will succeed when this object finishes
 * its initial setup, or will fail if a fatal error occurs during this
 * initial setup.
 *
 * \param features Which features should be tested.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when this object has finished or failed its initial setup.
 */
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
 * \fn Account::optionalInterface(InterfaceSupportedChecking check) const
 *
 * Get a pointer to a valid instance of a given %Account optional
 * interface class, associated with the same remote object the Account is
 * associated with, and destroyed at the same time the Account is
 * destroyed.
 *
 * If the list returned by interfaces() doesn't contain the name of the
 * interface requested <code>0</code> is returned. This check can be
 * bypassed by specifying #BypassInterfaceCheck for <code>check</code>, in
 * which case a valid instance is always returned.
 *
 * If the object is not ready, the list returned by interfaces() isn't
 * guaranteed to yet represent the full set of interfaces supported by the
 * remote object.
 * Hence the check might fail even if the remote object actually supports
 * the requested interface; using #BypassInterfaceCheck is suggested when
 * the Account is not suitably ready.
 *
 * \see OptionalInterfaceFactory::interface
 *
 * \tparam Interface Class of the optional interface to get.
 * \param check Should an instance be returned even if it can't be
 *              determined that the remote object supports the
 *              requested interface.
 * \return Pointer to an instance of the interface class, or <code>0</code>.
 */

/**
 * \fn DBus::propertiesInterface *Account::propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * Account interface relies on properties, so this interface is
 * always assumed to be present.
 */

/**
 * \fn AccountInterfaceAvatarInterface *Account::avatarInterface(InterfaceSupportedChecking check) const
 *
 * Convenience function for getting a Avatar interface proxy.
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
