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

#include <TelepathyQt4/Contact>
#include "TelepathyQt4/_gen/contact.moc.hpp"

#include <TelepathyQt4/AvatarData>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ContactCapabilities>
#include <TelepathyQt4/ContactLocation>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContactInfo>
#include <TelepathyQt4/PendingVoid>
#include <TelepathyQt4/Presence>
#include <TelepathyQt4/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT Contact::Private
{
    Private(Contact *parent, ContactManager *manager,
        const ReferencedHandles &handle)
        : parent(parent),
          manager(manager),
          handle(handle),
          caps(manager->supportedFeatures().contains(Contact::FeatureCapabilities) ?
                   ContactCapabilities(true) :  ContactCapabilities(
                           manager->connection()->capabilities().allClassSpecs(), false)),
          isAvatarTokenKnown(false),
          subscriptionState(PresenceStateNo),
          publishState(PresenceStateNo),
          blocked(false)
    {
    }

    Contact *parent;

    ContactManager *manager;
    ReferencedHandles handle;
    QString id;

    QSet<Feature> requestedFeatures;
    QSet<Feature> actualFeatures;

    QString alias;
    Presence presence;
    ContactCapabilities caps;
    ContactLocation location;
    InfoFields info;

    bool isAvatarTokenKnown;
    QString avatarToken;
    AvatarData avatarData;
    void updateAvatarData();

    PresenceState subscriptionState;
    PresenceState publishState;
    bool blocked;

    QSet<QString> groups;
};

struct TELEPATHY_QT4_NO_EXPORT Contact::InfoFields::Private : public QSharedData
{
    Private(const ContactInfoFieldList &allFields)
        : allFields(allFields) {}

    ContactInfoFieldList allFields;
};

Contact::InfoFields::InfoFields(const ContactInfoFieldList &allFields)
    : mPriv(new Private(allFields))
{
}

Contact::InfoFields::InfoFields()
{
}

Contact::InfoFields::InfoFields(const Contact::InfoFields &other)
    : mPriv(other.mPriv)
{
}

Contact::InfoFields::~InfoFields()
{
}

Contact::InfoFields &Contact::InfoFields::operator=(const Contact::InfoFields &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

ContactInfoFieldList Contact::InfoFields::fields(const QString &name) const
{
    if (!isValid()) {
        return ContactInfoFieldList();
    }

    ContactInfoFieldList ret;
    foreach (const ContactInfoField &field, mPriv->allFields) {
        if (field.fieldName == name) {
            ret.append(field);
        }
    }
    return ret;
}

ContactInfoFieldList Contact::InfoFields::allFields() const
{
    return isValid() ? mPriv->allFields : ContactInfoFieldList();
}

ContactManager *Contact::manager() const
{
    return mPriv->manager;
}

ReferencedHandles Contact::handle() const
{
    return mPriv->handle;
}

QString Contact::id() const
{
    return mPriv->id;
}

QSet<Contact::Feature> Contact::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

QSet<Contact::Feature> Contact::actualFeatures() const
{
    return mPriv->actualFeatures;
}

QString Contact::alias() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAlias)) {
        warning() << "Contact::alias() used on" << this
            << "for which FeatureAlias hasn't been requested - returning id";
        return id();
    }

    return mPriv->alias;
}

bool Contact::isAvatarTokenKnown() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        warning() << "Contact::isAvatarTokenKnown() used on" << this
            << "for which FeatureAvatarToken hasn't been requested - returning false";
        return false;
    }

    return mPriv->isAvatarTokenKnown;
}

QString Contact::avatarToken() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        warning() << "Contact::avatarToken() used on" << this
            << "for which FeatureAvatarToken hasn't been requested - returning \"\"";
        return QString();
    } else if (!isAvatarTokenKnown()) {
        warning() << "Contact::avatarToken() used on" << this
            << "for which the avatar token is not (yet) known - returning \"\"";
        return QString();
    }

    return mPriv->avatarToken;
}

