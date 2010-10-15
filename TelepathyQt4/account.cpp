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

#include "TelepathyQt4/connection-internal.h"
#include "TelepathyQt4/future-internal.h"

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingChannelRequest>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingStringList>
#include <TelepathyQt4/PendingVoid>
#include <TelepathyQt4/Profile>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>

#include <QQueue>
#include <QRegExp>
#include <QTimer>

#include <string.h>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT Account::Private
{
    Private(Account *parent, const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory);
    ~Private();

    void init();

    static void introspectMain(Private *self);
    static void introspectAvatar(Private *self);
    static void introspectProtocolInfo(Private *self);
    static void introspectCapabilities(Private *self);

    void updateProperties(const QVariantMap &props);
    void retrieveAvatar();
    bool processConnQueue();

    bool checkCapabilitiesChanged(bool profileChanged);

    // TODO remove once Conference.DRAFT support is removed and simplify code that uses it
    bool useConferenceDRAFT(const char *channelType,
            uint targetHandleType) const;
    void addConferenceRequestCommonParameters(
            const char *channelType,
            uint targetHandleType,
            const char *conferenceIface,
            const QList<ChannelPtr> &channels,
            QVariantMap &request);
    void addConferenceRequestParameters(
            const char *channelType,
            uint targetHandleType,
            const QList<ChannelPtr> &channels,
            const QStringList &initialInviteeContactsIdentifiers,
            QVariantMap &request);
    void addConferenceRequestParameters(
            const char *channelType,
            uint targetHandleType,
            const QList<ChannelPtr> &channels,
            const QList<ContactPtr> &initialInviteeContacts,
            QVariantMap &request);

    // Public object
    Account *parent;

    // Factories
    ConnectionFactoryConstPtr connFactory;
    ChannelFactoryConstPtr chanFactory;
    ContactFactoryConstPtr contactFactory;

    // Instance of generated interface class
    Client::AccountInterface *baseInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    QVariantMap parameters;
    bool valid;
    bool enabled;
    bool connectsAutomatically;
    bool hasBeenOnline;
    bool changingPresence;
    QString cmName;
    QString protocolName;
    QString serviceName;
    ProfilePtr profile;
    QString displayName;
    QString nickname;
    QString iconName;
    QQueue<QString> connObjPathQueue;
    ConnectionPtr connection;
    bool mayFinishCore, coreFinished;
    QString normalizedName;
    Avatar avatar;
    ConnectionManagerPtr cm;
    ProtocolInfo *protocolInfo;
    ConnectionStatus connectionStatus;
    ConnectionStatusReason connectionStatusReason;
    QString connectionError;
    QVariantMap connectionErrorDetails;
    SimplePresence automaticPresence;
    SimplePresence currentPresence;
    SimplePresence requestedPresence;
    bool usingConnectionCaps;
    ConnectionCapabilities *customCaps;
};

