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

#include <TelepathyQt4/Account>

#include "TelepathyQt4/_gen/account.moc.hpp"
#include "TelepathyQt4/_gen/cli-account.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-body.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingChannelRequest>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingStringList>
#include <TelepathyQt4/PendingVoidMethodCall>
#include <TelepathyQt4/ReferencedHandles>
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

namespace Tp
{

struct Account::Private
{
    Private(Account *parent);
    ~Private();

    void init();

    static void introspectMain(Private *self);
    static void introspectAvatar(Private *self);
    static void introspectProtocolInfo(Private *self);

    void updateProperties(const QVariantMap &props);
    void retrieveAvatar();

    // Public object
    Account *parent;

    // Instance of generated interface class
    Client::AccountInterface *baseInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    QStringList interfaces;
    QVariantMap parameters;
    bool valid;
    bool enabled;
    bool connectsAutomatically;
    bool hasBeenOnline;
    QString cmName;
    QString protocol;
    QString displayName;
    QString nickname;
    QString icon;
    QString connectionObjectPath;
    QString normalizedName;
    Avatar avatar;
    ConnectionManagerPtr cm;
    ProtocolInfo *protocolInfo;
    ConnectionStatus connectionStatus;
    ConnectionStatusReason connectionStatusReason;
    SimplePresence automaticPresence;
    SimplePresence currentPresence;
    SimplePresence requestedPresence;
    ConnectionPtr connection;
};

Account::Private::Private(Account *parent)
    : parent(parent),
      baseInterface(new Client::AccountInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      readinessHelper(parent->readinessHelper()),
      valid(false),
      enabled(false),
      connectsAutomatically(false),
      hasBeenOnline(false),
      protocolInfo(0),
      connectionStatus(ConnectionStatusDisconnected),
      connectionStatusReason(ConnectionStatusReasonNoneSpecified)
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
        warning() << "Account object path is not spec-compliant, "
            "trying again with a different account-specific part check";

        rx = QRegExp("^" TELEPATHY_ACCOUNT_OBJECT_PATH_BASE
               "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
               "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
               "/([_A-Za-z0-9]*)"  // account-specific part
               );
        if (rx.exactMatch(parent->objectPath())) {
            cmName = rx.cap(1);
            protocol = rx.cap(2);
        } else {
            warning() << "Not a valid Account object path:" <<
                parent->objectPath();
        }
    }

    ReadinessHelper::Introspectables introspectables;

    // As Account does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features(),                                                             // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    ReadinessHelper::Introspectable introspectableAvatar(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << FeatureCore,                                              // dependsOnFeatures (core)
        QStringList() << TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectAvatar,
        this);
    introspectables[FeatureAvatar] = introspectableAvatar;

    ReadinessHelper::Introspectable introspectableProtocolInfo(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << FeatureCore,                                              // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectProtocolInfo,
        this);
    introspectables[FeatureProtocolInfo] = introspectableProtocolInfo;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(Features() << FeatureCore);

    init();
}

Account::Private::~Private()
{
}

/**
 * \class Account
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/account.h> <TelepathyQt4/Account>
 *
 * Object representing a Telepathy account.
 *
 * If the account is deleted from the AccountManager, this object
 * will not be deleted automatically; however, it will emit invalidated()
 * and will cease to be useful.
 */

const Feature Account::FeatureCore = Feature(Account::staticMetaObject.className(), 0, true);
const Feature Account::FeatureAvatar = Feature(Account::staticMetaObject.className(), 1);
const Feature Account::FeatureProtocolInfo = Feature(Account::staticMetaObject.className(), 2);

AccountPtr Account::create(const QString &busName,
        const QString &objectPath)
{
    return AccountPtr(new Account(busName, objectPath));
}

AccountPtr Account::create(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath)
{
    return AccountPtr(new Account(bus, busName, objectPath));
}


