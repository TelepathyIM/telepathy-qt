#include <tests/lib/test.h>
#include <tests/lib/test-thread-helper.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/DBusService>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/PendingString>

#include <TelepathyQt/_gen/svc-connection-manager.h>

#include <QVariantMap>

using namespace Tp;

class TestSvcLowlevel : public Test
{
    Q_OBJECT
public:
    TestSvcLowlevel(QObject *parent = 0)
        : Test(parent)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testSvcLowlevel();

    void cleanup();
    void cleanupTestCase();

private:
    struct TestCMData {
        DBusServicePtr cm;
        DBusObjectPtr proto;
    };
    static void setupTestCM(TestCMData &data);
};

void TestSvcLowlevel::initTestCase()
{
    initTestCaseImpl();
}

void TestSvcLowlevel::init()
{
    initImpl();
}

void TestSvcLowlevel::setupTestCM(TestCMData &data)
{
    Tp::DBusError err;

    /* setup the protocol object */
    DBusObjectPtr protocol = DBusService::create();
    Service::ProtocolAdapteePtr protoAdaptee = Service::ProtocolAdaptee::create();
    Service::ProtocolInterfaceAddressingAdapteePtr protoAddrAdaptee =
            Service::ProtocolInterfaceAddressingAdaptee::create();

    ParamSpec param;
    param.name = QStringLiteral("account");
    param.flags = ConnMgrParamFlagRequired;
    param.signature = QStringLiteral("s");
    param.defaultValue = QDBusVariant(QVariant::fromValue(QStringLiteral("foo")));
    protoAdaptee->setParameters(ParamSpecList() << param);
    protoAdaptee->setVCardField(QStringLiteral("x-test"));
    protoAdaptee->setEnglishName(QStringLiteral("TestProto"));
    protoAdaptee->setInterfaces(QStringList() << TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING);

    QCOMPARE(protoAdaptee->parameters(), ParamSpecList() << param);
    QCOMPARE(protoAdaptee->vcardField(), QStringLiteral("x-test"));
    QCOMPARE(protoAdaptee->englishName(), QStringLiteral("TestProto"));

    protoAddrAdaptee->setAddressableURISchemes(QStringList() << QStringLiteral("test"));
    protoAddrAdaptee->setAddressableVCardFields(QStringList() << QStringLiteral("x-test"));

    protoAddrAdaptee->implementNormalizeContactURI(
        [](const QString &contact,
           const Service::ProtocolInterfaceAddressingAdaptee::NormalizeContactURIContextPtr &ctx) {
            ctx->setFinished(contact + QStringLiteral("_normalized"));
        });

    protoAddrAdaptee->implementNormalizeVCardAddress(
        [](const QString &field, const QString &address,
           const Service::ProtocolInterfaceAddressingAdaptee::NormalizeVCardAddressContextPtr &ctx) {
            ctx->setFinishedWithError(TP_QT_ERROR_INVALID_ARGUMENT, QStringLiteral("Invalid argument"));
        });

    QVERIFY(!protocol->isRegistered());
    QVERIFY(!protoAdaptee->isRegistered());
    QVERIFY(!protoAddrAdaptee->isRegistered());
    QVERIFY(protoAdaptee->dbusObject().isNull());
    QVERIFY(protoAddrAdaptee->dbusObject().isNull());

    protocol->plugInterfaceAdaptee(protoAdaptee);

    QVERIFY(!protocol->isRegistered());
    QVERIFY(!protoAdaptee->isRegistered());
    QVERIFY(!protoAddrAdaptee->isRegistered());
    QCOMPARE(protoAdaptee->dbusObject().data(), protocol.data());
    QVERIFY(protoAddrAdaptee->dbusObject().isNull());

    QCOMPARE(protocol->interfaces().size(), 1);
    QVERIFY(protocol->interfaces().contains(TP_QT_IFACE_PROTOCOL));

    protocol->plugInterfaceAdaptee(protoAddrAdaptee);

    QVERIFY(!protocol->isRegistered());
    QVERIFY(!protoAdaptee->isRegistered());
    QVERIFY(!protoAddrAdaptee->isRegistered());
    QCOMPARE(protoAdaptee->dbusObject().data(), protocol.data());
    QCOMPARE(protoAddrAdaptee->dbusObject().data(), protocol.data());

    QCOMPARE(protocol->interfaces().size(), 2);
    QVERIFY(protocol->interfaces().contains(TP_QT_IFACE_PROTOCOL));
    QVERIFY(protocol->interfaces().contains(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING));


    /* setup the cm object */
    DBusServicePtr cm = DBusService::create();
    Service::ConnectionManagerAdapteePtr cmAdaptee = Service::ConnectionManagerAdaptee::create();

    QVariantMap props;
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".Interfaces"),
            QVariant::fromValue(protoAdaptee->interfaces()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".Parameters"),
            QVariant::fromValue(protoAdaptee->parameters()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".ConnectionInterfaces"),
            QVariant::fromValue(protoAdaptee->connectionInterfaces()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".RequestableChannelClasses"),
            QVariant::fromValue(protoAdaptee->requestableChannelClasses()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".VCardField"),
            QVariant::fromValue(protoAdaptee->vcardField()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".EnglishName"),
            QVariant::fromValue(protoAdaptee->englishName()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".Icon"),
            QVariant::fromValue(protoAdaptee->icon()));
    props.insert(TP_QT_IFACE_PROTOCOL + QStringLiteral(".AuthenticationTypes"),
            QVariant::fromValue(protoAdaptee->authenticationTypes()));
    ProtocolPropertiesMap protocols;
    protocols.insert(QStringLiteral("testproto"), props);
    cmAdaptee->setProtocols(protocols);

    QVERIFY(!cm->isRegistered());
    QVERIFY(!cmAdaptee->isRegistered());
    QVERIFY(cmAdaptee->dbusObject().isNull());

    cm->plugInterfaceAdaptee(cmAdaptee);

    QVERIFY(!cm->isRegistered());
    QVERIFY(!cmAdaptee->isRegistered());
    QCOMPARE(cmAdaptee->dbusObject().data(), cm.data());

    QCOMPARE(cm->interfaces().size(), 1);
    QVERIFY(cm->interfaces().contains(TP_QT_IFACE_CONNECTION_MANAGER));


    /* register the objects */
    QVERIFY(cm->registerService(TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE + QStringLiteral("testcm"),
                TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE + QStringLiteral("testcm"), &err));
    QVERIFY(!err.isValid());
    QVERIFY(cm->isRegistered());
    QVERIFY(cmAdaptee->isRegistered());
    QCOMPARE(cm->objectPath(), TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE + QStringLiteral("testcm"));
    QCOMPARE(cm->busName(), TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE + QStringLiteral("testcm"));

    QVERIFY(protocol->registerObject(
                TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE + QStringLiteral("testcm/testproto"), &err));
    QVERIFY(!err.isValid());
    QVERIFY(protocol->isRegistered());
    QVERIFY(protoAdaptee->isRegistered());
    QVERIFY(protoAddrAdaptee->isRegistered());
    QCOMPARE(protocol->objectPath(),
        TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE + QStringLiteral("testcm/testproto"));

    /* save objects */
    data.cm = cm;
    data.proto = protocol;
}

