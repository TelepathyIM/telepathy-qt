/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#include <TelepathyQt/BaseProtocol>
#include "TelepathyQt/base-protocol-internal.h"

#include "TelepathyQt/_gen/base-protocol.moc.hpp"
#include "TelepathyQt/_gen/base-protocol-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>

#include <QDBusObjectPath>
#include <QString>
#include <QStringList>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseProtocol::Private
{
    Private(BaseProtocol *parent, const QDBusConnection &dbusConnection,
            const QString &name)
        : parent(parent),
          name(name),
          adaptee(new BaseProtocol::Adaptee(dbusConnection, parent))
    {
    }

    BaseProtocol *parent;
    QString name;

    BaseProtocol::Adaptee *adaptee;

    QHash<QString, AbstractProtocolInterfacePtr> interfaces;
    QStringList connInterfaces;
    ProtocolParameterList parameters;
    RequestableChannelClassSpecList rccSpecs;
    QString vcardField;
    QString englishName;
    QString iconName;
    QStringList authTypes;
    CreateConnectionCallback createConnectionCb;
    IdentifyAccountCallback identifyAccountCb;
    NormalizeContactCallback normalizeContactCb;
};

BaseProtocol::Adaptee::Adaptee(const QDBusConnection &dbusConnection, BaseProtocol *protocol)
    : QObject(protocol),
      mProtocol(protocol)
{
    (void) new Service::ProtocolAdaptor(dbusConnection, this, protocol->dbusObject());
}

BaseProtocol::Adaptee::~Adaptee()
{
}

QStringList BaseProtocol::Adaptee::interfaces() const
{
    QStringList ret;
    foreach (const AbstractProtocolInterfacePtr &iface, mProtocol->interfaces()) {
        ret << iface->interfaceName();
    }
    return ret;
}

QStringList BaseProtocol::Adaptee::connectionInterfaces() const
{
    return mProtocol->connectionInterfaces();
}

ParamSpecList BaseProtocol::Adaptee::parameters() const
{
    ParamSpecList ret;
    foreach (const ProtocolParameter &param, mProtocol->parameters()) {
         ParamSpec paramSpec = param.bareParameter();
         if (!(paramSpec.flags & ConnMgrParamFlagHasDefault)) {
             // we cannot pass QVariant::Invalid over D-Bus, lets build a dummy value
             // that should be ignored according to the spec
             paramSpec.defaultValue = QDBusVariant(
                     parseValueWithDBusSignature(QString(), paramSpec.signature));
         }
         ret << paramSpec;
    }
    return ret;
}

RequestableChannelClassList BaseProtocol::Adaptee::requestableChannelClasses() const
{
    return mProtocol->requestableChannelClasses().bareClasses();
}

QString BaseProtocol::Adaptee::vcardField() const
{
    return mProtocol->vcardField();
}

QString BaseProtocol::Adaptee::englishName() const
{
    return mProtocol->englishName();
}

QString BaseProtocol::Adaptee::icon() const
{
    return mProtocol->iconName();
}

QStringList BaseProtocol::Adaptee::authenticationTypes() const
{
    return mProtocol->authenticationTypes();
}

void BaseProtocol::Adaptee::identifyAccount(const QVariantMap &parameters,
        const Tp::Service::ProtocolAdaptor::IdentifyAccountContextPtr &context)
{
    DBusError error;
    QString accountId;
    accountId = mProtocol->identifyAccount(parameters, &error);
    if (accountId.isEmpty()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(accountId);
}

void BaseProtocol::Adaptee::normalizeContact(const QString &contactId,
        const Tp::Service::ProtocolAdaptor::NormalizeContactContextPtr &context)
{
    DBusError error;
    QString normalizedContactId;
    normalizedContactId = mProtocol->normalizeContact(contactId, &error);
    if (normalizedContactId.isEmpty()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(normalizedContactId);
}

/**
 * \class BaseProtocol
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-protocol.h <TelepathyQt/BaseProtocol>
 *
 * \brief Base class for protocol implementations.
 *
 * A Protocol is a D-Bus object that implements an IM protocol (for instance, jabber or msn).
 * The BaseProtocol class provides an easy way to implement a Protocol D-Bus object,
 * by providing the basic functionality itself and allowing you to extend it by setting
 * the appropriate properties and callbacks.
 *
 * A BaseProtocol instance cannot be registered by itself on the bus. You should add it
 * to a BaseConnectionManager instance using BaseConnectionManager::addProtocol(). When
 * the BaseConnectionManager is registered on the bus, all the BaseProtocol instances
 * will also be registered.
 */

/**
 * Constructs a new BaseProtocol object.
 *
 * \param dbusConnection The D-Bus connection to use.
 * \param name The name of this protocol.
 */
BaseProtocol::BaseProtocol(const QDBusConnection &dbusConnection, const QString &name)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, name))
{
}