Account::Private::Private(Account *parent, const ConnectionFactoryConstPtr &connFactory,
        const ChannelFactoryConstPtr &chanFactory, const ContactFactoryConstPtr &contactFactory)
    : parent(parent),
      connFactory(connFactory),
      chanFactory(chanFactory),
      contactFactory(contactFactory),
      baseInterface(new Client::AccountInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent)),
      readinessHelper(parent->readinessHelper()),
      valid(false),
      enabled(false),
      connectsAutomatically(false),
      hasBeenOnline(false),
      changingPresence(false),
      mayFinishCore(false),
      coreFinished(false),
      protocolInfo(0),
      connectionStatus(ConnectionStatusDisconnected),
      connectionStatusReason(ConnectionStatusReasonNoneSpecified),
      usingConnectionCaps(false),
      customCaps(0)
{
    automaticPresence.type = currentPresence.type = requestedPresence.type
        = ConnectionPresenceTypeUnknown;
    automaticPresence.status = currentPresence.status = requestedPresence.status
        = QLatin1String("unknown");

    // FIXME: QRegExp probably isn't the most efficient possible way to parse
    //        this :-)
    QRegExp rx(QLatin1String("^" TELEPATHY_ACCOUNT_OBJECT_PATH_BASE
                "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
                "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
                "/([_A-Za-z][_A-Za-z0-9]*)"  // account-specific part
                ));

    if (rx.exactMatch(parent->objectPath())) {
        cmName = rx.cap(1);
        protocolName = rx.cap(2);
    } else {
        warning() << "Account object path is not spec-compliant, "
            "trying again with a different account-specific part check";

        rx = QRegExp(QLatin1String("^" TELEPATHY_ACCOUNT_OBJECT_PATH_BASE
                    "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(1) is the CM
                    "/([_A-Za-z][_A-Za-z0-9]*)"  // cap(2) is the protocol
                    "/([_A-Za-z0-9]*)"  // account-specific part
                    ));
        if (rx.exactMatch(parent->objectPath())) {
            cmName = rx.cap(1);
            protocolName = rx.cap(2);
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
        QSet<uint>() << 0,                                                            // makesSenseForStatuses
        Features() << FeatureCore,                                                    // dependsOnFeatures (core)
        QStringList() << QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR), // dependsOnInterfaces
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

    ReadinessHelper::Introspectable introspectableCapabilities(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << FeatureCore << FeatureProtocolInfo << FeatureProfile,     // dependsOnFeatures
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectCapabilities,
        this);
    introspectables[FeatureCapabilities] = introspectableCapabilities;

    readinessHelper->addIntrospectables(introspectables);

    if (connFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the conn factory is not the proxy connection for"
            << parent->objectPath();
    }

    if (chanFactory->dbusConnection().name() != parent->dbusConnection().name()) {
        warning() << "  The D-Bus connection in the channel factory is not the proxy connection for"
            << parent->objectPath();
    }

    init();
}

Account::Private::~Private()
{
    delete customCaps;
}

bool Account::Private::checkCapabilitiesChanged(bool profileChanged)
{
    /* when the capabilities changed:
     *
     * - We were using the connection caps and now we don't have connection or
     *   the connection we have is not connected (changed to CM caps)
     * - We were using the CM caps and now we have a connected connection
     *   (changed to new connection caps)
     */
    bool changed = false;

    if (usingConnectionCaps &&
        (!parent->haveConnection() ||
         connection->status() != Connection::StatusConnected)) {
        usingConnectionCaps = false;
        changed = true;
    } else if (!usingConnectionCaps &&
        parent->haveConnection() &&
        connection->status() == Connection::StatusConnected) {
        usingConnectionCaps = true;
        changed = true;
    } else if (!usingConnectionCaps && profileChanged) {
        changed = true;
    }

    if (changed && parent->isReady(FeatureCapabilities)) {
        emit parent->capabilitiesChanged(parent->capabilities());
    }

    return changed;
}

bool Account::Private::useConferenceDRAFT(const char *channelType,
        uint targetHandleType) const
{
    // default to Conference
    ConnectionCapabilities *caps = parent->capabilities();
    if (!caps) {
        return false;
    }

    RequestableChannelClassList rccs = caps->requestableChannelClasses();
    QString rccChannelType;
    uint rccTargetHandleType;
    foreach (const RequestableChannelClass &rcc, rccs) {
        rccChannelType = qdbus_cast<QString>(rcc.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        if (rccChannelType == QLatin1String(channelType)) {
            if (targetHandleType != HandleTypeNone) {
                rccTargetHandleType = qdbus_cast<uint>(rcc.fixedProperties.value(
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
                if (rccTargetHandleType != targetHandleType) {
                    continue;
                }
            }

            if (rcc.allowedProperties.contains(QLatin1String(
                            TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels"))) {
                return false;
            }
            if (rcc.allowedProperties.contains(QLatin1String(
                            TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE ".InitialChannels"))) {
                return true;
            }
        }
    }
    return false;
}

void Account::Private::addConferenceRequestCommonParameters(
        const char *channelType,
        uint targetHandleType,
        const char *conferenceIface,
        const QList<ChannelPtr> &channels,
        QVariantMap &request)
{
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(channelType));
    if (targetHandleType != HandleTypeNone) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                       targetHandleType);
    }

    ObjectPathList objectPaths;
    foreach (const ChannelPtr &channel, channels) {
        objectPaths << QDBusObjectPath(channel->objectPath());
    }

    request.insert(QString(QLatin1String("%1.InitialChannels"))
                    .arg(QLatin1String(conferenceIface)),
                qVariantFromValue(objectPaths));
}

void Account::Private::addConferenceRequestParameters(
        const char *channelType,
        uint targetHandleType,
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        QVariantMap &request)
{
    const char *conferenceIface;
    if (!useConferenceDRAFT(channelType, targetHandleType)) {
        conferenceIface = TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE;
    } else {
        conferenceIface = TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE;
    }
    addConferenceRequestCommonParameters(channelType, targetHandleType,
            conferenceIface, channels, request);

    if (!initialInviteeContactsIdentifiers.isEmpty()) {
        request.insert(QString(QLatin1String("%1.InitialInviteeIDs"))
                    .arg(QLatin1String(conferenceIface)),
                initialInviteeContactsIdentifiers);
    }
}

void Account::Private::addConferenceRequestParameters(
        const char *channelType,
        uint targetHandleType,
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        QVariantMap &request)
{
    const char *conferenceIface;
    if (!useConferenceDRAFT(channelType, targetHandleType)) {
        conferenceIface = TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CONFERENCE;
    } else {
        conferenceIface = TP_FUTURE_INTERFACE_CHANNEL_INTERFACE_CONFERENCE;
    }
    addConferenceRequestCommonParameters(channelType, targetHandleType,
            conferenceIface, channels, request);

    if (!initialInviteeContacts.isEmpty()) {
        UIntList handles;
        foreach (const ContactPtr &contact, initialInviteeContacts) {
            if (!contact) {
                continue;
            }
            handles << contact->handle()[0];
        }
        if (!handles.isEmpty()) {
            request.insert(QString(QLatin1String("%1.InitialInviteeHandles"))
                        .arg(QLatin1String(conferenceIface)),
                    qVariantFromValue(handles));
        }
    }
}

/**
 * \class Account
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/account.h <TelepathyQt4/Account>
 *
 * \brief The Account class provides an object representing a Telepathy account.
 *
 * Account adds the following features compared to using
 * Client::AccountManagerInterface directly:
 * <ul>
 *  <li>Status tracking</li>
 *  <li>Getting the list of supported interfaces automatically</li>
 * </ul>
 *
 * The remote object accessor functions on this object (isValidAccount(),
 * isEnabled(), and so on) don't make any D-Bus calls; instead, they return/use
 * values cached from a previous introspection run. The introspection process
 * populates their values in the most efficient way possible based on what the
 * service implements. Their return value is mostly undefined until the
 * introspection process is completed, i.e. isReady() returns true. See the
 * individual accessor descriptions for more details.
 *
 * Signals are emitted to indicate that properties have changed, for example
 * displayNameChanged(), iconNameChanged(), etc.
 *
 * Convenience methods to create channels using the channel dispatcher such as
 * ensureTextChat(), createFileTransfer() are provided.
 *
 * To avoid unnecessary D-Bus traffic, some methods only return valid
 * information after a specific feature has been enabled by calling
 * becomeReady() with the desired set of features as an argument, and waiting
 * for the resulting PendingOperation to finish. For instance, to retrieve the
 * account protocol information, it is necessary to call becomeReady() with
 * Account::FeatureProtocolInfo included in the argument.
 * The required features are documented by each method.
 *
 * If the account is deleted from the AccountManager, this object
 * will not be deleted automatically; however, it will emit invalidated()
 * with error code #TELEPATHY_QT4_ERROR_OBJECT_REMOVED and will cease to
 * be useful.
 *
 * \section account_usage_sec Usage
 *
 * \subsection account_create_sec Creating an account object
 *
 * The easiest way to create account objects is through AccountManager. One can
 * just use the AccountManager convenience methods such as
 * AccountManager::validAccounts() to get a list of account objects representing
 * valid accounts.
 *
 * If you already know the object path, you can just call create().
 * For example:
 *
 * \code AccountPtr acc = Account::create(busName, objectPath); \endcode
 *
 * An AccountPtr object is returned, which will automatically keep
 * track of object lifetime.
 *
 * You can also provide a D-Bus connection as a QDBusConnection:
 *
 * \code
 *
 * AccountPtr acc = Account::create(QDBusConnection::sessionBus(),
 *         busName, objectPath);
 *
 * \endcode
 *
 * \subsection account_ready_sec Making account ready to use
 *
 * An Account object needs to become ready before usage, meaning that the
 * introspection process finished and the object accessors can be used.
 *
 * To make the object ready, use becomeReady() and wait for the
 * PendingOperation::finished() signal to be emitted.
 *
 * \code
 *
 * class MyClass : public QObject
 * {
 *     QOBJECT
 *
 * public:
 *     MyClass(QObject *parent = 0);
 *     ~MyClass() { }
 *
 * private Q_SLOTS:
 *     void onAccountReady(Tp::PendingOperation*);
 *
 * private:
 *     AccountPtr acc;
 * };
 *
 * MyClass::MyClass(const QString &busName, const QString &objectPath,
 *         QObject *parent)
 *     : QObject(parent)
 *       acc(Account::create(busName, objectPath))
 * {
 *     connect(acc->becomeReady(),
 *             SIGNAL(finished(Tp::PendingOperation*)),
 *             SLOT(onAccountReady(Tp::PendingOperation*)));
 * }
 *
 * void MyClass::onAccountReady(Tp::PendingOperation *op)
 * {
 *     if (op->isError()) {
 *         qWarning() << "Account cannot become ready:" <<
 *             op->errorName() << "-" << op->errorMessage();
 *         return;
 *     }
 *
 *     // Account is now ready
 *     qDebug() << "Display name:" << acc->displayName();
 * }
 *
 * \endcode
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the Account
 * object usable.
 *
 * Note that this feature must be enabled in order to use most Account methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature Account::FeatureCore = Feature(QLatin1String(Account::staticMetaObject.className()), 0, true);

/**
 * Feature used in order to access account avatar info.
 *
 * See avatar specific methods' documentation for more details.
 */
const Feature Account::FeatureAvatar = Feature(QLatin1String(Account::staticMetaObject.className()), 1);

/**
 * Feature used in order to access account protocol info.
 *
 * See protocol info specific methods' documentation for more details.
 */
const Feature Account::FeatureProtocolInfo = Feature(QLatin1String(Account::staticMetaObject.className()), 2);

/**
 * Feature used in order to access account capabilities.
 *
 * This feature will enable FeatureProtocolInfo and FeatureProfile.
 *
 * See capabilities specific methods' documentation for more details.
 */
const Feature Account::FeatureCapabilities = Feature(QLatin1String(Account::staticMetaObject.className()), 3);

/**
 * Feature used in order to access account profile info.
 *
 * See profile specific methods' documentation for more details.
 */
const Feature Account::FeatureProfile = FeatureProtocolInfo;
// FeatureProfile is the same as FeatureProtocolInfo for now, as it only needs
// the protocol info, cm name and protocol name to build a fake profile. Make it
// a full-featured feature if needed later.

/**
 * Create a new Account object using QDBusConnection::sessionBus().
 *
 * The instance will use a connection factory creating Tp::Connection objects with no features
 * ready, and a channel factory creating stock Telepathy-Qt4 channel subclasses, as appropriate,
 * with no features ready.
 *
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \return An AccountPtr object pointing to the newly created Account object.
 */
AccountPtr Account::create(const QString &busName,
        const QString &objectPath)
{
    return AccountPtr(new Account(busName, objectPath));
}

/**
 * Create a new Account object using the given \a bus.
 *
 * The instance will use a connection factory creating Tp::Connection objects with no features
 * ready, and a channel factory creating stock Telepathy-Qt4 channel subclasses, as appropriate,
 * with no features ready.
 *
 * \param bus QDBusConnection to use.
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \return An AccountPtr object pointing to the newly created Account object.
 */
AccountPtr Account::create(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath)
{
    return AccountPtr(new Account(bus, busName, objectPath));
}

/**
 * Create a new Account object using QDBusConnection::sessionBus() and the given factories.
 *
 * A warning is printed if the factories are not for QDBusConnection::sessionBus().
 *
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return An AccountPtr object pointing to the newly created Account object.
 */
AccountPtr Account::create(const QString &busName, const QString &objectPath,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return AccountPtr(new Account(QDBusConnection::sessionBus(), busName, objectPath,
                connectionFactory, channelFactory, contactFactory));
}

/**
 * Create a new Account object using the given \a bus and the given factories.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 * \return An AccountPtr object pointing to the newly created Account object.
 */
AccountPtr Account::create(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
{
    return AccountPtr(new Account(bus, busName, objectPath, connectionFactory, channelFactory,
                contactFactory));
}

/**
 * Construct a new Account object using QDBusConnection::sessionBus().
 *
 * The instance will use a connection factory creating Tp::Connection objects with no features
 * ready, and a channel factory creating stock Telepathy-Qt4 channel subclasses, as appropriate,
 * with no features ready.
 *
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 */
Account::Account(const QString &busName, const QString &objectPath)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            busName, objectPath),
      OptionalInterfaceFactory<Account>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this,
                  ConnectionFactory::create(QDBusConnection::sessionBus()),
                  ChannelFactory::create(QDBusConnection::sessionBus()),
                  ContactFactory::create()))
{
}

/**
 * Construct a new Account object using the given \a bus.
 *
 * The instance will use a connection factory creating Tp::Connection objects with no features
 * ready, and a channel factory creating stock Telepathy-Qt4 channel subclasses, as appropriate,
 * with no features ready.
 *
 * \param bus QDBusConnection to use.
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 */
Account::Account(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath)
    : StatelessDBusProxy(bus, busName, objectPath),
      OptionalInterfaceFactory<Account>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, ConnectionFactory::create(bus),
                  ChannelFactory::create(bus),
                  ContactFactory::create()))
{
}

