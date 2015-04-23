/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#include <TelepathyQt/Contact>

#include "TelepathyQt/_gen/contact.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/future-internal.h"

#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/LocationInfo>
#include <TelepathyQt/PendingContactInfo>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/Presence>
#include <TelepathyQt/ReferencedHandles>

namespace Tp
{

struct TP_QT_NO_EXPORT Contact::Private
{
    Private(Contact *parent, ContactManager *manager,
        const ReferencedHandles &handle)
        : parent(parent),
          manager(ContactManagerPtr(manager)),
          handle(handle),
          caps(manager->supportedFeatures().contains(Contact::FeatureCapabilities) ?
                   ContactCapabilities(true) :  ContactCapabilities(
                           manager->connection()->capabilities().allClassSpecs(), false)),
          isContactInfoKnown(false), isAvatarTokenKnown(false),
          subscriptionState(SubscriptionStateUnknown),
          publishState(SubscriptionStateUnknown),
          blocked(false)
    {
    }

    void updateAvatarData();

    Contact *parent;

    WeakPtr<ContactManager> manager;
    ReferencedHandles handle;
    QString id;

    Features requestedFeatures;
    Features actualFeatures;

    QString alias;
    QMap<QString, QString> vcardAddresses;
    QStringList uris;
    Presence presence;
    ContactCapabilities caps;
    LocationInfo location;

    bool isContactInfoKnown;
    InfoFields info;

    bool isAvatarTokenKnown;
    QString avatarToken;
    AvatarData avatarData;

    SubscriptionState subscriptionState;
    SubscriptionState publishState;
    QString publishStateMessage;
    bool blocked;

    QSet<QString> groups;

    QStringList clientTypes;
};

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

    parent->manager()->requestContactAvatars(QList<ContactPtr>() << ContactPtr(parent));
}

struct TP_QT_NO_EXPORT Contact::InfoFields::Private : public QSharedData
{
    Private(const ContactInfoFieldList &allFields)
        : allFields(allFields) {}

    ContactInfoFieldList allFields;
};

/**
 * \class Contact::InfoFields
 * \ingroup clientconn
 * \headerfile TelepathyQt/contact.h <TelepathyQt/Contact>
 *
 * \brief The Contact::InfoFields class represents the information of a
 * Telepathy contact.
 */

/**
 * Construct a info fields instance with the given fields. The instance will indicate that
 * it is valid.
 */
Contact::InfoFields::InfoFields(const ContactInfoFieldList &allFields)
    : mPriv(new Private(allFields))
{
}

/**
 * Constructs a new invalid InfoFields instance.
 */
Contact::InfoFields::InfoFields()
{
}

/**
 * Copy constructor.
 */