/**
 * Class destructor.
 */
BaseProtocol::~BaseProtocol()
{
    delete mPriv;
}

/**
 * Return the protocol's name, as given on the constructor.
 *
 * \return The protocol's name.
 */
QString BaseProtocol::name() const
{
    return mPriv->name;
}

/**
 * Return the immutable properties of this protocol object.
 *
 * Immutable properties cannot change after the object has been registered
 * on the bus with registerObject().
 *
 * \return The immutable properties of this protocol object.
 */
QVariantMap BaseProtocol::immutableProperties() const
{
    QVariantMap ret;
    foreach (const AbstractProtocolInterfacePtr &iface, mPriv->interfaces) {
        ret.unite(iface->immutableProperties());
    }
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Interfaces"),
            QVariant::fromValue(mPriv->adaptee->interfaces()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Parameters"),
            QVariant::fromValue(mPriv->adaptee->parameters()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".ConnectionInterfaces"),
            QVariant::fromValue(mPriv->adaptee->connectionInterfaces()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".RequestableChannelClasses"),
            QVariant::fromValue(mPriv->adaptee->requestableChannelClasses()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".VCardField"),
            QVariant::fromValue(mPriv->adaptee->vcardField()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".EnglishName"),
            QVariant::fromValue(mPriv->adaptee->englishName()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Icon"),
            QVariant::fromValue(mPriv->adaptee->icon()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".AuthenticationTypes"),
            QVariant::fromValue(mPriv->adaptee->authenticationTypes()));
    return ret;
}

/**
 * Return the list of interface names that have been set with
 * setConnectionInterfaces().
 *
 * This list is exposed as the ConnectionInterfaces property of
 * this Protocol object on the bus and represents interface names
 * that might be in the Interfaces property of a Connection to this protocol.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return The list of interface names that have been set with
 * setConnectionInterfaces().
 * \sa setConnectionInterfaces()
 */
QStringList BaseProtocol::connectionInterfaces() const
{
    return mPriv->connInterfaces;
}

/**
 * Set the interface names that may appear on Connection objects of
 * this protocol.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param connInterfaces The list of interface names to set.
 * \sa connectionInterfaces()
 */
void BaseProtocol::setConnectionInterfaces(const QStringList &connInterfaces)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setConnectionInterfaces: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->connInterfaces = connInterfaces;
}

/**
 * Return the list of parameters that have been set with setParameters().
 *
 * This list is exposed as the Parameters property of this Protocol object
 * on the bus and represents the parameters which may be specified in the
 * Parameters property of an Account for this protocol.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return The list of parameters that have been set with setParameters().
 * \sa setParameters()
 */
ProtocolParameterList BaseProtocol::parameters() const
{
    return mPriv->parameters;
}

/**
 * Set the parameters that may be specified in the Parameters property
 * of an Account for this protocol.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param parameters The list of parameters to set.
 * \sa parameters()
 */
void BaseProtocol::setParameters(const ProtocolParameterList &parameters)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setParameters: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->parameters = parameters;
}

/**
 * Return the list of requestable channel classes that have been set
 * with setRequestableChannelClasses().
 *
 * This list is exposed as the RequestableChannelClasses property of
 * this Protocol object on the bus and represents the channel classes
 * which might be requestable from a Connection to this protocol.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \returns The list of requestable channel classes that have been set
 * with setRequestableChannelClasses()
 * \sa setRequestableChannelClasses()
 */
