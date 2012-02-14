/*
 * This file is part of TelepathyQt4Yell
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4Yell_call_channel_h_HEADER_GUARD_
#define _TelepathyQt4Yell_call_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_YELL_HEADER
#error IN_TELEPATHY_QT4_YELL_HEADER
#endif

#include <TelepathyQt4Yell/Channel>
#include <TelepathyQt4Yell/Constants>
#include <TelepathyQt4Yell/Types>

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>
#include <TelepathyQt/SharedPtr>

namespace Tpy
{

class CallChannel;

typedef QList<CallContentPtr> CallContents;
typedef QList<CallStreamPtr> CallStreams;

class TELEPATHY_QT4_YELL_EXPORT CallStream : public Tp::StatefulDBusProxy,
                    public Tp::OptionalInterfaceFactory<CallStream>
{
    Q_OBJECT
    Q_DISABLE_COPY(CallStream)

public:
    ~CallStream();

    CallContentPtr content() const;

    Tp::Contacts members() const;

    SendingState localSendingState() const;
    SendingState remoteSendingState(const Tp::ContactPtr &contact) const;

    Tp::PendingOperation *requestSending(bool send);
    Tp::PendingOperation *requestReceiving(const Tp::ContactPtr &contact, bool receive);

Q_SIGNALS:
    void localSendingStateChanged(Tpy::SendingState localSendingState);
    void remoteSendingStateChanged(
            const QHash<Tp::ContactPtr, Tpy::SendingState> &remoteSendingStates);
    void remoteMembersRemoved(const Tp::Contacts &remoteMembers);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void gotRemoteMembersContacts(Tp::PendingOperation *op);

    void onRemoteMembersChanged(const Tpy::ContactSendingStateMap &updates,
            const Tpy::UIntList &removed);
    void onLocalSendingStateChanged(uint);

private:
    friend class CallChannel;
    friend class CallContent;

    static const Tp::Feature FeatureCore;

    CallStream(const CallContentPtr &content, const QDBusObjectPath &streamPath);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_YELL_EXPORT PendingCallContent : public Tp::PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingCallContent)

public:
    ~PendingCallContent();

    CallContentPtr content() const;

private Q_SLOTS:
    void gotContent(QDBusPendingCallWatcher *watcher);

    void onContentReady(Tp::PendingOperation *op);
    void onContentRemoved(const Tpy::CallContentPtr &content);

private:
    friend class CallChannel;

    PendingCallContent(const CallChannelPtr &channel,
            const QString &contentName, Tp::MediaStreamType type);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_YELL_EXPORT CallContent : public Tp::StatefulDBusProxy,
                    public Tp::OptionalInterfaceFactory<CallContent>
{
    Q_OBJECT
    Q_DISABLE_COPY(CallContent)

public:
    ~CallContent();

    CallChannelPtr channel() const;

    QString name() const;
    Tp::MediaStreamType type() const;

    CallContentDisposition disposition() const;

    CallStreams streams() const;

Q_SIGNALS:
    void streamAdded(const Tpy::CallStreamPtr &stream);
    void streamRemoved(const Tpy::CallStreamPtr &stream);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onStreamsAdded(const Tpy::ObjectPathList &streamPath);
    void onStreamsRemoved(const Tpy::ObjectPathList &streamPath);
    void onStreamReady(Tp::PendingOperation *op);

private:
    friend class CallChannel;
    friend class PendingCallContent;

    static const Tp::Feature FeatureCore;

    CallContent(const CallChannelPtr &channel,
            const QDBusObjectPath &contentPath);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_YELL_EXPORT CallChannel : public Tp::Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(CallChannel)

public:
    static const Tp::Feature FeatureContents;
    static const Tp::Feature FeatureLocalHoldState;

    // TODO: add helpers to ensure/create call channel using Account

    static CallChannelPtr create(const Tp::ConnectionPtr &connection,
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

    Tp::PendingOperation *accept();
    Tp::PendingOperation *hangup(CallStateChangeReason reason,
            const QString &detailedReason, const QString &message);

    CallContents contents() const;
    CallContents contentsForType(Tp::MediaStreamType type) const;
    PendingCallContent *requestContent(const QString &name, Tp::MediaStreamType type);
    Tp::PendingOperation *removeContent(const CallContentPtr &content,
            ContentRemovalReason reason, const QString &detailedReason, const QString &message);

    Tp::LocalHoldState localHoldState() const;
    Tp::LocalHoldStateReason localHoldStateReason() const;
    Tp::PendingOperation *requestHold(bool hold);

Q_SIGNALS:
    void contentAdded(const Tpy::CallContentPtr &content);
    void contentRemoved(const Tpy::CallContentPtr &content);
    void stateChanged(Tpy::CallState state);

    void localHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);

protected:
    CallChannel(const Tp::ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const Tp::Feature &coreFeature = Tp::Channel::FeatureCore);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onContentAdded(const QDBusObjectPath &contentPath);
    void onContentRemoved(const QDBusObjectPath &contentPath);
    void onContentReady(Tp::PendingOperation *op);
    void onCallStateChanged(uint state, uint flags,
            const Tpy::CallStateReason &stateReason, const QVariantMap &stateDetails);

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
