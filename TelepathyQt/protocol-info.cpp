/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#include <TelepathyQt/ProtocolInfo>

#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingString>

namespace Tp
{

struct TP_QT_NO_EXPORT ProtocolInfo::Private : public QSharedData
{
    Private()
        : dbusConnection(QDBusConnection::sessionBus()), // make the compiler happy
          addressingIface(0)
    {
    }

    Private(const ConnectionManagerPtr &cm, const QString &name)
        : dbusConnection(cm->dbusConnection()),
          busName(cm->busName()),
          cmName(cm->name()),
          name(name),
          iconName(QString(QLatin1String("im-%1")).arg(name)),
          addressingIface(0)
    {
        QString escapedProtocolName = name;
        escapedProtocolName.replace(QLatin1Char('-'), QLatin1Char('_'));
        objectPath = QString(QLatin1String("%1/%2")).arg(cm->objectPath()).arg(escapedProtocolName);
    }

    ~Private()
    {
        delete addressingIface;
    }

    Client::ProtocolInterfaceAddressingInterface *addressingInterface()
    {
        if (!addressingIface) {
            addressingIface = new Client::ProtocolInterfaceAddressingInterface(
                    dbusConnection, busName, objectPath);
        }

        return addressingIface;
    }

    QDBusConnection dbusConnection;
    QString busName;
    QString objectPath;
    QString cmName;
    QString name;
    ProtocolParameterList params;
    ConnectionCapabilities caps;
    QString vcardField;
    QString englishName;
    QString iconName;
    PresenceSpecList statuses;
    AvatarSpec avatarRequirements;
    QStringList addressableVCardFields;
    QStringList addressableUriSchemes;

