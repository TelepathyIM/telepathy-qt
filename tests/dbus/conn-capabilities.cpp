#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionCapabilities>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestConnCapabilities : public Test
{
    Q_OBJECT

public:
    TestConnCapabilities(QObject *parent = 0)
        : Test(parent), conn(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCapabilities();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *conn;
};

void TestConnCapabilities::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-capabilities");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);
}

void TestConnCapabilities::init()
{
    initImpl();
}

void TestConnCapabilities::testCapabilities()
{
    TestConnHelper *conn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            NULL);
    QCOMPARE(conn->isReady(), false);

    // Before the connection is Ready, it doesn't guarantee support for anything but doesn't crash
    // either if we ask it for something
    QCOMPARE(conn->client()->capabilities().textChats(), false);
    QCOMPARE(conn->client()->capabilities().textChatrooms(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaAudioCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaVideoCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(conn->client()->capabilities().upgradingStreamedMediaCalls(), false);

    QCOMPARE(conn->connect(), true);

    // Now we should have the real information on what the connection supports
    QCOMPARE(conn->client()->capabilities().textChats(), true);
    QCOMPARE(conn->client()->capabilities().textChatrooms(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaAudioCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaVideoCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(conn->client()->capabilities().upgradingStreamedMediaCalls(), false);

    // Now, invalidate the connection by disconnecting it
    QCOMPARE(conn->disconnect(), true);

    // Check that no support for anything is again reported
    QCOMPARE(conn->client()->capabilities().textChats(), false);
    QCOMPARE(conn->client()->capabilities().textChatrooms(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaAudioCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaVideoCalls(), false);
    QCOMPARE(conn->client()->capabilities().streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(conn->client()->capabilities().upgradingStreamedMediaCalls(), false);

    delete conn;
}

void TestConnCapabilities::cleanup()
{
    cleanupImpl();
}

void TestConnCapabilities::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnCapabilities)
#include "_gen/conn-capabilities.cpp.moc.hpp"
