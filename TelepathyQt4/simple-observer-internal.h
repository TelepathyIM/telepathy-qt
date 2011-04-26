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
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/ClientRegistrar>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private
{
    Private(SimpleObserver *parent,
            const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QString &contactIdentifier,
            bool requiresNormalization,
            const QList<ChannelClassFeatures> &extraChannelFeatures);

    bool filterChannel(const AccountPtr &channelAccount, const ChannelPtr &channel);

    void processChannelsQueue();
    void processNewChannelsQueue();
    void processChannelsInvalidationQueue();

    class FakeAccountFactory;
    class Observer;
    class ChannelWrapper;
    struct NewChannelsInfo;
    struct ChannelInvadationInfo;

    SimpleObserver *parent;
    AccountPtr account;
    ChannelClassSpecList channelFilter;
    QString contactIdentifier;
    QString normalizedContactIdentifier;
    QList<ChannelClassFeatures> extraChannelFeatures;
    SharedPtr<Observer> observer;
    QQueue<void (SimpleObserver::Private::*)()> channelsQueue;
    QQueue<ChannelInvadationInfo> channelsInvalidationQueue;
    QQueue<NewChannelsInfo> newChannelsQueue;
    static QHash<QPair<QString, ChannelClassSpecList>, QWeakPointer<Observer> > observers;
    static uint numObservers;
};

class TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::FakeAccountFactory :
                public AccountFactory
{
    Q_OBJECT

public:
    static SharedPtr<FakeAccountFactory> create(const QDBusConnection &bus)
    {
        return SharedPtr<FakeAccountFactory>(new FakeAccountFactory(bus));
    }

    ~FakeAccountFactory() { }

private:
    friend class Observer;

    FakeAccountFactory(const QDBusConnection &bus)
        : AccountFactory(bus, Features())
    {
    }

    AccountPtr construct(const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory) const
    {
        foreach (const AccountPtr &account, mAccounts) {
            if (account->objectPath() == objectPath) {
                return account;
            }
        }
        return AccountFactory::construct(busName, objectPath, connFactory,
                chanFactory, contactFactory);
    }

    QList<AccountPtr> accounts() const { return mAccounts; }
    void registerAccount(const AccountPtr &account) { mAccounts.append(account); }

    QList<AccountPtr> mAccounts;
};

class TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::Observer : public QObject,
                public AbstractClientObserver
{
    Q_OBJECT
    Q_DISABLE_COPY(Observer)

public:
    struct ContextInfo
    {
        ContextInfo() {}
        ContextInfo(const MethodInvocationContextPtr<> &context,
                const AccountPtr &account,
                const QList<ChannelPtr> &channels)
            : context(context),
              account(account),
              channels(channels)
        {
        }

        MethodInvocationContextPtr<> context;
        AccountPtr account;
        QList<ChannelPtr> channels;
    };

    Observer(const ClientRegistrarPtr &cr,
             const SharedPtr<FakeAccountFactory> &fakeAccountFactory,
             const ChannelClassSpecList &channelFilter,
             const QList<ChannelClassFeatures> &extraChannelFeatures);
    ~Observer();

    ClientRegistrarPtr clientRegistrar() const { return mCr; }
    SharedPtr<FakeAccountFactory> fakeAccountFactory() const { return mFakeAccountFactory; }
    QList<ChannelClassFeatures> extraChannelFeatures() const { return mExtraChannelFeatures; }

    QList<AccountPtr> accounts() const { return mAccounts; }
    void registerAccount(const AccountPtr &account)
    {
        mAccounts.append(account);
        mFakeAccountFactory->registerAccount(account);
    }

    QList<ChannelPtr> channels() const { return mChannels.keys(); }

    void observeChannels(
            const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const ObserverInfo &observerInfo);

Q_SIGNALS:
    void newChannels(const Tp::AccountPtr &channelsAccount, const QList<Tp::ChannelPtr> &channels);
    void channelInvalidated(const Tp::AccountPtr &channelAccount, const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

private Q_SLOTS:
    void onChannelInvalidated(const Tp::AccountPtr &channelAccount, const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);
    void onChannelsReady(Tp::PendingOperation *op);

private:
    Features featuresFor(const ChannelClassSpec &channelClass) const;

    ClientRegistrarPtr mCr;
    SharedPtr<FakeAccountFactory> mFakeAccountFactory;
    QList<ChannelClassFeatures> mExtraChannelFeatures;
    QList<AccountPtr> mAccounts;
    QHash<ChannelPtr, ChannelWrapper*> mChannels;
    QHash<ChannelPtr, ChannelWrapper*> mIncompleteChannels;
    QHash<PendingOperation*, ContextInfo*> mObserveChannelsInfo;
};

class TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::ChannelWrapper :
                public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelWrapper)

public:
    ChannelWrapper(const AccountPtr &channelAccount, const ChannelPtr &channel,
            const Features &extraChannelFeatures);
    ~ChannelWrapper() { }

    AccountPtr channelAccount() const { return mChannelAccount; }
    ChannelPtr channel() const { return mChannel; }
    Features extraChannelFeatures() const { return mExtraChannelFeatures; }

    PendingOperation *becomeReady();

Q_SIGNALS:
    void channelInvalidated(const Tp::AccountPtr &channelAccount, const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

private Q_SLOTS:
    void onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
            const QString &errorMessage);

private:
    AccountPtr mChannelAccount;
    ChannelPtr mChannel;
    Features mExtraChannelFeatures;
};

struct TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::NewChannelsInfo
{
    NewChannelsInfo();
    NewChannelsInfo(const QList<ChannelPtr> &channels)
        : channels(channels)
    {
    }

    QList<ChannelPtr> channels;
};

struct TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::ChannelInvadationInfo
{
    ChannelInvadationInfo();
    ChannelInvadationInfo(const ChannelPtr &channel, const QString &errorName,
            const QString &errorMessage)
        : channel(channel),
          errorName(errorName),
          errorMessage(errorMessage)
    {
    }

    ChannelPtr channel;
    QString errorName;
    QString errorMessage;
};

} // Tp