RequestableChannelClassSpecList BaseProtocol::requestableChannelClasses() const
{
    return mPriv->rccSpecs;
}

/**
 * Set the channel classes which might be requestable from a Connection
 * to this protocol.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param rccSpecs The list of requestable channel classes to set.
 * \sa requestableChannelClasses()
 */
void BaseProtocol::setRequestableChannelClasses(const RequestableChannelClassSpecList &rccSpecs)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setRequestableChannelClasses: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->rccSpecs = rccSpecs;
}

/**
 * Return the name of the vcard field that has been set with setVCardField().
 *
 * This is exposed as the VCardField property of this Protocol object on
 * the bus and represents the name of the most common vcard field used for
 * this protocol's contact identifiers, normalized to lower case.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return The name of the vcard field that has been set with setVCardField().
 * \sa setVCardField()
 */
QString BaseProtocol::vcardField() const
{
    return mPriv->vcardField;
}

/**
 * Set the name of the most common vcard field used for
 * this protocol's contact identifiers, normalized to lower case.
 *
 * For example, this would be x-jabber for Jabber/XMPP
 * (including Google Talk), or tel for the PSTN.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param vcardField The name of the vcard field to set.
 * \sa vcardField()
 */
void BaseProtocol::setVCardField(const QString &vcardField)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setVCardField: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->vcardField = vcardField;
}

/**
 * Return the name that has been set with setEnglishName().
 *
 * This is exposed as the EnglishName property of this Protocol object on
 * the bus and represents the name of the protocol in a form suitable
 * for display to users, such as "AIM" or "Yahoo!".
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return The name that has been set with setEnglishName().
 * \sa setEnglishName()
 */
QString BaseProtocol::englishName() const
{
    return mPriv->englishName;
}

/**
 * Set the name of the protocol in a form suitable for display to users,
 * such as "AIM" or "Yahoo!".
 *
 * This string should be in the C (english) locale. Clients are expected
 * to lookup a translation on their own translation catalogs and fall back
 * to this name if they have no translation for it.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param englishName The name to set.
 * \sa englishName()
 */
void BaseProtocol::setEnglishName(const QString &englishName)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setEnglishName: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->englishName = englishName;
}

/**
 * Return the icon name that has been set with setIconName().
 *
 * This is exposed as the Icon property of this Protocol object on
 * the bus and represents the name of an icon in the system's icon
 * theme suitable for this protocol, such as "im-msn".
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return The icon name set with setIconName().
 * \sa setIconName()
 */
QString BaseProtocol::iconName() const
{
    return mPriv->iconName;
}

/**
 * Set the name of an icon in the system's icon theme suitable
 * for this protocol, such as "im-msn".
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param iconName The icon name to set.
 * \sa iconName()
 */
void BaseProtocol::setIconName(const QString &iconName)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setIconName: cannot change property after "
            "registration, immutable property";
        return;
    }
     mPriv->iconName = iconName;
}

/**
 * Return the list of interfaces that have been set with setAuthenticationTypes().
 *
 * This is exposed as the AuthenticationTypes property of this Protocol object
 * on the bus and represents a list of D-Bus interfaces which provide
 * information as to what kind of authentication channels can possibly appear
 * before the connection reaches the CONNECTED state.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return The list of authentication types that have been
 * set with setAuthenticationTypes().
 * \sa setAuthenticationTypes()
 */
QStringList BaseProtocol::authenticationTypes() const
{
    return mPriv->authTypes;
}

/**
 * Set a list of D-Bus interfaces which provide information as to
 * what kind of authentication channels can possibly appear before
 * the connection reaches the CONNECTED state.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param authenticationTypes The list of interfaces to set.
 * \sa authenticationTypes()
 */
void BaseProtocol::setAuthenticationTypes(const QStringList &authenticationTypes)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setAuthenticationTypes: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->authTypes = authenticationTypes;
}

