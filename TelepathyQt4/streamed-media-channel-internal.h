/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include "TelepathyQt4/future-internal.h"

using TpFuture::Client::CallContentInterface;
using TpFuture::Client::CallStreamInterface;
using TpFuture::Client::ChannelTypeCallInterface;

namespace Tp
{

enum IfaceType {
    IfaceTypeStreamedMedia,
    IfaceTypeCall,
};

/* ====== PendingMediaStreams ====== */
struct TELEPATHY_QT4_NO_EXPORT PendingMediaStreams::Private
{
    Private(PendingMediaStreams *parent, const StreamedMediaChannelPtr &channel)
        : parent(parent), channel(channel), contentsReady(0)
    {
    }

    inline ChannelTypeCallInterface *callInterface() const
    {
        return StreamedMediaChannelPtr(channel)->interface<ChannelTypeCallInterface>();
    }

    PendingMediaStreams *parent;
    StreamedMediaChannelPtr channel;
    MediaContents contents;
    uint numContents;
    uint contentsReady;
};

/* ====== MediaStream ====== */
struct TELEPATHY_QT4_NO_EXPORT MediaStream::Private
{
    Private(MediaStream *parent, const MediaContentPtr &content,
            const MediaStreamInfo &info);
    Private(MediaStream *parent, const MediaContentPtr &content,
            const QDBusObjectPath &objectPath);

    // SM specific methods
    static void introspectSMContact(Private *self);

    PendingOperation *updateSMDirection(bool send, bool receive);
    SendingState localSendingStateFromSMDirection();
    SendingState remoteSendingStateFromSMDirection();

    // Call specific methods
    static void introspectCallMainProperties(Private *self);

    void processCallSendersChanged();

    class CallProxy;
    struct CallSendersChangedInfo;

    IfaceType ifaceType;
    MediaStream *parent;
    ReadinessHelper *readinessHelper;
    WeakPtr<MediaContent> content;

    // SM specific fields
    uint SMId;
    uint SMContactHandle;
    ContactPtr SMContact;
    uint SMDirection;
    uint SMPendingSend;
    uint SMState;

    // Call specific fields
    CallStreamInterface *callBaseInterface;
    Client::DBus::PropertiesInterface *callPropertiesInterface;
    CallProxy *callProxy;
    QDBusObjectPath callObjectPath;
    TpFuture::ContactSendingStateMap senders;
    QHash<uint, ContactPtr> sendersContacts;
    bool buildingCallSenders;
    QQueue<CallSendersChangedInfo *> callSendersChangedQueue;
    CallSendersChangedInfo *currentCallSendersChangedInfo;
};

class TELEPATHY_QT4_NO_EXPORT MediaStream::Private::CallProxy : public QObject
{
    Q_OBJECT

public:
    CallProxy(MediaStream::Private *priv, QObject *parent)
        : QObject(parent), mPriv(priv)
    {
    }

    ~CallProxy() {}

private Q_SLOTS:
    void onCallSendersChanged(const TpFuture::ContactSendingStateMap &,
        const TpFuture::UIntList &);

private:
    MediaStream::Private *mPriv;
};

struct TELEPATHY_QT4_NO_EXPORT MediaStream::Private::CallSendersChangedInfo
{
    CallSendersChangedInfo(const TpFuture::ContactSendingStateMap &updates,
            const UIntList &removed)
        : updates(updates),
          removed(removed)
    {
    }

    TpFuture::ContactSendingStateMap updates;
    UIntList removed;
};

/* ====== PendingMediaContent ====== */
struct TELEPATHY_QT4_NO_EXPORT PendingMediaContent::Private
{
    Private(PendingMediaContent *parent, const StreamedMediaChannelPtr &channel)
        : parent(parent), channel(channel)
    {
    }

    inline ChannelTypeCallInterface *callInterface() const
    {
        return StreamedMediaChannelPtr(channel)->interface<ChannelTypeCallInterface>();
    }

    PendingMediaContent *parent;
    StreamedMediaChannelPtr channel;
    MediaContentPtr content;
};

/* ====== MediaContent ====== */
struct TELEPATHY_QT4_NO_EXPORT MediaContent::Private
{
    Private(MediaContent *parent,
            const StreamedMediaChannelPtr &channel,
            const QString &name,
            const MediaStreamInfo &streamInfo);
    Private(MediaContent *parent,
            const StreamedMediaChannelPtr &channel,
            const QDBusObjectPath &objectPath);

    // SM specific methods
    static void introspectSMStream(Private *self);

    // Call specific methods
    static void introspectCallMainProperties(Private *self);

    MediaStreamPtr lookupStreamByCallObjectPath(
            const QDBusObjectPath &streamPath);

    // general methods
    void checkIntrospectionCompleted();

    void addStream(const MediaStreamPtr &stream);

    IfaceType ifaceType;
    MediaContent *parent;
    ReadinessHelper *readinessHelper;
    WeakPtr<StreamedMediaChannel> channel;
    QString name;
    uint type;
    uint creatorHandle;
    ContactPtr creator;

    MediaStreams incompleteStreams;
    MediaStreams streams;

    // SM specific fields
    MediaStreamPtr SMStream;
    MediaStreamInfo SMStreamInfo;

    // Call specific fields
    CallContentInterface *callBaseInterface;
    Client::DBus::PropertiesInterface *callPropertiesInterface;
    QDBusObjectPath callObjectPath;
};

/* ====== StreamedMediaChannel ====== */
struct TELEPATHY_QT4_NO_EXPORT StreamedMediaChannel::Private
{
    Private(StreamedMediaChannel *parent);
    ~Private();

    static void introspectContents(Private *self);
    void introspectSMStreams();
    void introspectCallContents();

    static void introspectLocalHoldState(Private *self);

    inline ChannelTypeCallInterface *callInterface() const
    {
        return parent->interface<ChannelTypeCallInterface>();
    }

    // Public object
    StreamedMediaChannel *parent;

    // Mandatory properties interface proxy
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    IfaceType ifaceType;

    // Introspection

    MediaContents incompleteContents;
    MediaContents contents;

    LocalHoldState localHoldState;
    LocalHoldStateReason localHoldStateReason;

    // Call speficic fields
    bool callHardwareStreaming;

    uint numContents;
};

} // Tp
