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

#include <TelepathyQt/ContactManager>
#include "TelepathyQt/contact-manager-internal.h"

#include "TelepathyQt/_gen/contact-manager.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/future-internal.h"

#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContactAttributes>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/Utils>

#include <QMap>

namespace Tp
{

struct TP_QT_NO_EXPORT ContactManager::Private
{
    Private(ContactManager *parent, Connection *connection);
    ~Private();

    // avatar specific methods
    bool buildAvatarFileName(QString token, bool createDir,
        QString &avatarFileName, QString &mimeTypeFileName);
    Features realFeatures(const Features &features);
    QSet<QString> interfacesForFeatures(const Features &features);

    ContactManager *parent;
    WeakPtr<Connection> connection;
    ContactManager::Roster *roster;

    QHash<uint, WeakPtr<Contact> > contacts;

    QHash<Feature, bool> tracking;
    Features supportedFeatures;

    // avatar
    QSet<ContactPtr> requestAvatarsQueue;
    bool requestAvatarsIdle;

    // contact info
    PendingRefreshContactInfo *refreshInfoOp;
};

ContactManager::Private::Private(ContactManager *parent, Connection *connection)
    : parent(parent),
      connection(connection),
      roster(new ContactManager::Roster(parent)),
      requestAvatarsIdle(false),
      refreshInfoOp(0)
{
}

ContactManager::Private::~Private()
{
    delete refreshInfoOp;
    delete roster;
}

bool ContactManager::Private::buildAvatarFileName(QString token, bool createDir,
        QString &avatarFileName, QString &mimeTypeFileName)
{
    QString cacheDir = QString(QLatin1String(qgetenv("XDG_CACHE_HOME")));
    if (cacheDir.isEmpty()) {
        cacheDir = QString(QLatin1String("%1/.cache")).arg(QLatin1String(qgetenv("HOME")));
    }

    ConnectionPtr conn(parent->connection());
    QString path = QString(QLatin1String("%1/telepathy/avatars/%2/%3")).
        arg(cacheDir).arg(conn->cmName()).arg(conn->protocolName());

    if (createDir && !QDir().mkpath(path)) {
        return false;
    }

    avatarFileName = QString(QLatin1String("%1/%2")).arg(path).arg(escapeAsIdentifier(token));
    mimeTypeFileName = QString(QLatin1String("%1.mime")).arg(avatarFileName);

    return true;
}

Features ContactManager::Private::realFeatures(const Features &features)
{
    Features ret(features);
    ret.unite(parent->connection()->contactFactory()->features());
    // FeatureAvatarData depends on FeatureAvatarToken
    if (ret.contains(Contact::FeatureAvatarData) &&
        !ret.contains(Contact::FeatureAvatarToken)) {
        ret.insert(Contact::FeatureAvatarToken);
    }
    return ret;
}

QSet<QString> ContactManager::Private::interfacesForFeatures(const Features &features)
{
    Features supported = parent->supportedFeatures();
    QSet<QString> ret;
    foreach (const Feature &feature, features) {
        parent->ensureTracking(feature);

        if (supported.contains(feature)) {
            // Only query interfaces which are reported as supported to not get an error
            ret.insert(parent->featureToInterface(feature));
        }
    }
    return ret;
}

ContactManager::PendingRefreshContactInfo::PendingRefreshContactInfo(const ConnectionPtr &conn)
    : PendingOperation(conn),
      mConn(conn)
{
}

ContactManager::PendingRefreshContactInfo::~PendingRefreshContactInfo()
{
}

void ContactManager::PendingRefreshContactInfo::addContact(Contact *contact)
{
    mToRequest.insert(contact->handle()[0]);
}

void ContactManager::PendingRefreshContactInfo::refreshInfo()
{
    Q_ASSERT(!mToRequest.isEmpty());

    if (!mConn->isValid()) {
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
        return;
    }

    if (!mConn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_INFO)) {
        setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Connection does not support ContactInfo interface"));
        return;
    }

    debug() << "Calling ContactInfo.RefreshContactInfo for" << mToRequest.size() << "handles";
    Client::ConnectionInterfaceContactInfoInterface *contactInfoInterface =
        mConn->interface<Client::ConnectionInterfaceContactInfoInterface>();
    Q_ASSERT(contactInfoInterface);
    PendingVoid *nested = new PendingVoid(
            contactInfoInterface->RefreshContactInfo(mToRequest.toList()),
            mConn);
    connect(nested,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onRefreshInfoFinished(Tp::PendingOperation*)));
}

void ContactManager::PendingRefreshContactInfo::onRefreshInfoFinished(PendingOperation *op)
{
    if (op->isError()) {
        warning() << "ContactInfo.RefreshContactInfo failed with" <<
            op->errorName() << "-" << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        debug() << "Got reply to ContactInfo.RefreshContactInfo";
        setFinished();
    }
}

/**
 * \class ContactManager
 * \ingroup clientconn
 * \headerfile TelepathyQt/contact-manager.h <TelepathyQt/ContactManager>
 *
 * \brief The ContactManager class is responsible for managing contacts.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Construct a new ContactManager object.
 *
 * \param connection The connection owning this ContactManager.
 */
ContactManager::ContactManager(Connection *connection)
    : Object(),
      mPriv(new Private(this, connection))
{
}

/**
 * Class destructor.
 */
ContactManager::~ContactManager()
{
    delete mPriv;
}

