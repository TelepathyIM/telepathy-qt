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

#ifndef _TelepathyQt_contact_h_HEADER_GUARD_
#define _TelepathyQt_contact_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/Feature>
#include <TelepathyQt/Object>
#include <TelepathyQt/Types>

#include <QSet>
#include <QVariantMap>

namespace Tp
{

struct AvatarData;
class ContactCapabilities;
class LocationInfo;
class ContactManager;
class PendingContactInfo;
class PendingOperation;
class PendingStringList;
class Presence;
class ReferencedHandles;

class TP_QT_EXPORT Contact : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(Contact)

public:
    static const Feature FeatureAlias;
    static const Feature FeatureAvatarData;
    static const Feature FeatureAvatarToken;
    static const Feature FeatureCapabilities;
    static const Feature FeatureInfo;
    static const Feature FeatureLocation;
    static const Feature FeatureSimplePresence;
    static const Feature FeatureAddresses;
    static const Feature FeatureClientTypes;

    enum PresenceState {
         PresenceStateNo,
         PresenceStateAsk,
         PresenceStateYes
    };

    class InfoFields
    {
    public:
        InfoFields();
        InfoFields(const ContactInfoFieldList &fields);
        InfoFields(const InfoFields &other);
        ~InfoFields();

        bool isValid() const { return mPriv.constData() != 0; }

        InfoFields &operator=(const InfoFields &other);

        ContactInfoFieldList fields(const QString &name) const;

        ContactInfoFieldList allFields() const;

    private:
        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    ~Contact();

    ContactManagerPtr manager() const;

    ReferencedHandles handle() const;

    // TODO filter: exact, prefix, substring match
    QString id() const;

    Features requestedFeatures() const;
    Features actualFeatures() const;

    // TODO filter: exact, prefix, substring match
    QString alias() const;

    QMap<QString, QString> vcardAddresses() const;
    QStringList uris() const;

    bool isAvatarTokenKnown() const;
    QString avatarToken() const;
    AvatarData avatarData() const;
    void requestAvatarData();

    /*
     * TODO filter:
     *  - exact match of presence().type(), presence().status()
     *  - ANY 1 of a number of presence types/statuses
     *  - presence().type() greater or less than a set value
     *  - have/don't have presence().message() AND exact/prefix/substring
     */
    Presence presence() const;

    // TODO filter: the same as Account filtering by caps
    ContactCapabilities capabilities() const;

    // TODO filter: is it available, how accurate, are they near me
    LocationInfo location() const;

    // TODO filter: having a specific field, having ANY field,
    // (field: exact, contents: exact/prefix/substring)
    bool isContactInfoKnown() const;
    InfoFields infoFields() const;
    PendingOperation *refreshInfo();
    PendingContactInfo *requestInfo();

    /*
     * Filters on exact values of these, but also the "in your contact list at all or not" usecase
     */
    bool isSubscriptionStateKnown() const;
    bool isSubscriptionRejected() const;
    PresenceState subscriptionState() const;
    bool isPublishStateKnown() const;
    bool isPublishCancelled() const;
    PresenceState publishState() const;
    QString publishStateMessage() const;

    PendingOperation *requestPresenceSubscription(const QString &message = QString());
    PendingOperation *removePresenceSubscription(const QString &message = QString());
    PendingOperation *authorizePresencePublication(const QString &message = QString());
    PendingOperation *removePresencePublication(const QString &message = QString());

    /*
     * Filter on being blocked or not
     */
    bool isBlocked() const;
    PendingOperation *block();
    PendingOperation *blockAndReportAbuse();
    PendingOperation *unblock();

    /*
     * Filter on the groups they're in - to show a specific group only
     *
     * Also prefix/substring match on ANY of the groups of the contact
     */
    QStringList groups() const;
    PendingOperation *addToGroup(const QString &group);
    PendingOperation *removeFromGroup(const QString &group);

    QStringList clientTypes() const;
    PendingStringList *requestClientTypes();

Q_SIGNALS:
    void aliasChanged(const QString &alias);

    void avatarTokenChanged(const QString &avatarToken);
    void avatarDataChanged(const Tp::AvatarData &avatarData);

    void presenceChanged(const Tp::Presence &presence);

    void capabilitiesChanged(const Tp::ContactCapabilities &caps);

    void locationUpdated(const Tp::LocationInfo &location);

    void infoFieldsChanged(const Tp::Contact::InfoFields &infoFields);

    void subscriptionStateChanged(Tp::Contact::PresenceState state);

    void publishStateChanged(Tp::Contact::PresenceState state, const QString &message);

    void blockStatusChanged(bool blocked);

    void addedToGroup(const QString &group);
    void removedFromGroup(const QString &group);

    void clientTypesChanged(const QStringList &clientTypes);

    // TODO: consider how the Renaming interface should work and map to Contacts
    // I guess it would be something like:
    // void renamedTo(Tp::ContactPtr)
    // with that contact getting the same features requested as the current one. Or would we rather
    // want to signal that change right away with a handle?

protected:
    Contact(ContactManager *manager, const ReferencedHandles &handle,
            const Features &requestedFeatures, const QVariantMap &attributes);

    virtual void augment(const Features &requestedFeatures, const QVariantMap &attributes);

private:
    static const Feature FeatureRosterGroups;

    TP_QT_NO_EXPORT void receiveAlias(const QString &alias);
    TP_QT_NO_EXPORT void receiveAvatarToken(const QString &avatarToken);
    TP_QT_NO_EXPORT void setAvatarToken(const QString &token);
    TP_QT_NO_EXPORT void receiveAvatarData(const AvatarData &);
    TP_QT_NO_EXPORT void receiveSimplePresence(const SimplePresence &presence);
    TP_QT_NO_EXPORT void receiveCapabilities(const RequestableChannelClassList &caps);
    TP_QT_NO_EXPORT void receiveLocation(const QVariantMap &location);
    TP_QT_NO_EXPORT void receiveInfo(const ContactInfoFieldList &info);
    TP_QT_NO_EXPORT void receiveAddresses(const QMap<QString, QString> &addresses,
            const QStringList &uris);
    TP_QT_NO_EXPORT void receiveClientTypes(const QStringList &clientTypes);

    TP_QT_NO_EXPORT static PresenceState subscriptionStateToPresenceState(uint subscriptionState);
    TP_QT_NO_EXPORT void setSubscriptionState(SubscriptionState state);
    TP_QT_NO_EXPORT void setPublishState(SubscriptionState state, const QString &message = QString());
    TP_QT_NO_EXPORT void setBlocked(bool value);

    TP_QT_NO_EXPORT void setAddedToGroup(const QString &group);
    TP_QT_NO_EXPORT void setRemovedFromGroup(const QString &group);

    struct Private;
    friend class Connection;
    friend class ContactFactory;
    friend class ContactManager;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::Contact::InfoFields);

#endif