AvatarData Contact::avatarData() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarData)) {
        warning() << "Contact::avatarData() used on" << this
            << "for which FeatureAvatarData hasn't been requested - returning \"\"";
        return AvatarData();
    }

    return mPriv->avatarData;
}

Presence Contact::presence() const
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        warning() << "Contact::presence() used on" << this
            << "for which FeatureSimplePresence hasn't been requested - returning Unknown";
        return Presence();
    }

    return mPriv->presence;
}

/**
 * Return the capabilities for this contact.
 *
 * User interfaces can use this information to show or hide UI components.
 *
 * Change notification is advertised through capabilitiesChanged().
 *
 * If ContactManager::supportedFeatures() contains Contact::FeatureCapabilities,
 * the returned object will be a ContactCapabilities object, where
 * CapabilitiesBase::isSpecificToContact() will be \c true; if that feature
 * isn't present, this returned object is the subset of
 * Contact::manager()::connection()::capabilities()
 * and CapabilitiesBase::isSpecificToContact() will be \c false.
 *
 * This method requires Contact::FeatureCapabilities to be enabled.
 *
 * @return An object representing the contact capabilities.
 */
ContactCapabilities Contact::capabilities() const
{
    if (!mPriv->requestedFeatures.contains(FeatureCapabilities)) {
        warning() << "Contact::capabilities() used on" << this
            << "for which FeatureCapabilities hasn't been requested - returning 0";
        return ContactCapabilities(false);
    }

    return mPriv->caps;
}

/**
 * Return the location for this contact.
 *
 * Change notification is advertised through locationUpdated().
 *
 * This method requires Contact::FeatureLocation to be enabled.
 *
 * @return An object representing the contact location which will return \c false for
 *         ContactLocation::isValid() if FeatureLocation is not ready.
 */
ContactLocation Contact::location() const
{
    if (!mPriv->requestedFeatures.contains(FeatureLocation)) {
        warning() << "Contact::location() used on" << this
            << "for which FeatureLocation hasn't been requested - returning 0";
        return ContactLocation();
    }

    return mPriv->location;
}

/**
 * Return the information for this contact.
 *
 * Change notification is advertised through infoFieldsChanged().
 *
 * Note that this method only return cached information. In order to refresh the
 * information use refreshInfo().
 *
 * This method requires Contact::FeatureInfo to be enabled.
 *
 * \return An object representing the contact information.
 */
Contact::InfoFields Contact::infoFields() const
{
    if (!mPriv->requestedFeatures.contains(FeatureInfo)) {
        warning() << "Contact::infoFields() used on" << this
            << "for which FeatureInfo hasn't been requested - returning empty "
               "InfoFields";
        return InfoFields();
    }

    return mPriv->info;
}

/**
 * Refresh information for the given contact.
 *
 * Once the information is retrieved infoFieldsChanged() will be emitted.
 *
 * This method requires Contact::FeatureInfo to be enabled.
 *
 * \return A PendingOperation, which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa infoFieldsChanged()
 */
PendingOperation *Contact::refreshInfo()
{
   if (!mPriv->requestedFeatures.contains(FeatureInfo)) {
        warning() << "Contact::refreshInfo() used on" << this
            << "for which FeatureInfo hasn't been requested - failing";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("FeatureInfo needs to be enabled in order to "
                    "use this method"), this);
    }

    ConnectionPtr connection = mPriv->manager->connection();
    if (!connection->hasInterface(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_INFO)) {
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("Connection does not support ContactInfo interface"),
                this);
    }

    Client::ConnectionInterfaceContactInfoInterface *contactInfoInterface =
        connection->interface<Client::ConnectionInterfaceContactInfoInterface>();
    return new PendingVoid(
            contactInfoInterface->RefreshContactInfo(
                UIntList() << mPriv->handle[0]), this);
}