/**
 * Return the connection owning this ContactManager.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr ContactManager::connection() const
{
    return ConnectionPtr(mPriv->connection);
}

/**
 * Return the features that are expected to work on contacts on this ContactManager connection.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \return The supported features as a set of Feature objects.
 */
Features ContactManager::supportedFeatures() const
{
    if (mPriv->supportedFeatures.isEmpty() &&
        connection()->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS)) {
        Features allFeatures = Features()
            << Contact::FeatureAlias
            << Contact::FeatureAvatarToken
            << Contact::FeatureAvatarData
            << Contact::FeatureSimplePresence
            << Contact::FeatureCapabilities
            << Contact::FeatureLocation
            << Contact::FeatureInfo
            << Contact::FeatureRosterGroups
            << Contact::FeatureAddresses
            << Contact::FeatureClientTypes;
        QStringList interfaces = connection()->lowlevel()->contactAttributeInterfaces();
        foreach (const Feature &feature, allFeatures) {
            if (interfaces.contains(featureToInterface(feature))) {
                mPriv->supportedFeatures.insert(feature);
            }
        }

        debug() << mPriv->supportedFeatures.size() << "contact features supported using" << this;
    }

    return mPriv->supportedFeatures;
}

/**
 * Return the progress made in retrieving the contact list.
 *
 * Change notification is via the stateChanged() signal.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return The contact list state as #ContactListState.
 * \sa stateChanged()
 */
ContactListState ContactManager::state() const
{
    return mPriv->roster->state();
}

/**
 * Return a list of relevant contacts (a reasonable guess as to what should
 * be displayed as "the contact list").
 *
 * This may include any or all of: contacts whose presence the user receives,
 * contacts who are allowed to see the user's presence, contacts stored in
 * some persistent contact list on the server, contacts who the user
 * has blocked from communicating with them, or contacts who are relevant
 * in some other way.
 *
 * User interfaces displaying a contact list will probably want to filter this
 * list and display some suitable subset of it.
 *
 * On protocols where there is no concept of presence or a centrally-stored
 * contact list (like IRC), this method may return an empty list.
 *
 * Change notification is via the allKnownContactsChanged() signal.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return A set of pointers to the Contact objects.
 * \sa allKnownContactsChanged()
 */
Contacts ContactManager::allKnownContacts() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        warning() << "Calling allKnownContacts() before FeatureRoster is ready";
        return Contacts();
    }

    return mPriv->roster->allKnownContacts();
}

/**
 * Return a list of user-defined contact list groups' names.
 *
 * Change notification is via the groupAdded(), groupRemoved() and groupRenamed() signals.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \return The list of user-defined contact list groups names.
 * \sa groupMembersChanged(), groupAdded(), groupRemoved(), groupRenamed()
 */
QStringList ContactManager::allKnownGroups() const
{
    if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return QStringList();
    }

    return mPriv->roster->allKnownGroups();
}

/**
 * Attempt to add an user-defined contact list group named \a group.
 *
 * On some protocols (e.g. XMPP) empty groups are not represented on the server,
 * so disconnecting from the server and reconnecting might cause empty groups to
 * vanish.
 *
 * The returned pending operation will finish successfully if the group already
 * exists.
 *
 * Change notification is via the groupAdded() signal.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to add an user-defined contact list group.
 * \sa allKnownGroups(), groupAdded(), addContactsToGroup()
 */
PendingOperation *ContactManager::addGroup(const QString &group)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    return mPriv->roster->addGroup(group);
}

/**
 * Attempt to remove an user-defined contact list group named \a group.
 *
 * Change notification is via the groupRemoved() signal.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \return A PendingOperation which will emit PendingOperation::finished()
 *         when an attempt has been made to remove an user-defined contact list group.
 * \sa allKnownGroups(), groupRemoved(), removeContactsFromGroup()
 */
PendingOperation *ContactManager::removeGroup(const QString &group)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    return mPriv->roster->removeGroup(group);
}

/**
 * Return the contacts in the given user-defined contact list group
 * named \a group.
 *
 * Change notification is via the groupMembersChanged() signal.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \return A set of pointers to the Contact objects, or an empty set if the group does not exist.
 * \sa allKnownGroups(), groupMembersChanged()
 */
Contacts ContactManager::groupContacts(const QString &group) const
{
    if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return Contacts();
    }

    return mPriv->roster->groupContacts(group);
}

/**
 * Attempt to add the given \a contacts to the user-defined contact list
 * group named \a group.
 *
 * Change notification is via the groupMembersChanged() signal.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \param contacts Contacts to add.
 * \return A PendingOperation which will emit PendingOperation::finished()
 *         when an attempt has been made to add the contacts to the user-defined
 *         contact list group.
 * \sa groupMembersChanged(), groupContacts()
 */
PendingOperation *ContactManager::addContactsToGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    return mPriv->roster->addContactsToGroup(group, contacts);
}

/**
 * Attempt to remove the given \a contacts from the user-defined contact list
 * group named \a group.
 *
 * Change notification is via the groupMembersChanged() signal.
 *
 * This method requires Connection::FeatureRosterGroups to be ready.
 *
 * \param group The group name.
 * \param contacts Contacts to remove.
 * \return A PendingOperation which will PendingOperation::finished
 *         when an attempt has been made to remove the contacts from the user-defined
 *         contact list group.
 * \sa groupMembersChanged(), groupContacts()
 */
