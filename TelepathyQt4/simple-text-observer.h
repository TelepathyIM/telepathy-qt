/**
 * This file is part of TelepathyQt4
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

#ifndef _TelepathyQt4_simple_text_observer_h_HEADER_GUARD_
#define _TelepathyQt4_simple_text_observer_h_HEADER_GUARD_

#include <TelepathyQt4/AbstractClientObserver>
#include <TelepathyQt4/Types>

#include <QObject>

namespace Tp
{

class Message;
class ReceivedMessage;

class TELEPATHY_QT4_EXPORT SimpleTextObserver : public QObject, public AbstractClientObserver
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

Q_SIGNALS:
    void messageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void messageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onChannelInvalidated(const Tp::TextChannelPtr &channel);
    TELEPATHY_QT4_NO_EXPORT void onChannelMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    TELEPATHY_QT4_NO_EXPORT void onChannelMessageReceived(const Tp::ReceivedMessage &message,
            const Tp::TextChannelPtr &channel);

private:
    TELEPATHY_QT4_NO_EXPORT SimpleTextObserver(const ClientRegistrarPtr &cr,
            const ChannelClassSpecList &channelFilter,
            const AccountPtr &account, const QString &contactIdentifier);

    TELEPATHY_QT4_NO_EXPORT void observeChannels(
            const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const ObserverInfo &observerInfo);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
