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
    QString vCardField;
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

QString BaseProtocol::Adaptee::vCardField() const
{
    return mProtocol->vCardField();
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
        context->setFinishedWithError(error);
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
        context->setFinishedWithError(error);
        return;
    }
    context->setFinished(normalizedContactId);
}

BaseProtocol::BaseProtocol(const QDBusConnection &dbusConnection, const QString &name)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, name))
{
}

BaseProtocol::~BaseProtocol()
{
    delete mPriv;
}

QString BaseProtocol::name() const
{
    return mPriv->name;
}

QVariantMap BaseProtocol::immutableProperties() const
{
    QVariantMap ret;
    foreach (const AbstractProtocolInterfacePtr &iface, mPriv->interfaces) {
        ret.unite(iface->immutableProperties());
    }
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Interfaces"), qVariantFromValue(mPriv->adaptee->interfaces()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Parameters"), qVariantFromValue(mPriv->adaptee->parameters()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".ConnectionInterfaces"), qVariantFromValue(mPriv->adaptee->connectionInterfaces()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".RequestableChannelClasses"), qVariantFromValue(mPriv->adaptee->requestableChannelClasses()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".VCardField"), qVariantFromValue(mPriv->adaptee->vCardField()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".EnglishName"), qVariantFromValue(mPriv->adaptee->englishName()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Icon"), qVariantFromValue(mPriv->adaptee->icon()));
    ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".AuthenticationTypes"), qVariantFromValue(mPriv->adaptee->authenticationTypes()));
    return ret;
}

QStringList BaseProtocol::connectionInterfaces() const
{
    return mPriv->connInterfaces;
}

void BaseProtocol::setConnectionInterfaces(const QStringList &connInterfaces)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setConnectionInterfaces: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->connInterfaces = connInterfaces;
}

ProtocolParameterList BaseProtocol::parameters() const
{
    return mPriv->parameters;
}

void BaseProtocol::setParameters(const ProtocolParameterList &parameters)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setParameters: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->parameters = parameters;
}

RequestableChannelClassSpecList BaseProtocol::requestableChannelClasses() const
{
    return mPriv->rccSpecs;
}

void BaseProtocol::setRequestableChannelClasses(const RequestableChannelClassSpecList &rccSpecs)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setRequestableChannelClasses: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->rccSpecs = rccSpecs;
}

QString BaseProtocol::vCardField() const
{
    return mPriv->vCardField;
}

void BaseProtocol::setVCardField(const QString &vCardField)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setVCardField: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->vCardField = vCardField;
}

QString BaseProtocol::englishName() const
{
    return mPriv->englishName;
}

void BaseProtocol::setEnglishName(const QString &englishName)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setEnglishName: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->englishName = englishName;
}

QString BaseProtocol::iconName() const
{
    return mPriv->iconName;
}

void BaseProtocol::setIconName(const QString &iconName)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setIconName: cannot change property after "
            "registration, immutable property";
        return;
    }
     mPriv->iconName = iconName;
}

QStringList BaseProtocol::authenticationTypes() const
{
    return mPriv->authTypes;
}

void BaseProtocol::setAuthenticationTypes(const QStringList &authenticationTypes)
{
    if (isRegistered()) {
        warning() << "BaseProtocol::setAuthenticationTypes: cannot change property after "
            "registration, immutable property";
        return;
    }
    mPriv->authTypes = authenticationTypes;
}

void BaseProtocol::setCreateConnectionCallback(const CreateConnectionCallback &cb)
{
    mPriv->createConnectionCb = cb;
}

BaseConnectionPtr BaseProtocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (!mPriv->createConnectionCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return BaseConnectionPtr();
    }
    return mPriv->createConnectionCb(parameters, error);
}

void BaseProtocol::setIdentifyAccountCallback(const IdentifyAccountCallback &cb)
{
    mPriv->identifyAccountCb = cb;
}

