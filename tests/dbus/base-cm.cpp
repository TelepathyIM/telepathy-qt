#include <tests/lib/test.h>
#include <tests/lib/test-thread-helper.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/BaseProtocol>
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
    void testProtocols();

    void cleanup();
    void cleanupTestCase();

private:
    static void testNoProtocolsCreateCM(BaseConnectionManagerPtr &cm);
    static void testProtocolsCreateCM(BaseConnectionManagerPtr &cm);
};

void TestBaseCM::initTestCase()
{
    initTestCaseImpl();
}

void TestBaseCM::init()
{
    initImpl();
}

void TestBaseCM::testNoProtocolsCreateCM(BaseConnectionManagerPtr &cm)
{
    cm = BaseConnectionManager::create(QLatin1String("testcm"));
    Tp::DBusError err;
    QVERIFY(cm->registerObject(&err));
    QVERIFY(!err.isValid());

    QCOMPARE(cm->protocols().size(), 0);
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

    TestThreadHelper<BaseConnectionManagerPtr> helper;
    TEST_THREAD_HELPER_EXECUTE(&helper, &testNoProtocolsCreateCM);

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
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
}

void TestBaseCM::testProtocolsCreateCM(BaseConnectionManagerPtr &cm)
{
    cm = BaseConnectionManager::create(QLatin1String("testcm"));

    BaseProtocolPtr protocol = BaseProtocol::create(QLatin1String("myprotocol"));
    QVERIFY(!protocol.isNull());
    QVERIFY(cm->addProtocol(protocol));

    QVERIFY(cm->hasProtocol(QLatin1String("myprotocol")));
    QCOMPARE(cm->protocol(QLatin1String("myprotocol")), protocol);
    QCOMPARE(cm->protocols().size(), 1);

    QVERIFY(!cm->hasProtocol(QLatin1String("otherprotocol")));
    QVERIFY(cm->protocol(QLatin1String("otherprotocol")).isNull());

    //can't add the same protocol twice
    QVERIFY(!cm->addProtocol(protocol));

    Tp::DBusError err;
    QVERIFY(cm->registerObject(&err));
    QVERIFY(!err.isValid());

    //can't add another protocol after registerObject()
    protocol = BaseProtocol::create(QLatin1String("otherprotocol"));
    QVERIFY(!protocol.isNull());
    QVERIFY(!cm->addProtocol(protocol));
    QCOMPARE(cm->protocols().size(), 1);
    protocol.reset();

    QVariantMap props = cm->immutableProperties();
    QVERIFY(props.contains(TP_QT_IFACE_CONNECTION_MANAGER + QLatin1String(".Protocols")));

    ProtocolPropertiesMap protocols = qvariant_cast<Tp::ProtocolPropertiesMap>(
            props[TP_QT_IFACE_CONNECTION_MANAGER + QLatin1String(".Protocols")]);
    QVERIFY(protocols.contains(QLatin1String("myprotocol")));
    QVERIFY(!protocols.contains(QLatin1String("otherprotocol")));
}

void TestBaseCM::testProtocols()
{
    qDebug() << "Creating CM";

    TestThreadHelper<BaseConnectionManagerPtr> helper;
    TEST_THREAD_HELPER_EXECUTE(&helper, &testProtocolsCreateCM);

    qDebug() << "Introspecting CM";

    ConnectionManagerPtr cliCM = ConnectionManager::create(
            QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QLatin1String("myprotocol")));

    PendingConnection *pc = cliCM->lowlevel()->requestConnection(
            QLatin1String("myprotocol"), QVariantMap());
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
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
