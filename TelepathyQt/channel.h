/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt_channel_h_HEADER_GUARD_
#define _TelepathyQt_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-channel.h>

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBus>
#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/OptionalInterfaceFactory>
#include <TelepathyQt/ReadinessHelper>
#include <TelepathyQt/SharedPtr>
#include <TelepathyQt/Types>

#include <QSet>
#include <QSharedDataPointer>
#include <QVariantMap>

namespace Tp
{

class Connection;
class PendingOperation;
class PendingReady;

class TP_QT_EXPORT Channel : public StatefulDBusProxy,
                public OptionalInterfaceFactory<Channel>
{
    Q_OBJECT
    Q_DISABLE_COPY(Channel)

public:
    static const Feature FeatureCore;
    static const Feature FeatureConferenceInitialInviteeContacts;

    static ChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~Channel();

    ConnectionPtr connection() const;

    QVariantMap immutableProperties() const;

    QString channelType() const;

    HandleType targetHandleType() const;
    uint targetHandle() const;
    QString targetId() const;
    ContactPtr targetContact() const;

    bool isRequested() const;
    ContactPtr initiatorContact() const;

    PendingOperation *requestClose();
    PendingOperation *requestLeave(const QString &message = QString(),
            ChannelGroupChangeReason reason = ChannelGroupChangeReasonNone);

    ChannelGroupFlags groupFlags() const;

    bool groupCanAddContacts() const;
    bool groupCanAddContactsWithMessage() const;
    bool groupCanAcceptContactsWithMessage() const;
    PendingOperation *groupAddContacts(const QList<ContactPtr> &contacts,
            const QString &message = QString());
    bool groupCanRescindContacts() const;
    bool groupCanRescindContactsWithMessage() const;
    bool groupCanRemoveContacts() const;
    bool groupCanRemoveContactsWithMessage() const;
    bool groupCanRejectContactsWithMessage() const;
    bool groupCanDepartWithMessage() const;
    PendingOperation *groupRemoveContacts(const QList<ContactPtr> &contacts,
            const QString &message = QString(),
            ChannelGroupChangeReason reason = ChannelGroupChangeReasonNone);

    Contacts groupContacts(bool includeSelfContact = true) const;
    Contacts groupLocalPendingContacts(bool includeSelfContact = true) const;
    Contacts groupRemotePendingContacts(bool includeSelfContact = true) const;

    class GroupMemberChangeDetails
    {
    public:
        GroupMemberChangeDetails();
        GroupMemberChangeDetails(const GroupMemberChangeDetails &other);
        ~GroupMemberChangeDetails();

        GroupMemberChangeDetails &operator=(const GroupMemberChangeDetails &other);

        bool isValid() const { return mPriv.constData() != 0; }

        bool hasActor() const;
        ContactPtr actor() const;

        bool hasReason() const { return allDetails().contains(QLatin1String("change-reason")); }
        ChannelGroupChangeReason reason() const { return (ChannelGroupChangeReason) qdbus_cast<uint>(allDetails().value(QLatin1String("change-reason"))); }

        bool hasMessage() const { return allDetails().contains(QLatin1String("message")); }
        QString message () const { return qdbus_cast<QString>(allDetails().value(QLatin1String("message"))); }

        bool hasError() const { return allDetails().contains(QLatin1String("error")); }
        QString error() const { return qdbus_cast<QString>(allDetails().value(QLatin1String("error"))); }

        bool hasDebugMessage() const { return allDetails().contains(QLatin1String("debug-message")); }
        QString debugMessage() const { return qdbus_cast<QString>(allDetails().value(QLatin1String("debug-message"))); }

        QVariantMap allDetails() const;

    private:
        friend class Channel;
        friend class Contact;
        friend class ContactManager;

        TP_QT_NO_EXPORT GroupMemberChangeDetails(const ContactPtr &actor, const QVariantMap &details);

        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    GroupMemberChangeDetails groupLocalPendingContactChangeInfo(const ContactPtr &contact) const;
    GroupMemberChangeDetails groupSelfContactRemoveInfo() const;

    bool groupAreHandleOwnersAvailable() const;
    HandleOwnerMap groupHandleOwners() const;

    bool groupIsSelfContactTracked() const;
    ContactPtr groupSelfContact() const;

    bool isConference() const;
    Contacts conferenceInitialInviteeContacts() const;
    QList<ChannelPtr> conferenceChannels() const;
    QList<ChannelPtr> conferenceInitialChannels() const;
    QHash<uint, ChannelPtr> conferenceOriginalChannels() const;

    bool supportsConferenceMerging() const;
    PendingOperation *conferenceMergeChannel(const ChannelPtr &channel);

    bool supportsConferenceSplitting() const;
    PendingOperation *conferenceSplitChannel();

Q_SIGNALS:
    void groupFlagsChanged(Tp::ChannelGroupFlags flags,
            Tp::ChannelGroupFlags added, Tp::ChannelGroupFlags removed);

    void groupCanAddContactsChanged(bool canAddContacts);
    void groupCanRemoveContactsChanged(bool canRemoveContacts);
    void groupCanRescindContactsChanged(bool canRescindContacts);

    void groupMembersChanged(
            const Tp::Contacts &groupMembersAdded,
            const Tp::Contacts &groupLocalPendingMembersAdded,
            const Tp::Contacts &groupRemotePendingMembersAdded,
            const Tp::Contacts &groupMembersRemoved,
            const Tp::Channel::GroupMemberChangeDetails &details);

    void groupHandleOwnersChanged(const Tp::HandleOwnerMap &owners,
            const Tp::UIntList &added, const Tp::UIntList &removed);

    void groupSelfContactChanged();

    void conferenceChannelMerged(const Tp::ChannelPtr &channel);
    void conferenceChannelRemoved(const Tp::ChannelPtr &channel,
            const Tp::Channel::GroupMemberChangeDetails &details);

protected:
    Channel(const ConnectionPtr &connection,const QString &objectPath,
            const QVariantMap &immutableProperties, const Feature &coreFeature);

    Client::ChannelInterface *baseInterface() const;

    bool groupSelfHandleIsLocalPending() const;

protected Q_SLOTS:
    PendingOperation *groupAddSelfHandle();

private Q_SLOTS:
    TP_QT_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotChannelType(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotHandle(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotInterfaces(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onClosed();

    TP_QT_NO_EXPORT void onConnectionReady(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onConnectionInvalidated();

    TP_QT_NO_EXPORT void gotGroupProperties(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotGroupFlags(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotAllMembers(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotLocalPendingMembersWithInfo(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotContacts(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onGroupFlagsChanged(uint added, uint removed);
    TP_QT_NO_EXPORT void onMembersChanged(const QString &message,
            const Tp::UIntList &added, const Tp::UIntList &removed,
            const Tp::UIntList &localPending, const Tp::UIntList &remotePending,
            uint actor, uint reason);
    TP_QT_NO_EXPORT void onMembersChangedDetailed(
            const Tp::UIntList &added, const Tp::UIntList &removed,
            const Tp::UIntList &localPending, const Tp::UIntList &remotePending,
            const QVariantMap &details);
    TP_QT_NO_EXPORT void onHandleOwnersChanged(const Tp::HandleOwnerMap &added, const Tp::UIntList &removed);
    TP_QT_NO_EXPORT void onSelfHandleChanged(uint selfHandle);

    TP_QT_NO_EXPORT void gotConferenceProperties(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotConferenceInitialInviteeContacts(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onConferenceChannelMerged(const QDBusObjectPath &channel, uint channelSpecificHandle,
            const QVariantMap &properties);
    TP_QT_NO_EXPORT void onConferenceChannelMerged(const QDBusObjectPath &channel);
    TP_QT_NO_EXPORT void onConferenceChannelRemoved(const QDBusObjectPath &channel, const QVariantMap &details);
    TP_QT_NO_EXPORT void onConferenceChannelRemoved(const QDBusObjectPath &channel);
    TP_QT_NO_EXPORT void gotConferenceChannelRemovedActorContact(Tp::PendingOperation *op);

private:
    class PendingLeave;
    friend class PendingLeave;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::Channel::GroupMemberChangeDetails);

#endif
