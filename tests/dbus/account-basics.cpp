#include <QtCore/QEventLoop>
#include <QtTest/QtTest>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountCapabilityFilter>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/AccountPropertyFilter>
#include <TelepathyQt4/AccountSet>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <tests/lib/test.h>

using namespace Tp;

class TestAccountBasics : public Test
{
    Q_OBJECT

public:
    TestAccountBasics(QObject *parent = 0)
        : Test(parent), mServiceNameChanged(false), mAccountsCount(0), mGotAvatarChanged(false)
    { }

protected Q_SLOTS:
    void onNewAccount(const Tp::AccountPtr &);
    void onAccountServiceNameChanged(const QString &);
    void onAvatarChanged(const Tp::Avatar &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();

private:
    QStringList pathsForAccountList(const QList<Tp::AccountPtr> &list);
    QStringList pathsForAccountSet(const Tp::AccountSetPtr &set);

    Tp::AccountManagerPtr mAM;
    bool mServiceNameChanged;
    QString mServiceName;
    int mAccountsCount;
    bool mGotAvatarChanged;
};

void TestAccountBasics::onNewAccount(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountsCount++;
    mLoop->exit(0);
}

void TestAccountBasics::onAccountServiceNameChanged(const QString &serviceName)
{
    mServiceNameChanged = true;
    mServiceName = serviceName;
    mLoop->exit(0);
}

void TestAccountBasics::onAvatarChanged(const Tp::Avatar &avatar)
{
    qDebug() << "on avatar changed";
    mGotAvatarChanged = true;
    QCOMPARE(avatar.avatarData, QByteArray("asdfg"));
    QCOMPARE(avatar.MIMEType, QString(QLatin1String("image/jpeg")));
    mLoop->exit(0);
}

QStringList TestAccountBasics::pathsForAccountList(const QList<Tp::AccountPtr> &list)
{
    QStringList ret;
    Q_FOREACH (const AccountPtr &account, list) {
        ret << account->objectPath();
    }
    return ret;
}

QStringList TestAccountBasics::pathsForAccountSet(const Tp::AccountSetPtr &set)
{
    QStringList ret;
    Q_FOREACH (const AccountPtr &account, set->accounts()) {
        ret << account->objectPath();
    }
    return ret;
}

void TestAccountBasics::initTestCase()
{
    initTestCaseImpl();

    mAM = AccountManager::create(AccountFactory::create(QDBusConnection::sessionBus(),
                Account::FeatureCore | Account::FeatureCapabilities));
    QCOMPARE(mAM->isReady(), false);
}

void TestAccountBasics::init()
{
    mGotAvatarChanged = false;

    initImpl();
}

void TestAccountBasics::testBasics()
{
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVERIFY(connect(mAM.data(),
                    SIGNAL(newAccount(const Tp::AccountPtr &)),
                    SLOT(onNewAccount(const Tp::AccountPtr &))));

    QVariantMap parameters;
    parameters[QLatin1String("account")] = QLatin1String("foobar");
    PendingAccount *pacc = mAM->createAccount(QLatin1String("foo"),
            QLatin1String("bar"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pacc->account());

    while (mAccountsCount != 1) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mAM->interfaces(), QStringList());

    QStringList paths;
    QCOMPARE(pathsForAccountSet(mAM->validAccountsSet()),
            QStringList() <<
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QCOMPARE(pathsForAccountSet(mAM->invalidAccountsSet()),
            QStringList());
    QCOMPARE(pathsForAccountList(mAM->allAccounts()),
            QStringList() <<
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));

    AccountPtr acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->displayName(), QString(QLatin1String("foobar (account 0)")));

    qDebug() << "making Account::FeatureAvatar ready";

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    qDebug() << "connecting to avatarChanged()";

    QVERIFY(connect(acc.data(),
                    SIGNAL(avatarChanged(const Tp::Avatar &)),
                    SLOT(onAvatarChanged(const Tp::Avatar &))));

    qDebug() << "setting the avatar";