Contact::InfoFields::InfoFields(const Contact::InfoFields &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
Contact::InfoFields::~InfoFields()
{
}

/**
 * Assignment operator.
 */
Contact::InfoFields &Contact::InfoFields::operator=(const Contact::InfoFields &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

/**
 * Return a list containing all fields whose name are \a name.
 *
 * \param name The name used to match the fields.
 * \return A list of ContactInfoField objects.
 */
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

/**
 * Return a list containing all fields describing the contact information.
 *
 * \return The contact information as a list of ContactInfoField objects.
 */
ContactInfoFieldList Contact::InfoFields::allFields() const
{
    return isValid() ? mPriv->allFields : ContactInfoFieldList();
}

/**
 * \class Contact
 * \ingroup clientconn
 * \headerfile TelepathyQt/contact.h <TelepathyQt/Contact>
 *
 * \brief The Contact class represents a Telepathy contact.
 *
 * The accessor functions on this object (id(), alias(), and so on) don't make any D-Bus calls;
 * instead, they return/use values cached from a previous introspection run.
 * The introspection process populates their values in the most efficient way possible based on what
 * the service implements.
 *
 * To avoid unnecessary D-Bus traffic, some accessors only return valid
 * information after specific features have been enabled.
 * For instance, to retrieve the contact avatar token, it is necessary to
 * enable the feature Contact::FeatureAvatarToken.
 * See the individual methods descriptions for more details.
 *
 * Contact features can be enabled by constructing a ContactFactory and enabling
 * the desired features, and passing it to AccountManager, Account or ClientRegistrar
 * when creating them as appropriate. However, if a particular
 * feature is only ever used in a specific circumstance, such as an user opening
 * some settings dialog separate from the general view of the application,
 * features can be later enabled as needed by calling ContactManager::upgradeContacts() with the
 * additional features, and waiting for the resulting PendingOperation to finish.
 *
 * As an addition to accessors, signals are emitted to indicate that properties have
 * changed, for example aliasChanged(), avatarTokenChanged(), etc.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature used in order to access contact alias info.
 *
 * \sa alias(), aliasChanged()
 */
const Feature Contact::FeatureAlias = Feature(QLatin1String(Contact::staticMetaObject.className()), 0, false);

/**
 * Feature used in order to access contact avatar data info.
 *
 * Enabling this feature will also enable FeatureAvatarToken.
 *
 * \sa avatarData(), avatarDataChanged()
 */
const Feature Contact::FeatureAvatarData = Feature(QLatin1String(Contact::staticMetaObject.className()), 1, false);

/**
 * Feature used in order to access contact avatar token info.
 *
 * \sa isAvatarTokenKnown(), avatarToken(), avatarTokenChanged()
 */
const Feature Contact::FeatureAvatarToken = Feature(QLatin1String(Contact::staticMetaObject.className()), 2, false);

/**
 * Feature used in order to access contact capabilities info.
 *
 * \sa capabilities(), capabilitiesChanged()
 */
const Feature Contact::FeatureCapabilities = Feature(QLatin1String(Contact::staticMetaObject.className()), 3, false);

/**
 * Feature used in order to access contact info fields.
 *
 * \sa infoFields(), infoFieldsChanged()
 */
const Feature Contact::FeatureInfo = Feature(QLatin1String(Contact::staticMetaObject.className()), 4, false);

/**
 * Feature used in order to access contact location info.
 *
 * \sa location(), locationUpdated()
 */
const Feature Contact::FeatureLocation = Feature(QLatin1String(Contact::staticMetaObject.className()), 5, false);

/**
 * Feature used in order to access contact presence info.
 *
 * \sa presence(), presenceChanged()
 */
const Feature Contact::FeatureSimplePresence = Feature(QLatin1String(Contact::staticMetaObject.className()), 6, false);

/**
 * Feature used in order to access contact roster groups.
 *
 * \sa groups(), addedToGroup(), removedFromGroup()
 */
const Feature Contact::FeatureRosterGroups = Feature(QLatin1String(Contact::staticMetaObject.className()), 7, false);

/**
 * Feature used in order to access contact addressable addresses info.
 *
 * \sa vcardAddresses(), uris()
 */
const Feature Contact::FeatureAddresses = Feature(QLatin1String(Contact::staticMetaObject.className()), 8, false);

/**
 * Feature used in order to access contact client types info.
 *
 * \sa clientTypes(), requestClientTypes(), clientTypesChanged()
 */
const Feature Contact::FeatureClientTypes = Feature(QLatin1String(Contact::staticMetaObject.className()), 9, false);

/**
 * Construct a new Contact object.
 *
 * \param manager ContactManager owning this contact.
 * \param handle The contact handle.
 * \param requestedFeatures The contact requested features.
 * \param attributes The contact attributes.
 */
Contact::Contact(ContactManager *manager, const ReferencedHandles &handle,
        const Features &requestedFeatures, const QVariantMap &attributes)
    : Object(),
      mPriv(new Private(this, manager, handle))
{
    mPriv->requestedFeatures.unite(requestedFeatures);
    mPriv->id = qdbus_cast<QString>(attributes[
            TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")]);
}

/**
 * Class destructor.
 */
Contact::~Contact()
{
    debug() << "Contact" << id() << "destroyed";
    delete mPriv;
}

/**
 * Return the contact nanager owning this contact.
 *
 * \return A pointer to the ContactManager object.
 */
ContactManagerPtr Contact::manager() const
{
    return ContactManagerPtr(mPriv->manager);
}

/**
 * Return the handle of this contact.
 *
 * \return The handle as a ReferencedHandles object.
 */
ReferencedHandles Contact::handle() const
{
    return mPriv->handle;
}

/**
 * Return the identifier of this contact.
 *
 * \return The identifier.
 */
QString Contact::id() const
{
    return mPriv->id;
}

/**
 * Return the features requested on this contact.
 *
 * \return The requested features as a set of Feature objects.
 */
Features Contact::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

/**
 * Return the features that are actually enabled on this contact.
 *
 * \return The actual features as a set of Feature objects.
 */
Features Contact::actualFeatures() const
{
    return mPriv->actualFeatures;
}

/**
 * Return the alias of this contact.
 *
 * Change notification is via the aliasChanged() signal.
 *
 * This method requires Contact::FeatureAlias to be ready.
 *
 * \return The alias.
 */
QString Contact::alias() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAlias)) {
        warning() << "Contact::alias() used on" << this
            << "for which FeatureAlias hasn't been requested - returning id";
        return id();
    }

    return mPriv->alias;
}

/**
 * Return the various vcard addresses that identify this contact.
 *
 * This method requires Contact::FeatureAddresses to be ready.
 *
 * \return The vcard addresses identifying this contact.
 * \sa ContactManager::contactsForVCardAddresses(), uris()
 */
QMap<QString, QString> Contact::vcardAddresses() const
{
    return mPriv->vcardAddresses;
}

/**
 * Return the various URI addresses that identify this contact.
 *
 * This method requires Contact::FeatureAddresses to be ready.
 *
 * \return The URI addresses identifying this contact.
 * \sa ContactManager::contactsForUris(), vcardAddresses()
 */
QStringList Contact::uris() const
{
    return mPriv->uris;
}

/**
 * Return whether the avatar token of this contact is known.
 *
 * This method requires Contact::FeatureAvatarToken to be ready.
 *
 * \return \c true if the avatar token is known, \c false otherwise.
 * \sa avatarToken()
 */
bool Contact::isAvatarTokenKnown() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarToken)) {
        warning() << "Contact::isAvatarTokenKnown() used on" << this
            << "for which FeatureAvatarToken hasn't been requested - returning false";
        return false;
    }

    return mPriv->isAvatarTokenKnown;
}

