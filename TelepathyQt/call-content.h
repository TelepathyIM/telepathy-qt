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

#ifndef _TelepathyQt_call_content_h_HEADER_GUARD_
#define _TelepathyQt_call_content_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-call-content.h>

#include <TelepathyQt/CallStream>

namespace Tp
{

typedef QList<CallContentPtr> CallContents;

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
    void streamRemoved(const Tp::CallStreamPtr &stream, const Tp::CallStateReason &reason);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void onStreamsAdded(const Tp::ObjectPathList &streamPath);
    void onStreamsRemoved(const Tp::ObjectPathList &streamPath, const Tp::CallStateReason &reason);
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

} // Tp

#endif
