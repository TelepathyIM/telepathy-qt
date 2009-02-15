#include <QtCore/QEventLoop>
#include <QtTest/QtTest>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingAccount>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/PendingReadyAccount>
#include <TelepathyQt4/Client/PendingReadyAccountManager>

#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestAccountBasics : public Test
{
    Q_OBJECT

public:
    TestAccountBasics(QObject *parent = 0)
        : Test(parent), mAM(0)
    { }

protected Q_SLOTS:
    void onAvatarChanged(const Telepathy::Avatar &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();

private:
    AccountManager *mAM;
};

void TestAccountBasics::onAvatarChanged(const Telepathy::Avatar &avatar)
{
    qDebug() << "on avatar changed";
    QCOMPARE(avatar.avatarData, QByteArray("asdfg"));
    QCOMPARE(avatar.MIMEType, QString("image/jpeg"));
    mLoop->exit(0);
}

void TestAccountBasics::initTestCase()
{
    initTestCaseImpl();

    mAM = new AccountManager();
    QCOMPARE(mAM->isReady(), false);
}

void TestAccountBasics::init()
{
    initImpl();
}

void TestAccountBasics::testBasics()
{
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVariantMap parameters;
    parameters["account"] = "foobar";
    PendingAccount *pacc = mAM->createAccount("foo",
            "bar", "foobar", parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(!pacc->account().isNull());

    QCOMPARE(mAM->interfaces(), QStringList());

    QCOMPARE(mAM->validAccountPaths(),
             QStringList() <<
               "/org/freedesktop/Telepathy/Account/foo/bar/Account0");
    QCOMPARE(mAM->invalidAccountPaths(),
             QStringList());
    QCOMPARE(mAM->allAccountPaths(),
             QStringList() <<
               "/org/freedesktop/Telepathy/Account/foo/bar/Account0");

    QSharedPointer<Account> acc = mAM->accountForPath(
            "/org/freedesktop/Telepathy/Account/foo/bar/Account0");
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->displayName(), QString("foobar (account 0)"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->avatar().MIMEType, QString("image/png"));

    QVERIFY(connect(acc.data(),
                    SIGNAL(avatarChanged(const Telepathy::Avatar &)),
                    SLOT(onAvatarChanged(const Telepathy::Avatar &))));

    Telepathy::Avatar avatar = { QByteArray("asdfg"), "image/jpeg" };
    QVERIFY(connect(acc->setAvatar(avatar),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // wait for avatarChanged signal
    QCOMPARE(mLoop->exec(), 0);

    pacc = mAM->createAccount("spurious",
            "normal", "foobar", parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    acc = mAM->accountForPath(
            "/org/freedesktop/Telepathy/Account/spurious/normal/Account0");
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    acc = mAM->accountForPath(
            "/org/freedesktop/Telepathy/Account/spurious/normal/Account0");
    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    ProtocolInfo *protocolInfo = acc->protocolInfo();
    QCOMPARE((bool) protocolInfo, !((ProtocolInfo *) 0));
    QCOMPARE(protocolInfo->hasParameter("account"), true);
    QCOMPARE(protocolInfo->hasParameter("password"), true);
    QCOMPARE(protocolInfo->hasParameter("register"), true);

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->avatar().MIMEType, QString("image/png"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo | Account::FeatureAvatar),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->avatar().MIMEType, QString("image/png"));
    protocolInfo = acc->protocolInfo();
    QCOMPARE((bool) protocolInfo, !((ProtocolInfo *) 0));
}

void TestAccountBasics::cleanup()
{
    cleanupImpl();
}

void TestAccountBasics::cleanupTestCase()
{
    if (mAM) {
        delete mAM;
        mAM = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountBasics)
#include "_gen/account-basics.cpp.moc.hpp"