/**
 * Set a callback that will be called to create a new connection, when this
 * has been requested by a client.
 *
 * \param cb The callback to set.
 * \sa createConnection()
 */
void BaseProtocol::setCreateConnectionCallback(const CreateConnectionCallback &cb)
{
    mPriv->createConnectionCb = cb;
}

/**
 * Create a new connection object by calling the callback that has been set
 * with setCreateConnectionCallback().
 *
 * \param parameters The connection parameters.
 * \param error A pointer to a DBusError instance where any possible error
 * will be stored.
 * \return A pointer to the new connection, or a null BaseConnectionPtr if
 * no connection could be created, in which case \a error will contain an
 * appropriate error.
 * \sa setCreateConnectionCallback()
 */
BaseConnectionPtr BaseProtocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (!mPriv->createConnectionCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseConnectionPtr();
    }
    return mPriv->createConnectionCb(parameters, error);
}

/**
 * Set a callback that will be called from a client to identify an account.
 *
 * This callback will be called when the IdentifyAccount method on the Protocol
 * D-Bus object has been called.
 *
 * \param cb The callback to set.
 * \sa identifyAccount()
 */
void BaseProtocol::setIdentifyAccountCallback(const IdentifyAccountCallback &cb)
{
    mPriv->identifyAccountCb = cb;
}

/**
 * Return a string which uniquely identifies the account to which
 * the given parameters would connect, by calling the callback that
 * has been set with setIdentifyAccountCallback().
 *
 * \param parameters The connection parameters, as they would
 * be provided to createConnection().
 * \param error A pointer to a DBusError instance where any possible error
 * will be stored.
 * \return A string which uniquely identifies the account to which
 * the given parameters would connect, or an empty string if no callback
 * to create this string has been set with setIdentifyAccountCallback().
 * \sa setIdentifyAccountCallback()
 */
QString BaseProtocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (!mPriv->identifyAccountCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->identifyAccountCb(parameters, error);
}

/**
 * Set a callback that will be called from a client to normalize a contact id.
 *
 * \param cb The callback to set.
 * \sa normalizeContact()
 */
void BaseProtocol::setNormalizeContactCallback(const NormalizeContactCallback &cb)
{
    mPriv->normalizeContactCb = cb;
}

/**
 * Return a normalized version of the given \a contactId, by calling the callback
 * that has been set with setNormalizeContactCallback().
 *
 * \param contactId The contact ID to normalize.
 * \param error A pointer to a DBusError instance where any possible error
 * will be stored.
 * \return A normalized version of the given \a contactId, or an empty string
 * if no callback to do the normalization has been set with setNormalizeContactCallback().
 * \sa setNormalizeContactCallback()
 */
QString BaseProtocol::normalizeContact(const QString &contactId, Tp::DBusError *error)
{
    if (!mPriv->normalizeContactCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->normalizeContactCb(contactId, error);
}

/**
 * Return a list of interfaces that have been plugged into this Protocol
 * D-Bus object with plugInterface().
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \return A list containing all the Protocol interface implementation objects.
 * \sa plugInterface(), interface()
 */
QList<AbstractProtocolInterfacePtr> BaseProtocol::interfaces() const
{
    return mPriv->interfaces.values();
}

/**
 * Return a pointer to the interface with the given name.
 *
 * \param interfaceName The D-Bus name of the interface,
 * ex. TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING.
 * \return A pointer to the AbstractProtocolInterface object that implements
 * the D-Bus interface with the given name, or a null pointer if such an interface
 * has not been plugged into this object.
 * \sa plugInterface(), interfaces()
 */
AbstractProtocolInterfacePtr BaseProtocol::interface(const QString &interfaceName) const
{
    return mPriv->interfaces.value(interfaceName);
}

/**
 * Plug a new interface into this Protocol D-Bus object.
 *
 * This property is immutable and cannot change after this Protocol
 * object has been registered on the bus with registerObject().
 *
 * \param interface An AbstractProtocolInterface instance that implements
 * the interface that is to be plugged.
 * \return \c true on success or \c false otherwise
 * \sa interfaces(), interface()
 */
bool BaseProtocol::plugInterface(const AbstractProtocolInterfacePtr &interface)
{
    if (isRegistered()) {
        warning() << "Unable to plug protocol interface " << interface->interfaceName() <<
            "- protocol already registered";
        return false;
    }

    if (interface->isRegistered()) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
            "- interface already registered";
        return false;
    }

    if (mPriv->interfaces.contains(interface->interfaceName())) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
            "- another interface with same name already plugged";
        return false;
    }

    debug() << "Interface" << interface->interfaceName() << "plugged";
    mPriv->interfaces.insert(interface->interfaceName(), interface);
    return true;
}

