#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingStringList>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/simple-manager.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestCmBasics : public Test
{
    Q_OBJECT

public:
    TestCmBasics(QObject *parent = 0)
        : Test(parent), mCMService(0), mCM(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();

private:
    TpBaseConnectionManager *mCMService;
    Telepathy::Client::ConnectionManager *mCM;
};

void TestCmBasics::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("cm-basics");
    tp_debug_set_flags("all");

    mCMService = TP_BASE_CONNECTION_MANAGER(g_object_new(
        SIMPLE_TYPE_CONNECTION_MANAGER,
        NULL));
    QVERIFY(mCMService != 0);
    QVERIFY(tp_base_connection_manager_register(mCMService));
}

void TestCmBasics::init()
{
    initImpl();

    mCM = new ConnectionManager("simple");
    QCOMPARE(mCM->isReady(), false);
}

void TestCmBasics::testBasics()
{
    QVERIFY(connect(mCM->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);

    // calling becomeReady() twice is a no-op
    QVERIFY(connect(mCM->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);

    QCOMPARE(mCM->interfaces(), QStringList());
    QCOMPARE(mCM->supportedProtocols(), QStringList() << "simple");

    Q_FOREACH (ProtocolInfo *info, mCM->protocols()) {
        QVERIFY(info != 0);
        QVERIFY(info->cmName() == "simple");
        QVERIFY(info->name() == "simple");

        QCOMPARE(info->hasParameter("account"), true);
        QCOMPARE(info->hasParameter("not-there"), false);

        Q_FOREACH (ProtocolParameter *param, info->parameters()) {
            QCOMPARE(param->name(), QString::fromAscii("account"));
            QCOMPARE(param->dbusSignature().signature(),
                         QLatin1String("s"));
            QCOMPARE(param->isRequired(), true);
            QCOMPARE(param->isSecret(), false);
        }
        QCOMPARE(info->canRegister(), false);
    }

    QCOMPARE(mCM->supportedProtocols(), QStringList() << "simple");
}

void TestCmBasics::cleanup()
{
    if (mCM) {
        delete mCM;
        mCM = 0;
    }

    cleanupImpl();
}

void TestCmBasics::cleanupTestCase()
{
    if (mCMService) {
        g_object_unref(mCMService);
        mCMService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestCmBasics)
#include "_gen/cm-basics.cpp.moc.hpp"