void TestSvcLowlevel::testSvcLowlevel()
{
    qDebug() << "Introspecting non-existing CM";

    ConnectionManagerPtr cliCM = ConnectionManager::create(
            QStringLiteral("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "Creating CM";

    TestThreadHelper<TestCMData> helper;
    TEST_THREAD_HELPER_EXECUTE(&helper, &setupTestCM);

    qDebug() << "Introspecting new CM";

    cliCM = ConnectionManager::create(QStringLiteral("testcm"));
    pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QStringLiteral("testproto")));

    QCOMPARE(cliCM->protocol(QStringLiteral("testproto")).vcardField(), QStringLiteral("x-test"));
    QCOMPARE(cliCM->protocol(QStringLiteral("testproto")).englishName(), QStringLiteral("TestProto"));
    QCOMPARE(cliCM->protocol(QStringLiteral("testproto")).addressableUriSchemes().size(), 1);
    QCOMPARE(cliCM->protocol(QStringLiteral("testproto")).addressableUriSchemes().first(),
            QStringLiteral("test"));
    QCOMPARE(cliCM->protocol(QStringLiteral("testproto")).addressableVCardFields().size(), 1);
    QCOMPARE(cliCM->protocol(QStringLiteral("testproto")).addressableVCardFields().first(),
            QStringLiteral("x-test"));

    ProtocolParameterList params = cliCM->protocol(QStringLiteral("testproto")).parameters();
    QCOMPARE(params.size(), 1);
    QCOMPARE(params.first().name(), QStringLiteral("account"));
    QVERIFY(params.first().isRequired());
    QVERIFY(!params.first().isSecret());
    QVERIFY(!params.first().isRequiredForRegistration());
    QCOMPARE(params.first().dbusSignature().signature(), QStringLiteral("s"));
    QCOMPARE(params.first().type(), QVariant::String);

    //FIXME this fails, not sure why. The returned defaultValue() is an empty QVariant()
    //QCOMPARE(params.first().defaultValue(), QVariant::fromValue(QStringLiteral("foo")));

    qDebug() << "Calling NormalizeContactUri";

    PendingString *ps = cliCM->protocol(QStringLiteral("testproto")).normalizeContactUri(
            QStringLiteral("foo"));
    QString psResult;
    connect(ps, &PendingOperation::finished, [&](PendingOperation *op) {
        TEST_VERIFY_OP (op);
        psResult = static_cast<PendingString*>(op)->result();
        mLoop->exit(0);
    });
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(psResult, QStringLiteral("foo_normalized"));

    qDebug() << "Calling NormalizeVCardAddress";

    ps = cliCM->protocol(QStringLiteral("testproto")).normalizeVCardAddress(
            QStringLiteral("testField"), QStringLiteral("testAddress"));
    connect(ps, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_INVALID_ARGUMENT);

    qDebug() << "Requesting connection";

    PendingConnection *pc = cliCM->lowlevel()->requestConnection(
            QLatin1String("testproto"), QVariantMap());
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
}

void TestSvcLowlevel::cleanup()
{
    cleanupImpl();
}

void TestSvcLowlevel::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestSvcLowlevel)
#include "_gen/svc-lowlevel.cpp.moc.hpp"

