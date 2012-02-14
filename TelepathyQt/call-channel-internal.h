/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2010-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

namespace Tp
{

/* ====== CallStream ====== */
struct TP_QT_NO_EXPORT CallStream::Private
{
    Private(CallStream *parent, const CallContentPtr &content);

    static void introspectMainProperties(Private *self);

    void processRemoteMembersChanged();

    struct RemoteMembersChangedInfo;

    // Public object
    CallStream *parent;

    WeakPtr<CallContent> content;

    // Mandatory proxies
    Client::CallStreamInterface *streamInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint localSendingState;
    ContactSendingStateMap remoteMembers;
    QHash<uint, ContactPtr> remoteMembersContacts;
    bool buildingRemoteMembers;
    QQueue<RemoteMembersChangedInfo *> remoteMembersChangedQueue;
    RemoteMembersChangedInfo *currentRemoteMembersChangedInfo;
};

struct TP_QT_NO_EXPORT CallStream::Private::RemoteMembersChangedInfo
{
    RemoteMembersChangedInfo(const ContactSendingStateMap &updates, const UIntList &removed)
        : updates(updates),
          removed(removed)
    {
    }

    ContactSendingStateMap updates;
    UIntList removed;
};

/* ====== PendingCallContent ====== */
struct TP_QT_NO_EXPORT PendingCallContent::Private
{
    Private(PendingCallContent *parent, const CallChannelPtr &channel)
        : parent(parent),
          channel(channel)
    {
    }

    PendingCallContent *parent;
    CallChannelPtr channel;
    CallContentPtr content;
};

/* ====== CallContent ====== */
struct TP_QT_NO_EXPORT CallContent::Private
{
    Private(CallContent *parent, const CallChannelPtr &channel);

    static void introspectMainProperties(Private *self);
    void checkIntrospectionCompleted();

    CallStreamPtr addStream(const QDBusObjectPath &streamPath);
    CallStreamPtr lookupStream(const QDBusObjectPath &streamPath);

    // Public object
    CallContent *parent;

    WeakPtr<CallChannel> channel;

    // Mandatory proxies
    Client::CallContentInterface *contentInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    QString name;
    uint type;
    uint disposition;
    CallStreams streams;
    CallStreams incompleteStreams;
};

/* ====== CallChannel ====== */
struct TP_QT_NO_EXPORT CallChannel::Private
{
    Private(CallChannel *parent);
    ~Private();

    static void introspectContents(Private *self);
    static void introspectLocalHoldState(Private *self);

    CallContentPtr addContent(const QDBusObjectPath &contentPath);
    CallContentPtr lookupContent(const QDBusObjectPath &contentPath) const;

    class PendingRemoveContent;

    // Public object
    CallChannel *parent;

    // Mandatory proxies
    Client::ChannelTypeCallInterface *callInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint state;
    uint flags;
    CallStateReason stateReason;
    QVariantMap stateDetails;
    bool hardwareStreaming;
    uint initialTransportType;
    bool initialAudio;
    bool initialVideo;
    QString initialAudioName;
    QString initialVideoName;
    bool mutableContents;
    CallContents contents;
    CallContents incompleteContents;

    uint localHoldState;
    uint localHoldStateReason;
};

class TP_QT_NO_EXPORT CallChannel::Private::PendingRemoveContent :
                    public PendingOperation
{
    Q_OBJECT

public:
    PendingRemoveContent(const CallChannelPtr &channel, const CallContentPtr &content,
            ContentRemovalReason reason, const QString &detailedReason, const QString &message);

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *watcher);
    void onContentRemoved(const Tp::CallContentPtr &content);

private:
    CallChannelPtr channel;
    CallContentPtr content;
};

} // Tp