/**
 * Request information for the given contact.
 *
 * This method is useful for UIs that don't care about notification of changes
 * in the contact information but want to show the contact information
 * (e.g. right-click on a contact and show the contact info).
 *
 * \return A PendingContactInfo, which will emit PendingContactInfo::finished
 *         when the information has been retrieved or an error occurred.
 */
PendingContactInfo *Contact::requestInfo()
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return new PendingContactInfo(self);
}

Contact::PresenceState Contact::subscriptionState() const
{
    return mPriv->subscriptionState;
}

Contact::PresenceState Contact::publishState() const
{
    return mPriv->publishState;
}

PendingOperation *Contact::requestPresenceSubscription(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->requestPresenceSubscription(
            QList<ContactPtr >() << self,
            message);
}

PendingOperation *Contact::removePresenceSubscription(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->removePresenceSubscription(
            QList<ContactPtr>() << self,
            message);
}

PendingOperation *Contact::authorizePresencePublication(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->authorizePresencePublication(
            QList<ContactPtr>() << self,
            message);
}

PendingOperation *Contact::removePresencePublication(const QString &message)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->removePresencePublication(
            QList<ContactPtr>() << self,
            message);
}

bool Contact::isBlocked() const
{
    return mPriv->blocked;
}

PendingOperation *Contact::block(bool value)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->blockContacts(
            QList<ContactPtr>() << self,
            value);
}

/**
 * Return the names of the user-defined contact list groups to which the contact
 * belongs.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \return List of user-defined contact list groups names for a given contact.
 * \sa addToGroup(), removedFromGroup()
 */
QStringList Contact::groups() const
{
    return mPriv->groups.toList();
}

/**
 * Attempt to add the contact to the user-defined contact list
 * group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \param group Group name.
 * \return A pending operation which will return when an attempt has been made
 *         to add the contact to the user-defined contact list group.
 */
PendingOperation *Contact::addToGroup(const QString &group)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->addContactsToGroup(
            group, QList<ContactPtr>() << self);
}

/**
 * Attempt to remove the contact from the user-defined contact list
 * group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be enabled.
 *
 * \param group Group name.
 * \return A pending operation which will return when an attempt has been made
 *         to remove the contact from the user-defined contact list group.
 */
PendingOperation *Contact::removeFromGroup(const QString &group)
{
    ContactPtr self =
        mPriv->manager->lookupContactByHandle(mPriv->handle[0]);
    return mPriv->manager->removeContactsFromGroup(
            group, QList<ContactPtr>() << self);
}

Contact::~Contact()
{
    debug() << "Contact" << id() << "destroyed";
    delete mPriv;
}

Contact::Contact(ContactManager *manager, const ReferencedHandles &handle,
        const QSet<Feature> &requestedFeatures, const QVariantMap &attributes)
    : QObject(0), mPriv(new Private(this, manager, handle))
{
    augment(requestedFeatures, attributes);
}

