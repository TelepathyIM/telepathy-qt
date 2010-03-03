#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/PendingReady>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/echo2/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConnCapabilities : public Test
{
    Q_OBJECT

public:
    TestConnCapabilities(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCapabilities();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ExampleEcho2Connection *mConnService;
    ConnectionPtr mConn;
};

void TestConnCapabilities::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestConnCapabilities::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-capabilities");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_ECHO_2_CONNECTION(g_object_new(
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "foo", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = name;
    mConnPath = connPath;

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath);
}

void TestConnCapabilities::init()
{
    initImpl();
}

void TestConnCapabilities::testCapabilities()
{
    QVERIFY(mConn->capabilities() == 0);
    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(mConn->status(), Connection::StatusConnected);
    QVERIFY(mConn->requestsInterface() != 0);
    QVERIFY(mConn->capabilities() != 0);
    QCOMPARE(mConn->capabilities()->supportsTextChats(), true);
    QCOMPARE(mConn->capabilities()->supportsTextChatrooms(), false);
    QCOMPARE(mConn->capabilities()->supportsMediaCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsAudioCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(false), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(true), false);
    QCOMPARE(mConn->capabilities()->supportsUpgradingCalls(), false);
}

void TestConnCapabilities::cleanup()
{
    cleanupImpl();
}

void TestConnCapabilities::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for invalidated
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *,
                                               const QString &, const QString &)),
                            SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnCapabilities)
#include "_gen/conn-capabilities.cpp.moc.hpp"