/**
 * Construct a new Account object.
 *
 * \param busName The account's well-known bus name
 *                (sometimes called a "service name").
 * \param objectPath Account object path.
 */
Account::Account(const QString &busName, const QString &objectPath)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            busName, objectPath),
      OptionalInterfaceFactory<Account>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
}

/**
 * Construct a new Account object.
 *
 * \param bus QDBusConnection to use
 * \param busName The account's well-known bus name
 *                (sometimes called a "service name").
 * \param objectPath Account object path.
 */
Account::Account(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath)
    : StatelessDBusProxy(bus, busName, objectPath),
      OptionalInterfaceFactory<Account>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
Account::~Account()
{
    delete mPriv;
}

/**
 * Get whether this is a valid account.
 *
 * If true, this account is considered by the account manager to be complete
 * and usable. If false, user action is required to make it usable, and it will
 * never attempt to connect (for instance, this might be caused by the absence
 * of a required parameter).
 *
 * \return \c true if the account is valid, \c false otherwise.
 */
bool Account::isValidAccount() const
{
    return mPriv->valid;
}

/**
 * Get whether this account is enabled.
 *
 * Gives the users the possibility to prevent an account from
 * being used. This flag does not change the validity of the account.
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
 * Check whether this account has ever been put online successfully.
 *
 * This property cannot change from true to false, only from false to true.
 * When the account successfully goes online for the first time, or when it
 * is detected that this has already happened, the firstOnline() signal is
 * emitted.
 *
 * \return Whether the account has even been online.
 */
bool Account::hasBeenOnline() const
{
    return mPriv->hasBeenOnline;
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
const Avatar &Account::avatar() const
{
    if (!isReady(Features() << FeatureAvatar)) {
        warning() << "Trying to retrieve avatar from account, but "
                     "avatar is not supported or was not requested. "
                     "Use becomeReady(FeatureAvatar)";
    }

    return mPriv->avatar;
}

/**
 * Set this account avatar.
 *
 * \param avatar The avatar's MIME type and data.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::setAvatar(const Avatar &avatar)
{
    if (!mPriv->interfaces.contains(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR)) {
        return new PendingFailure(this, TELEPATHY_ERROR_NOT_IMPLEMENTED,
                "Account does not support Avatar");
    }

    return new PendingVoidMethodCall(this,
            propertiesInterface()->Set(
                TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,
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
 * On success, the pending operation returned by this method will produce a
 * list of strings, which are the names of parameters whose changes will not
 * take effect until the account is disconnected and reconnected (for instance
 * by calling Account::reconnect()).
 *
 * \param set Parameters to set.
 * \param unset Parameters to unset.
 * \return A PendingStringList which will emit PendingOperation::finished
 *         when the call has finished
 */
PendingStringList *Account::updateParameters(const QVariantMap &set,
        const QStringList &unset)
{
    return new PendingStringList(
            baseInterface()->UpdateParameters(set, unset),
            this);
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
    if (!isReady(Features() << FeatureProtocolInfo)) {
        warning() << "Trying to retrieve protocol info from account, but "
                     "protocol info is not supported or was not requested. "
                     "Use becomeReady(FeatureProtocolInfo)";
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
ConnectionStatus Account::connectionStatus() const
{
    return mPriv->connectionStatus;
}

/**
 * Get the connection status reason of this account.
 *
 * \return Value indication the status reason of this account conneciton.
 */
ConnectionStatusReason Account::connectionStatusReason() const
{
    return mPriv->connectionStatusReason;
}

/**
 * Return whether this account have a connection object that can be retrieved
 * using connection().
 *
 * \return \c true if a connection object can be retrieved, \c false otherwise
 */
bool Account::haveConnection() const
{
    return !mPriv->connectionObjectPath.isEmpty();
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
ConnectionPtr Account::connection() const
{
    if (mPriv->connectionObjectPath.isEmpty()) {
        return ConnectionPtr();
    }
    QString objectPath = mPriv->connectionObjectPath;
    QString busName = objectPath.mid(1).replace('/', '.');
    if (!mPriv->connection) {
        mPriv->connection = Connection::create(dbusConnection(),
                busName, objectPath);
    }
    return mPriv->connection;
}

/**
 * Set the presence status that this account should have if it is brought
 * online.
 *
 * \return Presence status that will be set when this account is put online.
 */
SimplePresence Account::automaticPresence() const
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
        const SimplePresence &value)
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
SimplePresence Account::currentPresence() const
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
SimplePresence Account::requestedPresence() const
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
        const SimplePresence &value)
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
 * If this account is currently connected, disconnect and reconnect it. If it
 * is currently trying to connect, cancel the attempt to connect and start
 * another. If it is currently disconnected, do nothing.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::reconnect()
{
    return new PendingVoidMethodCall(this, baseInterface()->Reconnect());
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

PendingChannelRequest *Account::ensureTextChat(
        const QString &contactIdentifier,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, false, this);
}

PendingChannelRequest *Account::ensureTextChat(
        const ContactPtr &contact,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : 0);
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, false, this);
}

