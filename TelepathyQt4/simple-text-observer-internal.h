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

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Message>
#include <TelepathyQt4/TextChannel>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT SimpleTextObserver::Private
{
    Private(SimpleTextObserver *parent, const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization);

    void processMessageQueue();
    bool filterMessage(const Message &message, const TextChannelPtr &textChannel);

    class FakeAccountFactory;
    class Observer;
    class TextChannelWrapper;
    class TextMessageInfo;

    SimpleTextObserver *parent;
    AccountPtr account;
    QString contactIdentifier;
    QString normalizedContactIdentifier;
    bool requiresNormalization;
    QList<TextMessageInfo> messageQueue;
    SharedPtr<Observer> observer;
    static QHash<AccountPtr, SharedPtr<Observer> > observers;
    static uint numObservers;
};

class TELEPATHY_QT4_NO_EXPORT SimpleTextObserver::Private::Observer : public QObject,
                public AbstractClientObserver
{
    Q_OBJECT
    Q_DISABLE_COPY(Observer)

public:
    Observer(const ClientRegistrarPtr &cr,
             const ChannelClassSpecList &channelFilter,
             const AccountPtr &account);
    ~Observer();

    ClientRegistrarPtr clientRegistrar() const { return mCr; }
    AccountPtr account() const { return mAccount; }

    void observeChannels(
            const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const ObserverInfo &observerInfo);

Q_SIGNALS:
    void messageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void messageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);

private Q_SLOTS:
    void onChannelInvalidated(const Tp::TextChannelPtr &channel);

private:
    ClientRegistrarPtr mCr;
    AccountPtr mAccount;
    QHash<TextChannelPtr, TextChannelWrapper*> mChannels;
};


class TELEPATHY_QT4_NO_EXPORT SimpleTextObserver::Private::FakeAccountFactory :
                public AccountFactory
{
public:
    static AccountFactoryPtr create(const AccountPtr &account)
    {
        return AccountFactoryPtr(new FakeAccountFactory(account));
    }

    ~FakeAccountFactory() { }

    AccountPtr account() const { return mAccount; }

protected:
    AccountPtr construct(const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory) const
    {
        if (mAccount->objectPath() != objectPath) {
            return AccountFactory::construct(busName, objectPath, connFactory, chanFactory,
                    contactFactory);
        }
        return mAccount;
    }

private:
    FakeAccountFactory(const AccountPtr &account)
        : AccountFactory(account->dbusConnection(), Features()),
          mAccount(account)
    {
    }

    AccountPtr mAccount;
};

class TELEPATHY_QT4_NO_EXPORT SimpleTextObserver::Private::TextChannelWrapper :
                public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TextChannelWrapper)

public:
    TextChannelWrapper(const Tp::TextChannelPtr &channel);
    ~TextChannelWrapper() { }

    TextChannelPtr channel() const { return mChannel; }

Q_SIGNALS:
    void channelInvalidated(const Tp::TextChannelPtr &channel);
    void channelMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void channelMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);

private Q_SLOTS:
    void onChannelInvalidated();
    void onChannelReady();
    void onChannelMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken);
    void onChannelMessageReceived(const Tp::ReceivedMessage &message);

private:
    TextChannelPtr mChannel;
};

struct TELEPATHY_QT4_NO_EXPORT SimpleTextObserver::Private::TextMessageInfo
{
    enum Type {
        Sent = 0,
        Received
    };

    TextMessageInfo(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel)
        : message(message),
          channel(channel),
          type(Received)
    {
    }

    TextMessageInfo(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel)
        : message(message),
          flags(flags),
          sentMessageToken(sentMessageToken),
          channel(channel),
          type(Sent)
    {
    }

    Tp::Message message;
    Tp::MessageSendingFlags flags;
    QString sentMessageToken;
    TextChannelPtr channel;
    Type type;
};

} // Tp