/**
 * Construct a new Account object using the given \a bus and the given factories.
 *
 * A warning is printed if the factories are not for \a bus.
 *
 * \param bus QDBusConnection to use.
 * \param busName The account well-known bus name (sometimes called a "service
 *                name"). This is usually the same as the account manager
 *                bus name #TELEPATHY_ACCOUNT_MANAGER_BUS_NAME.
 * \param objectPath The account object path.
 * \param connectionFactory The connection factory to use.
 * \param channelFactory The channel factory to use.
 * \param contactFactory The contact factory to use.
 */
Account::Account(const QDBusConnection &bus,
        const QString &busName, const QString &objectPath,
        const ConnectionFactoryConstPtr &connectionFactory,
        const ChannelFactoryConstPtr &channelFactory,
        const ContactFactoryConstPtr &contactFactory)
    : StatelessDBusProxy(bus, busName, objectPath),
      OptionalInterfaceFactory<Account>(this),
      ReadyObject(this, FeatureCore),
      mPriv(new Private(this, connectionFactory, channelFactory, contactFactory))
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
 * Get the connection factory used by this account.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ConnectionFactoryConstPtr Account::connectionFactory() const
{
    return mPriv->connFactory;
}

/**
 * Get the channel factory used by this account.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ChannelFactoryConstPtr Account::channelFactory() const
{
    return mPriv->chanFactory;
}

/**
 * Get the contact factory used by this account.
 *
 * Only read access is provided. This allows constructing object instances and examining the object
 * construction settings, but not changing settings. Allowing changes would lead to tricky
 * situations where objects constructed at different times by the account would have unpredictably
 * different construction settings (eg. subclass).
 *
 * \return Read-only pointer to the factory.
 */
ContactFactoryConstPtr Account::contactFactory() const
{
    return mPriv->contactFactory;
}

/**
 * Return whether this is a valid account.
 *
 * If true, this account is considered by the account manager to be complete
 * and usable. If false, user action is required to make it usable, and it will
 * never attempt to connect (for instance, this might be caused by the absence
 * of a required parameter).
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return \c true if the account is valid, \c false otherwise.
 * \sa validityChanged()
 */
bool Account::isValidAccount() const
{
    return mPriv->valid;
}

/**
 * Return whether this account is enabled.
 *
 * Gives the users the possibility to prevent an account from
 * being used. This flag does not change the validity of the account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return \c true if the account is enabled, \c false otherwise.
 * \sa stateChanged()
 */
bool Account::isEnabled() const
{
    return mPriv->enabled;
}

/**
 * Set whether this account should be enabled or disabled.
 *
 * \param value Whether this account should be enabled or disabled.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa stateChanged()
 */
PendingOperation *Account::setEnabled(bool value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("Enabled"),
                QDBusVariant(value)),
            this);
}

/**
 * Return the connection manager name of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The connection manager name of this account.
 */
QString Account::cmName() const
{
    return mPriv->cmName;
}

/**
 * Return the protocol name of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The protocol name of this account.
 */
QString Account::protocol() const
{
    return mPriv->protocolName;
}

/**
 * Return the protocol name of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The protocol name of this account.
 */
QString Account::protocolName() const
{
    return mPriv->protocolName;
}

/**
 * Return the service name of this account.
 *
 * Note that this method will fallback to protocolName() if service name
 * is not known.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The service name of this account.
 * \sa serviceNameChanged(), protocolName()
 */
QString Account::serviceName() const
{
    if (mPriv->serviceName.isEmpty()) {
        return mPriv->protocolName;
    }
    return mPriv->serviceName;
}

/**
 * Set the service name of this account.
 *
 * \param value The service name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa serviceNameChanged()
 */
PendingOperation *Account::setServiceName(const QString &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("Service"),
                QDBusVariant(value)),
            this);
}

/**
 * Return the profile used for this account.
 *
 * Note that if a profile for serviceName() is not available, a fake profile
 * (Profile::isFake() will return \c true) will be returned in case protocolInfo() returns non-NULL.
 *
 * The fake profile will contain the following info:
 *  - Profile::type() will return "IM"
 *  - Profile::provider() will return an empty string
 *  - Profile::serviceName() will return cmName()-serviceName()
 *  - Profile::name() and Profile::protocolName() will return protocolName()
 *  - Profile::iconName() will return "im-protocolName()"
 *  - Profile::cmName() will return cmName()
 *  - Profile::parameters() will return a list matching CM default parameters for protocol with name
 *    protocolName()
 *  - Profile::presences() will return an empty list and
 *    Profile::allowOtherPresences() will return \c true, meaning that CM
 *    presences should be used
 *  - Profile::unsupportedChannelClasses() will return an empty list
 *
 * This method requires Account::FeatureProfile to be enabled.
 *
 * \return The profile for this account.
 * \sa profileChanged()
 */
ProfilePtr Account::profile() const
{
    if (!isReady(FeatureProfile)) {
        return ProfilePtr();
    }

    if (!mPriv->profile) {
        mPriv->profile = Profile::createForServiceName(serviceName());
        if (!mPriv->profile->isValid()) {
            if (mPriv->protocolInfo) {
                mPriv->profile = ProfilePtr(new Profile(
                            QString(QLatin1String("%1-%2")).arg(mPriv->cmName).arg(serviceName()),
                            mPriv->cmName,
                            mPriv->protocolName,
                            mPriv->protocolInfo));
            } else {
                warning() << "Cannot create profile as neither a .profile is installed for service" <<
                    serviceName() << "nor protocol info can be retrieved";
            }
        }
    }
    return mPriv->profile;
}

/**
 * Return the display name of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The display name of this account.
 * \sa displayNameChanged()
 */
QString Account::displayName() const
{
    return mPriv->displayName;
}

/**
 * Set the display name of this account.
 *
 * \param value The display name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa displayNameChanged()
 */
PendingOperation *Account::setDisplayName(const QString &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("DisplayName"),
                QDBusVariant(value)),
            this);
}

/**
 * Return the icon name of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The icon name of this account.
 * \sa iconChanged()
 */
QString Account::icon() const
{
    return iconName();
}

/**
 * Return the icon name of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * If the account has no icon, and Account::FeatureProfile is enabled, the icon from the result of
 * profile() will be used.
 *
 * If neither the account nor the profile has an icon, and Account::FeatureProtocolInfo is
 * enabled, the icon from protocolInfo() will be used if set.
 *
 * As a last resort, "im-" + protocolName() will be returned.
 *
 * This matches the fallbacks recommended by the Telepathy specification.
 *
 * \return The icon name of this account.
 * \sa iconNameChanged()
 */
QString Account::iconName() const
{
    if (mPriv->iconName.isEmpty()) {
        if (isReady(FeatureProfile) && !profile().isNull()) {
            QString iconName = profile()->iconName();
            if (!iconName.isEmpty()) {
                return iconName;
            }
        }

        if (isReady(FeatureProtocolInfo) && protocolInfo() != NULL) {
            return protocolInfo()->iconName();
        }

        return QString(QLatin1String("im-%1")).arg(protocolName());
    }

    return mPriv->iconName;
}

/**
 * Set the icon name of this account.
 *
 * \param value The icon name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa iconChanged()
 */
PendingOperation *Account::setIcon(const QString &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("Icon"),
                QDBusVariant(value)),
            this);
}

/**
 * Set the icon name of this account.
 *
 * \param value The icon name of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa iconNameChanged()
 */
PendingOperation *Account::setIconName(const QString &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("Icon"),
                QDBusVariant(value)),
            this);
}

/**
 * Return the nickname of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The nickname of this account.
 * \sa nicknameChanged()
 */
QString Account::nickname() const
{
    return mPriv->nickname;
}

/**
 * Set the nickname of this account.
 *
 * \param value The nickname of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa nicknameChanged()
 */
