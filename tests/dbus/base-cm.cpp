#include <tests/lib/test.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingConnection>

using namespace Tp;

class TestBaseCM : public Test
{
    Q_OBJECT
public:
    TestBaseCM(QObject *parent = 0)
        : Test(parent)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testNoProtocols();

    void cleanup();
    void cleanupTestCase();

};

void TestBaseCM::initTestCase()
{
    initTestCaseImpl();
}

void TestBaseCM::init()
{
    initImpl();
}

void TestBaseCM::testNoProtocols()
{
    qDebug() << "Introspecting non-existing CM";

    ConnectionManagerPtr cliCM = ConnectionManager::create(
            QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "Creating CM";

    BaseConnectionManagerPtr cm = BaseConnectionManager::create(
            QLatin1String("testcm"));
    Tp::DBusError err;
    QVERIFY(cm->registerObject(&err));
    QVERIFY(!err.isValid());

    qDebug() << "Introspecting new CM";

    cliCM = ConnectionManager::create(QLatin1String("testcm"));
    pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 0);

    qDebug() << "Requesting connection";

    PendingConnection *pc = cliCM->lowlevel()->requestConnection(
            QLatin1String("jabber"), QVariantMap());
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
}

void TestBaseCM::cleanup()
{
    cleanupImpl();
}

void TestBaseCM::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestBaseCM)
#include "_gen/base-cm.cpp.moc.hpp"