PendingChannelRequest *Account::ensureTextChatroom(
        const QString &roomName,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeRoom);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   roomName);
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, false, this);
}

PendingChannelRequest *Account::ensureMediaCall(
        const QString &contactIdentifier,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, false, this);
}

PendingChannelRequest *Account::ensureMediaCall(
        const ContactPtr &contact,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : 0);
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, false, this);
}

// advanced
PendingChannelRequest *Account::createChannel(
        const QVariantMap &request,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, true, this);
}

PendingChannelRequest *Account::ensureChannel(
        const QVariantMap &request,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    return new PendingChannelRequest(dbusConnection(), objectPath(),
            request, userActionTime, preferredHandler, false, this);
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
Client::AccountInterface *Account::baseInterface() const
{
    return mPriv->baseInterface;
}

/**** Private ****/
void Account::Private::init()
{
    if (!parent->isValid()) {
        return;
    }

    parent->connect(baseInterface,
            SIGNAL(Removed()),
            SLOT(onRemoved()));
    parent->connect(baseInterface,
            SIGNAL(AccountPropertyChanged(const QVariantMap &)),
            SLOT(onPropertyChanged(const QVariantMap &)));
}

void Account::Private::introspectMain(Account::Private *self)
{
    Client::DBus::PropertiesInterface *properties = self->parent->propertiesInterface();
    Q_ASSERT(properties != 0);

    debug() << "Calling Properties::GetAll(Account)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(
                TELEPATHY_INTERFACE_ACCOUNT), self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher *)));
}

void Account::Private::introspectAvatar(Account::Private *self)
{
    debug() << "Calling GetAvatar(Account)";
    // we already checked if avatar interface exists, so bypass avatar interface
    // checking
    Client::AccountInterfaceAvatarInterface *iface =
        self->parent->avatarInterface(BypassInterfaceCheck);

    // If we are here it means the user cares about avatar, so
    // connect to avatar changed signal, so we update the avatar
    // when it changes.
    self->parent->connect(iface,
            SIGNAL(AvatarChanged()),
            SLOT(onAvatarChanged()));

    self->retrieveAvatar();
}

void Account::Private::introspectProtocolInfo(Account::Private *self)
{
    Q_ASSERT(!self->cm);

    self->cm = ConnectionManager::create(
            self->parent->dbusConnection(),
            self->cmName);
    self->parent->connect(self->cm->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onConnectionManagerReady(Tp::PendingOperation *)));
}