PendingOperation *Account::setNickname(const QString &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("Nickname"),
                QDBusVariant(value)),
            this);
}

/**
 * Return the avatar of this account.
 *
 * This method requires Account::FeatureAvatar to be enabled.
 *
 * \return The avatar of this account.
 * \sa avatarChanged()
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
 * Set avatar of this account.
 *
 * \param avatar The avatar of this account.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa avatarChanged()
 */
PendingOperation *Account::setAvatar(const Avatar &avatar)
{
    if (!interfaces().contains(QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR))) {
        return new PendingFailure(
                QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Account does not support Avatar"), this);
    }

    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR),
                QLatin1String("Avatar"),
                QDBusVariant(QVariant::fromValue(avatar))),
            this);
}

/**
 * Return the parameters of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The parameters of this account.
 * \sa parametersChanged()
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
 * by calling reconnect()).
 *
 * \param set Parameters to set.
 * \param unset Parameters to unset.
 * \return A PendingStringList which will emit PendingStringList::finished
 *         when the call has finished
 * \sa parametersChanged(), reconnect()
 */
PendingStringList *Account::updateParameters(const QVariantMap &set,
        const QStringList &unset)
{
    return new PendingStringList(
            baseInterface()->UpdateParameters(set, unset),
            this);
}

/**
 * Return the protocol info of this account protocol.
 *
 * This method requires Account::FeatureProtocolInfo to be enabled.
 *
 * The returned pointer points to an internal data structure, which should not be deleted by the
 * caller.
 *
 * \return The protocol info of this account protocol.
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
 * Return the capabilities for this account.
 *
 * This method requires Account::FeatureCapabilities to be enabled.
 *
 * Note that this method will return the connection() capabilities if the
 * account is online and ready. If the account is disconnected, it will fallback
 * to return the subtraction of the protocolInfo() capabilities and the profile unsupported
 * capabilities.
 *
 * \return The capabilities for this account or 0 if FeatureCapabilities is not
 *         ready or the capabilities are unknown (e.g. the connection is offline and protocolInfo()
 *         returns 0).
 */
ConnectionCapabilities *Account::capabilities() const
{
    if (!isReady(FeatureCapabilities)) {
        warning() << "Trying to retrieve capabilities from account, but "
                     "FeatureCapabilities was not requested. "
                     "Use becomeReady(FeatureCapabilities)";
        return 0;
    }

    // if the connection is online and ready use its caps
    if (mPriv->connection &&
        mPriv->connection->status() == Connection::StatusConnected) {
        return mPriv->connection->capabilities();
    }

    // if we are here it means FeatureProtocolInfo and FeatureProfile are ready, as
    // FeatureCapabilities depend on them, so let's use the subtraction of protocol info caps rccs
    // and profile unsupported rccs.
    //
    // However, if we failed to introspect the CM (eg. this is a test), then let's not try to use
    // the protocolInfo because it'll be NULL! Profile may also be NULL in case a .profile for
    // serviceName() is not present and protocolInfo is NULL.
    ProtocolInfo *pi = protocolInfo();
    if (!pi) {
        return NULL;
    }
    ProfilePtr pr = profile();
    if (!pr) {
        return pi->capabilities();
    }

    if (mPriv->customCaps) {
        return mPriv->customCaps;
    }

    RequestableChannelClassList piRccs = pi->capabilities()->requestableChannelClasses();
    RequestableChannelClassList prUnsuportedRccs = pr->unsupportedChannelClasses();
    RequestableChannelClassList rccs;
    bool unsupported;
    foreach (const RequestableChannelClass &piRcc, piRccs) {
        unsupported = false;
        foreach (const RequestableChannelClass &prUnsuportedRcc, prUnsuportedRccs) {
            if (piRcc.fixedProperties == prUnsuportedRcc.fixedProperties) {
                unsupported = true;
                break;
            }
        }
        if (!unsupported) {
            rccs.append(piRcc);
        }
    }
    mPriv->customCaps = new ConnectionCapabilities(rccs);
    return mPriv->customCaps;
}

/**
 * Return whether this account should be put online automatically whenever
 * possible.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return \c true if it should try to connect automatically, \c false
 *         otherwise.
 * \sa connectsAutomaticallyPropertyChanged()
 */
bool Account::connectsAutomatically() const
{
    return mPriv->connectsAutomatically;
}

/**
 * Set whether this account should be put online automatically whenever
 * possible.
 *
 * \param value Value indicating if this account should be put online whenever
 *              possible.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa connectsAutomaticallyPropertyChanged()
 */
PendingOperation *Account::setConnectsAutomatically(bool value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("ConnectAutomatically"),
                QDBusVariant(value)),
            this);
}

/**
 * Return whether this account has ever been put online successfully.
 *
 * This property cannot change from true to false, only from false to true.
 * When the account successfully goes online for the first time, or when it
 * is detected that this has already happened, the firstOnline() signal is
 * emitted.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return Whether the account has ever been online.
 */
bool Account::hasBeenOnline() const
{
    return mPriv->hasBeenOnline;
}

/**
 * Return the status of this account connection.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The status of this account connection.
 * \sa connectionStatusChanged()
 */
ConnectionStatus Account::connectionStatus() const
{
    return mPriv->connectionStatus;
}

/**
 * Return the status reason of this account connection.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The status reason of this account connection.
 * \sa connectionStatusChanged()
 */
ConnectionStatusReason Account::connectionStatusReason() const
{
    return mPriv->connectionStatusReason;
}

/**
 * Return the D-Bus error name for the last disconnection or connection failure,
 * (in particular, #TELEPATHY_ERROR_CANCELLED if it was disconnected by user
 * request), or an empty string if the account is connected.
 *
 * One can receive change notifications on this property by connecting
 * to the statusChanged() signal.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The D-Bus error name for the last disconnection or connection failure.
 * \sa connectionErrorDetails(), connectionStatus(), connectionStatusReason(), statusChanged()
 */
QString Account::connectionError() const
{
    return mPriv->connectionError;
}

/**
 * Return a map containing extensible error details related to
 * connectionError().
 *
 * The keys for this map are defined by
 * <a href="http://telepathy.freedesktop.org/spec/">the Telepathy D-Bus
 * Interface Specification</a>. They will typically include
 * <literal>debug-message</literal>, which is a debugging message in the C
 * locale.
 *
 * One can receive change notifications on this property by connecting
 * to the statusChanged() signal.
 *
 * Connection::ErrorDetails can be used to wrap the returned map for more convenient access. In a
 * future API/ABI incompatible version we'll change this method to return one.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return A map containing extensible error details related to
 *         connectionError().
 * \sa connectionError(), connectionStatus(), connectionStatusReason(), statusChanged(),
 * Connection::ErrorDetails.
 */
QVariantMap Account::connectionErrorDetails() const
{
    return mPriv->connectionErrorDetails;
}

/**
 * Return whether this account has a connection object that can be retrieved
 * using connection().
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return \c true if a connection object can be retrieved, \c false otherwise.
 * \sa connection(), haveConnectionChanged()
 */
bool Account::haveConnection() const
{
    return !mPriv->connection.isNull();
}

/**
 * Return the ConnectionPtr object of this account.
 *
 * Note that the returned ConnectionPtr object will not be cached by the Account
 * instance; applications should do it themselves.
 *
 * Remember to call Connection::becomeReady on the new connection to
 * make sure it is ready before using it.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return A ConnectionPtr object pointing to the Connection object of this
 *         account, or a null ConnectionPtr if this account does not currently
 *         have a connection or if an error occurred.
 * \sa haveConnection(), haveConnectionChanged()
 */
ConnectionPtr Account::connection() const
{
    return mPriv->connection;
}

/**
 * Return whether this account's connection is changing presence.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return Whether this account's connection is changing presence.
 * \sa changingPresence(), currentPresenceChanged(), setRequestedPresence()
 */
bool Account::isChangingPresence() const
{
    return mPriv->changingPresence;
}

/**
 * Return the presence status that this account will have set on it by the
 * account manager if it brings it online automatically.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The presence status that will be set by the account manager if this
 *         account is brought online automatically by it.
 * \sa automaticPresenceChanged()
 */
SimplePresence Account::automaticPresence() const
{
    return mPriv->automaticPresence;
}

/**
 * Set the presence status that this account should have if it is brought
 * online automatically by the account manager.
 *
 * \param value The presence status to set when this account is brought
 *        online automatically by the account manager.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa automaticPresenceChanged(), setRequestedPresence()
 */
PendingOperation *Account::setAutomaticPresence(
        const SimplePresence &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("AutomaticPresence"),
                QDBusVariant(QVariant::fromValue(value))),
            this);
}