PendingOperation *ContactManager::removeContactsFromGroup(const QString &group,
        const QList<ContactPtr> &contacts)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRosterGroups)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRosterGroups is not ready"),
                connection());
    }

    return mPriv->roster->removeContactsFromGroup(group, contacts);
}

/**
 * Return whether subscribing to additional contacts' presence is supported.
 *
 * In some protocols, the list of contacts whose presence can be seen is
 * fixed, so we can't subscribe to the presence of additional contacts.
 *
 * Notably, in link-local XMPP, you can see the presence of everyone on the
 * local network, and trying to add more subscriptions would be meaningless.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if Contact::requestPresenceSubscription() and
 *         requestPresenceSubscription() are likely to succeed, \c false otherwise.
 * \sa requestPresenceSubscription(), subscriptionRequestHasMessage()
 */
bool ContactManager::canRequestPresenceSubscription() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canRequestPresenceSubscription();
}

/**
 * Return whether a message can be sent when subscribing to contacts'
 * presence.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if the message argument to Contact::requestPresenceSubscription() and
 *         requestPresenceSubscription() is actually used, \c false otherwise.
 * \sa canRemovePresenceSubscription(), requestPresenceSubscription()
 */
bool ContactManager::subscriptionRequestHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->subscriptionRequestHasMessage();
}

/**
 * Attempt to subscribe to the presence of the given contacts.
 *
 * This operation is sometimes called "adding contacts to the buddy
 * list" or "requesting authorization".
 *
 * On most protocols, the contacts will need to give permission
 * before the user will be able to receive their presence: if so, they will
 * be in presence state Contact::PresenceStateAsk until they authorize
 * or deny the request.
 *
 * The returned PendingOperation will return successfully when a request to
 * subscribe to the contacts' presence has been submitted, or fail if this
 * cannot happen. In particular, it does not wait for the contacts to give
 * permission for the presence subscription.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts whose presence is desired
 * \param message A message from the user which is either transmitted to the
 *                contacts, or ignored, depending on the protocol
 * \return A PendingOperation which will PendingOperation::finished()
 *         when an attempt has been made to subscribe to the contacts' presence.
 * \sa canRequestPresenceSubscription(), subscriptionRequestHasMessage()
 */
PendingOperation *ContactManager::requestPresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    return mPriv->roster->requestPresenceSubscription(contacts, message);
}

/**
 * Return whether the user can stop receiving the presence of a contact
 * whose presence they have subscribed to.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if Contact::removePresenceSubscription() and
 *         removePresenceSubscription() are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateYes,
 *         \c false otherwise.
 * \sa removePresenceSubscription(), subscriptionRemovalHasMessage()
 */
bool ContactManager::canRemovePresenceSubscription() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canRemovePresenceSubscription();
}

/**
 * Return whether a message can be sent when removing an existing subscription
 * to the presence of a contact.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if the message argument to Contact::removePresenceSubscription() and
 *         removePresenceSubscription() is actually used,
 *         for contacts with subscription state Contact::PresenceStateYes,
 *         \c false otherwise.
 * \sa canRemovePresencePublication(), removePresenceSubscription()
 */
bool ContactManager::subscriptionRemovalHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->subscriptionRemovalHasMessage();
}

/**
 * Return whether the user can cancel a request to subscribe to a contact's
 * presence before that contact has responded.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if Contact::removePresenceSubscription() and
 *         removePresenceSubscription() are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateAsk,
 *         \c false otherwise.
 * \sa removePresenceSubscription(), subscriptionRescindingHasMessage()
 */
bool ContactManager::canRescindPresenceSubscriptionRequest() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canRescindPresenceSubscriptionRequest();
}

/**
 * Return whether a message can be sent when cancelling a request to
 * subscribe to the presence of a contact.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if the message argument to Contact::removePresenceSubscription() and
 *         removePresenceSubscription() is actually used,
 *         for contacts with subscription state Contact::PresenceStateAsk,
 *         \c false otherwise.
 * \sa canRescindPresenceSubscriptionRequest(), removePresenceSubscription()
 */
bool ContactManager::subscriptionRescindingHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->subscriptionRescindingHasMessage();
}

/**
 * Attempt to stop receiving the presence of the given contacts, or cancel
 * a request to subscribe to their presence that was previously sent.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts whose presence is no longer required
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A PendingOperation which will PendingOperation::finished()
 *         when an attempt has been made to remove any subscription to the contacts' presence.
 * \sa canRemovePresenceSubscription(), canRescindPresenceSubscriptionRequest(),
 *     subscriptionRemovalHasMessage(), subscriptionRescindingHasMessage()
 */
PendingOperation *ContactManager::removePresenceSubscription(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    return mPriv->roster->removePresenceSubscription(contacts, message);
}

/**
 * Return true if the publication of the user's presence to contacts can be
 * authorized.
 *
 * This is always true, unless the protocol has no concept of authorizing
 * publication (in which case contacts' publication status can never be
 * Contact::PresenceStateAsk).
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if Contact::authorizePresencePublication() and
 *         authorizePresencePublication() are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateAsk,
 *         \c false otherwise.
 * \sa publicationAuthorizationHasMessage(), authorizePresencePublication()
 */
bool ContactManager::canAuthorizePresencePublication() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canAuthorizePresencePublication();
}

/**
 * Return whether a message can be sent when authorizing a request from a
 * contact that the user's presence is published to them.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if the message argument to Contact::authorizePresencePublication() and
 *         authorizePresencePublication() is actually used,
 *         for contacts with subscription state Contact::PresenceStateAsk,
 *         \c false otherwise.
 * \sa canAuthorizePresencePublication(), authorizePresencePublication()
 */
