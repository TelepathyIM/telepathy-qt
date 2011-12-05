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

#include <TelepathyQt/Account>
#include <TelepathyQt/Message>
#include <TelepathyQt/SimpleObserver>
#include <TelepathyQt/TextChannel>

namespace Tp
{

struct TP_QT_NO_EXPORT SimpleTextObserver::Private
{
    Private(SimpleTextObserver *parent, const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization);
    ~Private();

    class TextChannelWrapper;

    SimpleTextObserver *parent;
    AccountPtr account;
    QString contactIdentifier;
    SimpleObserverPtr observer;
    QHash<ChannelPtr, TextChannelWrapper*> channels;
};

class TP_QT_NO_EXPORT SimpleTextObserver::Private::TextChannelWrapper :
                public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TextChannelWrapper)

public:
    TextChannelWrapper(const Tp::TextChannelPtr &channel);
    ~TextChannelWrapper() { }

Q_SIGNALS:
    void channelMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void channelMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);

private Q_SLOTS:
    void onChannelMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken);
    void onChannelMessageReceived(const Tp::ReceivedMessage &message);

private:
    TextChannelPtr mChannel;
};

} // Tp