    Tp::Avatar avatar = { QByteArray("asdfg"), QLatin1String("image/jpeg") };
    QVERIFY(connect(acc->setAvatar(avatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "making Account::FeatureAvatar ready again (redundantly)";

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);

    // We might have got it already in the earlier mainloop runs
    if (!mGotAvatarChanged) {
        qDebug() << "waiting for the avatarChanged signal";
        QCOMPARE(mLoop->exec(), 0);
    } else {
        qDebug() << "not waiting for avatarChanged because we already got it";
    }

    qDebug() << "creating another account";

    pacc = mAM->createAccount(QLatin1String("spurious"),
            QLatin1String("normal"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    while (mAccountsCount != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }

    acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"));
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"));

    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // At this point, there's a set icon
    QCOMPARE(acc->iconName(), QLatin1String("bob.png")); // ?!??

    // Unset that
    QVERIFY(connect(acc->setIconName(QString()),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // Now that it's unset, an icon name is formed from the protocol name
    QCOMPARE(acc->iconName(), QLatin1String("im-normal"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureProtocolInfo), true);

    // This time it's fetched from the protocol object (although it probably internally just
    // infers it from the protocol name too)
    QCOMPARE(acc->iconName(), QLatin1String("im-normal"));

    QVERIFY(acc->serviceName() != acc->protocolName());
    QCOMPARE(acc->serviceName(), QString(QLatin1String("bob_service")));
    connect(acc.data(),
            SIGNAL(serviceNameChanged(const QString &)),
            SLOT(onAccountServiceNameChanged(const QString &)));
    connect(acc->setServiceName(acc->protocolName()),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation *)));
    // wait for setServiceName finish
    QCOMPARE(mLoop->exec(), 0);
    // wait for serviceNameChanged
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->serviceName(), acc->protocolName());
    QCOMPARE(mServiceName, acc->serviceName());

    ProtocolInfo *protocolInfo = acc->protocolInfo();
    QCOMPARE((bool) protocolInfo, !((ProtocolInfo *) 0));
    QCOMPARE(protocolInfo->hasParameter(QLatin1String("account")), true);
    QCOMPARE(protocolInfo->hasParameter(QLatin1String("password")), true);
    QCOMPARE(protocolInfo->hasParameter(QLatin1String("register")), true);

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar | Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar | Account::FeatureProtocolInfo), true);

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));
    protocolInfo = acc->protocolInfo();
    QCOMPARE((bool) protocolInfo, !((ProtocolInfo *) 0));

    QList<AccountPtr> allAccounts = mAM->allAccounts();

    QVariantMap filter;
    Tp::AccountSetPtr filteredAccountSet;

    filter.insert(QLatin1String("protocolName"), QLatin1String("foo"));
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filter));
    QCOMPARE(filteredAccountSet->isFilterValid(), true);

    filter.clear();
    filter.insert(QLatin1String("protocolFoo"), acc->protocolName());
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filter));
    QCOMPARE(filteredAccountSet->isFilterValid(), false);

    filter.clear();
    filter.insert(QLatin1String("protocolName"), allAccounts.first()->protocolName());
    filteredAccountSet = mAM->filterAccounts(filter);
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QVERIFY(filteredAccountSet->accounts().contains(allAccounts.first()));

    filteredAccountSet = mAM->accountsByProtocol(allAccounts.first()->protocolName());
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QVERIFY(filteredAccountSet->accounts().contains(allAccounts.first()));

    filter.clear();
    filter.insert(QLatin1String("protocolFoo"), acc->protocolName());
    filteredAccountSet = mAM->filterAccounts(filter);
    QCOMPARE(filteredAccountSet->isFilterValid(), false);
    QVERIFY(filteredAccountSet->accounts().isEmpty());

    QCOMPARE(mAM->validAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->validAccountsSet()->accounts().isEmpty(), false);
    QCOMPARE(mAM->invalidAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->invalidAccountsSet()->accounts().isEmpty(), true);
    QCOMPARE(mAM->enabledAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->enabledAccountsSet()->accounts().isEmpty(), false);
    QCOMPARE(mAM->disabledAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->disabledAccountsSet()->accounts().isEmpty(), true);

    filter.clear();
    filter.insert(QLatin1String("cmName"), QLatin1String("spurious"));
    AccountSetPtr spuriousAccountSet = AccountSetPtr(new AccountSet(mAM, filter));
    QCOMPARE(spuriousAccountSet->isFilterValid(), true);

    filteredAccountSet = mAM->textChatAccountsSet();
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QCOMPARE(filteredAccountSet->accounts(), spuriousAccountSet->accounts());

    QCOMPARE(mAM->textChatAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->textChatRoomAccountsSet()->accounts().isEmpty(), true);
    QCOMPARE(mAM->mediaCallAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->mediaCallAccountsSet()->accounts().isEmpty(), true);
    QCOMPARE(mAM->audioCallAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->audioCallAccountsSet()->accounts().isEmpty(), true);
    QCOMPARE(mAM->videoCallAccountsSet(false)->isFilterValid(), true);
    QCOMPARE(mAM->videoCallAccountsSet(false)->accounts().isEmpty(), true);
    QCOMPARE(mAM->videoCallAccountsSet(true)->isFilterValid(), true);
    QCOMPARE(mAM->videoCallAccountsSet(true)->accounts().isEmpty(), true);
    QCOMPARE(mAM->fileTransferAccountsSet()->isFilterValid(), true);
    QCOMPARE(mAM->fileTransferAccountsSet()->accounts().isEmpty(), true);

    QList<AccountFilterConstPtr> filterChain;

    AccountPropertyFilterPtr cmNameFilter = AccountPropertyFilter::create();
    cmNameFilter->addProperty(QLatin1String("cmName"), QLatin1String("spurious"));

    /* match fixedProperties is complete and allowedProperties is a subset of
     * the allowed properties */
    filterChain.clear();
    RequestableChannelClassList rccs;
    RequestableChannelClass rcc;
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"));
    rccs.append(rcc);
    filterChain.append(AccountCapabilityFilter::create(rccs));
    filterChain.append(cmNameFilter);
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filterChain));
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QCOMPARE(filteredAccountSet->accounts(), spuriousAccountSet->accounts());

    /* match fixedProperties and allowedProperties is complete */
    filterChain.clear();
    rccs.clear();
    rcc.fixedProperties.clear();
    rcc.allowedProperties.clear();
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"));
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"));
    rccs.append(rcc);
    filterChain.append(AccountCapabilityFilter::create(rccs));
    filterChain.append(cmNameFilter);
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filterChain));
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QCOMPARE(filteredAccountSet->accounts(), spuriousAccountSet->accounts());

    /* should not match as fixedProperties lack TargetHandleType */
    filterChain.clear();
    rccs.clear();
    rcc.fixedProperties.clear();
    rcc.allowedProperties.clear();
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"));
    rccs.append(rcc);
    filterChain.append(AccountCapabilityFilter::create(rccs));
    filterChain.append(cmNameFilter);
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filterChain));
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QCOMPARE(filteredAccountSet->accounts().isEmpty(), true);

    /* should not match as fixedProperties has more than expected */
    filterChain.clear();
    rccs.clear();
    rcc.fixedProperties.clear();
    rcc.allowedProperties.clear();
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
            (uint) HandleTypeContact);
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"));
    rccs.append(rcc);
    filterChain.append(AccountCapabilityFilter::create(rccs));
    filterChain.append(cmNameFilter);
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filterChain));
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QCOMPARE(filteredAccountSet->accounts().isEmpty(), true);

    /* should not match as allowedProperties has TargetFoo that is not allowed */
    filterChain.clear();
    rccs.clear();
    rcc.fixedProperties.clear();
    rcc.allowedProperties.clear();
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    rcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            (uint) HandleTypeContact);
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"));
    rcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetFoo"));
    rccs.append(rcc);
    filterChain.append(AccountCapabilityFilter::create(rccs));
    filterChain.append(cmNameFilter);
    filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filterChain));
    QCOMPARE(filteredAccountSet->isFilterValid(), true);
    QCOMPARE(filteredAccountSet->accounts().isEmpty(), true);
}

void TestAccountBasics::cleanup()
{
    cleanupImpl();
}

void TestAccountBasics::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountBasics)
#include "_gen/account-basics.cpp.moc.hpp"
