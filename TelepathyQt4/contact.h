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

#ifndef _TelepathyQt4_cli_contact_h_HEADER_GUARD_
#define _TelepathyQt4_cli_contact_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QVariantMap>

#include <TelepathyQt4/Types>

namespace Tp
{

class ContactManager;
class PendingOperation;
class ReferencedHandles;

class Contact : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Contact);

public:
    enum Feature {
        FeatureAlias,
        FeatureAvatarToken,
        FeatureSimplePresence,
        _Padding = 0xFFFFFFFF
    };

    enum PresenceState {
         PresenceStateNo,
         PresenceStateAsk,
         PresenceStateYes
    };

    ContactManager *manager() const;

    ReferencedHandles handle() const;
    QString id() const;

    QSet<Feature> requestedFeatures() const;
    QSet<Feature> actualFeatures() const;

    QString alias() const;

    bool isAvatarTokenKnown() const;
    QString avatarToken() const;

    QString presenceStatus() const;
    uint presenceType() const;
    QString presenceMessage() const;

    PresenceState subscriptionState() const;
    PresenceState publishState() const;

    PendingOperation *requestPresenceSubscription(const QString &message = QString());
    PendingOperation *removePresenceSubscription(const QString &message = QString());
    PendingOperation *authorizePresencePublication(const QString &message = QString());
    PendingOperation *removePresencePublication(const QString &message = QString());

    bool isBlocked() const;
    PendingOperation *block(bool value = true);

    ~Contact();

Q_SIGNALS:
    void aliasChanged(const QString &alias);
    void avatarTokenChanged(const QString &avatarToken);
    void simplePresenceChanged(const QString &status, uint type, const QString &presenceMessage);

    void subscriptionStateChanged(Tp::Contact::PresenceState state);
    void publishStateChanged(Tp::Contact::PresenceState state);
    void blockStatusChanged(bool blocked);

    // TODO: consider how the Renaming interface should work and map to Contacts
    // I guess it would be something like:
    // void renamedTo(Tp::ContactPtr)
    // with that contact getting the same features requested as the current one. Or would we rather
    // want to signal that change right away with a handle?

private:
    Contact(ContactManager *manager, const ReferencedHandles &handle,
            const QSet<Feature> &requestedFeatures, const QVariantMap &attributes);

    void augment(const QSet<Feature> &requestedFeatures, const QVariantMap &attributes);

    void receiveAlias(const QString &alias);
    void receiveAvatarToken(const QString &avatarToken);
    void receiveSimplePresence(const SimplePresence &presence);

    void setSubscriptionState(PresenceState state);
    void setPublishState(PresenceState state);
    void setBlocked(bool value);

    struct Private;
    friend class ContactManager;
    friend struct Private;
    Private *mPriv;
};

typedef QSharedPointer<Contact> ContactPtr;
typedef QSet<ContactPtr> Contacts;

inline uint qHash(const ContactPtr &contact)
{
    return qHash(contact.data());
}

} // Tp

#endif
