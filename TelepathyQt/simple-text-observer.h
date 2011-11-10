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

#ifndef _TelepathyQt_simple_text_observer_h_HEADER_GUARD_
#define _TelepathyQt_simple_text_observer_h_HEADER_GUARD_

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

#include <QObject>

namespace Tp
{

class Message;
class PendingOperation;
class ReceivedMessage;

class TP_QT_EXPORT SimpleTextObserver : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(SimpleTextObserver)

public:
    static SimpleTextObserverPtr create(const AccountPtr &account);
    static SimpleTextObserverPtr create(const AccountPtr &account,
            const ContactPtr &contact);
    static SimpleTextObserverPtr create(const AccountPtr &account,
            const QString &contactIdentifier);

    virtual ~SimpleTextObserver();

    AccountPtr account() const;
    QString contactIdentifier() const;

    QList<TextChannelPtr> textChats() const;

Q_SIGNALS:
    void messageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void messageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onNewChannels(const QList<Tp::ChannelPtr> &channels);
    TP_QT_NO_EXPORT void onChannelInvalidated(const Tp::ChannelPtr &channel);

private:
    TP_QT_NO_EXPORT static SimpleTextObserverPtr create(const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization);

    TP_QT_NO_EXPORT SimpleTextObserver(const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