/**
 * Return the actual presence of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The actual presence of this account.
 * \sa currentPresenceChanged(), setRequestedPresence(), requestedPresence(), automaticPresence()
 */
SimplePresence Account::currentPresence() const
{
    return mPriv->currentPresence;
}

/**
 * Return the requested presence of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The requested presence of this account.
 * \sa requestedPresenceChanged(), setRequestedPresence(), currentPresence(), automaticPresence()
 */
SimplePresence Account::requestedPresence() const
{
    return mPriv->requestedPresence;
}

/**
 * Set the requested presence.
 *
 * When requested presence is changed, the account manager should attempt to
 * manipulate the connection to make currentPresence() match requestedPresence()
 * as closely as possible.
 *
 * \param value The requested presence.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa requestedPresenceChanged(), currentPresence(), automaticPresence(), setAutomaticPresence()
 */
PendingOperation *Account::setRequestedPresence(
        const SimplePresence &value)
{
    return new PendingVoid(
            propertiesInterface()->Set(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                QLatin1String("RequestedPresence"),
                QDBusVariant(QVariant::fromValue(value))),
            this);
}

/**
 * Return whether this account is online.
 *
 * \return \c true if this account is online, otherwise \c false.
 */
bool Account::isOnline() const
{
    return mPriv->currentPresence.type != ConnectionPresenceTypeOffline;
}

/**
 * Return the unique identifier of this account.
 *
 * This identifier should be unique per AccountManager implementation,
 * i.e. at least per QDBusConnection.
 *
 * \return The unique identifier of this account.
 */
QString Account::uniqueIdentifier() const
{
    QString path = objectPath();
    return path.right(path.length() -
            strlen("/org/freedesktop/Telepathy/Account/"));
}

/**
 * Return the connection object path of this account.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return The connection object path of this account.
 * \sa haveConnectionChanged(), haveConnection(), connection()
 */
QString Account::connectionObjectPath() const
{
    return !mPriv->connection.isNull() ? mPriv->connection->objectPath() : QString();
}

/**
 * Return the normalized user ID of the local user of this account.
 *
 * It is unspecified whether this user ID is globally unique.
 *
 * As currently implemented, IRC user IDs are only unique within the same
 * IRCnet. On some saner protocols, the user ID includes a DNS name which
 * provides global uniqueness.
 *
 * If this value is not known yet (which will always be the case for accounts
 * that have never been online), it will be an empty string.
 *
 * It is possible that this value will change if the connection manager's
 * normalization algorithm changes.
 *
 * This method requires Account::FeatureCore to be enabled.
 *
 * \return Account normalized user ID of the local user.
 * \sa normalizedNameChanged()
 */
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
    return new PendingVoid(baseInterface()->Reconnect(), this);
}

/**
 * Delete this account.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *Account::remove()
{
    return new PendingVoid(baseInterface()->Remove(), this);
}

/**
 * Start a request to ensure that a text channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to chat with.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureTextChat(
        const QString &contactIdentifier,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a text channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to chat with.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureTextChat(
        const ContactPtr &contact,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a text chat room with the given
 * room name \a roomName exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param roomName The name of the chat room.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureTextChatroom(
        const QString &roomName,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeRoom);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   roomName);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a media channel with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureMediaCall(
        const QString &contactIdentifier,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a media channel with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * \param contact The contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureMediaCall(
        const ContactPtr &contact,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that an audio call with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureAudioCall(
        const QString &contactIdentifier,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitialAudio"),
                   true);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that an audio call with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contact The contact to call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureAudioCall(
        const ContactPtr &contact,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitialAudio"),
                   true);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a video call with the given
 * contact \a contactIdentifier exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contactIdentifier The identifier of the contact to call.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureVideoCall(
        const QString &contactIdentifier,
        bool withAudio,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitialVideo"),
                   true);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);

    if (withAudio) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitialAudio"),
                       true);
    }

    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a video call with the given
 * contact \a contact exists, creating it if necessary.
 *
 * See ensureChannel() for more details.
 *
 * This will only work on relatively modern connection managers,
 * like telepathy-gabble 0.9.0 or later.
 *
 * \param contact The contact to call.
 * \param withAudio true if both audio and video are required, false for a
 *                  video-only call.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::ensureVideoCall(
        const ContactPtr &contact,
        bool withAudio,
        QDateTime userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitialVideo"),
                   true);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);

    if (withAudio) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".InitialAudio"),
                       true);
    }

    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * Start a request to create a file transfer channel with the given
 * contact \a contact.
 *
 * \param contactIdentifier The identifier of the contact to send a file.
 * \param fileName The suggested filename for the receiver.
 * \param properties The desired properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createFileTransfer(
        const QString &contactIdentifier,
        const FileTransferChannelCreationProperties &properties,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   contactIdentifier);

    QFileInfo fileInfo(properties.suggestedFileName());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Filename"),
                   fileInfo.fileName());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentType"),
                   properties.contentType());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Size"),
                   properties.size());

    if (properties.hasContentHash()) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentHashType"),
                       (uint) properties.contentHashType());
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentHash"),
                       properties.contentHash());
    }

    if (properties.hasDescription()) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Description"),
                       properties.description());
    }

    if (properties.hasLastModificationTime()) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Date"),
                       (qulonglong) properties.lastModificationTime().toTime_t());
    }

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a file transfer channel with the given
 * contact \a contact.
 *
 * \param contact The contact to send a file.
 * \param properties The desired properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createFileTransfer(
        const ContactPtr &contact,
        const FileTransferChannelCreationProperties &properties,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact ? contact->handle().at(0) : (uint) 0);

    QFileInfo fileInfo(properties.suggestedFileName());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Filename"),
                   fileInfo.fileName());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentType"),
                   properties.contentType());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Size"),
                   properties.size());

    if (properties.hasContentHash()) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentHashType"),
                       (uint) properties.contentHashType());
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentHash"),
                       properties.contentHash());
    }

    if (properties.hasDescription()) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Description"),
                       properties.description());
    }

    if (properties.hasLastModificationTime()) {
        request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Date"),
                       (qulonglong) properties.lastModificationTime().toTime_t());
    }

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a conference media call with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceMediaCall(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    mPriv->addConferenceRequestParameters(
            TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            HandleTypeNone,
            channels, initialInviteeContactsIdentifiers, request);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a conference media call with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceMediaCall(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    // TODO may we use Channel.Type.StreamedMedia here or Channel.Type.Call
    //      should be used?
    mPriv->addConferenceRequestParameters(
            TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            HandleTypeNone,
            channels, initialInviteeContacts, request);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a conference text chat with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChat(
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    mPriv->addConferenceRequestParameters(
            TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT,
            HandleTypeNone,
            channels, initialInviteeContactsIdentifiers, request);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a conference text chat with the given
 * channels \a channels.
 *
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChat(
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    mPriv->addConferenceRequestParameters(
            TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT,
            HandleTypeNone,
            channels, initialInviteeContacts, request);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a conference text chat room with the given
 * channels \a channels and room name \a roomName.
 *
 * \param roomName The room name.
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          identifiers to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChatRoom(
        const QString &roomName,
        const QList<ChannelPtr> &channels,
        const QStringList &initialInviteeContactsIdentifiers,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   roomName);
    mPriv->addConferenceRequestParameters(
            TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT,
            HandleTypeRoom,
            channels, initialInviteeContactsIdentifiers, request);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a conference text chat room with the given
 * channels \a channels and room name \a roomName.
 *
 * \param roomName The room name.
 * \param channels The conference channels.
 * \param initialInviteeContactsIdentifiers A list of additional contacts
 *                                          to be invited to this
 *                                          conference when it is created.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa ensureChannel(), createChannel()
 */
PendingChannelRequest *Account::createConferenceTextChatRoom(
        const QString &roomName,
        const QList<ChannelPtr> &channels,
        const QList<ContactPtr> &initialInviteeContacts,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   roomName);
    mPriv->addConferenceRequestParameters(
            TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT,
            HandleTypeRoom,
            channels, initialInviteeContacts, request);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a contact search channel with the given
 * server \a server and limit \a limit.
 *
 * \param server For protocols which support searching for contacts on multiple servers with
 *               different DNS names (like XMPP), the DNS name of the server to be searched,
 *               e.g. "characters.shakespeare.lit". Otherwise, an empty string.
 * \param limit The desired maximum number of results that should be returned by a doing a search.
 *              If the protocol does not support specifying a limit for the number of results
 *              returned at a time, this will be ignored.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \return A PendingChannelRequest which will emit PendingChannelRequest::finished
 *         when the call has finished.
 * \sa createChannel()
 */
