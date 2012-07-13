/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt_simple_call_observer_h_HEADER_GUARD_
#define _TelepathyQt_simple_call_observer_h_HEADER_GUARD_

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

#include <QObject>

namespace Tp
{

class PendingOperation;

class TP_QT_EXPORT SimpleCallObserver : public QObject,
                public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(SimpleCallObserver)
    Q_FLAGS(CallDirection CallDirections)

public:
    enum CallDirection {
        CallDirectionIncoming = 0x01,
        CallDirectionOutgoing = 0x02,
        CallDirectionBoth = CallDirectionIncoming | CallDirectionOutgoing
    };
    Q_DECLARE_FLAGS(CallDirections, CallDirection)

    static SimpleCallObserverPtr create(const AccountPtr &account,
            CallDirection direction = CallDirectionBoth);
    static SimpleCallObserverPtr create(const AccountPtr &account,
            const ContactPtr &contact,
            CallDirection direction = CallDirectionBoth);
    static SimpleCallObserverPtr create(const AccountPtr &account,
            const QString &contactIdentifier,
            CallDirection direction = CallDirectionBoth);

    virtual ~SimpleCallObserver();

    AccountPtr account() const;
    QString contactIdentifier() const;
    CallDirection direction() const;

    QList<CallChannelPtr> calls() const;
    TP_QT_DEPRECATED QList<StreamedMediaChannelPtr> streamedMediaCalls() const;

Q_SIGNALS:
    void callStarted(const Tp::CallChannelPtr &channel);
    void callEnded(const Tp::CallChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

    void streamedMediaCallStarted(const Tp::StreamedMediaChannelPtr &channel);
    void streamedMediaCallEnded(const Tp::StreamedMediaChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onNewChannels(const QList<Tp::ChannelPtr> &channels);
    TP_QT_NO_EXPORT void onChannelInvalidated(const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

private:
    TP_QT_NO_EXPORT static SimpleCallObserverPtr create(
            const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization,
            CallDirection direction);

    TP_QT_NO_EXPORT SimpleCallObserver(const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization,
            CallDirection direction);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