void Account::Private::updateProperties(const QVariantMap &props)
{
    debug() << "Account::updateProperties: changed:";

    if (props.contains("Interfaces")) {
        interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
        debug() << " Interfaces:" << interfaces;
    }

    if (props.contains("DisplayName") &&
        displayName != qdbus_cast<QString>(props["DisplayName"])) {
        displayName = qdbus_cast<QString>(props["DisplayName"]);
        debug() << " Display Name:" << displayName;
        emit parent->displayNameChanged(displayName);
    }

    if (props.contains("Icon") &&
        icon != qdbus_cast<QString>(props["Icon"])) {
        icon = qdbus_cast<QString>(props["Icon"]);
        debug() << " Icon:" << icon;
        emit parent->iconChanged(icon);
    }

    if (props.contains("Nickname") &&
        nickname != qdbus_cast<QString>(props["Nickname"])) {
        nickname = qdbus_cast<QString>(props["Nickname"]);
        debug() << " Nickname:" << nickname;
        emit parent->nicknameChanged(nickname);
    }

    if (props.contains("NormalizedName") &&
        normalizedName != qdbus_cast<QString>(props["NormalizedName"])) {
        normalizedName = qdbus_cast<QString>(props["NormalizedName"]);
        debug() << " Normalized Name:" << normalizedName;
        emit parent->normalizedNameChanged(normalizedName);
    }

    if (props.contains("Valid") &&
        valid != qdbus_cast<bool>(props["Valid"])) {
        valid = qdbus_cast<bool>(props["Valid"]);
        debug() << " Valid:" << (valid ? "true" : "false");
        emit parent->validityChanged(valid);
    }

    if (props.contains("Enabled") &&
        enabled != qdbus_cast<bool>(props["Enabled"])) {
        enabled = qdbus_cast<bool>(props["Enabled"]);
        debug() << " Enabled:" << (enabled ? "true" : "false");
        emit parent->stateChanged(enabled);
    }

    if (props.contains("ConnectAutomatically") &&
        connectsAutomatically !=
                qdbus_cast<bool>(props["ConnectAutomatically"])) {
        connectsAutomatically =
                qdbus_cast<bool>(props["ConnectAutomatically"]);
        debug() << " Connects Automatically:" << (connectsAutomatically ? "true" : "false");
        emit parent->connectsAutomaticallyPropertyChanged(connectsAutomatically);
    }

    if (props.contains("HasBeenOnline") &&
        !hasBeenOnline &&
        qdbus_cast<bool>(props["HasBeenOnline"])) {
        hasBeenOnline = true;
        debug() << " HasBeenOnline changed to true";
        // don't emit firstOnline unless we're already ready, that would be
        // misleading - we'd emit it just before any already-used account
        // became ready
        if (parent->isReady()) {
            emit parent->firstOnline();
        }
    }

    if (props.contains("Parameters") &&
        parameters != qdbus_cast<QVariantMap>(props["Parameters"])) {
        parameters = qdbus_cast<QVariantMap>(props["Parameters"]);
        debug() << " Parameters:" << parameters;
        emit parent->parametersChanged(parameters);
    }

    if (props.contains("AutomaticPresence") &&
        automaticPresence != qdbus_cast<SimplePresence>(
                props["AutomaticPresence"])) {
        automaticPresence = qdbus_cast<SimplePresence>(
                props["AutomaticPresence"]);
        debug() << " Automatic Presence:" << automaticPresence.type <<
            "-" << automaticPresence.status;
        emit parent->automaticPresenceChanged(automaticPresence);
    }

    if (props.contains("CurrentPresence") &&
        currentPresence != qdbus_cast<SimplePresence>(
                props["CurrentPresence"])) {
        currentPresence = qdbus_cast<SimplePresence>(
                props["CurrentPresence"]);
        debug() << " Current Presence:" << currentPresence.type <<
            "-" << currentPresence.status;
        emit parent->currentPresenceChanged(currentPresence);
    }

    if (props.contains("RequestedPresence") &&
        requestedPresence != qdbus_cast<SimplePresence>(
                props["RequestedPresence"])) {
        requestedPresence = qdbus_cast<SimplePresence>(
                props["RequestedPresence"]);
        debug() << " Requested Presence:" << requestedPresence.type <<
            "-" << requestedPresence.status;
        emit parent->requestedPresenceChanged(requestedPresence);
    }

    if (props.contains("Connection")) {
        QString path = qdbus_cast<QDBusObjectPath>(props["Connection"]).path();
        if (path.isEmpty()) {
            debug() << " The map contains \"Connection\" but it's empty as a QDBusObjectPath!";
            debug() << " Trying QString (known bug in some MC/dbus-glib versions)";
            path = qdbus_cast<QString>(props["Connection"]);
        }

        debug() << " Connection Object Path:" << path;
        if (path == QLatin1String("/")) {
            path = QString();
        }

        if (connectionObjectPath != path) {
            connection.reset();
            connectionObjectPath = path;
            emit parent->haveConnectionChanged(!path.isEmpty());
        }
    }

    if (props.contains("ConnectionStatus") || props.contains("ConnectionStatusReason")) {
        bool changed = false;

        if (props.contains("ConnectionStatus") &&
            connectionStatus != ConnectionStatus(
                    qdbus_cast<uint>(props["ConnectionStatus"]))) {
            connectionStatus = ConnectionStatus(
                    qdbus_cast<uint>(props["ConnectionStatus"]));
            debug() << " Connection Status:" << connectionStatus;
            changed = true;
        }

        if (props.contains("ConnectionStatusReason") &&
            connectionStatusReason != ConnectionStatusReason(
                    qdbus_cast<uint>(props["ConnectionStatusReason"]))) {
            connectionStatusReason = ConnectionStatusReason(
                    qdbus_cast<uint>(props["ConnectionStatusReason"]));
            debug() << " Connection StatusReason:" << connectionStatusReason;
            changed = true;
        }

        if (changed) {
            emit parent->connectionStatusChanged(
                    connectionStatus, connectionStatusReason);
        }
    }
}