PendingChannelRequest *Account::createContactSearchChannel(
        const QString &server,
        uint limit,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH ".Server"),
                   server);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH ".Limit"), limit);

    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to create a channel.
 * This initially just creates a PendingChannelRequest object,
 * which can be used to track the success or failure of the request,
 * or to cancel it.
 *
 * Helper methods for text chat, text chat room, media call and conference are
 * provided and should be used if appropriate.
 *
 * \param request A dictionary containing desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \sa createChannel()
 */
PendingChannelRequest *Account::createChannel(
        const QVariantMap &request,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    return new PendingChannelRequest(request, userActionTime, preferredHandler, true,
            AccountPtr(this));
}

/**
 * Start a request to ensure that a channel exists, creating it if necessary.
 * This initially just creates a PendingChannelRequest object,
 * which can be used to track the success or failure of the request,
 * or to cancel it.
 *
 * Helper methods for text chat, text chat room, media call and conference are
 * provided and should be used if appropriate.
 *
 * \param request A dictionary containing desirable properties.
 * \param userActionTime The time at which user action occurred, or QDateTime()
 *                       if this channel request is for some reason not
 *                       involving user action.
 * \param preferredHandler Either the well-known bus name (starting with
 *                         org.freedesktop.Telepathy.Client.) of the preferred
 *                         handler for this channel, or an empty string to
 *                         indicate that any handler would be acceptable.
 * \sa createChannel()
 */
PendingChannelRequest *Account::ensureChannel(
        const QVariantMap &request,
        const QDateTime &userActionTime,
        const QString &preferredHandler)
{
    return new PendingChannelRequest(request, userActionTime, preferredHandler, false,
            AccountPtr(this));
}

/**
 * \fn void Account::serviceNameChanged(const QString &serviceName);
 *
 * This signal is emitted when the value of serviceName() of this account
 * changes.
 *
 * \param serviceName The new service name of this account.
 * \sa serviceName(), setServiceName()
 */

/**
 * \fn void Account::profileChanged(const Tp::ProfilePtr &profile);
 *
 * This signal is emitted when the value of profile() of this account
 * changes.
 *
 * \param profile The new profile of this account.
 * \sa profile()
 */

/**
 * \fn void Account::displayNameChanged(const QString &displayName);
 *
 * This signal is emitted when the value of displayName() of this account
 * changes.
 *
 * \param displayName The new display name of this account.
 * \sa displayName(), setDisplayName()
 */

/**
 * \fn void Account::iconChanged(const QString &icon);
 *
 * This signal is emitted when the value of icon() of this account changes.
 *
 * \param icon The new icon name of this account.
 * \sa icon(), setIcon()
 */

/**
 * \fn void Account::iconNameChanged(const QString &iconName);
 *
 * This signal is emitted when the value of iconName() of this account changes.
 *
 * \param iconName The new icon name of this account.
 * \sa iconName(), setIconName()
 */

/**
 * \fn void Account::nicknameChanged(const QString &nickname);
 *
 * This signal is emitted when the value of nickname() of this account changes.
 *
 * \param nickname The new nickname of this account.
 * \sa nickname(), setNickname()
 */

/**
 * \fn void Account::normalizedNameChanged(const QString &normalizedName);
 *
 * This signal is emitted when the value of normalizedName() of this account
 * changes.
 *
 * \param normalizedName The new normalized name of this account.
 * \sa normalizedName()
 */

/**
 * \fn void Account::validityChanged(bool validity);
 *
 * This signal is emitted when the value of isValidAccount() of this account
 * changes.
 *
 * \param validity The new validity of this account.
 * \sa isValidAccount()
 */

/**
 * \fn void Account::stateChanged(bool state);
 *
 * This signal is emitted when the value of isEnabled() of this account
 * changes.
 *
 * \param state The new state of this account.
 * \sa isEnabled()
 */

/**
 * \fn void Account::connectsAutomaticallyPropertyChanged(bool connectsAutomatically);
 *
 * This signal is emitted when the value of connectsAutomatically() of this
 * account changes.
 *
 * \param connectsAutomatically The new value of connects automatically property
 *                              of this account.
 * \sa isEnabled()
 */

/**
 * \fn void Account::firstOnline();
 *
 * This signal is emitted when this account is first put online.
 *
 * \sa hasBeenOnline()
 */

/**
 * \fn void Account::parametersChanged(const QVariantMap &parameters);
 *
 * This signal is emitted when the value of parameters() of this
 * account changes.
 *
 * \param parameters The new parameters of this account.
 * \sa parameters()
 */

/**
 * \fn void Account::changingPresence(bool value);
 *
 * This signal is emitted when the value of isChangingPresence() of this
 * account changes.
 *
 * \param value Whether this account's connection is changing presence.
 * \sa isChangingPresence()
 */

/**
 * \fn void Account::automaticPresenceChanged(const Tp::SimplePresence &automaticPresence) const;
 *
 * This signal is emitted when the value of automaticPresence() of this
 * account changes.
 *
 * \param automaticPresence The new value of automatic presence property of this
 *                          account.
 * \sa automaticPresence()
 */

/**
 * \fn void Account::currentPresenceChanged(const Tp::SimplePresence &currentPresence) const;
 *
 * This signal is emitted when the value of currentPresence() of this
 * account changes.
 *
 * \param currentPresence The new value of current presence property of this
 *                        account.
 * \sa currentPresence()
 */

/**
 * \fn void Account::requestedPresenceChanged(const Tp::SimplePresence &requestedPresence) const;
 *
 * This signal is emitted when the value of requestedPresence() of this
 * account changes.
 *
 * \param requestedPresence The new value of requested presence property of this
 *                          account.
 * \sa requestedPresence()
 */

/**
 * \fn void Account::onlinenessChanged(bool online) const;
 *
 * This signal is emitted when the value of isOnline() of this
 * account changes.
 *
 * \param online Whether this account is online.
 * \sa currentPresence()
 */

/**
 * \fn void Account::avatarChanged(const Tp::Avatar &avatar);
 *
 * This signal is emitted when the value of avatar() of this
 * account changes.
 *
 * \param avatar The new avatar of this account.
 * \sa avatar()
 */

/**
 * \fn void Account::connectionStatusChanged(Tp::ConnectionStatus status, Tp::ConnectionStatusReason statusReason);
 *
 * This signal is emitted when the value of connectionStatus() of this
 * account changes.
 *
 * \param status The new status of this account connection.
 * \param statusReason The new status reason of this account connection.
 * \sa connectionStatus(), connectionStatusReason()
 */

/**
 * \fn void Account::statusChanged(Tp::ConnectionStatus status,
 *                                 Tp::ConnectionStatusReason statusReason,
 *                                 const QString &errorName,
 *                                 const QVariantMap &errorDetails);
 *
 * This signal is emitted when the connection status of this account changes.
 *
 * Connection::ErrorDetails can be used to wrap the errorDetails map for more convenient access. In
 * a future API/ABI incompatible version we'll change this signal to already include one.
 *
 * \param status The new status of this account connection.
 * \param statusReason The new status reason of this account connection.
 * \param errorName The D-Bus error name for the last disconnection or
 *                   connection failure,
 * \param errorDetails A map containing extensible error details related to
 *                     errorName.
 * \sa connectionStatus(), connectionStatusReason(), connectionError(), connectionErrorDetails(),
 * Connection::ErrorDetails
 */

/**
 * \fn void Account::haveConnectionChanged(bool haveConnection);
 *
 * This signal is emitted when the value of haveConnection() of this
 * account changes.
 *
 * \param haveConnection Whether this account have a connection.
 * \sa haveConnection(), connection()
 */

/**
 * \fn Client::DBus::PropertiesInterface *Account::propertiesInterface() const
 *
 * Convenience function for getting a PropertiesInterface interface proxy object
 * for this account. The Account interface relies on properties, so this
 * interface is always assumed to be present.
 *
 * \return A pointer to the existing Client::DBus::PropertiesInterface object
 *         for this Account object.
 */

/**
 * \fn Client::AccountInterfaceAvatarInterface *Account::avatarInterface(
 *             InterfaceSupportedChecking check) const;
 *
 * Convenience function for getting a AvatarInterface interface proxy object for
 * this account.
 *
 * \return A pointer to the existing Client::AccountInterfaceAvatarInterface
 *         object for this Account object.
 */

