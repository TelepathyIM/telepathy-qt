#include <QtCore/QEventLoop>
#include <QtTest/QtTest>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <tests/lib/test.h>

using namespace Tp;

class TestDBusProperties : public Test
{
    Q_OBJECT

public:
    TestDBusProperties(QObject *parent = 0)
        : Test(parent), mAccountsCount(0)
    { }

protected Q_SLOTS:
    void onNewAccount(const Tp::AccountPtr &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testDBusProperties();

    void cleanup();
    void cleanupTestCase();

private:
    AccountManagerPtr mAM;
    int mAccountsCount;
};

void TestDBusProperties::onNewAccount(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountsCount++;
    mLoop->exit(0);
}

void TestDBusProperties::initTestCase()
{
    initTestCaseImpl();

    mAM = AccountManager::create(AccountFactory::create(QDBusConnection::sessionBus(),
                Account::FeatureCore | Account::FeatureCapabilities));
    QCOMPARE(mAM->isReady(), false);
}

void TestDBusProperties::init()
{
    initImpl();
}

void TestDBusProperties::testDBusProperties()
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

    AccountPtr acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));

    while (!acc->isReady()) {
        mLoop->processEvents();
    }

    QCOMPARE(mLoop->exec(), 0);

    const QString oldDisplayName = QLatin1String("foobar (account 0)");
    QCOMPARE(acc->displayName(), oldDisplayName);

    Client::AccountInterface *cliAccount = acc->interface<Client::AccountInterface>();

    QCOMPARE(cliAccount->DisplayName(), oldDisplayName);
    QString currDisplayName;
    QVERIFY(waitForProperty(cliAccount->requestPropertyDisplayName(), &currDisplayName));
    QCOMPARE(currDisplayName, oldDisplayName);

    const QString newDisplayName = QLatin1String("Foo bar account");
    QVERIFY(connect(cliAccount->setPropertyDisplayName(newDisplayName),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliAccount->DisplayName(), newDisplayName);
    QVERIFY(waitForProperty(cliAccount->requestPropertyDisplayName(), &currDisplayName));
    QCOMPARE(currDisplayName, newDisplayName);
}

void TestDBusProperties::cleanup()
{
    cleanupImpl();
}

void TestDBusProperties::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestDBusProperties)
#include "_gen/dbus-properties.cpp.moc.hpp"
