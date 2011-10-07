/**
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/Contact>

#include "TelepathyQt4/_gen/contact.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/AvatarData>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ContactCapabilities>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/LocationInfo>
#include <TelepathyQt4/PendingContactInfo>
#include <TelepathyQt4/PendingVoid>
#include <TelepathyQt4/Presence>
#include <TelepathyQt4/ReferencedHandles>

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
          isContactInfoKnown(false), isAvatarTokenKnown(false),
          subscriptionState(SubscriptionStateUnknown),
          publishState(SubscriptionStateUnknown),
          blocked(false)
    {
    }

    void updateAvatarData();

    Contact *parent;

    QWeakPointer<ContactManager> manager;
    ReferencedHandles handle;
    QString id;

    Features requestedFeatures;
    Features actualFeatures;

    QString alias;
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

    ContactPtr self = parent->manager()->lookupContactByHandle(handle[0]);
    parent->manager()->requestContactAvatars(QList<ContactPtr>() << self);
}

struct TELEPATHY_QT4_NO_EXPORT Contact::InfoFields::Private : public QSharedData
{
    Private(const ContactInfoFieldList &allFields)
        : allFields(allFields) {}

    ContactInfoFieldList allFields;
};

/**
 * \class Contact::InfoFields
 * \ingroup clientconn
 * \headerfile TelepathyQt4/contact.h <TelepathyQt4/Contact>
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
 * \headerfile TelepathyQt4/contact.h <TelepathyQt4/Contact>
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
 * See alias specific methods' documentation for more details.
 *
 * \sa alias(), aliasChanged()
 */
const Feature Contact::FeatureAlias = Feature(QLatin1String(Contact::staticMetaObject.className()), 0, false);

/**
 * Feature used in order to access contact avatar data info.
 *
 * Enabling this feature will also enable FeatureAvatarToken.
 *
 * See avatar data specific methods' documentation for more details.
 *
 * \sa avatarData(), avatarDataChanged()
 */
const Feature Contact::FeatureAvatarData = Feature(QLatin1String(Contact::staticMetaObject.className()), 1, false);

/**
 * Feature used in order to access contact avatar token info.
 *
 * See avatar token specific methods' documentation for more details.
 *
 * \sa isAvatarTokenKnown(), avatarToken(), avatarTokenChanged()
 */
const Feature Contact::FeatureAvatarToken = Feature(QLatin1String(Contact::staticMetaObject.className()), 2, false);

/**
 * Feature used in order to access contact capabilities info.
 *
 * See capabilities specific methods' documentation for more details.
 *
 * \sa capabilities(), capabilitiesChanged()
 */
const Feature Contact::FeatureCapabilities = Feature(QLatin1String(Contact::staticMetaObject.className()), 3, false);

/**
 * Feature used in order to access contact info fields.
 *
 * See info fields specific methods' documentation for more details.
 *
 * \sa infoFields(), infoFieldsChanged()
 */
const Feature Contact::FeatureInfo = Feature(QLatin1String(Contact::staticMetaObject.className()), 4, false);

/**
 * Feature used in order to access contact location info.
 *
 * See location specific methods' documentation for more details.
 *
 * \sa location(), locationUpdated()
 */
const Feature Contact::FeatureLocation = Feature(QLatin1String(Contact::staticMetaObject.className()), 5, false);

/**
 * Feature used in order to access contact presence info.
 *
 * See presence specific methods' documentation for more details.
 *
 * \sa presence(), presenceChanged()
 */
const Feature Contact::FeatureSimplePresence = Feature(QLatin1String(Contact::staticMetaObject.className()), 6, false);

/**
 * Feature used in order to access contact roster groups.
 *
 * See roster groups specific methods' documentation for more details.
 *
 * \sa groups(), addedToGroup(), removedFromGroup()
 */