    Client::ProtocolInterfaceAddressingInterface *addressingIface;
};

/**
 * \class ProtocolInfo
 * \ingroup clientcm
 * \headerfile TelepathyQt/protocol-info.h <TelepathyQt/ProtocolInfo>
 *
 * \brief The ProtocolInfo class represents a <a
 * href="http://telepathy.freedesktop.org/spec/Protocol.html">Telepathy Protocol</a>.
 */

ProtocolInfo::ProtocolInfo()
{
}

/**
 * Construct a new ProtocolInfo object.
 *
 * \param cm Connection manager owning this ProtocolInfo.
 * \param name Protocol name.
 */
ProtocolInfo::ProtocolInfo(const ConnectionManagerPtr &cm, const QString &name)
    : mPriv(new Private(cm, name))
{
}

ProtocolInfo::ProtocolInfo(const ProtocolInfo &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
ProtocolInfo::~ProtocolInfo()
{
}

ProtocolInfo &ProtocolInfo::operator=(const ProtocolInfo &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

/**
 * Return the short name of the connection manager (e.g. "gabble") for this protocol.
 *
 * \return The name of the connection manager for this protocol.
 */
QString ProtocolInfo::cmName() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->cmName;
}

/**
 * Return the string identifying this protocol as described in the \telepathy_spec
 * (e.g. "jabber").
 *
 * This identifier is not intended to be displayed to users directly; user
 * interfaces are responsible for mapping them to localized strings.
 *
 * \return A string identifying this protocol.
 */
QString ProtocolInfo::name() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->name;
}

/**
 * Return all supported parameters for this protocol. The parameters' names
 * may either be the well-known strings specified by the \telepathy_spec
 * (e.g. "account" and "password"), or implementation-specific strings.
 *
 * \return A list of parameters for this protocol.
 */
ProtocolParameterList ProtocolInfo::parameters() const
{
    if (!isValid()) {
        return ProtocolParameterList();
    }

    return mPriv->params;
}

/**
 * Return whether a given parameter can be passed to the connection
 * manager when creating a connection to this protocol.
 *
 * \param name The name of a parameter.
 * \return true if the given parameter exists.
 */
bool ProtocolInfo::hasParameter(const QString &name) const
{
    if (!isValid()) {
        return false;
    }

    foreach (const ProtocolParameter &param, mPriv->params) {
        if (param.name() == name) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether it might be possible to register new accounts on this
 * protocol, by setting the special parameter named
 * <code>register</code> to <code>true</code>.
 *
 * \return The same thing as hasParameter("register").
 * \sa hasParameter()
 */
bool ProtocolInfo::canRegister() const
{
    if (!isValid()) {
        return false;
    }

    return hasParameter(QLatin1String("register"));
}

/**
 * Return the capabilities that are expected to be available from a connection
 * to this protocol, i.e. those for which Connection::createChannel() can
 * reasonably be expected to succeed.
 * User interfaces can use this information to show or hide UI components.
 *
 * @return An object representing the capabilities expected to be available from
 *         a connection to this protocol.
 */
ConnectionCapabilities ProtocolInfo::capabilities() const
{
    if (!isValid()) {
        return ConnectionCapabilities();
    }

    return mPriv->caps;
}

/**
 * Return the name of the most common vcard field used for this protocol's
 * contact identifiers, normalized to lower case.
 *
 * One valid use of this field is to answer the question: given a contact's
 * vcard containing an X-JABBER field, how can you communicate with the contact?
 * By iterating through protocols looking for an x-jabber VCardField, one can
 * build up a list of protocols that handle x-jabber, then offer the user a list
 * of accounts for those protocols and/or the option to create a new account for
 * one of those protocols.
 * It is not necessarily valid to interpret contacts' identifiers as values of
 * this vcard field. For instance, telepathy-sofiasip supports contacts whose
 * identifiers are of the form sip:jenny@example.com or tel:8675309, which would
 * not normally both be represented by any single vcard field.
 *
 * \return The most common vcard field used for this protocol's contact
 *         identifiers, or an empty string if there is no such field.
 */
QString ProtocolInfo::vcardField() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->vcardField;
}

/**
 * Return the English-language name of this protocol, such as "AIM" or "Yahoo!".
 *
 * The name can be used as a fallback if an application doesn't have a localized name for this
 * protocol.
 *
 * If the manager file or the CM service doesn't specify the english name, it is inferred from this
 * protocol name, such that for example "google-talk" becomes "Google Talk", but "local-xmpp"
 * becomes "Local Xmpp".
 *
 * \return An English-language name for this protocol.
 */
QString ProtocolInfo::englishName() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->englishName;
}

/**
 * Return the name of an icon for this protocol in the system's icon theme, such as "im-msn".
 *
 * If the manager file or the CM service doesn't specify the icon name, "im-<protocolname>" is
 * assumed.
 *
 * \return The likely name of an icon for this protocol.
 */
QString ProtocolInfo::iconName() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->iconName;
}

/**
 * Return a list of PresenceSpec representing the possible presence statuses
 * from a connection to this protocol.
 *
 * \return A list of PresenceSpec representing the possible presence statuses
 *         from a connection to this protocol.
 */
PresenceSpecList ProtocolInfo::allowedPresenceStatuses() const
{
    if (!isValid()) {
        return PresenceSpecList();
    }

    return mPriv->statuses;
}

/**
 * Return the requirements (size limits, supported MIME types, etc)
 * for avatars used on to this protocol.
 *
 * \return The requirements for avatars used on this protocol.
 */
AvatarSpec ProtocolInfo::avatarRequirements() const
{
    if (!isValid()) {
        return AvatarSpec();
    }

    return mPriv->avatarRequirements;
}

/**
 * Return the vcard fields that can be used to request a contact with on this protocol,
 * normalized to lower case.
 *
 * \return The vcard fields normalized to lower case.
 * \sa addressableUriSchemes()
 */
QStringList ProtocolInfo::addressableVCardFields() const
{
    if (!isValid()) {
        return QStringList();
    }

    return mPriv->addressableVCardFields;
}

/**
 * Return the URI schemes that are supported by this protocol.
 *
 * \return The URI schemes.
 * \sa addressableVCardFields()
 */
QStringList ProtocolInfo::addressableUriSchemes() const
{
    if (!isValid()) {
        return QStringList();
    }

    return mPriv->addressableUriSchemes;
}

