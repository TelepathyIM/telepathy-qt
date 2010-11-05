#include <QtCore/QEventLoop>
#include <QtTest/QtTest>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingVariantMap>
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

    void expectSuccessfulAllProperties(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testDBusProperties();

    void cleanup();
    void cleanupTestCase();

private:
    AccountManagerPtr mAM;
    int mAccountsCount;
    QVariantMap mAllProperties;
};

void TestDBusProperties::onNewAccount(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountsCount++;
    mLoop->exit(0);
}

void TestDBusProperties::expectSuccessfulAllProperties(PendingOperation *op)
{
    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mAllProperties = QVariantMap();
        mLoop->exit(1);
    } else {
        Tp::PendingVariantMap *pvm = qobject_cast<Tp::PendingVariantMap*>(op);
        mAllProperties = pvm->result();
        mLoop->exit(0);
    }
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

    AccountPtr acc = Account::create(mAM->busName(),
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

    QString currDisplayName;
    QVERIFY(waitForProperty(cliAccount->requestPropertyDisplayName(), &currDisplayName));
    QCOMPARE(currDisplayName, oldDisplayName);

    QVERIFY(connect(cliAccount->requestAllProperties(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulAllProperties(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAllProperties[QLatin1String("DisplayName")].value<QString>(), oldDisplayName);
    QVERIFY(mAllProperties[QLatin1String("Interfaces")].value<QStringList>().contains(
                QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_INTERFACE_AVATAR)));

    const QString newDisplayName = QLatin1String("Foo bar account");
    QVERIFY(connect(cliAccount->setPropertyDisplayName(newDisplayName),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(waitForProperty(cliAccount->requestPropertyDisplayName(), &currDisplayName));
    QCOMPARE(currDisplayName, newDisplayName);

    QVERIFY(connect(cliAccount->requestAllProperties(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulAllProperties(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAllProperties[QLatin1String("DisplayName")].value<QString>(), newDisplayName);
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
