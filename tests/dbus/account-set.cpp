#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountCapabilityFilter>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountPropertyFilter>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/AndFilter>
#include <TelepathyQt/NotFilter>
#include <TelepathyQt/OrFilter>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestAccountSet : public Test
{
    Q_OBJECT

public:
    TestAccountSet(QObject *parent = 0)
        : Test(parent),
          mConn(0)
    { }

protected Q_SLOTS:
    void onAccountAdded(const Tp::AccountPtr &);
    void onAccountRemoved(const Tp::AccountPtr &);
    void onCreateAccountFinished(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();
    void testFilters();

    void cleanup();
    void cleanupTestCase();

private:
    void createAccount(const char *cmName, const char *protocolName,
            const char *displayName, const QVariantMap &parameters);
    void removeAccount(const AccountPtr &acc);
    QStringList pathsForAccounts(const QList<Tp::AccountPtr> &list);
    QStringList pathsForAccounts(const Tp::AccountSetPtr &set);

    AccountManagerPtr mAM;
    TestConnHelper *mConn;
    AccountPtr mAccountCreated;
    AccountPtr mAccountAdded;
    AccountPtr mAccountRemoved;
};

void TestAccountSet::onAccountAdded(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountAdded = acc;
    qDebug() << "ACCOUNT ADDED:" << acc->objectPath();
}

void TestAccountSet::onAccountRemoved(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountRemoved = acc;
    qDebug() << "ACCOUNT REMOVED:" << acc->objectPath();
}

void TestAccountSet::onCreateAccountFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingAccount *pa = qobject_cast<PendingAccount*>(op);
    mAccountCreated = pa->account();

    // the account should appear in the valid accounts set first, so mAccountAdded must be non-null
    QVERIFY(mAccountAdded);
    QVERIFY(mAccountCreated);

    qDebug() << "ACCOUNT CREATED:" << mAccountAdded->objectPath();
    mLoop->exit(0);
}