bool ContactManager::publicationAuthorizationHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->publicationAuthorizationHasMessage();
}

/**
 * If the given contacts have asked the user to publish presence to them,
 * grant permission for this publication to take place.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts who should be allowed to receive the user's
 *                 presence
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A PendingOperation which will emit PendingOperation::fininshed
 *         when an attempt has been made to authorize publication of the user's presence
 *         to the contacts.
 * \sa canAuthorizePresencePublication(), publicationAuthorizationHasMessage()
 */
PendingOperation *ContactManager::authorizePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    return mPriv->roster->authorizePresencePublication(contacts, message);
}

/**
 * Return whether a message can be sent when rejecting a request from a
 * contact that the user's presence is published to them.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if the message argument to Contact::removePresencePublication() and
 *         removePresencePublication() is actually used,
 *         for contacts with subscription state Contact::PresenceStateAsk,
 *         \c false otherwise.
 * \sa canRemovePresencePublication(), removePresencePublication()
 */
bool ContactManager::publicationRejectionHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->publicationRejectionHasMessage();
}

/**
 * Return true if the publication of the user's presence to contacts can be
 * removed, even after permission has been given.
 *
 * (Rejecting requests for presence to be published is always allowed.)
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if Contact::removePresencePublication() and
 *         removePresencePublication() are likely to succeed
 *         for contacts with subscription state Contact::PresenceStateYes,
 *         \c false otherwise.
 * \sa publicationRemovalHasMessage(), removePresencePublication()
 */
bool ContactManager::canRemovePresencePublication() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canRemovePresencePublication();
}

/**
 * Return whether a message can be sent when revoking earlier permission
 * that the user's presence is published to a contact.
 *
 * If no message will actually be sent, user interfaces should avoid prompting
 * the user for a message, and use an empty string for the message argument.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if the message argument to Contact::removePresencePublication and
 *         removePresencePublication() is actually used,
 *         for contacts with subscription state Contact::PresenceStateYes,
 *         \c false otherwise.
 * \sa canRemovePresencePublication(), removePresencePublication()
 */
bool ContactManager::publicationRemovalHasMessage() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->publicationRemovalHasMessage();
}

/**
 * If the given contacts have asked the user to publish presence to them,
 * deny this request (this should always succeed, unless a network error
 * occurs).
 *
 * If the given contacts already have permission to receive
 * the user's presence, attempt to revoke that permission (this might not
 * be supported by the protocol - canRemovePresencePublication
 * indicates whether it is likely to succeed).
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts who should no longer be allowed to receive the
 *                 user's presence
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A PendingOperation which will emit PendingOperation::finished()
 *         when an attempt has been made to remove any publication of the user's presence to the
 *         contacts.
 * \sa canRemovePresencePublication(), publicationRejectionHasMessage(),
 *     publicationRemovalHasMessage()
 */
PendingOperation *ContactManager::removePresencePublication(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    return mPriv->roster->removePresencePublication(contacts, message);
}

/**
 * Remove completely contacts from the server. It has the same effect than
 * calling removePresencePublication() and removePresenceSubscription(),
 * but also remove from 'stored' list if it exists.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts who should be removed
 * \message A message from the user which is either transmitted to the
 *          contacts, or ignored, depending on the protocol
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when an attempt has been made to remove any publication of the user's presence to
 *         the contacts.
 */
PendingOperation *ContactManager::removeContacts(
        const QList<ContactPtr> &contacts, const QString &message)
{
    if (!connection()->isValid()) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"),
                connection());
    } else if (!connection()->isReady(Connection::FeatureRoster)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureRoster is not ready"),
                connection());
    }

    return mPriv->roster->removeContacts(contacts, message);
}

/**
 * Return whether this protocol has a list of blocked contacts.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if blockContacts() is likely to succeed, \c false otherwise.
 */
bool ContactManager::canBlockContacts() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canBlockContacts();
}

/**
 * Return whether this protocol can report abusive contacts.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \return \c true if reporting abuse when blocking contacts is supported, \c false otherwise.
 */
bool ContactManager::canReportAbuse() const
{
    if (!connection()->isReady(Connection::FeatureRoster)) {
        return false;
    }

    return mPriv->roster->canReportAbuse();
}

/**
 * Block the given contacts. Blocked contacts cannot send messages
 * to the user; depending on the protocol, blocking a contact may
 * have other effects.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts that should be blocked.
 * \return A PendingOperation which will emit PendingOperation::finished()
 *         when an attempt has been made to take the requested action.
 * \sa canBlockContacts(), unblockContacts(), blockContactsAndReportAbuse()
 */
PendingOperation *ContactManager::blockContacts(const QList<ContactPtr> &contacts)
{
    return mPriv->roster->blockContacts(contacts, true, false);
}

/**
 * Block the given contacts and additionally report abusive behaviour
 * to the server.
 *
 * If reporting abusive behaviour is not supported by the protocol,
 * this method has the same effect as blockContacts().
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts who should be added to the list of blocked contacts.
 * \return A PendingOperation which will emit PendingOperation::finished()
 *         when an attempt has been made to take the requested action.
 * \sa canBlockContacts(), canReportAbuse(), blockContacts()
 */