/**
 * Attempt to normalize the given \a vcardAddress.
 *
 * For example, a vcard TEL field formatted as +1 (206) 555 1234,
 * could be normalized to +12065551234.
 *
 * If a vcard address X would be normalized to Y, a successful ContactManager
 * contact request using ContactManager::contactsForVCardAddresses() for
 * vcard address X would result in a contact with Y reported as an
 * address that can identify it in Contact::vcardAddresses().
 *
 * \param vcardField The vcard field the \a vcardAddress belongs to.
 * \param vcardAddress The address to normalize.
 * \return A PendingString which will emit PendingString::finished
 *         when the address has been normalized or an error occurred.
 * \sa normalizeContactUri()
 */
PendingString *ProtocolInfo::normalizeVCardAddress(const QString &vcardField,
        const QString &vcardAddress)
{
    if (!isValid()) {
        return new PendingString(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Protocol object is invalid"));
    }

    Client::ProtocolInterfaceAddressingInterface *iface = mPriv->addressingInterface();
    if (!iface->isValid()) {
        // cm is still valid but no Protocol object found
        return new PendingString(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("ConnectionManager does not support Protocol.I.Addressing"));
    }

    return new PendingString(iface->NormalizeVCardAddress(vcardField, vcardAddress),
            SharedPtr<RefCounted>());
}

/**
 * Attempt to normalize the given contact \a uri.
 *
 * If the URI has extra information beyond what's necessary to identify a particular contact, such
 * as an XMPP resource or an action to carry out, this extra information wil be removed.
 *
 * An example would be xmpp:romeo@Example.Com/Empathy?message;body=Hello, which would be normalized
 * to xmpp:romeo@example.com.
 *
 * If a URI address X would be normalized to Y, a successful ContactManager
 * contact request using ContactManager::contactsForUris() for
 * URI address X would result in a contact with Y reported as an
 * address that can identify it in Contact::uris().
 *
 * \param uri The URI to normalize.
 * \return A PendingString which will emit PendingString::finished
 *         when the \a uri has been normalized or an error occurred.
 * \sa normalizeVCardAddress()
 */
PendingString *ProtocolInfo::normalizeContactUri(const QString &uri)
{
    if (!isValid()) {
        return new PendingString(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Protocol object is invalid"));
    }

    Client::ProtocolInterfaceAddressingInterface *iface = mPriv->addressingInterface();
    if (!iface->isValid()) {
        // cm is still valid but no Protocol object found
        return new PendingString(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("ConnectionManager does not support Protocol.I.Addressing"));
    }

    return new PendingString(iface->NormalizeContactURI(uri),
            SharedPtr<RefCounted>());
}

void ProtocolInfo::addParameter(const ParamSpec &spec)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    QVariant defaultValue;
    if (spec.flags & ConnMgrParamFlagHasDefault) {
        defaultValue = spec.defaultValue.variant();
    }

    uint flags = spec.flags;
    if (spec.name.endsWith(QLatin1String("password"))) {
        flags |= ConnMgrParamFlagSecret;
    }

    ProtocolParameter param(spec.name,
            QDBusSignature(spec.signature),
            (ConnMgrParamFlag) flags,
            defaultValue);
    mPriv->params.append(param);
}

void ProtocolInfo::setVCardField(const QString &vcardField)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->vcardField = vcardField;
}

void ProtocolInfo::setEnglishName(const QString &englishName)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->englishName = englishName;
}

void ProtocolInfo::setIconName(const QString &iconName)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->iconName = iconName;
}

void ProtocolInfo::setRequestableChannelClasses(
        const RequestableChannelClassList &caps)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->caps.updateRequestableChannelClasses(caps);
}

void ProtocolInfo::setAllowedPresenceStatuses(const PresenceSpecList &statuses)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->statuses = statuses;
}

void ProtocolInfo::setAvatarRequirements(const AvatarSpec &avatarRequirements)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->avatarRequirements = avatarRequirements;
}

void ProtocolInfo::setAddressableVCardFields(const QStringList &vcardFields)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->addressableVCardFields = vcardFields;
}

void ProtocolInfo::setAddressableUriSchemes(const QStringList &uriSchemes)
{
    if (!isValid()) {
        mPriv = new Private;
    }

    mPriv->addressableUriSchemes = uriSchemes;
}

} // Tp
