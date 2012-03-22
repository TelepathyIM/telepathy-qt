/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#ifndef _TelepathyQt_call_stream_h_HEADER_GUARD_
#define _TelepathyQt_call_stream_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-call-stream.h>

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/OptionalInterfaceFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>
#include <TelepathyQt/SharedPtr>

namespace Tp
{

typedef QList<CallStreamPtr> CallStreams;

class TP_QT_EXPORT CallStream : public StatefulDBusProxy,
                    public OptionalInterfaceFactory<CallStream>
{
    Q_OBJECT
    Q_DISABLE_COPY(CallStream)

public:
    ~CallStream();

    CallContentPtr content() const;

    Contacts remoteMembers() const;
    bool canRequestReceiving() const;

    SendingState localSendingState() const;
    SendingState remoteSendingState(const ContactPtr &contact) const;

    PendingOperation *requestSending(bool send);
    PendingOperation *requestReceiving(const ContactPtr &contact, bool receive);

Q_SIGNALS:
    void localSendingStateChanged(Tp::SendingState localSendingState,
            const Tp::CallStateReason &reason);
    void remoteSendingStateChanged(
            const QHash<Tp::ContactPtr, Tp::SendingState> &remoteSendingStates,
            const Tp::CallStateReason &reason);
    void remoteMembersRemoved(const Tp::Contacts &remoteMembers,
            const Tp::CallStateReason &reason);

private Q_SLOTS:
    TP_QT_NO_EXPORT void gotMainProperties(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void gotRemoteMembersContacts(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onRemoteMembersChanged(const Tp::ContactSendingStateMap &updates,
            const Tp::HandleIdentifierMap &identifiers,
            const Tp::UIntList &removed,
            const Tp::CallStateReason &reason);
    TP_QT_NO_EXPORT void onLocalSendingStateChanged(uint, const Tp::CallStateReason &reason);

private:
    friend class CallChannel;
    friend class CallContent;

    TP_QT_NO_EXPORT static const Feature FeatureCore;

    TP_QT_NO_EXPORT CallStream(const CallContentPtr &content, const QDBusObjectPath &streamPath);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