PendingOperation *ContactManager::blockContactsAndReportAbuse(
        const QList<ContactPtr> &contacts)
{
    return mPriv->roster->blockContacts(contacts, true, true);
}

/**
 * Unblock the given contacts.
 *
 * This method requires Connection::FeatureRoster to be ready.
 *
 * \param contacts Contacts that should be unblocked.
 * \return A PendingOperation which will emit PendingOperation::finished()
 *         when an attempt has been made to take the requested action.
 * \sa canBlockContacts(), blockContacts(), blockContactsAndReportAbuse()
 */
PendingOperation *ContactManager::unblockContacts(const QList<ContactPtr> &contacts)
{
    return mPriv->roster->blockContacts(contacts, false, false);
}

PendingContacts *ContactManager::contactsForHandles(const UIntList &handles,
        const Features &features)
{
    QMap<uint, ContactPtr> satisfyingContacts;
    QSet<uint> otherContacts;
    Features missingFeatures;

    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), handles, features, Features(),
                QStringList(), satisfyingContacts, otherContacts,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), handles, features, Features(),
                QStringList(), satisfyingContacts, otherContacts,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    Features realFeatures = mPriv->realFeatures(features);

    ConnectionLowlevelPtr connLowlevel = connection()->lowlevel();

    if (connLowlevel->hasImmortalHandles() && realFeatures.isEmpty()) {
        // try to avoid a roundtrip if all handles have an id set and no feature was requested
        foreach (uint handle, handles) {
            if (connLowlevel->hasContactId(handle)) {
                ContactPtr contact = ensureContact(handle,
                        connLowlevel->contactId(handle), realFeatures);
                satisfyingContacts.insert(handle, contact);
            }
        }
    }

    foreach (uint handle, handles) {
        ContactPtr contact = lookupContactByHandle(handle);
        if (contact) {
            if ((realFeatures - contact->requestedFeatures()).isEmpty()) {
                // Contact exists and has all the requested features
                satisfyingContacts.insert(handle, contact);
            } else {
                // Contact exists but is missing features
                otherContacts.insert(handle);
                missingFeatures.unite(realFeatures - contact->requestedFeatures());
            }
        } else {
            // Contact doesn't exist - we need to get all of the features (same as unite(features))
            missingFeatures = realFeatures;
            otherContacts.insert(handle);
        }
    }

    QSet<QString> interfaces = mPriv->interfacesForFeatures(missingFeatures);

    PendingContacts *contacts =
        new PendingContacts(ContactManagerPtr(this), handles, features, missingFeatures,
                interfaces.toList(), satisfyingContacts, otherContacts);
    return contacts;
}

PendingContacts *ContactManager::contactsForHandles(const ReferencedHandles &handles,
        const Features &features)
{
    return contactsForHandles(handles.toList(), features);
}

PendingContacts *ContactManager::contactsForHandles(const HandleIdentifierMap &handles,
        const Features &features)
{
    connection()->lowlevel()->injectContactIds(handles);

    return contactsForHandles(handles.keys(), features);
}

PendingContacts *ContactManager::contactsForIdentifiers(const QStringList &identifiers,
        const Features &features)
{
    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), identifiers,
                PendingContacts::ForIdentifiers, features, QStringList(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), identifiers,
                PendingContacts::ForIdentifiers, features, QStringList(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    Features realFeatures = mPriv->realFeatures(features);

    PendingContacts *contacts = new PendingContacts(ContactManagerPtr(this), identifiers,
            PendingContacts::ForIdentifiers, realFeatures, QStringList());
    return contacts;
}

/**
 * Request contacts and enable their \a features using a given field in their vcards.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \param vcardField The vcard field of the addresses we are requesting.
 *                   Supported fields can be found in ProtocolInfo::addressableVCardFields().
 * \param vcardAddresses The addresses to get contacts for. The address types must match
 *                       the given vcard field.
 * \param features The Contact features to enable.
 * \return A PendingContacts, which will emit PendingContacts::finished
 *         when the contacts are retrieved or an error occurred.
 * \sa contactsForHandles(), contactsForIdentifiers(), contactsForUris(),
 *     ProtocolInfo::normalizeVCardAddress()
 */
PendingContacts *ContactManager::contactsForVCardAddresses(const QString &vcardField,
        const QStringList &vcardAddresses, const Features &features)
{
    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), vcardField, vcardAddresses,
                features, QStringList(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), vcardField, vcardAddresses,
                features, QStringList(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    Features realFeatures = mPriv->realFeatures(features);
    QSet<QString> interfaces = mPriv->interfacesForFeatures(realFeatures);

    PendingContacts *contacts = new PendingContacts(ContactManagerPtr(this), vcardField,
            vcardAddresses, realFeatures, interfaces.toList());
    return contacts;
}

/**
 * Request contacts and enable their \a features using the given URI addresses.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \param uris The URI addresses to get contacts for.
 *             Supported schemes can be found in ProtocolInfo::addressableUriSchemes().
 * \param features The Contact features to enable.
 * \return A PendingContacts, which will emit PendingContacts::finished
 *         when the contacts are retrieved or an error occurred.
 * \sa contactsForHandles(), contactsForIdentifiers(), contactsForVCardAddresses(),
 *     ProtocolInfo::normalizeContactUri()
 */