void Contact::augment(const QSet<Feature> &requestedFeatures, const QVariantMap &attributes)
{
    mPriv->requestedFeatures.unite(requestedFeatures);

    mPriv->id = qdbus_cast<QString>(attributes[
            QLatin1String(TELEPATHY_INTERFACE_CONNECTION "/contact-id")]);

    foreach (Feature feature, requestedFeatures) {
        QString maybeAlias;
        SimplePresence maybePresence;
        RequestableChannelClassList maybeCaps;
        QVariantMap maybeLocation;
        ContactInfoFieldList maybeInfo;

        switch (feature) {
            case FeatureAlias:
                maybeAlias = qdbus_cast<QString>(attributes.value(
                            QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING "/alias")));

                if (!maybeAlias.isEmpty()) {
                    receiveAlias(maybeAlias);
                } else if (mPriv->alias.isEmpty()) {
                    mPriv->alias = mPriv->id;
                }
                break;

            case FeatureAvatarToken:
                if (attributes.contains(
                            QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS "/token"))) {
                    receiveAvatarToken(qdbus_cast<QString>(attributes.value(
                                    QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS "/token"))));
                } else {
                    if (manager()->supportedFeatures().contains(FeatureAvatarToken)) {
                        // AvatarToken being supported but not included in the mapping indicates
                        // that the avatar token is not known - however, the feature is working fine
                        mPriv->actualFeatures.insert(FeatureAvatarToken);
                    }
                    // In either case, the avatar token can't be known
                    mPriv->isAvatarTokenKnown = false;
                    mPriv->avatarToken = QLatin1String("");
                }
                break;

            case FeatureAvatarData:
                if (manager()->supportedFeatures().contains(FeatureAvatarData)) {
                    mPriv->actualFeatures.insert(FeatureAvatarData);
                    mPriv->updateAvatarData();
                }
                break;

            case FeatureSimplePresence:
                maybePresence = qdbus_cast<SimplePresence>(attributes.value(
                            QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE "/presence")));

                if (!maybePresence.status.isEmpty()) {
                    receiveSimplePresence(maybePresence);
                } else {
                    mPriv->presence.setStatus(ConnectionPresenceTypeUnknown,
                            QLatin1String("unknown"), QLatin1String(""));
                }
                break;

            case FeatureCapabilities:
                maybeCaps = qdbus_cast<RequestableChannelClassList>(attributes.value(
                            QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES "/capabilities")));

                if (!maybeCaps.isEmpty()) {
                    receiveCapabilities(maybeCaps);
                } else {
                    if (manager()->supportedFeatures().contains(FeatureCapabilities) &&
                        mPriv->requestedFeatures.contains(FeatureCapabilities)) {
                        // Capabilities being supported but not updated in the
                        // mapping indicates that the capabilities is not known -
                        // however, the feature is working fine.
                        mPriv->actualFeatures.insert(FeatureCapabilities);
                    }
                }
                break;

            case FeatureLocation:
                maybeLocation = qdbus_cast<QVariantMap>(attributes.value(
                            QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_LOCATION "/location")));

                if (!maybeLocation.isEmpty()) {
                    receiveLocation(maybeLocation);
                } else {
                    if (manager()->supportedFeatures().contains(FeatureLocation) &&
                        mPriv->requestedFeatures.contains(FeatureLocation)) {
                        // Location being supported but not updated in the
                        // mapping indicates that the location is not known -
                        // however, the feature is working fine
                        mPriv->actualFeatures.insert(FeatureLocation);
                    }
                }
                break;

            case FeatureInfo:
                maybeInfo = qdbus_cast<ContactInfoFieldList>(attributes.value(
                            QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CONTACT_INFO "/info")));

                if (!maybeInfo.isEmpty()) {
                    receiveInfo(maybeInfo);
                } else {
                    if (manager()->supportedFeatures().contains(FeatureInfo) &&
                        mPriv->requestedFeatures.contains(FeatureInfo)) {
                        // Info being supported but not updated in the
                        // mapping indicates that the info is not known -
                        // however, the feature is working fine
                        mPriv->actualFeatures.insert(FeatureInfo);
                    }
                }
                break;

            default:
                warning() << "Unknown feature" << feature << "encountered when augmenting Contact";
                break;
        }
    }
}

void Contact::receiveAlias(const QString &alias)
{
    if (!mPriv->requestedFeatures.contains(FeatureAlias)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureAlias);

    if (mPriv->alias != alias) {
        mPriv->alias = alias;
        emit aliasChanged(alias);
    }
}

void Contact::setAvatarToken(const QString &token)
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureAvatarToken);

    if (!mPriv->isAvatarTokenKnown || mPriv->avatarToken != token) {
        mPriv->isAvatarTokenKnown = true;
        mPriv->avatarToken = token;
        emit avatarTokenChanged(mPriv->avatarToken);
    }
}