void Account::Private::retrieveAvatar()
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            parent->propertiesInterface()->Get(
                TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR,
                "Avatar"), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotAvatar(QDBusPendingCallWatcher *)));
}

void Account::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Account)";
        mPriv->updateProperties(reply.value());

        mPriv->readinessHelper->setInterfaces(mPriv->interfaces);

        debug() << "Account basic functionality is ready";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());

        warning().nospace() <<
            "GetAll(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    watcher->deleteLater();
}

void Account::gotAvatar(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to GetAvatar(Account)";
        mPriv->avatar = qdbus_cast<Avatar>(reply);

        // first time
        if (!mPriv->readinessHelper->actualFeatures().contains(FeatureAvatar)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureAvatar, true);
        }

        emit avatarChanged(mPriv->avatar);
    } else {
        // check if the feature is already there, and for some reason retrieveAvatar
        // failed when called the second time
        if (!mPriv->readinessHelper->missingFeatures().contains(FeatureAvatar)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureAvatar, false, reply.error());
        }

        warning().nospace() <<
            "GetAvatar(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    watcher->deleteLater();
}

void Account::onAvatarChanged()
{
    debug() << "Avatar changed, retrieving it";
    mPriv->retrieveAvatar();
}

void Account::onConnectionManagerReady(PendingOperation *operation)
{
    bool error = operation->isError();
    if (!error) {
        foreach (ProtocolInfo *info, mPriv->cm->protocols()) {
            if (info->name() == mPriv->protocol) {
                mPriv->protocolInfo = info;
                break;
            }
        }

        error = (mPriv->protocolInfo == 0);
    }

    if (!error) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureProtocolInfo, true);
    }
    else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureProtocolInfo, false,
                operation->errorName(), operation->errorMessage());
    }
}

void Account::onPropertyChanged(const QVariantMap &delta)
{
    mPriv->updateProperties(delta);
}

void Account::onRemoved()
{
    mPriv->valid = false;
    mPriv->enabled = false;
    // This is the closest error we have at the moment
    invalidate(QLatin1String(TELEPATHY_ERROR_CANCELLED),
            QLatin1String("Account removed from AccountManager"));
}

} // Tp
