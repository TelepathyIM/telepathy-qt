#include <QtCore/QEventLoop>
#include <QtTest/QtTest>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
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
        : Test(parent), mAccountsCount(0)
    { }

protected Q_SLOTS:
    void onAccountCreated(const QString &);
    void onAvatarChanged(const Tp::Avatar &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();

private:
    AccountManagerPtr mAM;
    int mAccountsCount;
};

void TestAccountBasics::onAccountCreated(const QString &acc)
{
    Q_UNUSED(acc);

    mAccountsCount++;
    mLoop->exit(0);
}

void TestAccountBasics::onAvatarChanged(const Tp::Avatar &avatar)
{
    qDebug() << "on avatar changed";
    QCOMPARE(avatar.avatarData, QByteArray("asdfg"));
    QCOMPARE(avatar.MIMEType, QString(QLatin1String("image/jpeg")));
    mLoop->exit(0);
}

void TestAccountBasics::initTestCase()
{
    initTestCaseImpl();

    mAM = AccountManager::create();
    QCOMPARE(mAM->isReady(), false);
}

void TestAccountBasics::init()
{
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
                    SIGNAL(accountCreated(const QString &)),
                    SLOT(onAccountCreated(const QString &))));

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

    QCOMPARE(mAM->validAccountPaths(),
            QStringList() <<
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QCOMPARE(mAM->invalidAccountPaths(),
            QStringList());
    QCOMPARE(mAM->allAccountPaths(),
            QStringList() <<
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));

    AccountPtr acc = mAM->accountForPath(
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->displayName(), QString(QLatin1String("foobar (account 0)")));

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    QVERIFY(connect(acc.data(),
                    SIGNAL(avatarChanged(const Tp::Avatar &)),
                    SLOT(onAvatarChanged(const Tp::Avatar &))));

    Tp::Avatar avatar = { QByteArray("asdfg"), QLatin1String("image/jpeg") };
    QVERIFY(connect(acc->setAvatar(avatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);

    // wait for avatarChanged signal
    QCOMPARE(mLoop->exec(), 0);

    pacc = mAM->createAccount(QLatin1String("spurious"),
            QLatin1String("normal"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    while (mAccountsCount != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }

    acc = mAM->accountForPath(
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"));
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    acc = mAM->accountForPath(
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"));
    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureProtocolInfo), true);

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