void Contact::Private::updateAvatarData()
{
    /* If token is NULL, it means that CM doesn't know the token. In that case we
     * have to request the avatar data to get the token. This happens with XMPP
     * for offline contacts. We don't want to bypass the avatar cache, so we won't
     * update avatar. */
    if (avatarToken.isNull()) {
        return;
    }

    /* If token is empty (""), it means the contact has no avatar. */
    if (avatarToken.isEmpty()) {
        debug() << "Contact" << parent->id() << "has no avatar";
        avatarData = AvatarData();
        emit parent->avatarDataChanged(avatarData);
        return;
    }

    manager->requestContactAvatar(parent);
}

void Contact::receiveAvatarToken(const QString &token)
{
    setAvatarToken(token);

    if (mPriv->actualFeatures.contains(FeatureAvatarData)) {
        mPriv->updateAvatarData();
    }
}

void Contact::receiveAvatarData(const AvatarData &avatar)
{
    mPriv->avatarData = avatar;
    emit avatarDataChanged(mPriv->avatarData);
}

void Contact::receiveSimplePresence(const SimplePresence &presence)
{
    if (!mPriv->requestedFeatures.contains(FeatureSimplePresence)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureSimplePresence);

    if (mPriv->presence.status() != presence.status ||
        mPriv->presence.statusMessage() != presence.statusMessage) {
        mPriv->presence.setStatus(presence);
        emit presenceChanged(mPriv->presence);
    }
}

void Contact::receiveCapabilities(const RequestableChannelClassList &caps)
{
    if (!mPriv->requestedFeatures.contains(FeatureCapabilities)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureCapabilities);

    if (mPriv->caps.allClassSpecs().bareClasses() != caps) {
        mPriv->caps.updateRequestableChannelClasses(caps);
        emit capabilitiesChanged(mPriv->caps);
    }
}

void Contact::receiveLocation(const QVariantMap &location)
{
    if (!mPriv->requestedFeatures.contains(FeatureLocation)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureLocation);

    if (mPriv->location.allDetails() != location) {
        mPriv->location.updateData(location);
        emit locationUpdated(mPriv->location);
    }
}

void Contact::receiveInfo(const ContactInfoFieldList &info)
{
    if (!mPriv->requestedFeatures.contains(FeatureInfo)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureInfo);

    if (mPriv->info.allFields() != info) {
        mPriv->info = InfoFields(info);
        emit infoFieldsChanged(mPriv->info);
    }
}

void Contact::setSubscriptionState(Contact::PresenceState state)
{
    if (mPriv->subscriptionState == state) {
        return;
    }
    mPriv->subscriptionState = state;
    emit subscriptionStateChanged(state);
}

void Contact::setPublishState(Contact::PresenceState state)
{
    if (mPriv->publishState == state) {
        return;
    }
    mPriv->publishState = state;
    emit publishStateChanged(state);
}

void Contact::setBlocked(bool value)
{
    if (mPriv->blocked == value) {
        return;
    }
    mPriv->blocked = value;
    emit blockStatusChanged(value);
}

void Contact::setAddedToGroup(const QString &group)
{
    if (!mPriv->groups.contains(group)) {
        mPriv->groups.insert(group);
        emit addedToGroup(group);
    }
}

void Contact::setRemovedFromGroup(const QString &group)
{
    if (mPriv->groups.remove(group)) {
        emit removedFromGroup(group);
    }
}

/**
 * \fn void Contact::presenceChanged(const Tp::Presence &presence)
 *
 * This signal is emitted when the value of presence() of this contact changes.
 *
 * \param presence The new presence.
 * \sa presence()
 */

/**
 * \fn void Contact::infoFieldsChanged(const Tp::Contact::InfoFields &infoFields)
 *
 * This signal is emitted when the value of infoFields() of this contact changes.
 *
 * \param InfoFields The new info.
 * \sa infoFields()
 */

} // Tp
