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

Account::Private::PendingReady::PendingReady(Account *parent)
    : PendingOperation(parent)
{
}

Account::Private::Private(Account *parent)
    : QObject(parent),
      baseInterface(new AccountInterface(parent->dbusConnection(),
                        parent->busName(), parent->objectPath(), parent)),
      ready(false),
      pendingReady(0),
      features(0),
      valid(false),
      enabled(false),
      connectsAutomatically(false),
      connectionStatus(Telepathy::ConnectionStatusDisconnected),
      connectionStatusReason(Telepathy::ConnectionStatusReasonNoneSpecified)
{
    // FIXME: QRegExp probably isn't the most efficient possible way to parse
    //        this :-)
    QRegExp rx("^" TELEPATHY_ACCOUNT_OBJECT_PATH_BASE
               "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
               "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
               "/" // account-specific part
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
            SIGNAL(AccountPropertyChanged(QVariantMap *)),
            SLOT(onPropertyChanged(QVariantMap *)));

    introspectQueue.enqueue(&Private::callGetAll);
    QTimer::singleShot(0, this, SLOT(continueIntrospection()));
}

Account::Private::~Private()
{
    delete baseInterface;
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

#if 0
void Account::Private::callGetAvatar()
{
}
#endif

void Account::Private::updateProperties(const QVariantMap &props)
{
    if (props.contains("Interfaces")) {
        interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
    }

    if (props.contains("DisplayName")) {
        displayName = qdbus_cast<QString>(props["DisplayName"]);
    }

    if (props.contains("Icon")) {
        icon = qdbus_cast<QString>(props["Icon"]);
    }

    if (props.contains("Nickname")) {
        nickname = qdbus_cast<QString>(props["Nickname"]);
    }

    if (props.contains("NormalizedName")) {
        normalizedName = qdbus_cast<QString>(props["NormalizedName"]);
    }

    if (props.contains("Valid")) {
        valid = qdbus_cast<bool>(props["Valid"]);
    }

    if (props.contains("Enabled")) {
        enabled = qdbus_cast<bool>(props["Enabled"]);
    }

    if (props.contains("ConnectAutomatically")) {
        connectsAutomatically =
                qdbus_cast<bool>(props["ConnectAutomatically"]);
    }

    if (props.contains("Parameters")) {
        parameters = qdbus_cast<QVariantMap>(props["Parameters"]);
    }

    if (props.contains("AutomaticPresence")) {
        automaticPresence = qdbus_cast<Telepathy::SimplePresence>(
                props["AutomaticPresence"]);
    }

    if (props.contains("CurrentPresence")) {
        currentPresence = qdbus_cast<Telepathy::SimplePresence>(
                props["CurrentPresence"]);
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

    if (props.contains("ConnectionStatus")) {
        connectionStatus = Telepathy::ConnectionStatus(
                qdbus_cast<uint>(props["ConnectionStatus"]));
    }

    if (props.contains("ConnectionStatusReason")) {
        connectionStatusReason = Telepathy::ConnectionStatusReason(
                qdbus_cast<uint>(props["ConnectionStatusReason"]));
    }
}

void Account::Private::onGetAllAccountReturn(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Account)";
        updateProperties(reply.value());
    } else {
        warning().nospace() <<
            "GetAll(Account) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }
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
}

void Account::Private::continueIntrospection()
{
    if (!ready) {
        if (introspectQueue.isEmpty()) {
            debug() << "Account is ready";
            ready = true;

            if (pendingReady) {
                pendingReady->setFinished();
                // it will delete itself later
                pendingReady = 0;
            }
        }
        else {
            (this->*(introspectQueue.dequeue()))();
        }
    }
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
    : StatelessDBusProxy(am->dbusConnection(), am->busName(), objectPath.path(),
            parent),
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
    return mPriv->avatarData;
}

QString Account::avatarMimeType() const
{
    return mPriv->avatarMimeType;
}

#if 0
PendingOperation *Account::setAvatar(const QByteArray &data,
        const QString &mimeType)
{
}
#endif

QVariantMap Account::parameters() const
{
    return mPriv->parameters;
}

#if 0
PendingOperation *Account::updateParameters(const QVariantMap &set,
        const QStringList &unset)
{
    return 0;
}

ProtocolInfo *Account::protocolInfo() const
{
    return 0;
}
#endif

Telepathy::SimplePresence Account::automaticPresence() const
{
    return mPriv->automaticPresence;
}

#if 0
PendingOperation *Account::setAutomaticPresence(
        const Telepathy::SimplePresence &value)
{
    return 0;
}
#endif

bool Account::connectsAutomatically() const
{
    return mPriv->connectsAutomatically;
}

#if 0
PendingOperation *Account::setConnectsAutomatically(bool value)
{
    return 0;
}
#endif

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

#if 0
PendingOperation *Account::setRequestedPresence(
        const Telepathy::SimplePresence &value)
{
    return 0;
}

QString Account::uniqueIdentifier() const
{
    QString result;
    // TODO
    return result;
}
#endif

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

    if (features != 0) {
        return new PendingFailure(this, "org.freedesktop.Telepathy.Qt.DoesntWork",
                "Unimplemented");
    }

    if (!mPriv->pendingReady) {
        mPriv->pendingReady = new Private::PendingReady(this);
    }
    return mPriv->pendingReady;
}

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