PendingContacts *ContactManager::contactsForUris(const QStringList &uris,
        const Features &features)
{
    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), uris,
                PendingContacts::ForUris, features, QStringList(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), uris,
                PendingContacts::ForUris, features, QStringList(),
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    Features realFeatures = mPriv->realFeatures(features);
    QSet<QString> interfaces = mPriv->interfacesForFeatures(realFeatures);

    PendingContacts *contacts = new PendingContacts(ContactManagerPtr(this), uris,
            PendingContacts::ForUris, realFeatures, interfaces.toList());
    return contacts;
}

PendingContacts *ContactManager::upgradeContacts(const QList<ContactPtr> &contacts,
        const Features &features)
{
    if (!connection()->isValid()) {
        return new PendingContacts(ContactManagerPtr(this), contacts, features,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingContacts(ContactManagerPtr(this), contacts, features,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    return new PendingContacts(ContactManagerPtr(this), contacts, features);
}

ContactPtr ContactManager::lookupContactByHandle(uint handle)
{
    ContactPtr contact;

    if (mPriv->contacts.contains(handle)) {
        contact = ContactPtr(mPriv->contacts.value(handle));
        if (!contact) {
            // Dangling weak pointer, remove it
            mPriv->contacts.remove(handle);
        }
    }

    return contact;
}

/**
 * Start a request to retrieve the avatar for the given \a contacts.
 *
 * Force the request of the avatar data. This method returns directly, emitting
 * Contact::avatarTokenChanged() and Contact::avatarDataChanged() signals once the token
 * and data are fetched from the server.
 *
 * This is only useful if the avatar token is unknown; see Contact::isAvatarTokenKnown().
 * It happens in the case of offline XMPP contacts, because the server does not
 * send the token for them and an explicit request of the avatar data is needed.
 *
 * This method requires Contact::FeatureAvatarData to be ready.
 *
 * \sa Contact::avatarData(), Contact::avatarDataChanged(),
 *     Contact::avatarToken(), Contact::avatarTokenChanged()
 */
void ContactManager::requestContactAvatars(const QList<ContactPtr> &contacts)
{
    if (contacts.isEmpty()) {
        return;
    }

    if (!mPriv->requestAvatarsIdle) {
        mPriv->requestAvatarsIdle = true;
        QTimer::singleShot(0, this, SLOT(doRequestAvatars()));
    }

    mPriv->requestAvatarsQueue.unite(contacts.toSet());
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
PendingOperation *ContactManager::refreshContactInfo(const QList<ContactPtr> &contacts)
{
    if (!mPriv->refreshInfoOp) {
        mPriv->refreshInfoOp = new PendingRefreshContactInfo(connection());
        QTimer::singleShot(0, this, SLOT(doRefreshInfo()));
    }

    foreach (const ContactPtr &contact, contacts) {
        mPriv->refreshInfoOp->addContact(contact.data());
    }

    return mPriv->refreshInfoOp;
}

void ContactManager::onAliasesChanged(const AliasPairList &aliases)
{
    debug() << "Got AliasesChanged for" << aliases.size() << "contacts";

    foreach (AliasPair pair, aliases) {
        ContactPtr contact = lookupContactByHandle(pair.handle);

        if (contact) {
            contact->receiveAlias(pair.alias);
        }
    }
}

void ContactManager::doRequestAvatars()
{
    Q_ASSERT(mPriv->requestAvatarsIdle);
    QSet<ContactPtr> contacts = mPriv->requestAvatarsQueue;
    Q_ASSERT(contacts.size() > 0);

    mPriv->requestAvatarsQueue.clear();
    mPriv->requestAvatarsIdle = false;

    int found = 0;
    UIntList notFound;
    foreach (const ContactPtr &contact, contacts) {
        if (!contact) {
            continue;
        }

        QString avatarFileName;
        QString mimeTypeFileName;
        bool success = (contact->isAvatarTokenKnown() &&
            mPriv->buildAvatarFileName(contact->avatarToken(), false,
                avatarFileName, mimeTypeFileName));

        /* Check if the avatar is already in the cache */
        if (success && QFile::exists(avatarFileName)) {
            QFile mimeTypeFile(mimeTypeFileName);
            mimeTypeFile.open(QIODevice::ReadOnly);
            QString mimeType = QString(QLatin1String(mimeTypeFile.readAll()));
            mimeTypeFile.close();

            found++;

            contact->receiveAvatarData(AvatarData(avatarFileName, mimeType));

            continue;
        }

        notFound << contact->handle()[0];
    }

    if (found > 0) {
        debug() << "Avatar(s) found in cache for" << found << "contact(s)";
    }

    if (found == contacts.size()) {
        return;
    }

    debug() << "Requesting avatar(s) for" << contacts.size() - found << "contact(s)";

    Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
        connection()->interface<Client::ConnectionInterfaceAvatarsInterface>();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        avatarsInterface->RequestAvatars(notFound),
        this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher,
        SLOT(deleteLater()));
}

void ContactManager::onAvatarUpdated(uint handle, const QString &token)
{
    debug() << "Got AvatarUpdate for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);
    if (contact) {
        contact->receiveAvatarToken(token);
    }
}

void ContactManager::onAvatarRetrieved(uint handle, const QString &token,
    const QByteArray &data, const QString &mimeType)
{
    QString avatarFileName;
    QString mimeTypeFileName;

    debug() << "Got AvatarRetrieved for contact with handle" << handle;

    bool success = mPriv->buildAvatarFileName(token, true, avatarFileName,
        mimeTypeFileName);

    if (success) {
        debug() << "Write avatar in cache for handle" << handle;
        debug() << "Filename:" << avatarFileName;
        debug() << "MimeType:" << mimeType;

        if (!QFile::exists(mimeTypeFileName)) {
            QTemporaryFile mimeTypeFile(mimeTypeFileName);
            if (mimeTypeFile.open()) {
                mimeTypeFile.write(mimeType.toLatin1());
                mimeTypeFile.setAutoRemove(false);
                if (!mimeTypeFile.rename(mimeTypeFileName)) {
                    mimeTypeFile.remove();
                }
            }
        }

        if (!QFile::exists(avatarFileName)) {
            QTemporaryFile avatarFile(avatarFileName);
            if (avatarFile.open()) {
                avatarFile.write(data);
                avatarFile.setAutoRemove(false);
                if (!avatarFile.rename(avatarFileName)) {
                    avatarFile.remove();
                }
            }
        }
    }

    ContactPtr contact = lookupContactByHandle(handle);
    if (contact) {
        contact->setAvatarToken(token);
        contact->receiveAvatarData(AvatarData(avatarFileName, mimeType));
    }
}

void ContactManager::onPresencesChanged(const SimpleContactPresences &presences)
{
    debug() << "Got PresencesChanged for" << presences.size() << "contacts";

    foreach (uint handle, presences.keys()) {
        ContactPtr contact = lookupContactByHandle(handle);

        if (contact) {
            contact->receiveSimplePresence(presences[handle]);
        }
    }
}

void ContactManager::onCapabilitiesChanged(const ContactCapabilitiesMap &caps)
{
    debug() << "Got ContactCapabilitiesChanged for" << caps.size() << "contacts";

    foreach (uint handle, caps.keys()) {
        ContactPtr contact = lookupContactByHandle(handle);

        if (contact) {
            contact->receiveCapabilities(caps[handle]);
        }
    }
}

void ContactManager::onLocationUpdated(uint handle, const QVariantMap &location)
{
    debug() << "Got LocationUpdated for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);

    if (contact) {
        contact->receiveLocation(location);
    }
}

void ContactManager::onContactInfoChanged(uint handle, const Tp::ContactInfoFieldList &info)
{
    debug() << "Got ContactInfoChanged for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);

    if (contact) {
        contact->receiveInfo(info);
    }
}

void ContactManager::onClientTypesUpdated(uint handle, const QStringList &clientTypes)
{
    debug() << "Got ClientTypesUpdated for contact with handle" << handle;

    ContactPtr contact = lookupContactByHandle(handle);

    if (contact) {
        contact->receiveClientTypes(clientTypes);
    }
}

void ContactManager::doRefreshInfo()
{
    PendingRefreshContactInfo *op = mPriv->refreshInfoOp;
    Q_ASSERT(op);
    mPriv->refreshInfoOp = 0;
    op->refreshInfo();
}

ContactPtr ContactManager::ensureContact(const ReferencedHandles &handle,
        const Features &features, const QVariantMap &attributes)
{
    uint bareHandle = handle[0];
    ContactPtr contact = lookupContactByHandle(bareHandle);

    if (!contact) {
        contact = connection()->contactFactory()->construct(this, handle, features, attributes);
        mPriv->contacts.insert(bareHandle, contact);
    }

    contact->augment(features, attributes);

    return contact;
}

ContactPtr ContactManager::ensureContact(uint bareHandle, const QString &id,
        const Features &features)
{
    ContactPtr contact = lookupContactByHandle(bareHandle);

    if (!contact) {
        QVariantMap attributes;
        attributes.insert(TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id"), id);

        contact = connection()->contactFactory()->construct(this,
                ReferencedHandles(connection(), HandleTypeContact, UIntList() << bareHandle),
                features, attributes);
        mPriv->contacts.insert(bareHandle, contact);

        // do not call augment here as this is a fake contact
    }

    return contact;
}

QString ContactManager::featureToInterface(const Feature &feature)
{
    if (feature == Contact::FeatureAlias) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING;
    } else if (feature == Contact::FeatureAvatarToken) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS;
    } else if (feature == Contact::FeatureAvatarData) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS;
    } else if (feature == Contact::FeatureSimplePresence) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE;
    } else if (feature == Contact::FeatureCapabilities) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES;
    } else if (feature == Contact::FeatureLocation) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_LOCATION;
    } else if (feature == Contact::FeatureInfo) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_INFO;
    } else if (feature == Contact::FeatureRosterGroups) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS;
    } else if (feature == Contact::FeatureAddresses) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING;
    } else if (feature == Contact::FeatureClientTypes) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_CLIENT_TYPES;
    } else {
        warning() << "ContactManager doesn't know which interface corresponds to feature"
            << feature;
        return QString();
    }
}

