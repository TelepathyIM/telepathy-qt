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
            const QList<ChannelFeatureSpec> &extraChannelFeatures);

    bool filterChannel(const Tp::ChannelPtr &channel);

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
    QList<ChannelFeatureSpec> extraChannelFeatures;
    SharedPtr<Observer> observer;
    static uint numObservers;
    QQueue<void (SimpleObserver::Private::*)()> channelsQueue;
    QQueue<ChannelInvadationInfo> channelsInvalidationQueue;
    QQueue<NewChannelsInfo> newChannelsQueue;
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
                const QList<ChannelPtr> &channels,
                const QDateTime &timestamp)
            : context(context),
              channels(channels),
              timestamp(timestamp)
        {
        }

        MethodInvocationContextPtr<> context;
        QList<ChannelPtr> channels;
        QDateTime timestamp;
        QHash<ChannelPtr, QDateTime> channelInvalidatedTimestamps;
    };

    Observer(const ClientRegistrarPtr &cr,
             const ChannelClassSpecList &channelFilter,
             const AccountPtr &account,
             const QList<ChannelFeatureSpec> &extraChannelFeatures);
    ~Observer();

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
    void newChannels(const QList<Tp::ChannelPtr> &channels, const QDateTime &timestamp);
    void channelInvalidated(const Tp::ChannelPtr &channel, const QString &errorName,
            const QString &errorMessage, const QDateTime &timestamp);

private Q_SLOTS:
    void onChannelInvalidated(const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);
    void onChannelsReady(Tp::PendingOperation *op);

private:
    Features featuresFor(const ChannelClassSpec &channelClass) const;

    ClientRegistrarPtr mCr;
    AccountPtr mAccount;
    QList<ChannelFeatureSpec> mExtraChannelFeatures;
    QHash<ChannelPtr, ChannelWrapper*> mChannels;
    QHash<ChannelPtr, ChannelWrapper*> mIncompleteChannels;
    QHash<PendingOperation*, ContextInfo*> mObserveChannelsInfo;
};

class TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::FakeAccountFactory :
                public AccountFactory
{
public:
    static AccountFactoryPtr create(const AccountPtr &account)
    {
        return AccountFactoryPtr(new FakeAccountFactory(account));
    }

    ~FakeAccountFactory() { }

private:
    FakeAccountFactory(const AccountPtr &account)
        : AccountFactory(account->dbusConnection(), Features()),
          mAccount(account)
    {
    }

    AccountPtr construct(const QString &busName, const QString &objectPath,
            const ConnectionFactoryConstPtr &connFactory,
            const ChannelFactoryConstPtr &chanFactory,
            const ContactFactoryConstPtr &contactFactory) const
    {
        if (mAccount->objectPath() != objectPath) {
            return AccountFactory::construct(busName, objectPath, connFactory,
                    chanFactory, contactFactory);
        }
        return mAccount;
    }

    AccountPtr mAccount;
};

class TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::ChannelWrapper :
                public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelWrapper)

public:
    ChannelWrapper(const Tp::ChannelPtr &channel,
            const Features &extraChannelFeatures);
    ~ChannelWrapper() { }

    PendingOperation *becomeReady();

Q_SIGNALS:
    void channelInvalidated(const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);

private Q_SLOTS:
    void onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
            const QString &errorMessage);

private:
    ChannelPtr mChannel;
    Features mExtraChannelFeatures;
};

struct TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::NewChannelsInfo
{
    NewChannelsInfo();
    NewChannelsInfo(const QList<ChannelPtr> &channels, const QDateTime &timestamp)
        : channels(channels),
          timestamp(timestamp)
    {
    }

    QList<ChannelPtr> channels;
    QDateTime timestamp;
};

struct TELEPATHY_QT4_NO_EXPORT SimpleObserver::Private::ChannelInvadationInfo
{
    ChannelInvadationInfo();
    ChannelInvadationInfo(const ChannelPtr &channel, const QString &errorName,
            const QString &errorMessage, const QDateTime &timestamp)
        : channel(channel),
          errorName(errorName),
          errorMessage(errorMessage),
          timestamp(timestamp)
    {
    }

    ChannelPtr channel;
    QString errorName;
    QString errorMessage;
    QDateTime timestamp;
};

} // Tp
