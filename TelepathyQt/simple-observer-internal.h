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
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassFeatures>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT SimpleObserver::Private
{
    Private(SimpleObserver *parent,
            const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QString &contactIdentifier,
            bool requiresNormalization,
            const QList<ChannelClassFeatures> &extraChannelFeatures);

    bool filterChannel(const AccountPtr &channelAccount, const ChannelPtr &channel);
    void insertChannels(const AccountPtr &channelsAccount, const QList<ChannelPtr> &channels);
    void removeChannel(const AccountPtr &channelAccount, const ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

    void processChannelsQueue();
    void processNewChannelsQueue();
    void processChannelsInvalidationQueue();

    class FakeAccountFactory;
    class Observer;
    class ChannelWrapper;
    struct NewChannelsInfo;
    struct ChannelInvalidationInfo;

    SimpleObserver *parent;
    AccountPtr account;
    ChannelClassSpecList channelFilter;
    QString contactIdentifier;
    QString normalizedContactIdentifier;
    QList<ChannelClassFeatures> extraChannelFeatures;
    ClientRegistrarPtr cr;
    SharedPtr<Observer> observer;
    QSet<ChannelPtr> channels;
    QQueue<void (SimpleObserver::Private::*)()> channelsQueue;
    QQueue<ChannelInvalidationInfo> channelsInvalidationQueue;
    QQueue<NewChannelsInfo> newChannelsQueue;
    static QHash<QPair<QString, QSet<ChannelClassSpec> >, WeakPtr<Observer> > observers;
    static uint numObservers;
};

class TP_QT_NO_EXPORT SimpleObserver::Private::FakeAccountFactory :
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
        if (mAccounts.contains(objectPath)) {
            return mAccounts.value(objectPath);
        }
        return AccountFactory::construct(busName, objectPath, connFactory,
                chanFactory, contactFactory);
    }

    QHash<QString, AccountPtr> accounts() const { return mAccounts; }
    void registerAccount(const AccountPtr &account)
    {
        mAccounts.insert(account->objectPath(), account);
    }

    QHash<QString, AccountPtr> mAccounts;
};

class TP_QT_NO_EXPORT SimpleObserver::Private::Observer : public QObject,
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

    Observer(const WeakPtr<ClientRegistrar> &cr,
             const SharedPtr<FakeAccountFactory> &fakeAccountFactory,
             const ChannelClassSpecList &channelFilter,
             const QString &observerName);
    ~Observer();

    WeakPtr<ClientRegistrar> clientRegistrar() const { return mCr; }
    SharedPtr<FakeAccountFactory> fakeAccountFactory() const { return mFakeAccountFactory; }

    QString observerName() const { return mObserverName; }

    QSet<ChannelClassFeatures> extraChannelFeatures() const { return mExtraChannelFeatures; }
    void registerExtraChannelFeatures(const QList<ChannelClassFeatures> &features)
    {
        mExtraChannelFeatures.unite(features.toSet());
    }

    QSet<AccountPtr> accounts() const { return mAccounts; }
    void registerAccount(const AccountPtr &account)
    {
        mAccounts.insert(account);
        mFakeAccountFactory->registerAccount(account);
    }

    QHash<ChannelPtr, ChannelWrapper*> channels() const { return mChannels; }

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

    WeakPtr<ClientRegistrar> mCr;
    SharedPtr<FakeAccountFactory> mFakeAccountFactory;
    QString mObserverName;
    QSet<ChannelClassFeatures> mExtraChannelFeatures;
    QSet<AccountPtr> mAccounts;
    QHash<ChannelPtr, ChannelWrapper*> mChannels;
    QHash<ChannelPtr, ChannelWrapper*> mIncompleteChannels;
    QHash<PendingOperation*, ContextInfo*> mObserveChannelsInfo;
};

class TP_QT_NO_EXPORT SimpleObserver::Private::ChannelWrapper :
                public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelWrapper)

public:
    ChannelWrapper(const AccountPtr &channelAccount, const ChannelPtr &channel,
            const Features &extraChannelFeatures, QObject *parent);
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

struct TP_QT_NO_EXPORT SimpleObserver::Private::NewChannelsInfo
{
    NewChannelsInfo();
    NewChannelsInfo(const AccountPtr &channelsAccount, const QList<ChannelPtr> &channels)
        : channelsAccount(channelsAccount),
          channels(channels)
    {
    }

    AccountPtr channelsAccount;
    QList<ChannelPtr> channels;
};

struct TP_QT_NO_EXPORT SimpleObserver::Private::ChannelInvalidationInfo
{
    ChannelInvalidationInfo();
    ChannelInvalidationInfo(const AccountPtr &channelAccount, const ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage)
        : channelAccount(channelAccount),
          channel(channel),
          errorName(errorName),
          errorMessage(errorMessage)
    {
    }

    AccountPtr channelAccount;
    ChannelPtr channel;
    QString errorName;
    QString errorMessage;
};

} // Tp