/**
 * Return the avatar token for this contact.
 *
 * Change notification is via the avatarTokenChanged() signal.
 *
 * This method requires Contact::FeatureAvatarToken to be ready.
 *
 * \return The avatar token.
 * \sa isAvatarTokenKnown(), avatarTokenChanged(), avatarData()
 */
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

/**
 * Return the actual avatar for this contact.
 *
 * Change notification is via the avatarDataChanged() signal.
 *
 * This method requires Contact::FeatureAvatarData to be ready.
 *
 * \return The avatar as an AvatarData object.
 * \sa avatarDataChanged(), avatarToken()
 */
AvatarData Contact::avatarData() const
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarData)) {
        warning() << "Contact::avatarData() used on" << this
            << "for which FeatureAvatarData hasn't been requested - returning \"\"";
        return AvatarData();
    }

    return mPriv->avatarData;
}

/**
 * Start a request to retrieve the avatar for this contact.
 *
 * Force the request of the avatar data. This method returns directly, emitting
 * avatarTokenChanged() and avatarDataChanged() signals once the token and data are
 * fetched from the server.
 *
 * This is only useful if the avatar token is unknown; see isAvatarTokenKnown().
 * It happens in the case of offline XMPP contacts, because the server does not
 * send the token for them and an explicit request of the avatar data is needed.
 *
 * This method requires Contact::FeatureAvatarData to be ready.
 *
 * \sa avatarData(), avatarDataChanged(), avatarToken(), avatarTokenChanged()
 */
