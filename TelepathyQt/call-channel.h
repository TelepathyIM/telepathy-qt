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

#ifndef _TelepathyQt_call_channel_h_HEADER_GUARD_
#define _TelepathyQt_call_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>
#include <TelepathyQt/SharedPtr>

namespace Tp
{

class CallChannel;

typedef QList<CallContentPtr> CallContents;
typedef QList<CallStreamPtr> CallStreams;

class TP_QT_EXPORT CallStream : public StatefulDBusProxy,
                    public OptionalInterfaceFactory<CallStream>
{
    Q_OBJECT
    Q_DISABLE_COPY(CallStream)

public:
    ~CallStream();

    CallContentPtr content() const;

    Contacts members() const;

    SendingState localSendingState() const;
    SendingState remoteSendingState(const ContactPtr &contact) const;

    PendingOperation *requestSending(bool send);
    PendingOperation *requestReceiving(const ContactPtr &contact, bool receive);

Q_SIGNALS:
    void localSendingStateChanged(Tp::SendingState localSendingState);
    void remoteSendingStateChanged(
            const QHash<Tp::ContactPtr, Tp::SendingState> &remoteSendingStates);
    void remoteMembersRemoved(const Tp::Contacts &remoteMembers);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void gotRemoteMembersContacts(Tp::PendingOperation *op);

    void onRemoteMembersChanged(const Tp::ContactSendingStateMap &updates,
            const Tp::UIntList &removed);
    void onLocalSendingStateChanged(uint);

private:
    friend class CallChannel;
    friend class CallContent;

    static const Feature FeatureCore;

    CallStream(const CallContentPtr &content, const QDBusObjectPath &streamPath);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT PendingCallContent : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingCallContent)

public:
    ~PendingCallContent();

    CallContentPtr content() const;

private Q_SLOTS:
    void gotContent(QDBusPendingCallWatcher *watcher);

    void onContentReady(Tp::PendingOperation *op);
    void onContentRemoved(const Tp::CallContentPtr &content);

private:
    friend class CallChannel;

    PendingCallContent(const CallChannelPtr &channel,
            const QString &contentName, MediaStreamType type);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT CallContent : public StatefulDBusProxy,
                    public OptionalInterfaceFactory<CallContent>
{
    Q_OBJECT
    Q_DISABLE_COPY(CallContent)

public:
    ~CallContent();

    CallChannelPtr channel() const;

    QString name() const;
    MediaStreamType type() const;

    CallContentDisposition disposition() const;

    CallStreams streams() const;

Q_SIGNALS:
    void streamAdded(const Tp::CallStreamPtr &stream);
    void streamRemoved(const Tp::CallStreamPtr &stream);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onStreamsAdded(const Tp::ObjectPathList &streamPath);
    void onStreamsRemoved(const Tp::ObjectPathList &streamPath);
    void onStreamReady(Tp::PendingOperation *op);

private:
    friend class CallChannel;
    friend class PendingCallContent;

    static const Feature FeatureCore;

    CallContent(const CallChannelPtr &channel,
            const QDBusObjectPath &contentPath);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT CallChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(CallChannel)

public:
    static const Feature FeatureContents;
    static const Feature FeatureLocalHoldState;

    // TODO: add helpers to ensure/create call channel using Account

    static CallChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~CallChannel();

    CallState state() const;
    CallFlags flags() const;
    CallStateReason stateReason() const;
    QVariantMap stateDetails() const;

    bool handlerStreamingRequired() const;
    StreamTransportType initialTransportType() const;
    bool hasInitialAudio() const;
    bool hasInitialVideo() const;
    QString initialAudioName() const;
    QString initialVideoName() const;
    bool hasMutableContents() const;

    PendingOperation *accept();
    PendingOperation *hangup(CallStateChangeReason reason,
            const QString &detailedReason, const QString &message);

    CallContents contents() const;
    CallContents contentsForType(MediaStreamType type) const;
    PendingCallContent *requestContent(const QString &name, MediaStreamType type);
    PendingOperation *removeContent(const CallContentPtr &content,
            ContentRemovalReason reason, const QString &detailedReason, const QString &message);

    LocalHoldState localHoldState() const;
    LocalHoldStateReason localHoldStateReason() const;
    PendingOperation *requestHold(bool hold);

Q_SIGNALS:
    void contentAdded(const Tp::CallContentPtr &content);
    void contentRemoved(const Tp::CallContentPtr &content);
    void stateChanged(Tp::CallState state);

    void localHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);

protected:
    CallChannel(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const Feature &coreFeature = Channel::FeatureCore);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onContentAdded(const QDBusObjectPath &contentPath);
    void onContentRemoved(const QDBusObjectPath &contentPath);
    void onContentReady(Tp::PendingOperation *op);
    void onCallStateChanged(uint state, uint flags,
            const Tp::CallStateReason &stateReason, const QVariantMap &stateDetails);

    void gotLocalHoldState(QDBusPendingCallWatcher *);
    void onLocalHoldStateChanged(uint, uint);

private:
    friend class PendingCallContent;

    CallContentPtr addContent(const QDBusObjectPath &contentPath);
    CallContentPtr lookupContent(const QDBusObjectPath &contentPath) const;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
