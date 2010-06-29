#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingStringList>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/debug.h>

#include <tests/lib/glib/simple-manager.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestCmBasics : public Test
{
    Q_OBJECT

public:
    TestCmBasics(QObject *parent = 0)
        : Test(parent), mCMService(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();

private:
    TpBaseConnectionManager *mCMService;
    Tp::ConnectionManagerPtr mCM;
};

void TestCmBasics::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("cm-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mCMService = TP_BASE_CONNECTION_MANAGER(g_object_new(
        TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER,
        NULL));
    QVERIFY(mCMService != 0);

    // This TpDBusDaemon is a workaround for fd.o#20165 (revert when we start
    // to depend on a telepathy-glib without this bug)
    TpDBusDaemon *dbus_daemon = tp_dbus_daemon_dup(0);
    QVERIFY(tp_base_connection_manager_register(mCMService));
    g_object_unref(dbus_daemon);
}

void TestCmBasics::init()
{
    initImpl();

    mCM = ConnectionManager::create(QLatin1String("simple"));
    QCOMPARE(mCM->isReady(), false);
}

void TestCmBasics::testBasics()
{
    QVERIFY(connect(mCM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);

    // calling becomeReady() twice is a no-op
    QVERIFY(connect(mCM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);

    QCOMPARE(mCM->interfaces(), QStringList());
    QCOMPARE(mCM->supportedProtocols(), QStringList() << QLatin1String("simple"));

    Q_FOREACH (ProtocolInfo *info, mCM->protocols()) {
        QVERIFY(info != 0);
        QVERIFY(info->cmName() == QLatin1String("simple"));
        QVERIFY(info->name() == QLatin1String("simple"));

        QCOMPARE(info->hasParameter(QLatin1String("account")), true);
        QCOMPARE(info->hasParameter(QLatin1String("not-there")), false);

        Q_FOREACH (ProtocolParameter *param, info->parameters()) {
            QCOMPARE(param->name(), QLatin1String("account"));
            QCOMPARE(param->dbusSignature().signature(),
                         QLatin1String("s"));
            QCOMPARE(param->isRequired(), true);
            QCOMPARE(param->isSecret(), false);
        }
        QCOMPARE(info->canRegister(), false);
    }

    QCOMPARE(mCM->supportedProtocols(), QStringList() << QLatin1String("simple"));
}

void TestCmBasics::cleanup()
{
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