void ContactManager::ensureTracking(const Feature &feature)
{
    if (mPriv->tracking[feature]) {
        return;
    }

    ConnectionPtr conn(connection());

    if (feature == Contact::FeatureAlias) {
        Client::ConnectionInterfaceAliasingInterface *aliasingInterface =
            conn->interface<Client::ConnectionInterfaceAliasingInterface>();

        connect(aliasingInterface,
                SIGNAL(AliasesChanged(Tp::AliasPairList)),
                SLOT(onAliasesChanged(Tp::AliasPairList)));
    } else if (feature == Contact::FeatureAvatarData) {
        Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
            conn->interface<Client::ConnectionInterfaceAvatarsInterface>();

        connect(avatarsInterface,
                SIGNAL(AvatarRetrieved(uint,QString,QByteArray,QString)),
                SLOT(onAvatarRetrieved(uint,QString,QByteArray,QString)));
    } else if (feature == Contact::FeatureAvatarToken) {
        Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
            conn->interface<Client::ConnectionInterfaceAvatarsInterface>();

        connect(avatarsInterface,
                SIGNAL(AvatarUpdated(uint,QString)),
                SLOT(onAvatarUpdated(uint,QString)));
    } else if (feature == Contact::FeatureCapabilities) {
        Client::ConnectionInterfaceContactCapabilitiesInterface *contactCapabilitiesInterface =
            conn->interface<Client::ConnectionInterfaceContactCapabilitiesInterface>();

        connect(contactCapabilitiesInterface,
                SIGNAL(ContactCapabilitiesChanged(Tp::ContactCapabilitiesMap)),
                SLOT(onCapabilitiesChanged(Tp::ContactCapabilitiesMap)));
    } else if (feature == Contact::FeatureInfo) {
        Client::ConnectionInterfaceContactInfoInterface *contactInfoInterface =
            conn->interface<Client::ConnectionInterfaceContactInfoInterface>();

        connect(contactInfoInterface,
                SIGNAL(ContactInfoChanged(uint,Tp::ContactInfoFieldList)),
                SLOT(onContactInfoChanged(uint,Tp::ContactInfoFieldList)));
    } else if (feature == Contact::FeatureLocation) {
        Client::ConnectionInterfaceLocationInterface *locationInterface =
            conn->interface<Client::ConnectionInterfaceLocationInterface>();

        connect(locationInterface,
                SIGNAL(LocationUpdated(uint,QVariantMap)),
                SLOT(onLocationUpdated(uint,QVariantMap)));
    } else if (feature == Contact::FeatureSimplePresence) {
        Client::ConnectionInterfaceSimplePresenceInterface *simplePresenceInterface =
            conn->interface<Client::ConnectionInterfaceSimplePresenceInterface>();

        connect(simplePresenceInterface,
                SIGNAL(PresencesChanged(Tp::SimpleContactPresences)),
                SLOT(onPresencesChanged(Tp::SimpleContactPresences)));
    } else if (feature == Contact::FeatureClientTypes) {
        Client::ConnectionInterfaceClientTypesInterface *clientTypesInterface =
            conn->interface<Client::ConnectionInterfaceClientTypesInterface>();

        connect(clientTypesInterface,
                SIGNAL(ClientTypesUpdated(uint,QStringList)),
                SLOT(onClientTypesUpdated(uint,QStringList)));
    } else if (feature == Contact::FeatureRosterGroups || feature == Contact::FeatureAddresses) {
        // nothing to do here, but we don't want to warn
        ;
    } else {
        warning() << " Unknown feature" << feature
            << "when trying to figure out how to connect change notification!";
    }

    mPriv->tracking[feature] = true;
}