void TestAccountSet::createAccount(const char *cmName, const char *protocolName,
        const char *displayName, const QVariantMap &parameters)
{
    AccountSetPtr accounts = mAM->validAccounts();

    // AccountSet listen to AM::newAccount to check for accounts matching its filter.
    //
    // PendingAccount calls AM.CreateAccount and waits for the call to finish.
    // Once the call is finished, if everything is fine, it checks if the account was already added
    // to the AM or waits till it gets added by connecting to AM::newAccount.
    // Once the newly created account appears in the AM, it signals PendingAccount::finished.
    //
    // So the signal ordering depends on whether the PendingAccount was created before the
    // AccountSet or not.
    //
    // In this case where we are creating the AccountSet before calling Am::createAccount,
    // the account will first appear in the set via AccountSet::accountAdded and after that the
    // PendingAccount operation will finish.

    mAccountCreated.reset();
    mAccountAdded.reset();

    QVERIFY(connect(accounts.data(),
                SIGNAL(accountAdded(Tp::AccountPtr)),
                SLOT(onAccountAdded(Tp::AccountPtr))));

    PendingAccount *pacc = mAM->createAccount(QLatin1String(cmName),
            QLatin1String(protocolName), QLatin1String(displayName), parameters);
    QVERIFY(connect(pacc,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onCreateAccountFinished(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // check that the added account is the same one that was created
    QCOMPARE(mAccountAdded, mAccountCreated);
}

void TestAccountSet::removeAccount(const AccountPtr &acc)
{
    QCOMPARE(acc->isValid(), true);
    AccountSetPtr accounts = mAM->validAccounts();
    QVERIFY(accounts->accounts().contains(acc));

    int oldAccountsCount = accounts->accounts().size();
    QVERIFY(connect(accounts.data(),
                SIGNAL(accountRemoved(Tp::AccountPtr)),
                SLOT(onAccountRemoved(Tp::AccountPtr))));

    QVERIFY(connect(acc->remove(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // wait the account to disappear from the set
    while (accounts->accounts().size() != oldAccountsCount - 1) {
        processDBusQueue(mConn->client().data());
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(acc->isValid(), false);
    QCOMPARE(acc->invalidationReason(), TP_QT_ERROR_OBJECT_REMOVED);
}

QStringList TestAccountSet::pathsForAccounts(const QList<Tp::AccountPtr> &list)
{
    QStringList ret;
    Q_FOREACH (const AccountPtr &account, list) {
        ret << account->objectPath();
    }
    return ret;
}

QStringList TestAccountSet::pathsForAccounts(const Tp::AccountSetPtr &set)
{
    QStringList ret;
    Q_FOREACH (const AccountPtr &account, set->accounts()) {
        ret << account->objectPath();
    }
    return ret;
}

void TestAccountSet::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("account-set");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mAM = AccountManager::create(AccountFactory::create(QDBusConnection::sessionBus(),
                Account::FeatureCore | Account::FeatureCapabilities));
    QCOMPARE(mAM->isReady(), false);

    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QCOMPARE(mAM->allAccounts().isEmpty(), true);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "echo2",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestAccountSet::init()
{
    mAccountAdded.reset();
    mAccountRemoved.reset();

    initImpl();
}

void TestAccountSet::testBasics()
{
    AccountSetPtr validAccounts = mAM->validAccounts();

    // create and remove the same account twice to check whether AccountSet::accountAdded/Removed is
    // properly emitted and the account becomes invalid after being removed
    for (int i = 0; i < 2; ++i) {
        // create the account
        QVariantMap parameters;
        parameters[QLatin1String("account")] = QLatin1String("foobar");
        createAccount("foo", "bar", "foobar", parameters);

        // check that the account is properly created and added to the set of valid accounts
        QCOMPARE(validAccounts->accounts().size(), 1);
        QStringList paths = QStringList() <<
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0");
        QCOMPARE(pathsForAccounts(validAccounts), paths);
        QCOMPARE(pathsForAccounts(mAM->invalidAccounts()), QStringList());
        QCOMPARE(pathsForAccounts(mAM->allAccounts()), paths);
        QCOMPARE(mAM->allAccounts(), validAccounts->accounts());

        // remove the account
        AccountPtr acc = validAccounts->accounts().first();
        QVERIFY(acc);
        removeAccount(acc);

        // check that the account is properly invalidated and removed from the set
        QCOMPARE(validAccounts->accounts().size(), 0);
        QCOMPARE(pathsForAccounts(validAccounts), QStringList());
        QCOMPARE(pathsForAccounts(mAM->invalidAccounts()), QStringList());
        QCOMPARE(pathsForAccounts(mAM->allAccounts()), QStringList());
        QCOMPARE(mAM->allAccounts(), validAccounts->accounts());
    }
}

void TestAccountSet::testFilters()
{
    QVariantMap parameters;
    parameters[QLatin1String("account")] = QLatin1String("foobar");
    createAccount("foo", "bar", "foobar", parameters);
    QCOMPARE(mAM->allAccounts().size(), 1);
    QCOMPARE(mAM->validAccounts()->accounts().size(), 1);
    AccountPtr fooAcc = mAM->allAccounts()[0];

    parameters.clear();
    parameters[QLatin1String("account")] = QLatin1String("spuriousnormal");
    createAccount("spurious", "normal", "spuriousnormal", parameters);
    QCOMPARE(mAM->allAccounts().size(), 2);
    QCOMPARE(mAM->validAccounts()->accounts().size(), 2);
    AccountPtr spuriousAcc = *(mAM->allAccounts().toSet() -= fooAcc).begin();

    Tp::AccountSetPtr filteredAccountSet;

    {
        QVariantMap filter;
        filter.insert(QLatin1String("protocolName"), QLatin1String("bar"));
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filter));
        QCOMPARE(filteredAccountSet->accounts().size(), 1);
        QVERIFY(filteredAccountSet->accounts().contains(fooAcc));

        filter.clear();
        filter.insert(QLatin1String("protocolName"), QLatin1String("normal"));
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, filter));
        QCOMPARE(filteredAccountSet->accounts().size(), 1);
        QVERIFY(filteredAccountSet->accounts().contains(spuriousAcc));
    }

    {
        QList<AccountFilterConstPtr> filterChain;
        AccountPropertyFilterPtr cmNameFilter0 = AccountPropertyFilter::create();
        cmNameFilter0->addProperty(QLatin1String("cmName"), QLatin1String("foo"));
        AccountPropertyFilterPtr cmNameFilter1 = AccountPropertyFilter::create();
        cmNameFilter1->addProperty(QLatin1String("cmName"), QLatin1String("spurious"));
        filterChain.append(cmNameFilter0);
        filterChain.append(cmNameFilter1);
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM,
                    OrFilter<Account>::create(filterChain)));
        QCOMPARE(filteredAccountSet->accounts().size(), 2);
        QVERIFY(filteredAccountSet->accounts().contains(fooAcc));
        QVERIFY(filteredAccountSet->accounts().contains(spuriousAcc));
    }

    {
        QList<AccountFilterConstPtr> filterChain;
        AccountPropertyFilterPtr cmNameFilter = AccountPropertyFilter::create();
        cmNameFilter->addProperty(QLatin1String("cmName"), QLatin1String("foo"));
        AccountCapabilityFilterPtr capsFilter = AccountCapabilityFilter::create();
        capsFilter->addRequestableChannelClassSubset(RequestableChannelClassSpec::textChat());
        filterChain.append(cmNameFilter);
        filterChain.append(capsFilter);
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, AndFilter<Account>::create(filterChain)));
        QCOMPARE(filteredAccountSet->accounts().size(), 0);
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, OrFilter<Account>::create(filterChain)));
        QCOMPARE(filteredAccountSet->accounts().size(), 2);
        QVERIFY(filteredAccountSet->accounts().contains(fooAcc));
        QVERIFY(filteredAccountSet->accounts().contains(spuriousAcc));

        filterChain.clear();
        cmNameFilter = AccountPropertyFilter::create();
        cmNameFilter->addProperty(QLatin1String("cmName"), QLatin1String("spurious"));
        capsFilter = AccountCapabilityFilter::create();
        capsFilter->setRequestableChannelClassesSubset(
                RequestableChannelClassSpecList() << RequestableChannelClassSpec::textChat());
        filterChain.append(cmNameFilter);
        filterChain.append(capsFilter);
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, AndFilter<Account>::create(filterChain)));
        QCOMPARE(filteredAccountSet->accounts().size(), 1);
        QVERIFY(filteredAccountSet->accounts().contains(spuriousAcc));

        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, NotFilter<Account>::create(AndFilter<Account>::create(filterChain))));
        QCOMPARE(filteredAccountSet->accounts().size(), 1);
        QVERIFY(filteredAccountSet->accounts().contains(fooAcc));
    }

    {
        // should not match as allowedProperties has TargetFoo that is not allowed
        RequestableChannelClassList rccs;
        RequestableChannelClass rcc;
        rcc.fixedProperties.insert(
                TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_TEXT);
        rcc.fixedProperties.insert(
                TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) HandleTypeContact);
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
        rcc.allowedProperties.append(
                TP_QT_IFACE_CHANNEL + QLatin1String(".TargetFoo"));
        rccs.append(rcc);
        filteredAccountSet = AccountSetPtr(new AccountSet(mAM, AccountCapabilityFilter::create(rccs)));
        QCOMPARE(filteredAccountSet->accounts().isEmpty(), true);
    }

    {
        // let's change a property and see
        AccountSetPtr enabledAccounts = mAM->enabledAccounts();
        QVERIFY(connect(enabledAccounts.data(),
                    SIGNAL(accountRemoved(Tp::AccountPtr)),
                    SLOT(onAccountRemoved(Tp::AccountPtr))));
        AccountSetPtr disabledAccounts = mAM->disabledAccounts();
        QVERIFY(connect(disabledAccounts.data(),
                    SIGNAL(accountAdded(Tp::AccountPtr)),
                    SLOT(onAccountAdded(Tp::AccountPtr))));

        QCOMPARE(enabledAccounts->accounts().size(), 2);
        QCOMPARE(disabledAccounts->accounts().size(), 0);

        QVERIFY(connect(fooAcc->setEnabled(false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        while (fooAcc->isEnabled() != false) {
            mLoop->processEvents();
        }

        processDBusQueue(mConn->client().data());

        QVERIFY(mAccountAdded);
        QVERIFY(mAccountRemoved);
        QCOMPARE(mAccountAdded, mAccountRemoved);

        QCOMPARE(enabledAccounts->accounts().size(), 1);
        QVERIFY(enabledAccounts->accounts().contains(spuriousAcc));
        QCOMPARE(disabledAccounts->accounts().size(), 1);
        QVERIFY(disabledAccounts->accounts().contains(fooAcc));
    }

    {
        QCOMPARE(mAM->invalidAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->onlineAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->offlineAccounts()->accounts().size(), 2);
        QCOMPARE(mAM->textChatAccounts()->accounts().size(), 1);
        QVERIFY(mAM->textChatAccounts()->accounts().contains(spuriousAcc));
        QCOMPARE(mAM->textChatroomAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->streamedMediaCallAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->streamedMediaAudioCallAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->streamedMediaVideoCallAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->streamedMediaVideoCallWithAudioAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->fileTransferAccounts()->accounts().size(), 0);
        QCOMPARE(mAM->accountsByProtocol(QLatin1String("bar"))->accounts().size(), 1);
        QVERIFY(mAM->accountsByProtocol(QLatin1String("bar"))->accounts().contains(fooAcc));
        QCOMPARE(mAM->accountsByProtocol(QLatin1String("normal"))->accounts().size(), 1);
        QVERIFY(mAM->accountsByProtocol(QLatin1String("normal"))->accounts().contains(spuriousAcc));
        QCOMPARE(mAM->accountsByProtocol(QLatin1String("noname"))->accounts().size(), 0);
    }
}

void TestAccountSet::cleanup()
{
    cleanupImpl();
}

void TestAccountSet::cleanupTestCase()
{
    if (mConn) {
        QCOMPARE(mConn->disconnect(), true);
        delete mConn;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountSet)
#include "_gen/account-set.cpp.moc.hpp"