void Contact::requestAvatarData()
{
    if (!mPriv->requestedFeatures.contains(FeatureAvatarData)) {
        warning() << "Contact::requestAvatarData() used on" << this
            << "for which FeatureAvatarData hasn't been requested - returning \"\"";
        return;
    }

    return manager()->requestContactAvatars(QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Return the actual presence of this contact.
 *
 * Change notification is via the presenceChanged() signal.
 *
 * This method requires Contact::FeatureSimplePresence to be ready.
 *
 * \return The presence as a Presence object.
 */
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
 * If ContactManager::supportedFeatures() contains Contact::FeatureCapabilities,
 * the returned object will be a ContactCapabilities object, where
 * CapabilitiesBase::isSpecificToContact() will be \c true; if that feature
 * isn't present, this returned object is the subset of
 * Contact::manager()::connection()::capabilities()
 * and CapabilitiesBase::isSpecificToContact() will be \c false.
 *
 * Change notification is via the capabilitiesChanged() signal.
 *
 * This method requires Contact::FeatureCapabilities to be ready.
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
 * Change notification is via the locationUpdated() signal.
 *
 * This method requires Contact::FeatureLocation to be ready.
 *
 * \return The contact location as a LocationInfo object.
 */
LocationInfo Contact::location() const
{
    if (!mPriv->requestedFeatures.contains(FeatureLocation)) {
        warning() << "Contact::location() used on" << this
            << "for which FeatureLocation hasn't been requested - returning 0";
        return LocationInfo();
    }

    return mPriv->location;
}

/**
 * Return whether the info card for this contact has been received.
 *
 * With some protocols (notably XMPP) information is not pushed from the server
 * and must be requested explicitely using refreshInfo() or requestInfo(). This
 * method can be used to know if the information is received from the server
 * or if an explicit request is needed.
 *
 * This method requires Contacat::FeatureInfo to be ready.
 *
 * \return \c true if the information is known; \c false otherwise.
 */
bool Contact::isContactInfoKnown() const
{
    if (!mPriv->requestedFeatures.contains(FeatureInfo)) {
        warning() << "Contact::isContactInfoKnown() used on" << this
            << "for which FeatureInfo hasn't been requested - returning false";
        return false;
    }

    return mPriv->isContactInfoKnown;
}

/**
 * Return the information for this contact.
 *
 * Note that this method only return cached information. In order to refresh the
 * information use refreshInfo().
 *
 * Change notification is via the infoFieldsChanged() signal.
 *
 * This method requires Contact::FeatureInfo to be ready.
 *
 * \return The contact info as a Contact::InfoFields object.
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
 * This method requires Contact::FeatureInfo to be ready.
 *
 * \return A PendingOperation, which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa infoFieldsChanged()
 */
PendingOperation *Contact::refreshInfo()
{
    ConnectionPtr conn = manager()->connection();
    if (!mPriv->requestedFeatures.contains(FeatureInfo)) {
        warning() << "Contact::refreshInfo() used on" << this
            << "for which FeatureInfo hasn't been requested - failing";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("FeatureInfo needs to be ready in order to "
                    "use this method"),
                ContactPtr(this));
    }

    return manager()->refreshContactInfo(QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Start a request to retrieve the information for this contact.
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
    return new PendingContactInfo(ContactPtr(this));
}

/**
 * Return whether the presence subscription state of this contact is known.
 *
 * \return \c true if the presence subscription state is known, \c false otherwise.
 * \sa subscriptionState(), isSubscriptionRejected()
 */
bool Contact::isSubscriptionStateKnown() const
{
    return mPriv->subscriptionState != SubscriptionStateUnknown;
}

/**
 * Return whether a request to see this contact's presence was denied.
 *
 * \return \c if the a request to see the presence subscription was denied, \c false otherwise.
 * \sa isSubscriptionStateKnown(), subscriptionState()
 */
bool Contact::isSubscriptionRejected() const
{
    return mPriv->subscriptionState == SubscriptionStateRemovedRemotely;
}

/**
 * Return the presence subscription state of this contact (i.e. whether the local user can retrieve
 * information about this contact's presence).
 *
 * \return The presence subscription state as Contact::PresenceState.
 * \sa isSubscriptionStateKnown(), isSubscriptionRejected()
 */
Contact::PresenceState Contact::subscriptionState() const
{
    return subscriptionStateToPresenceState(mPriv->subscriptionState);
}

/**
 * Return whether the presence publish state of this contact is known.
 *
 * \return \c true if the presence publish state is known, \c false otherwise.
 * \sa publishState(), isPublishCancelled()
 */
bool Contact::isPublishStateKnown() const
{
    return mPriv->publishState != SubscriptionStateUnknown;
}

/**
 * Return whether a request to publish presence information to this contact was cancelled.
 *
 * \return \c true if a request to publish presence information was cancelled,
 *         \c false otherwise.
 * \sa isPublishStateKnown(), publishState()
 */
bool Contact::isPublishCancelled() const
{
    return mPriv->publishState == SubscriptionStateRemovedRemotely;
}

/**
 * Return the presence publish state of this contact (i.e. whether this contact can retrieve
 * information about the local user's presence).
 *
 * \return The presence publish state as Contact::PresenceState.
 * \sa isSubscriptionStateKnown(), isSubscriptionRejected()
 */
Contact::PresenceState Contact::publishState() const
{
    return subscriptionStateToPresenceState(mPriv->publishState);
}

/**
 * If the publishState() is Contact::PresenceStateAsk, return an optional message that was sent
 * by the contact asking to receive the local user's presence; omitted if none was given.
 *
 * \return The message that was sent by the contact asking to receive the local user's presence.
 * \sa publishState()
 */
QString Contact::publishStateMessage() const
{
    return mPriv->publishStateMessage;
}

/**
 * Start a request that this contact allow the local user to subscribe to their presence (i.e. that
 * this contact's subscribe attribute becomes Contact::PresenceStateYes)
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa subscriptionState(), removePresenceSubscription()
 */
PendingOperation *Contact::requestPresenceSubscription(const QString &message)
{
    return manager()->requestPresenceSubscription(QList<ContactPtr>() << ContactPtr(this), message);
}

/**
 * Start a request for the local user to stop receiving presence from this contact.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa subscriptionState(), requestPresenceSubscription()
 */
PendingOperation *Contact::removePresenceSubscription(const QString &message)
{
    return manager()->removePresenceSubscription(QList<ContactPtr>() << ContactPtr(this), message);
}

/**
 * Start a request to authorize this contact's request to see the local user presence
 * (i.e. that this contact publish attribute becomes Contact::PresenceStateYes).
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa publishState(), removePresencePublication()
 */
PendingOperation *Contact::authorizePresencePublication(const QString &message)
{
    return manager()->authorizePresencePublication(QList<ContactPtr>() << ContactPtr(this), message);
}

/**
 * Start a request for the local user to stop sending presence to this contact.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request has been made.
 * \sa publishState(), authorizePresencePublication()
 */
PendingOperation *Contact::removePresencePublication(const QString &message)
{
    return manager()->removePresencePublication(QList<ContactPtr>() << ContactPtr(this), message);
}

/**
 * Return whether this contact is blocked.
 *
 * Change notification is via the blockStatusChanged() signal.
 *
 * \return \c true if blocked, \c false otherwise.
 * \sa block()
 */
bool Contact::isBlocked() const
{
    return mPriv->blocked;
}

/**
 * Block this contact. Blocked contacts cannot send messages to the user;
 * depending on the protocol, blocking a contact may have other effects.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to take the requested action.
 * \sa blockAndReportAbuse(), unblock()
 */
PendingOperation *Contact::block()
{
    return manager()->blockContacts(QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Block this contact and additionally report abusive behaviour
 * to the server.
 *
 * If reporting abusive behaviour is not supported by the protocol,
 * this method has the same effect as block().
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to take the requested action.
 * \sa ContactManager::canReportAbuse(), block(), unblock()
 */
PendingOperation *Contact::blockAndReportAbuse()
{
    return manager()->blockContactsAndReportAbuse(QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Unblock this contact.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to take the requested action.
 * \sa block(), blockAndReportAbuse()
 */
PendingOperation *Contact::unblock()
{
    return manager()->unblockContacts(QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Return the names of the user-defined roster groups to which the contact
 * belongs.
 *
 * Change notification is via the addedToGroup() and removedFromGroup() signals.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \return A list of user-defined contact list groups names.
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
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to to add the contact to the user-defined contact
 *         list group.
 * \sa groups(), removeFromGroup()
 */
PendingOperation *Contact::addToGroup(const QString &group)
{
    return manager()->addContactsToGroup(group, QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Attempt to remove the contact from the user-defined contact list
 * group named \a group.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to to remote the contact to the user-defined contact
 *         list group.
 * \sa groups(), addToGroup()
 */
PendingOperation *Contact::removeFromGroup(const QString &group)
{
    return manager()->removeContactsFromGroup(group, QList<ContactPtr>() << ContactPtr(this));
}

/**
 * Return the client types of this contact, if known.
 *
 * Client types are represented using the values documented by the XMPP registrar,
 * with some additional types. A contact can set one or more client types, or can simply
 * advertise itself as unknown - in this case, an empty list is returned.
 *
 * This method returns cached information and is more appropriate for "lazy"
 * client type finding, for instance displaying the client types (if available)
 * of everyone in your contact list. For getting latest up-to-date information from
 * the server you should use requestClientTypes() instead.
 *
 * This method requires FeatureClientTypes to be ready.
 *
 * \return A list of the client types advertised by this contact.
 * \sa requestClientTypes(), clientTypesChanged()
 */
QStringList Contact::clientTypes() const
{
    if (!mPriv->requestedFeatures.contains(FeatureClientTypes)) {
        warning() << "Contact::clientTypes() used on" << this
            << "for which FeatureClientTypes hasn't been requested - returning an empty list";
        return QStringList();
    }

    return mPriv->clientTypes;
}

/**
 * Return the current client types of the given contact.
 *
 * If necessary, this method will make a request to the server for up-to-date
 * information and wait for a reply. Therefore, this method is more appropriate
 * for use in a "Contact Information..." dialog; it can be used to show progress
 * information (while waiting for the method to return), and can distinguish
 * between various error conditions.
 *
 * This method requires FeatureClientTypes to be ready.
 *
 * \return A list of the client types advertised by this contact.
 * \sa clientTypes(), clientTypesChanged()
 */
PendingStringList *Contact::requestClientTypes()
{
    if (!mPriv->requestedFeatures.contains(FeatureClientTypes)) {
        warning() << "Contact::requestClientTypes() used on" << this
            << "for which FeatureClientTypes hasn't been requested - the operation will fail";
    }

    Client::ConnectionInterfaceClientTypesInterface *clientTypesInterface =
        manager()->connection()->interface<Client::ConnectionInterfaceClientTypesInterface>();

    return new PendingStringList(
            clientTypesInterface->RequestClientTypes(mPriv->handle.at(0)),
            ContactPtr(this));
}

void Contact::augment(const Features &requestedFeatures, const QVariantMap &attributes)
{
    mPriv->requestedFeatures.unite(requestedFeatures);

    mPriv->id = qdbus_cast<QString>(attributes[
            TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")]);

    if (attributes.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST +
                QLatin1String("/subscribe"))) {
        uint subscriptionState = qdbus_cast<uint>(attributes.value(
                     TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")));
        setSubscriptionState((SubscriptionState) subscriptionState);
    }

    if (attributes.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST +
                QLatin1String("/publish"))) {
        uint publishState = qdbus_cast<uint>(attributes.value(
                    TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")));
        QString publishRequest = qdbus_cast<QString>(attributes.value(
                    TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish-request")));
        setPublishState((SubscriptionState) publishState, publishRequest);
    }

    foreach (const Feature &feature, requestedFeatures) {
        QString maybeAlias;
        SimplePresence maybePresence;
        RequestableChannelClassList maybeCaps;
        QVariantMap maybeLocation;
        ContactInfoFieldList maybeInfo;

        if (feature == FeatureAlias) {
            maybeAlias = qdbus_cast<QString>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING + QLatin1String("/alias")));

            if (!maybeAlias.isEmpty()) {
                receiveAlias(maybeAlias);
            } else if (mPriv->alias.isEmpty()) {
                mPriv->alias = mPriv->id;
            }
        } else if (feature == FeatureAvatarData) {
            if (manager()->supportedFeatures().contains(FeatureAvatarData)) {
                mPriv->actualFeatures.insert(FeatureAvatarData);
                mPriv->updateAvatarData();
            }
        } else if (feature == FeatureAvatarToken) {
            if (attributes.contains(
                        TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS + QLatin1String("/token"))) {
                receiveAvatarToken(qdbus_cast<QString>(attributes.value(
                                TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS + QLatin1String("/token"))));
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
        } else if (feature == FeatureCapabilities) {
            maybeCaps = qdbus_cast<RequestableChannelClassList>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES + QLatin1String("/capabilities")));

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
        } else if (feature == FeatureInfo) {
            maybeInfo = qdbus_cast<ContactInfoFieldList>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_INFO + QLatin1String("/info")));

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
        } else if (feature == FeatureLocation) {
            maybeLocation = qdbus_cast<QVariantMap>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_LOCATION + QLatin1String("/location")));

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
        } else if (feature == FeatureSimplePresence) {
            maybePresence = qdbus_cast<SimplePresence>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE + QLatin1String("/presence")));

            if (!maybePresence.status.isEmpty()) {
                receiveSimplePresence(maybePresence);
            } else {
                mPriv->presence.setStatus(ConnectionPresenceTypeUnknown,
                        QLatin1String("unknown"), QLatin1String(""));
            }
        } else if (feature == FeatureRosterGroups) {
            QStringList groups = qdbus_cast<QStringList>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS + QLatin1String("/groups")));
            mPriv->groups = groups.toSet();
        } else if (feature == FeatureAddresses) {
            VCardFieldAddressMap addresses = qdbus_cast<VCardFieldAddressMap>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING + QLatin1String("/addresses")));
            QStringList uris = qdbus_cast<QStringList>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING + QLatin1String("/uris")));
            receiveAddresses(addresses, uris);
        } else if (feature == FeatureClientTypes) {
            QStringList maybeClientTypes = qdbus_cast<QStringList>(attributes.value(
                        TP_QT_IFACE_CONNECTION_INTERFACE_CLIENT_TYPES + QLatin1String("/client-types")));

            if (!maybeClientTypes.isEmpty()) {
                receiveClientTypes(maybeClientTypes);
            } else {
                if (manager()->supportedFeatures().contains(FeatureClientTypes) &&
                    mPriv->requestedFeatures.contains(FeatureClientTypes)) {
                    // ClientTypes being supported but not updated in the
                    // mapping indicates that the info is not known -
                    // however, the feature is working fine
                    mPriv->actualFeatures.insert(FeatureClientTypes);
                }
            }
        } else {
            warning() << "Unknown feature" << feature << "encountered when augmenting Contact";
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

void Contact::receiveAvatarToken(const QString &token)
{
    setAvatarToken(token);

    if (mPriv->actualFeatures.contains(FeatureAvatarData)) {
        mPriv->updateAvatarData();
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

void Contact::receiveAvatarData(const AvatarData &avatar)
{
    if (mPriv->avatarData.fileName != avatar.fileName) {
        mPriv->avatarData = avatar;
        emit avatarDataChanged(mPriv->avatarData);
    }
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
    mPriv->isContactInfoKnown = true;

    if (mPriv->info.allFields() != info) {
        mPriv->info = InfoFields(info);
        emit infoFieldsChanged(mPriv->info);
    }
}

void Contact::receiveAddresses(const QMap<QString, QString> &addresses,
        const QStringList &uris)
{
    if (!mPriv->requestedFeatures.contains(FeatureAddresses)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureAddresses);
    mPriv->vcardAddresses = addresses;
    mPriv->uris = uris;
}

void Contact::receiveClientTypes(const QStringList &clientTypes)
{
    if (!mPriv->requestedFeatures.contains(FeatureClientTypes)) {
        return;
    }

    mPriv->actualFeatures.insert(FeatureClientTypes);

    if (mPriv->clientTypes != clientTypes) {
        mPriv->clientTypes = clientTypes;
        emit clientTypesChanged(mPriv->clientTypes);
    }
}

Contact::PresenceState Contact::subscriptionStateToPresenceState(uint subscriptionState)
{
    switch (subscriptionState) {
        case SubscriptionStateAsk:
            return PresenceStateAsk;
        case SubscriptionStateYes:
            return PresenceStateYes;
        default:
            return PresenceStateNo;
    }
}

void Contact::setSubscriptionState(SubscriptionState state)
{
    if (mPriv->subscriptionState == state) {
        return;
    }

    mPriv->subscriptionState = state;

    emit subscriptionStateChanged(subscriptionStateToPresenceState(state));
}

void Contact::setPublishState(SubscriptionState state, const QString &message)
{
    if (mPriv->publishState == state && mPriv->publishStateMessage == message) {
        return;
    }

    mPriv->publishState = state;
    mPriv->publishStateMessage = message;

    emit publishStateChanged(subscriptionStateToPresenceState(state), message);
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
 * \fn void Contact::aliasChanged(const QString &alias)
 *
 * Emitted when the value of alias() changes.
 *
 * \param alias The new alias of this contact.
 * \sa alias()
 */

/**
 * \fn void Contact::avatarTokenChanged(const QString &avatarToken)
 *
 * Emitted when the value of avatarToken() changes.
 *
 * \param avatarToken The new avatar token of this contact.
 * \sa avatarToken()
 */

/**
 * \fn void Contact::avatarDataChanged(const QString &avatarToken)
 *
 * Emitted when the value of avatarData() changes.
 *
 * \param avatarData The new avatar of this contact.
 * \sa avatarData()
 */

/**
 * \fn void Contact::presenceChanged(const Tp::Presence &presence)
 *
 * Emitted when the value of presence() changes.
 *
 * \param presence The new presence of this contact.
 * \sa presence()
 */

/**
 * \fn void Contact::capabilitiesChanged(const Tp::ContactCapabilities &caps)
 *
 * Emitted when the value of capabilities() changes.
 *
 * \param caps The new capabilities of this contact.
 * \sa capabilities()
 */

/**
 * \fn void Contact::locationUpdated(const Tp::LocationInfo &location)
 *
 * Emitted when the value of location() changes.
 *
 * \param location The new location of this contact.
 * \sa location()
 */

/**
 * \fn void Contact::infoFieldsChanged(const Tp::Contact::InfoFields &infoFields)
 *
 * Emitted when the value of infoFields() changes.
 *
 * \param infoFields The new info of this contact.
 * \sa infoFields()
 */

/**
 * \fn void Contact::subscriptionStateChanged(Tp::Contact::PresenceState state)
 *
 * Emitted when the value of subscriptionState() changes.
 *
 * \param state The new subscription state of this contact.
 * \sa subscriptionState()
 */

/**
 * \fn void Contact::publishStateChanged(Tp::Contact::PresenceState state, const QString &message)
 *
 * Emitted when the value of publishState() changes.
 *
 * \param state The new publish state of this contact.
 * \param message The new user-defined status message of this contact.
 * \sa publishState()
 */

/**
 * \fn void Contact::blockStatusChanged(bool blocked)
 *
 * Emitted when the value of isBlocked() changes.
 *
 * \param blocked The new block status of this contact.
 * \sa isBlocked()
 */

/**
 * \fn void Contact::addedToGroup(const QString &group)
 *
 * Emitted when this contact is added to \a group of the contact list.
 *
 * \param group The group name.
 * \sa groups(), removedFromGroup()
 */

/**
 * \fn void Contact::removedFromGroup(const QString &group)
 *
 * Emitted when this contact is removed from \a group of the contact list.
 *
 * \param group The group name.
 * \sa groups(), addedToGroup()
 */

/**
 * \fn void Contact::clientTypesChanged(const QStringList &clientTypes)
 *
 * Emitted when the client types of this contact change or become known.
 *
 * \param clientTypes The contact's client types
 * \sa clientTypes(), requestClientTypes()
 */

} // Tp
