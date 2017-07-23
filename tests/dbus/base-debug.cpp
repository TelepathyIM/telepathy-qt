#include <TelepathyQt/BaseDebug>
#include <TelepathyQt/DebugReceiver>
#include <TelepathyQt/PendingReady>

#include <tests/lib/test.h>

static const int s_dbusOperationTimeout = 100;

class TestBaseDebug : public Test
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase();
    void init();

    void testObjectRegistration();
    void testMessages();
    void testDisabledMessages();

    void cleanup();
    void cleanupTestCase();
};

void TestBaseDebug::initTestCase()
{
    initTestCaseImpl();
}

void TestBaseDebug::init()
{
    initImpl();
}

void TestBaseDebug::testObjectRegistration()
{
    Tp::BaseDebug debugService;
    QVERIFY2(debugService.registerObject(), "Unable to register debug service object");
}

void TestBaseDebug::testMessages()
{
    Tp::BaseDebug debugService;
    QVERIFY2(debugService.registerObject(), "Unable to register debug service object");

    Tp::DebugReceiverPtr receiver = Tp::DebugReceiver::create(debugService.dbusConnection().baseService(), debugService.dbusConnection());

    Tp::PendingReady *getReadyOp = receiver->becomeReady();
    QTRY_VERIFY_WITH_TIMEOUT(getReadyOp->isFinished(), s_dbusOperationTimeout);
    QVERIFY(getReadyOp->isValid());

    QTRY_VERIFY2_WITH_TIMEOUT(receiver->isReady(), "Unable to get ready", s_dbusOperationTimeout);

    Tp::DebugMessageList messages;
    connect(receiver.data(), &Tp::DebugReceiver::newDebugMessage, [&messages](Tp::DebugMessage newMessage) {
        messages.append(newMessage);
    });

    debugService.newDebugMessage(QStringLiteral("dom"), Tp::DebugLevelMessage, QStringLiteral("message1"));
    QTest::qWait(s_dbusOperationTimeout);

    QVERIFY2(messages.isEmpty(), "There is a message received, but the interface should be disabled by default.");

    Tp::PendingOperation *enablingOp = receiver->setMonitoringEnabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(enablingOp->isFinished(), s_dbusOperationTimeout);
    QTRY_VERIFY(enablingOp->isValid());

    debugService.newDebugMessage(QStringLiteral("dom"), Tp::DebugLevelMessage, QStringLiteral("message2"));
    QTRY_VERIFY_WITH_TIMEOUT(messages.count() == 1, s_dbusOperationTimeout);

    Tp::PendingOperation *disablingOp = receiver->setMonitoringEnabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(disablingOp->isFinished(), s_dbusOperationTimeout);
    QTRY_VERIFY(disablingOp->isValid());

    debugService.newDebugMessage(QStringLiteral("dom"), Tp::DebugLevelMessage, QStringLiteral("message3"));

    Tp::PendingOperation *enablingOp2 = receiver->setMonitoringEnabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(enablingOp2->isFinished(), s_dbusOperationTimeout);
    QTRY_VERIFY(enablingOp2->isValid());

    QTest::qWait(s_dbusOperationTimeout);
    QVERIFY(messages.count() == 1);
    qDebug() << messages.last().message;
}

void TestBaseDebug::testDisabledMessages()
{
}

void TestBaseDebug::cleanup()
{
    cleanupImpl();
}

void TestBaseDebug::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestBaseDebug)
#include "_gen/base-debug.cpp.moc.hpp"