QString BaseProtocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error)
{
    if (!mPriv->identifyAccountCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->identifyAccountCb(parameters, error);
}

void BaseProtocol::setNormalizeContactCallback(const NormalizeContactCallback &cb)
{
    mPriv->normalizeContactCb = cb;
}

QString BaseProtocol::normalizeContact(const QString &contactId, Tp::DBusError *error)
{
    if (!mPriv->normalizeContactCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->normalizeContactCb(contactId, error);
}

QList<AbstractProtocolInterfacePtr> BaseProtocol::interfaces() const
{
    return mPriv->interfaces.values();
}

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

// AbstractProtocolInterface
AbstractProtocolInterface::AbstractProtocolInterface(const QString &interfaceName)
    : AbstractDBusServiceInterface(interfaceName)
{
}

AbstractProtocolInterface::~AbstractProtocolInterface()
{
}

// Proto.I.Addressing
BaseProtocolAddressingInterface::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
        QObject *dbusObject, BaseProtocolAddressingInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
    (void) new Service::ProtocolInterfaceAddressingAdaptor(dbusConnection, this, dbusObject);
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

void BaseProtocolAddressingInterface::Adaptee::normalizeVCardAddress(const QString& vCardField,
        const QString& vCardAddress,
        const Tp::Service::ProtocolInterfaceAddressingAdaptor::NormalizeVCardAddressContextPtr &context)
{
    DBusError error;
    QString normalizedAddress;
    normalizedAddress = mInterface->normalizeVCardAddress(vCardField, vCardAddress, &error);
    if (normalizedAddress.isEmpty()) {
        context->setFinishedWithError(error);
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
        context->setFinishedWithError(error);
        return;
    }
    context->setFinished(normalizedUri);
}

struct TP_QT_NO_EXPORT BaseProtocolAddressingInterface::Private
{
    Private()
        : adaptee(0)
    {
    }

    BaseProtocolAddressingInterface::Adaptee *adaptee;
    QStringList addressableVCardFields;
    QStringList addressableUriSchemes;
    NormalizeVCardAddressCallback normalizeVCardAddressCb;
    NormalizeContactUriCallback normalizeContactUriCb;
};

BaseProtocolAddressingInterface::BaseProtocolAddressingInterface()
    : AbstractProtocolInterface(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING),
      mPriv(new Private)
{
}

BaseProtocolAddressingInterface::~BaseProtocolAddressingInterface()
{
    delete mPriv;
}

QVariantMap BaseProtocolAddressingInterface::immutableProperties() const
{
    // no immutable property
    return QVariantMap();
}

QStringList BaseProtocolAddressingInterface::addressableVCardFields() const
{
    return mPriv->addressableVCardFields;
}

void BaseProtocolAddressingInterface::setAddressableVCardFields(const QStringList &vcardFields)
{
    mPriv->addressableVCardFields = vcardFields;
}

QStringList BaseProtocolAddressingInterface::addressableUriSchemes() const
{
    return mPriv->addressableUriSchemes;
}

void BaseProtocolAddressingInterface::setAddressableUriSchemes(const QStringList &uriSchemes)
{
    mPriv->addressableUriSchemes = uriSchemes;
}

void BaseProtocolAddressingInterface::setNormalizeVCardAddressCallback(
        const NormalizeVCardAddressCallback &cb)
{
    mPriv->normalizeVCardAddressCb = cb;
}

QString BaseProtocolAddressingInterface::normalizeVCardAddress(const QString &vCardField,
        const QString &vCardAddress, DBusError *error)
{
    if (!mPriv->normalizeVCardAddressCb.isValid()) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return QString();
    }
    return mPriv->normalizeVCardAddressCb(vCardField, vCardAddress, error);
}

void BaseProtocolAddressingInterface::setNormalizeContactUriCallback(
        const NormalizeContactUriCallback &cb)
{
    mPriv->normalizeContactUriCb = cb;
}

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
    Q_ASSERT(!mPriv->adaptee);
    mPriv->adaptee = new BaseProtocolAddressingInterface::Adaptee(
            dbusObject()->dbusConnection(), dbusObject(), this);
}

// Proto.I.Avatars
BaseProtocolAvatarsInterface::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
        QObject *dbusObject, BaseProtocolAvatarsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
    (void) new Service::ProtocolInterfaceAvatarsAdaptor(dbusConnection, this, dbusObject);
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
    Private()
        : adaptee(0)
    {
    }

    BaseProtocolAvatarsInterface::Adaptee *adaptee;
    AvatarSpec avatarDetails;
};

BaseProtocolAvatarsInterface::BaseProtocolAvatarsInterface()
    : AbstractProtocolInterface(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS),
      mPriv(new Private)
{
}

BaseProtocolAvatarsInterface::~BaseProtocolAvatarsInterface()
{
    delete mPriv;
}

QVariantMap BaseProtocolAvatarsInterface::immutableProperties() const
{
    QVariantMap ret;
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".SupportedAvatarMIMETypes"), mPriv->adaptee->supportedAvatarMIMETypes());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight"), mPriv->adaptee->minimumAvatarHeight());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarWidth"), mPriv->adaptee->minimumAvatarWidth());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarHeight"), mPriv->adaptee->recommendedAvatarHeight());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarWidth"), mPriv->adaptee->recommendedAvatarWidth());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarHeight"), mPriv->adaptee->maximumAvatarHeight());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarWidth"), mPriv->adaptee->maximumAvatarWidth());
    ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarBytes"), mPriv->adaptee->maximumAvatarBytes());
    return ret;
}

AvatarSpec BaseProtocolAvatarsInterface::avatarDetails() const
{
    return mPriv->avatarDetails;
}

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
    Q_ASSERT(!mPriv->adaptee);
    mPriv->adaptee = new BaseProtocolAvatarsInterface::Adaptee(
            dbusObject()->dbusConnection(), dbusObject(), this);
}

// Proto.I.Presence
BaseProtocolPresenceInterface::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
        QObject *dbusObject, BaseProtocolPresenceInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
    (void) new Service::ProtocolInterfacePresenceAdaptor(dbusConnection, this, dbusObject);
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
    Private()
        : adaptee(0)
    {
    }

    BaseProtocolPresenceInterface::Adaptee *adaptee;
    PresenceSpecList statuses;
};

BaseProtocolPresenceInterface::BaseProtocolPresenceInterface()
    : AbstractProtocolInterface(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE),
      mPriv(new Private)
{
}

BaseProtocolPresenceInterface::~BaseProtocolPresenceInterface()
{
    delete mPriv;
}

QVariantMap BaseProtocolPresenceInterface::immutableProperties() const
{
    // no immutable property
    return QVariantMap();
}

PresenceSpecList BaseProtocolPresenceInterface::statuses() const
{
    return mPriv->statuses;
}

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
    Q_ASSERT(!mPriv->adaptee);
    mPriv->adaptee = new BaseProtocolPresenceInterface::Adaptee(
            dbusObject()->dbusConnection(), dbusObject(), this);
}

}