/**
 * Reimplemented from DBusService.
 */
bool BaseProtocol::registerObject(const QString &busName, const QString &objectPath,
        DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    foreach (const AbstractProtocolInterfacePtr &iface, mPriv->interfaces) {
        if (!iface->registerInterface(dbusObject())) {
            // lets not fail if an optional interface fails registering, lets warn only
            warning() << "Unable to register interface" << iface->interfaceName() <<
                "for protocol" << mPriv->name;
        }
    }
    return DBusService::registerObject(busName, objectPath, error);
}

/**
 * \class AbstractProtocolInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-protocol.h <TelepathyQt/AbstractProtocolInterface>
 *
 * \brief Base class for all the Protocol object interface implementations.
 */

// AbstractProtocolInterface
AbstractProtocolInterface::AbstractProtocolInterface(const QString &interfaceName)
    : AbstractDBusServiceInterface(interfaceName)
{
}

AbstractProtocolInterface::~AbstractProtocolInterface()
{
}

// Proto.I.Addressing
BaseProtocolAddressingInterface::Adaptee::Adaptee(BaseProtocolAddressingInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseProtocolAddressingInterface::Adaptee::~Adaptee()
{
}

QStringList BaseProtocolAddressingInterface::Adaptee::addressableVCardFields() const
{
    return mInterface->addressableVCardFields();
}

QStringList BaseProtocolAddressingInterface::Adaptee::addressableURISchemes() const
{
    return mInterface->addressableUriSchemes();
}

void BaseProtocolAddressingInterface::Adaptee::normalizeVCardAddress(const QString& vcardField,
        const QString& vcardAddress,
        const Tp::Service::ProtocolInterfaceAddressingAdaptor::NormalizeVCardAddressContextPtr &context)
{
    DBusError error;
    QString normalizedAddress;
    normalizedAddress = mInterface->normalizeVCardAddress(vcardField, vcardAddress, &error);
    if (normalizedAddress.isEmpty()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(normalizedAddress);
}

void BaseProtocolAddressingInterface::Adaptee::normalizeContactURI(const QString& uri,
        const Tp::Service::ProtocolInterfaceAddressingAdaptor::NormalizeContactURIContextPtr &context)
{
    DBusError error;
    QString normalizedUri;
    normalizedUri = mInterface->normalizeContactUri(uri, &error);
    if (normalizedUri.isEmpty()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(normalizedUri);
}

struct TP_QT_NO_EXPORT BaseProtocolAddressingInterface::Private
{
    Private(BaseProtocolAddressingInterface *parent)
        : adaptee(new BaseProtocolAddressingInterface::Adaptee(parent))
    {
    }

    BaseProtocolAddressingInterface::Adaptee *adaptee;
    QStringList addressableVCardFields;
    QStringList addressableUriSchemes;
    NormalizeVCardAddressCallback normalizeVCardAddressCb;
    NormalizeContactUriCallback normalizeContactUriCb;
};

/**
 * \class BaseProtocolAddressingInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-protocol.h <TelepathyQt/BaseProtocolAddressingInterface>
 *
 * \brief Base class for implementations of Protocol.Interface.Addressing
 */

/**
 * Class constructor.
 */
BaseProtocolAddressingInterface::BaseProtocolAddressingInterface()
    : AbstractProtocolInterface(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseProtocolAddressingInterface::~BaseProtocolAddressingInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseProtocolAddressingInterface::immutableProperties() const
{
    // no immutable property
    return QVariantMap();
}

/**
 * Return the list of addressable vcard fields that have been set with
 * setAddressableVCardFields().
 *
 * This list is exposed as the AddressableVCardFields property of this
 * interface on the bus and represents the vcard fields that can be used
 * to request a contact for this protocol, normalized to lower case.
 *
 * \return The list of addressable vcard fields that have been set with
 * setAddressableVCardFields().
 * \sa setAddressableVCardFields()
 */
QStringList BaseProtocolAddressingInterface::addressableVCardFields() const
{
    return mPriv->addressableVCardFields;
}

/**
 * Set the list of vcard fields that can be used to request a contact for this protocol.
 *
 * All the field names should be normalized to lower case.
 *
 * \param vcardFields The list of vcard fields to set.
 * \sa addressableVCardFields()
 */
void BaseProtocolAddressingInterface::setAddressableVCardFields(const QStringList &vcardFields)
{
    mPriv->addressableVCardFields = vcardFields;
}

/**
 * Return the list of URI schemes that have been set with
 * setAddressableUriSchemes().
 *
 * This list is exposed as the AddressableURISchemes property of this interface
 * on the bus and represents the URI schemes that are supported by this
 * protocol, like "tel" or "sip".
 *
 * \return The list of addressable URI schemes that have been set with
 * setAddressableUriSchemes().
 * \sa setAddressableUriSchemes()
 */
QStringList BaseProtocolAddressingInterface::addressableUriSchemes() const
{
    return mPriv->addressableUriSchemes;
}

/**
 * Set the list of URI schemes that are supported by this protocol.
 *
 * \param uriSchemes The list of URI schemes to set.
 * \sa addressableUriSchemes()
 */
void BaseProtocolAddressingInterface::setAddressableUriSchemes(const QStringList &uriSchemes)
{
    mPriv->addressableUriSchemes = uriSchemes;
}

/**
 * Set a callback that will be called from a client to normalize a given
 * vcard address.
 *
 * This callback will be called when the NormalizeVCardAddress method
 * on the Protocol.Interface.Addressing D-Bus interface has been called.
 *
 * \param cb The callback to set.
 * \sa normalizeVCardAddress()
 */
void BaseProtocolAddressingInterface::setNormalizeVCardAddressCallback(
        const NormalizeVCardAddressCallback &cb)
{
    mPriv->normalizeVCardAddressCb = cb;
}

/**
 * Return a normalized version of the given \a vcardAddress, which corresponds
 * to the given \a vcardField, by calling the callback that has been set
 * with setNormalizeVCardAddressCallback().
 *
 * \param vcardField The vcard field of the address we are normalizing.
 * \param vcardAddress The address to normalize, which is assumed to belong to a contact.
 * \param error A pointer to a DBusError instance where any possible error
 * will be stored.
 * \return A normalized version of the given \a vcardAddress, or an empty
 * string if no callback to do the normalization has been set with
 * setNormalizeVCardAddressCallback().
 * \sa setNormalizeVCardAddressCallback()
 */
QString BaseProtocolAddressingInterface::normalizeVCardAddress(const QString &vcardField,
        const QString &vcardAddress, DBusError *error)
{
    if (!mPriv->normalizeVCardAddressCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->normalizeVCardAddressCb(vcardField, vcardAddress, error);
}

/**
 * Set a callback that will be called from a client to normalize a given contact URI.
 *
 * This callback will be called when the NormalizeContactURI method
 * on the Protocol.Interface.Addressing D-Bus interface has been called.
 *
 * \param cb The callback to set.
 * \sa normalizeContactUri()
 */
void BaseProtocolAddressingInterface::setNormalizeContactUriCallback(
        const NormalizeContactUriCallback &cb)
{
    mPriv->normalizeContactUriCb = cb;
}

/**
 * Return a normalized version of the given contact URI, \a uri, by calling
 * the callback that has been set with setNormalizeContactUriCallback().
 *
 * \param uri The contact URI to normalize.
 * \param error A pointer to a DBusError instance where any possible error
 * will be stored.
 * \return A normalized version of the given \a uri, or an empty string if no
 * callback to do the normalization has been set with setNormalizeContactUriCallback().
 * \sa setNormalizeContactUriCallback()
 */
QString BaseProtocolAddressingInterface::normalizeContactUri(const QString &uri, DBusError *error)
{
    if (!mPriv->normalizeContactUriCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->normalizeContactUriCb(uri, error);
}

void BaseProtocolAddressingInterface::createAdaptor()
{
    (void) new Service::ProtocolInterfaceAddressingAdaptor(dbusObject()->dbusConnection(),
                mPriv->adaptee, dbusObject());
}

// Proto.I.Avatars
BaseProtocolAvatarsInterface::Adaptee::Adaptee(BaseProtocolAvatarsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseProtocolAvatarsInterface::Adaptee::~Adaptee()
{
}

QStringList BaseProtocolAvatarsInterface::Adaptee::supportedAvatarMIMETypes() const
{
    return mInterface->avatarDetails().supportedMimeTypes();
}

uint BaseProtocolAvatarsInterface::Adaptee::minimumAvatarHeight() const
{
    return mInterface->avatarDetails().minimumHeight();
}

uint BaseProtocolAvatarsInterface::Adaptee::minimumAvatarWidth() const
{
    return mInterface->avatarDetails().minimumWidth();
}

uint BaseProtocolAvatarsInterface::Adaptee::recommendedAvatarHeight() const
{
    return mInterface->avatarDetails().recommendedHeight();
}

uint BaseProtocolAvatarsInterface::Adaptee::recommendedAvatarWidth() const
{
    return mInterface->avatarDetails().recommendedWidth();
}

uint BaseProtocolAvatarsInterface::Adaptee::maximumAvatarHeight() const
{
    return mInterface->avatarDetails().maximumHeight();
}

uint BaseProtocolAvatarsInterface::Adaptee::maximumAvatarWidth() const
{
    return mInterface->avatarDetails().maximumWidth();
}

uint BaseProtocolAvatarsInterface::Adaptee::maximumAvatarBytes() const
{
    return mInterface->avatarDetails().maximumBytes();
}

struct TP_QT_NO_EXPORT BaseProtocolAvatarsInterface::Private
{
    Private(BaseProtocolAvatarsInterface *parent)
        : adaptee(new BaseProtocolAvatarsInterface::Adaptee(parent))
    {
    }

    BaseProtocolAvatarsInterface::Adaptee *adaptee;
    AvatarSpec avatarDetails;
};

/**
 * \class BaseProtocolAvatarsInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-protocol.h <TelepathyQt/BaseProtocolAvatarsInterface>
 *
 * \brief Base class for implementations of Protocol.Interface.Avatars
 */

/**
 * Class constructor.
 */
BaseProtocolAvatarsInterface::BaseProtocolAvatarsInterface()
    : AbstractProtocolInterface(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseProtocolAvatarsInterface::~BaseProtocolAvatarsInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseProtocolAvatarsInterface::immutableProperties() const
{
    QVariantMap ret;
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".SupportedAvatarMIMETypes"),
            QVariant::fromValue(mPriv->adaptee->supportedAvatarMIMETypes()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight"),
            QVariant::fromValue(mPriv->adaptee->minimumAvatarHeight()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarWidth"),
            QVariant::fromValue(mPriv->adaptee->minimumAvatarWidth()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarHeight"),
            QVariant::fromValue(mPriv->adaptee->recommendedAvatarHeight()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarWidth"),
            QVariant::fromValue(mPriv->adaptee->recommendedAvatarWidth()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarHeight"),
            QVariant::fromValue(mPriv->adaptee->maximumAvatarHeight()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarWidth"),
            QVariant::fromValue(mPriv->adaptee->maximumAvatarWidth()));
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarBytes"),
            QVariant::fromValue(mPriv->adaptee->maximumAvatarBytes()));
    return ret;
}

/**
 * Return the AvatarSpec that has been set with setAvatarDetails().
 *
 * The contents of this AvatarSpec are exposed as the various properties
 * of this interface on the bus and represent the expected values of the
 * Connection.Interface.Avatars properties on connections of this protocol.
 *
 * This property is immutable and cannot change after this interface
 * has been registered on an object on the bus with registerInterface().
 *
 * \return The AvatarSpec that has been set with setAvatarDetails().
 * \sa setAvatarDetails()
 */
AvatarSpec BaseProtocolAvatarsInterface::avatarDetails() const
{
    return mPriv->avatarDetails;
}

/**
 * Set the avatar details that will be exposed on the properties of this
 * interface on the bus.
 *
 * This property is immutable and cannot change after this interface
 * has been registered on an object on the bus with registerInterface().
 *
 * \param details The details to set.
 * \sa avatarDetails()
 */
void BaseProtocolAvatarsInterface::setAvatarDetails(const AvatarSpec &details)
{
    if (isRegistered()) {
        warning() << "BaseProtocolAvatarsInterface::setAvatarDetails: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->avatarDetails = details;
}

void BaseProtocolAvatarsInterface::createAdaptor()
{
    (void) new Service::ProtocolInterfaceAvatarsAdaptor(dbusObject()->dbusConnection(),
                mPriv->adaptee, dbusObject());
}

// Proto.I.Presence
BaseProtocolPresenceInterface::Adaptee::Adaptee(BaseProtocolPresenceInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseProtocolPresenceInterface::Adaptee::~Adaptee()
{
}

SimpleStatusSpecMap BaseProtocolPresenceInterface::Adaptee::statuses() const
{
    return mInterface->statuses().bareSpecs();
}

struct TP_QT_NO_EXPORT BaseProtocolPresenceInterface::Private
{
    Private(BaseProtocolPresenceInterface *parent)
        : adaptee(new BaseProtocolPresenceInterface::Adaptee(parent))
    {
    }

    BaseProtocolPresenceInterface::Adaptee *adaptee;
    PresenceSpecList statuses;
};

/**
 * \class BaseProtocolPresenceInterface
 * \ingroup servicecm
 * \headerfile TelepathyQt/base-protocol.h <TelepathyQt/BaseProtocolPresenceInterface>
 *
 * \brief Base class for implementations of Protocol.Interface.Presence
 */

/**
 * Class constructor.
 */
BaseProtocolPresenceInterface::BaseProtocolPresenceInterface()
    : AbstractProtocolInterface(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
BaseProtocolPresenceInterface::~BaseProtocolPresenceInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseProtocolPresenceInterface::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE + QLatin1String(".Statuses"),
            QVariant::fromValue(mPriv->adaptee->statuses()));
    return map;
}

/**
 * Return the list of presence statuses that have been set with setStatuses().
 *
 * This list is exposed as the Statuses property of this interface on the bus
 * and represents the statuses that might appear in the
 * Connection.Interface.SimplePresence.Statuses property on a connection to
 * this protocol that supports SimplePresence.
 *
 * This property is immutable and cannot change after this interface
 * has been registered on an object on the bus with registerInterface().
 *
 * \return The list of presence statuses that have been set with setStatuses().
 * \sa setStatuses()
 */
PresenceSpecList BaseProtocolPresenceInterface::statuses() const
{
    return mPriv->statuses;
}

/**
 * Set the list of statuses that might appear in the
 * Connection.Interface.SimplePresence.Statuses property on a connection to
 * this protocol that supports SimplePresence.
 *
 * This property is immutable and cannot change after this interface
 * has been registered on an object on the bus with registerInterface().
 *
 * \param statuses The statuses list to set.
 * \sa statuses()
 */
void BaseProtocolPresenceInterface::setStatuses(const PresenceSpecList &statuses)
{
    if (isRegistered()) {
        warning() << "BaseProtocolPresenceInterface::setStatuses: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->statuses = statuses;
}

void BaseProtocolPresenceInterface::createAdaptor()
{
    (void) new Service::ProtocolInterfacePresenceAdaptor(dbusObject()->dbusConnection(),
                mPriv->adaptee, dbusObject());
}

}