PendingOperation *ContactManager::introspectRoster()
{
    return mPriv->roster->introspect();
}

PendingOperation *ContactManager::introspectRosterGroups()
{
    return mPriv->roster->introspectGroups();
}

void ContactManager::resetRoster()
{
    mPriv->roster->reset();
}

/**
 * \fn void ContactManager::presencePublicationRequested(const Tp::Contacts &contacts)
 *
 * Emitted whenever some contacts request for presence publication.
 *
 * \param contacts A set of contacts which requested presence publication.
 */

/**
 * \fn void ContactManager::groupAdded(const QString &group)
 *
 * Emitted when a new contact list group is created.
 *
 * \param group The group name.
 * \sa allKnownGroups()
 */

/**
 * \fn void ContactManager::groupRenamed(const QString &oldGroup, const QString &newGroup)
 *
 * Emitted when a new contact list group is renamed.
 *
 * \param oldGroup The old group name.
 * \param newGroup The new group name.
 * \sa allKnownGroups()
 */

/**
 * \fn void ContactManager::groupRemoved(const QString &group)
 *
 * Emitted when a contact list group is removed.
 *
 * \param group The group name.
 * \sa allKnownGroups()
 */

/**
 * \fn void ContactManager::groupMembersChanged(const QString &group,
 *          const Tp::Contacts &groupMembersAdded,
 *          const Tp::Contacts &groupMembersRemoved,
 *          const Tp::Channel::GroupMemberChangeDetails &details)
 *
 * Emitted whenever some contacts got removed or added from
 * a group.
 *
 * \param group The name of the group that changed.
 * \param groupMembersAdded A set of contacts which were added to the group \a group.
 * \param groupMembersRemoved A set of contacts which were removed from the group \a group.
 * \param details The change details.
 * \sa groupContacts()
 */

/**
 * \fn void ContactManager::allKnownContactsChanged(const Tp::Contacts &contactsAdded,
 *          const Tp::Contacts &contactsRemoved,
 *          const Tp::Channel::GroupMemberChangeDetails &details)
 *
 * Emitted whenever some contacts got removed or added from
 * ContactManager's known contact list. It is useful for monitoring which contacts
 * are currently known by ContactManager.
 *
 * Note that, in some protocols, this signal could stream newly added contacts
 * with both presence subscription and publication state set to No. Be sure to watch
 * over publication and/or subscription state changes if that is the case.
 *
 * \param contactsAdded A set of contacts which were added to the known contact list.
 * \param contactsRemoved A set of contacts which were removed from the known contact list.
 * \param details The change details.
 * \sa allKnownContacts()
 */

} // Tp
