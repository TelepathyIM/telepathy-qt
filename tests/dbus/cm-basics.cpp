#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingStringList>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/debug.h>

#include <tests/lib/glib/echo2/connection-manager.h>
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
        EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER,
        NULL));
    QVERIFY(mCMService != 0);

    mCMServiceLegacy = TP_BASE_CONNECTION_MANAGER(g_object_new(
        TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER,
        NULL));
    QVERIFY(mCMServiceLegacy != 0);

    QVERIFY(tp_base_connection_manager_register(mCMService));
    QVERIFY(tp_base_connection_manager_register(mCMServiceLegacy));
}

void TestCmBasics::init()
{
    initImpl();
}

void TestCmBasics::testBasics()
{
    mCM = ConnectionManager::create(QLatin1String("example_echo_2"));
    QCOMPARE(mCM->isReady(), false);

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
    QCOMPARE(mCM->supportedProtocols(), QStringList() << QLatin1String("example"));

    QVERIFY(mCM->hasProtocol(QLatin1String("example")));
    QVERIFY(!mCM->hasProtocol(QLatin1String("not-there")));

    ProtocolInfo *info = mCM->protocol(QLatin1String("example"));
    QVERIFY(info != 0);

    QCOMPARE(info->cmName(), QLatin1String("example_echo_2"));
    QCOMPARE(info->name(), QLatin1String("example"));

    QCOMPARE(info->hasParameter(QLatin1String("account")), true);
    QCOMPARE(info->hasParameter(QLatin1String("not-there")), false);

    QCOMPARE(info->parameters().size(), 1);

    ProtocolParameter *param = info->parameters().at(0);
    QCOMPARE(param->name(), QLatin1String("account"));
    QCOMPARE(static_cast<uint>(param->type()), static_cast<uint>(QVariant::String));
    QCOMPARE(param->defaultValue().isNull(), true);
    QCOMPARE(param->dbusSignature().signature(), QLatin1String("s"));
    QCOMPARE(param->isRequired(), true);
    QCOMPARE(param->isRequiredForRegistration(), true); // though it can't register!
    QCOMPARE(param->isSecret(), false);

    QVERIFY(*param == QLatin1String("account"));

    QCOMPARE(info->canRegister(), false);

    QCOMPARE(info->capabilities()->isSpecificToContact(), false);
    QCOMPARE(info->capabilities()->supportsTextChatrooms(), false);
    QCOMPARE(info->capabilities()->supportsTextChats(), true);
    QCOMPARE(info->capabilities()->supportsMediaCalls(), false);
    QCOMPARE(info->capabilities()->supportsAudioCalls(), false);
    QCOMPARE(info->capabilities()->supportsVideoCalls(false), false);
    QCOMPARE(info->capabilities()->supportsVideoCalls(true), false);
    QCOMPARE(info->capabilities()->supportsUpgradingCalls(), false);

    QCOMPARE(info->vcardField(), QLatin1String("x-telepathy-example"));
    QCOMPARE(info->englishName(), QLatin1String("Echo II example"));
    QCOMPARE(info->iconName(), QLatin1String("im-icq"));

    QCOMPARE(mCM->supportedProtocols(), QStringList() << QLatin1String("example"));
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