const Feature Contact::FeatureRosterGroups = Feature(QLatin1String(Contact::staticMetaObject.className()), 7, false);

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
            QLatin1String(TELEPATHY_INTERFACE_CONNECTION "/contact-id")]);
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

    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->requestContactAvatars(QList<ContactPtr>() << self);
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
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE),
                QLatin1String("FeatureInfo needs to be ready in order to "
                    "use this method"),
                ContactPtr(this));
    }

    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->refreshContactsInfo(QList<ContactPtr>() << self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return new PendingContactInfo(self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->requestPresenceSubscription(QList<ContactPtr >() << self, message);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->removePresenceSubscription(QList<ContactPtr>() << self, message);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->authorizePresencePublication(QList<ContactPtr>() << self, message);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->removePresencePublication(QList<ContactPtr>() << self, message);
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
 * \deprecated Use block() instead.
 */
PendingOperation *Contact::block(bool value)
{
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return value ? manager()->blockContacts(QList<ContactPtr>() << self)
                 : manager()->unblockContacts(QList<ContactPtr>() << self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->blockContacts(QList<ContactPtr>() << self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->blockContactsAndReportAbuse(QList<ContactPtr>() << self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->unblockContacts(QList<ContactPtr>() << self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->addContactsToGroup(group, QList<ContactPtr>() << self);
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
    ContactPtr self = manager()->lookupContactByHandle(mPriv->handle[0]);
    return manager()->removeContactsFromGroup(group, QList<ContactPtr>() << self);
}

void Contact::augment(const Features &requestedFeatures, const QVariantMap &attributes)
{
    mPriv->requestedFeatures.unite(requestedFeatures);

    mPriv->id = qdbus_cast<QString>(attributes[
            QLatin1String(TELEPATHY_INTERFACE_CONNECTION "/contact-id")]);

    if (attributes.contains(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_LIST +
                QLatin1String("/subscribe"))) {
        uint subscriptionState = qdbus_cast<uint>(attributes.value(
                     TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")));
        setSubscriptionState((SubscriptionState) subscriptionState);
    }

    if (attributes.contains(TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_LIST +
                QLatin1String("/publish"))) {
        uint publishState = qdbus_cast<uint>(attributes.value(
                    TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")));
        QString publishRequest = qdbus_cast<QString>(attributes.value(
                    TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish-request")));
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
                        QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING "/alias")));

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
        } else if (feature == FeatureCapabilities) {
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
        } else if (feature == FeatureInfo) {
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
        } else if (feature == FeatureLocation) {
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
        } else if (feature == FeatureSimplePresence) {
            maybePresence = qdbus_cast<SimplePresence>(attributes.value(
                        QLatin1String(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE "/presence")));

            if (!maybePresence.status.isEmpty()) {
                receiveSimplePresence(maybePresence);
            } else {
                mPriv->presence.setStatus(ConnectionPresenceTypeUnknown,
                        QLatin1String("unknown"), QLatin1String(""));
            }
        } else if (feature == FeatureRosterGroups) {
            QStringList groups = qdbus_cast<QStringList>(attributes.value(
                        TP_QT4_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS + QLatin1String("/groups")));
            mPriv->groups = groups.toSet();
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

    // FIXME (API/ABI break) remove signal with details
    emit subscriptionStateChanged(subscriptionStateToPresenceState(state),
            Channel::GroupMemberChangeDetails());

    emit subscriptionStateChanged(subscriptionStateToPresenceState(state));
}

void Contact::setPublishState(SubscriptionState state, const QString &message)
{
    if (mPriv->publishState == state && mPriv->publishStateMessage == message) {
        return;
    }

    mPriv->publishState = state;
    mPriv->publishStateMessage = message;

    // FIXME (API/ABI break) remove signal with details
    QVariantMap detailsMap;
    detailsMap.insert(QLatin1String("message"), message);
    emit publishStateChanged(subscriptionStateToPresenceState(state),
            Channel::GroupMemberChangeDetails(ContactPtr(), detailsMap));

    emit publishStateChanged(subscriptionStateToPresenceState(state), message);
}

void Contact::setBlocked(bool value)
{
    if (mPriv->blocked == value) {
        return;
    }

    mPriv->blocked = value;

    // FIXME (API/ABI break) remove signal with details
    emit blockStatusChanged(value, Channel::GroupMemberChangeDetails());

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
 * \param caps The new location of this contact.
 * \sa location()
 */

/**
 * \fn void Contact::infoFieldsChanged(const Tp::Contact::InfoFields &infoFields)
 *
 * Emitted when the value of infoFields() changes.
 *
 * \param InfoFields The new info of this contact.
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
 * \fn void Contact::subscriptionStateChanged(Tp::Contact::PresenceState state,
 *          const Tp::Channel::GroupMemberChangeDetails &details)
 *
 * \deprecated Use subscriptionStateChanged(Tp::Contact::PresenceState state) instead.
 */

/**
 * \fn void Contact::publishStateChanged(Tp::Contact::PresenceState state, const QString &message)
 *
 * Emitted when the value of publishState() changes.
 *
 * \param state The new publish state of this contact.
 * \sa publishState()
 */

/**
 * \fn void Contact::publishStateChanged(Tp::Contact::PresenceState state,
 *          const Tp::Channel::GroupMemberChangeDetails &details)
 *
 * \deprecated Use publishStateChanged(Tp::Contact::PresenceState state, const QString &message) instead.
 */

/**
 * \fn void Contact::blockStatusChanged(bool blocked)
 *
 * Emitted when the value of isBlocked() changes.
 *
 * \param status The new block status of this contact.
 * \sa isBlocked()
 */

/**
 * \fn void Contact::blockStatusChanged(bool blocked,
 *          const Tp::Channel::GroupMemberChangeDetails &details)
 *
 * \deprecated Use blockStatusChanged(bool blocked) instead.
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

void Contact::connectNotify(const char *signalName)
{
    if (qstrcmp(signalName, SIGNAL(subscriptionStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails))) == 0) {
        warning() << "Connecting to deprecated signal subscriptionStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails)";
    } else if (qstrcmp(signalName, SIGNAL(publishStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails))) == 0) {
        warning() << "Connecting to deprecated signal publishStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails)";
    } else if (qstrcmp(signalName, SIGNAL(blockStatusChanged(bool,Tp::Channel::GroupMemberChangeDetails))) == 0) {
        warning() << "Connecting to deprecated signal blockStatusChanged(bool,Tp::Channel::GroupMemberChangeDetails)";
    }
}

} // Tp
