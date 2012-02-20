/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_call_channel_h_HEADER_GUARD_
#define _TelepathyQt_call_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/CallContent>

namespace Tp
{

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
            const QString &contentName, MediaStreamType type, MediaStreamDirection direction);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT CallChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(CallChannel)

public:
    static const Feature FeatureCore;
    static const Feature FeatureCallState;
    static const Feature FeatureContents;
    static const Feature FeatureLocalHoldState;

    // TODO: FeatureCallMembers
    // TODO: add helpers to ensure/create call channel using Account

    static CallChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~CallChannel();

    bool handlerStreamingRequired() const;
    StreamTransportType initialTransportType() const;
    bool hasInitialAudio() const;
    bool hasInitialVideo() const;
    QString initialAudioName() const;
    QString initialVideoName() const;
    bool hasMutableContents() const;

    PendingOperation *setRinging();
    PendingOperation *setQueued();
    PendingOperation *accept();
    PendingOperation *hangup(CallStateChangeReason reason = CallStateChangeReasonUserRequested,
            const QString &detailedReason = QString(), const QString &message = QString());

    // FeatureCallState
    CallState callState() const;
    CallFlags callFlags() const;
    CallStateReason callStateReason() const;
    QVariantMap callStateDetails() const;

    // FeatureContents
    CallContents contents() const;
    CallContents contentsForType(MediaStreamType type) const;
    PendingCallContent *requestContent(const QString &name,
            MediaStreamType type, MediaStreamDirection direction);

    // FeatureLocalHoldState
    LocalHoldState localHoldState() const;
    LocalHoldStateReason localHoldStateReason() const;
    PendingOperation *requestHold(bool hold);

Q_SIGNALS:
    // FeatureCallState
    void callStateChanged(Tp::CallState state);
    void callFlagsChanged(Tp::CallFlags flags);

    // FeatureContents
    void contentAdded(const Tp::CallContentPtr &content);
    void contentRemoved(const Tp::CallContentPtr &content, const Tp::CallStateReason &reason);

    // FeatureLocalHoldState
    void localHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);

protected:
    CallChannel(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const Feature &coreFeature = Channel::FeatureCore);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);

    void gotCallState(QDBusPendingCallWatcher *watcher);
    void onCallStateChanged(uint state, uint flags,
            const Tp::CallStateReason &stateReason, const QVariantMap &stateDetails);

    void gotContents(QDBusPendingCallWatcher *watcher);
    void onContentAdded(const QDBusObjectPath &contentPath);
    void onContentRemoved(const QDBusObjectPath &contentPath, const Tp::CallStateReason &reason);
    void onContentReady(Tp::PendingOperation *op);

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