/**
 * Return the Client::AccountInterface interface proxy object for this account.
 * This method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing Client::AccountInterface object for this
 *         Account object.
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
            SIGNAL(AccountPropertyChanged(QVariantMap)),
            SLOT(onPropertyChanged(QVariantMap)));
}

void Account::Private::introspectMain(Account::Private *self)
{
    Client::DBus::PropertiesInterface *properties = self->parent->propertiesInterface();
    Q_ASSERT(properties != 0);

    debug() << "Calling Properties::GetAll(Account)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT)), self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
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
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onConnectionManagerReady(Tp::PendingOperation*)));
}

void Account::Private::introspectCapabilities(Account::Private *self)
{
    if (!self->connection) {
        // there is no connection, just make capabilities ready
        self->readinessHelper->setIntrospectCompleted(FeatureCapabilities, true);
        return;
    }

    self->parent->connect(self->connection->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onConnectionReady(Tp::PendingOperation*)));
}

void Account::Private::updateProperties(const QVariantMap &props)
{
    debug() << "Account::updateProperties: changed:";

    if (props.contains(QLatin1String("Interfaces"))) {
        parent->setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));
        debug() << " Interfaces:" << parent->interfaces();
    }

    QString oldIconName = parent->iconName();
    bool serviceNameChanged = false;
    bool profileChanged = false;
    if (props.contains(QLatin1String("Service")) &&
        serviceName != qdbus_cast<QString>(props[QLatin1String("Service")])) {
        /* The service name, which means the profile changed, even if we are using the connection
         * caps, whenever the connection goes offline (if ever) we need to recompute a new caps for
         * the new profile */
        if (customCaps) {
            delete customCaps;
            customCaps = 0;
        }

        serviceNameChanged = true;
        serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
        debug() << " Service Name:" << parent->serviceName();
        /* use parent->serviceName() here as if the service name is empty we are going to use the
         * protocol name */
        emit parent->serviceNameChanged(parent->serviceName());
        parent->notify("serviceName");

        /* if we had a profile and the service changed, it means the profile also changed */
        if (parent->isReady(Account::FeatureProfile)) {
            /* service name changed, let's recreate profile */
            profileChanged = true;
            profile.reset();
            emit parent->profileChanged(parent->profile());
            parent->notify("profile");
        }
    }

    if (props.contains(QLatin1String("DisplayName")) &&
        displayName != qdbus_cast<QString>(props[QLatin1String("DisplayName")])) {
        displayName = qdbus_cast<QString>(props[QLatin1String("DisplayName")]);
        debug() << " Display Name:" << displayName;
        emit parent->displayNameChanged(displayName);
        parent->notify("displayName");
    }

    if ((props.contains(QLatin1String("Icon")) &&
         oldIconName != qdbus_cast<QString>(props[QLatin1String("Icon")])) ||
        serviceNameChanged) {

        if (props.contains(QLatin1String("Icon"))) {
            iconName = qdbus_cast<QString>(props[QLatin1String("Icon")]);
        }

        QString newIconName = parent->iconName();
        if (oldIconName != newIconName) {
            debug() << " Icon:" << newIconName;
            emit parent->iconChanged(newIconName);
            emit parent->iconNameChanged(newIconName);
            parent->notify("iconName");
        }
    }

    if (props.contains(QLatin1String("Nickname")) &&
        nickname != qdbus_cast<QString>(props[QLatin1String("Nickname")])) {
        nickname = qdbus_cast<QString>(props[QLatin1String("Nickname")]);
        debug() << " Nickname:" << nickname;
        emit parent->nicknameChanged(nickname);
        parent->notify("nickname");
    }

    if (props.contains(QLatin1String("NormalizedName")) &&
        normalizedName != qdbus_cast<QString>(props[QLatin1String("NormalizedName")])) {
        normalizedName = qdbus_cast<QString>(props[QLatin1String("NormalizedName")]);
        debug() << " Normalized Name:" << normalizedName;
        emit parent->normalizedNameChanged(normalizedName);
        parent->notify("normalizedName");
    }

    if (props.contains(QLatin1String("Valid")) &&
        valid != qdbus_cast<bool>(props[QLatin1String("Valid")])) {
        valid = qdbus_cast<bool>(props[QLatin1String("Valid")]);
        debug() << " Valid:" << (valid ? "true" : "false");
        emit parent->validityChanged(valid);
        parent->notify("valid");
    }

    if (props.contains(QLatin1String("Enabled")) &&
        enabled != qdbus_cast<bool>(props[QLatin1String("Enabled")])) {
        enabled = qdbus_cast<bool>(props[QLatin1String("Enabled")]);
        debug() << " Enabled:" << (enabled ? "true" : "false");
        emit parent->stateChanged(enabled);
        parent->notify("enabled");
    }

    if (props.contains(QLatin1String("ConnectAutomatically")) &&
        connectsAutomatically !=
                qdbus_cast<bool>(props[QLatin1String("ConnectAutomatically")])) {
        connectsAutomatically =
                qdbus_cast<bool>(props[QLatin1String("ConnectAutomatically")]);
        debug() << " Connects Automatically:" << (connectsAutomatically ? "true" : "false");
        emit parent->connectsAutomaticallyPropertyChanged(connectsAutomatically);
        parent->notify("connectsAutomatically");
    }

    if (props.contains(QLatin1String("HasBeenOnline")) &&
        !hasBeenOnline &&
        qdbus_cast<bool>(props[QLatin1String("HasBeenOnline")])) {
        hasBeenOnline = true;
        debug() << " HasBeenOnline changed to true";
        // don't emit firstOnline unless we're already ready, that would be
        // misleading - we'd emit it just before any already-used account
        // became ready
        if (parent->isReady()) {
            emit parent->firstOnline();
        }
        parent->notify("hasBeenOnline");
    }

    if (props.contains(QLatin1String("Parameters")) &&
        parameters != qdbus_cast<QVariantMap>(props[QLatin1String("Parameters")])) {
        parameters = qdbus_cast<QVariantMap>(props[QLatin1String("Parameters")]);
        debug() << " Parameters:" << parameters;
        emit parent->parametersChanged(parameters);
        parent->notify("parameters");
    }

    if (props.contains(QLatin1String("AutomaticPresence")) &&
        automaticPresence != qdbus_cast<SimplePresence>(
                props[QLatin1String("AutomaticPresence")])) {
        automaticPresence = qdbus_cast<SimplePresence>(
                props[QLatin1String("AutomaticPresence")]);
        debug() << " Automatic Presence:" << automaticPresence.type <<
            "-" << automaticPresence.status;
        emit parent->automaticPresenceChanged(automaticPresence);
        parent->notify("automaticPresence");
    }

    if (props.contains(QLatin1String("CurrentPresence")) &&
        currentPresence != qdbus_cast<SimplePresence>(
                props[QLatin1String("CurrentPresence")])) {
        currentPresence = qdbus_cast<SimplePresence>(
                props[QLatin1String("CurrentPresence")]);
        debug() << " Current Presence:" << currentPresence.type <<
            "-" << currentPresence.status;
        emit parent->currentPresenceChanged(currentPresence);
        parent->notify("currentPresence");
        emit parent->onlinenessChanged(parent->isOnline());
        parent->notify("online");
    }

    if (props.contains(QLatin1String("RequestedPresence")) &&
        requestedPresence != qdbus_cast<SimplePresence>(
                props[QLatin1String("RequestedPresence")])) {
        requestedPresence = qdbus_cast<SimplePresence>(
                props[QLatin1String("RequestedPresence")]);
        debug() << " Requested Presence:" << requestedPresence.type <<
            "-" << requestedPresence.status;
        emit parent->requestedPresenceChanged(requestedPresence);
        parent->notify("requestedPresence");
    }

    if (props.contains(QLatin1String("ChangingPresence")) &&
        changingPresence != qdbus_cast<bool>(
                props[QLatin1String("ChangingPresence")])) {
        changingPresence = qdbus_cast<bool>(
                props[QLatin1String("ChangingPresence")]);
        debug() << " Changing Presence:" << changingPresence;
        emit parent->changingPresence(changingPresence);
        parent->notify("changingPresence");
    }

    if (props.contains(QLatin1String("Connection"))) {
        QString path = qdbus_cast<QDBusObjectPath>(props[QLatin1String("Connection")]).path();
        if (path.isEmpty()) {
            debug() << " The map contains \"Connection\" but it's empty as a QDBusObjectPath!";
            debug() << " Trying QString (known bug in some MC/dbus-glib versions)";
            path = qdbus_cast<QString>(props[QLatin1String("Connection")]);
        }

        debug() << " Connection Object Path:" << path;
        if (path == QLatin1String("/")) {
            path = QString();
        }

        connObjPathQueue.enqueue(path);

        if (connObjPathQueue.size() == 1) {
            processConnQueue();
        }

        // onConnectionBuilt for a previous path will make sure the path we enqueued is processed if
        // the queue wasn't empty (so is now size() > 1)
    }

    bool connectionStatusChanged = false;
    if (props.contains(QLatin1String("ConnectionStatus")) ||
        props.contains(QLatin1String("ConnectionStatusReason")) ||
        props.contains(QLatin1String("ConnectionError")) ||
        props.contains(QLatin1String("ConnectionErrorDetails"))) {
        ConnectionStatus oldConnectionStatus = connectionStatus;

        if (props.contains(QLatin1String("ConnectionStatus")) &&
            connectionStatus != ConnectionStatus(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatus")]))) {
            connectionStatus = ConnectionStatus(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatus")]));
            debug() << " Connection Status:" << connectionStatus;
            connectionStatusChanged = true;
        }

        if (props.contains(QLatin1String("ConnectionStatusReason")) &&
            connectionStatusReason != ConnectionStatusReason(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatusReason")]))) {
            connectionStatusReason = ConnectionStatusReason(
                    qdbus_cast<uint>(props[QLatin1String("ConnectionStatusReason")]));
            debug() << " Connection StatusReason:" << connectionStatusReason;
            connectionStatusChanged = true;
        }

        /* FIXME (API-BREAK)
         * Remove signal connectionStatusChanged in favor of statusChanged
         * signal */
        if (connectionStatusChanged) {
            emit parent->connectionStatusChanged(
                    connectionStatus, connectionStatusReason);
            parent->notify("connectionStatus");
            parent->notify("connectionStatusReason");
        }

        if (props.contains(QLatin1String("ConnectionError")) &&
            connectionError != qdbus_cast<QString>(
                props[QLatin1String("ConnectionError")])) {
            connectionError = qdbus_cast<QString>(
                    props[QLatin1String("ConnectionError")]);
            debug() << " Connection Error:" << connectionError;
            connectionStatusChanged = true;
        }

        if (props.contains(QLatin1String("ConnectionErrorDetails")) &&
            connectionErrorDetails != qdbus_cast<QVariantMap>(
                props[QLatin1String("ConnectionErrorDetails")])) {
            connectionErrorDetails = qdbus_cast<QVariantMap>(
                    props[QLatin1String("ConnectionErrorDetails")]);
            debug() << " Connection Error Details:" << connectionErrorDetails;
            connectionStatusChanged = true;
        }

        if (connectionStatusChanged) {
            /* Something other than status changed, let's not emit statusChanged
             * and keep the error/errorDetails, for the next interaction.
             * It may happen if ConnectionError changes and in another property
             * change the status changes to Disconnected, so we use the error
             * previously signalled. If the status changes to something other
             * than Disconnected later, the error is cleared. */
            if (oldConnectionStatus != connectionStatus) {
                /* We don't signal error for status other than Disconnected */
                if (connectionStatus != ConnectionStatusDisconnected) {
                    connectionError = QString();
                    connectionErrorDetails.clear();
                } else if (connectionError.isEmpty()) {
                    connectionError = ConnectionHelper::statusReasonToErrorName(
                            connectionStatusReason, oldConnectionStatus);
                }

                checkCapabilitiesChanged(profileChanged);

                emit parent->statusChanged(connectionStatus, connectionStatusReason,
                        connectionError, connectionErrorDetails);
                parent->notify("connectionError");
                parent->notify("connectionErrorDetails");
            } else {
                connectionStatusChanged = false;
            }
        }
    }

    if (!connectionStatusChanged && profileChanged) {
        checkCapabilitiesChanged(profileChanged);
    }
}

void Account::Private::retrieveAvatar()
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            parent->propertiesInterface()->Get(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR),
                QLatin1String("Avatar")), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotAvatar(QDBusPendingCallWatcher*)));
}

