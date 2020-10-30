#include <tests/lib/test.h>

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
    TestBaseCM(QObject *parent = nullptr)
        : Test(parent)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testNoProtocols();
    void testProtocols();

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
    QTRY_VERIFY(pr->isFinished());
    QVERIFY(pr->isError());

    qDebug() << "Creating CM";

    BaseConnectionManagerPtr svcCM = BaseConnectionManager::create(
                QLatin1String("testcm"));
    Tp::DBusError err;
    QVERIFY(svcCM->registerObject(&err));
    QVERIFY(!err.isValid());

    QCOMPARE(svcCM->protocols().size(), 0);

    qDebug() << "Introspecting new CM";

    cliCM = ConnectionManager::create(QLatin1String("testcm"));
    pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    QTRY_VERIFY(pr->isFinished());
    QVERIFY(!pr->isError());

    QCOMPARE(cliCM->supportedProtocols().size(), 0);

    qDebug() << "Requesting connection";

    PendingConnection *pc = cliCM->lowlevel()->requestConnection(
            QLatin1String("jabber"), QVariantMap());
    QTRY_VERIFY(pc->isFinished());
    QVERIFY(pc->isError());
    QCOMPARE(pc->errorName(), QString(TP_QT_ERROR_NOT_IMPLEMENTED));
}

void TestBaseCM::testProtocols()
{
    qDebug() << "Creating CM";

    BaseConnectionManagerPtr svcCM = BaseConnectionManager::create(
                QLatin1String("testcm"));

    BaseProtocolPtr protocol = BaseProtocol::create(QLatin1String("myprotocol"));
    QVERIFY(!protocol.isNull());
    QVERIFY(svcCM->addProtocol(protocol));

    QVERIFY(svcCM->hasProtocol(QLatin1String("myprotocol")));
    QCOMPARE(svcCM->protocol(QLatin1String("myprotocol")), protocol);
    QCOMPARE(svcCM->protocols().size(), 1);

    QVERIFY(!svcCM->hasProtocol(QLatin1String("otherprotocol")));
    QVERIFY(svcCM->protocol(QLatin1String("otherprotocol")).isNull());

    //can't add the same protocol twice
    QVERIFY(!svcCM->addProtocol(protocol));

    Tp::DBusError err;
    QVERIFY(svcCM->registerObject(&err));
    QVERIFY(!err.isValid());

    //can't add another protocol after registerObject()
    protocol = BaseProtocol::create(QLatin1String("otherprotocol"));
    QVERIFY(!protocol.isNull());
    QVERIFY(!svcCM->addProtocol(protocol));
    QCOMPARE(svcCM->protocols().size(), 1);
    protocol.reset();

    QVariantMap props = svcCM->immutableProperties();
    QVERIFY(props.contains(TP_QT_IFACE_CONNECTION_MANAGER + QLatin1String(".Protocols")));

    ProtocolPropertiesMap protocols = qvariant_cast<Tp::ProtocolPropertiesMap>(
            props[TP_QT_IFACE_CONNECTION_MANAGER + QLatin1String(".Protocols")]);
    QVERIFY(protocols.contains(QLatin1String("myprotocol")));
    QVERIFY(!protocols.contains(QLatin1String("otherprotocol")));

    qDebug() << "Introspecting CM";

    ConnectionManagerPtr cliCM = ConnectionManager::create(
            QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    QTRY_VERIFY(pr->isFinished());
    QVERIFY(!pr->isError());

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QLatin1String("myprotocol")));

    PendingConnection *pc = cliCM->lowlevel()->requestConnection(
            QLatin1String("myprotocol"), QVariantMap());
    QTRY_VERIFY(pc->isFinished());
    QVERIFY(pc->isError());
    QCOMPARE(pc->errorName(), QString(TP_QT_ERROR_NOT_IMPLEMENTED));
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