bool Account::Private::processConnQueue()
{
    while (!connObjPathQueue.isEmpty()) {
        QString path = connObjPathQueue.head();
        if (path.isEmpty()) {
            if (!connection.isNull()) {
                debug() << "Dropping connection for account" << parent->objectPath();

                connection.reset();
                emit parent->haveConnectionChanged(false);
                parent->notify("haveConnection");
                parent->notify("connection");
                parent->notify("connectionObjectPath");
            }

            connObjPathQueue.dequeue();
        } else {
            debug() << "Building connection" << path << "for account" << parent->objectPath();

            QString busName = path.mid(1).replace(QLatin1String("/"), QLatin1String("."));
            parent->connect(connFactory->proxy(busName, path, chanFactory, contactFactory),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onConnectionBuilt(Tp::PendingOperation*)));

            // No dequeue here, but only in onConnectionBuilt, so we will queue future changes
            return false; // Only move on to the next paths when that build finishes
        }
    }

    return true;
}

void Account::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(Account) for" << objectPath();
        mPriv->updateProperties(reply.value());

        mPriv->readinessHelper->setInterfaces(interfaces());
        mPriv->mayFinishCore = true;

        if (mPriv->connObjPathQueue.isEmpty()) {
            debug() << "Account basic functionality is ready";
            mPriv->coreFinished = true;
            mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
        } else {
            debug() << "Deferring finishing Account::FeatureCore until the connection is built";
        }
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

        // It could be in either of actual or missing from the first time in corner cases like the
        // object going away, so let's be prepared for both (only checking for actualFeatures here
        // actually used to trigger a rare bug)
        //
        // Anyway, the idea is to not do setIntrospectCompleted twice
        if (!mPriv->readinessHelper->actualFeatures().contains(FeatureAvatar) &&
                !mPriv->readinessHelper->missingFeatures().contains(FeatureAvatar)) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureAvatar, true);
        }

        emit avatarChanged(mPriv->avatar);
        notify("avatar");
    } else {
        // check if the feature is already there, and for some reason retrieveAvatar
        // failed when called the second time
        if (!mPriv->readinessHelper->actualFeatures().contains(FeatureAvatar) &&
                !mPriv->readinessHelper->missingFeatures().contains(FeatureAvatar)) {
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
            if (info->name() == mPriv->protocolName) {
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
        warning() << "Failed to find the protocol in the CM protocols for account" << objectPath();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureProtocolInfo, false,
                operation->errorName(), operation->errorMessage());
    }
}

void Account::onConnectionReady(PendingOperation *op)
{
    mPriv->checkCapabilitiesChanged(false);

    /* let's not fail if connection can't become ready, the caps will still
     * work, but return the CM caps instead. Also no need to call
     * setIntrospectCompleted if the feature was already set to complete once,
     * since this method will be called whenever the account connection
     * changes */
    if (!isReady(FeatureCapabilities)) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCapabilities, true);
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
    invalidate(QLatin1String(TELEPATHY_QT4_ERROR_OBJECT_REMOVED),
            QLatin1String("Account removed from AccountManager"));
    emit removed();
}

void Account::onConnectionBuilt(PendingOperation *op)
{
    PendingReady *readyOp = qobject_cast<PendingReady *>(op);
    Q_ASSERT(readyOp != NULL);

    if (op->isError()) {
        warning() << "Building connection" << mPriv->connObjPathQueue.head() << "failed with" <<
            op->errorName() << "-" << op->errorMessage();

        if (!mPriv->connection.isNull()) {
            mPriv->connection.reset();
            emit haveConnectionChanged(false);
            notify("haveConnection");
            notify("connection");
            notify("connectionObjectPath");
        }
    } else {
        bool hadConnection = !mPriv->connection.isNull();
        ConnectionPtr prevConn = mPriv->connection;
        QString prevConnPath = connectionObjectPath();

        mPriv->connection = ConnectionPtr::dynamicCast(readyOp->proxy());
        Q_ASSERT(mPriv->connection);

        debug() << "Connection" << connectionObjectPath() << "built for" << objectPath();

        // TODO: could we, in fact, do with just a connectionChanged(connection) signal and not need
        // haveConnectionChanged(bool) now that we always have a proxy object to include in the
        // signal?
        if (!hadConnection) {
            emit haveConnectionChanged(true);
            notify("haveConnection");
        }

        if (prevConn != mPriv->connection) {
            notify("connection");
        }

        if (prevConnPath != connectionObjectPath()) {
            notify("connectionObjectPath");
        }
    }

    mPriv->connObjPathQueue.dequeue();

    if (mPriv->processConnQueue() && !mPriv->coreFinished && mPriv->mayFinishCore) {
        debug() << "Account" << objectPath() << "basic functionality is ready (connections built)";
        mPriv->coreFinished = true;
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

void Account::notify(const char *propertyName)
{
    emit propertyChanged(QLatin1String(propertyName));
}

} // Tp
