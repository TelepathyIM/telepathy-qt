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
    bool mConnInvalidated;
};

void TestConnCapabilities::expectConnInvalidated()
{
    mConnInvalidated = true;
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
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "foo", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);
}

void TestConnCapabilities::init()
{
    mConnInvalidated = false;
    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());

    initImpl();
}

void TestConnCapabilities::testCapabilities()
{
    // Before the connection is Ready, it doesn't guarantee support for anything but doesn't crash
    // either if we ask it for something
    QVERIFY(mConn->capabilities() != 0);
    QCOMPARE(mConn->capabilities()->supportsTextChats(), false);
    QCOMPARE(mConn->capabilities()->supportsTextChatrooms(), false);
    QCOMPARE(mConn->capabilities()->supportsMediaCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsAudioCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(false), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(true), false);
    QCOMPARE(mConn->capabilities()->supportsUpgradingCalls(), false);

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(mConn->status(), Connection::StatusConnected);

    QVERIFY(mConn->requestsInterface() != 0);
    QVERIFY(mConn->capabilities() != 0);

    // Now we should have the real information on what the connection supports
    QCOMPARE(mConn->capabilities()->supportsTextChats(), true);
    QCOMPARE(mConn->capabilities()->supportsTextChatrooms(), false);
    QCOMPARE(mConn->capabilities()->supportsMediaCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsAudioCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(false), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(true), false);
    QCOMPARE(mConn->capabilities()->supportsUpgradingCalls(), false);

    // Now, invalidate the connection by disconnecting it
    QVERIFY(connect(mConn.data(),
                SIGNAL(invalidated(Tp::DBusProxy *,
                        const QString &, const QString &)),
                SLOT(expectConnInvalidated())));
    mConn->requestDisconnect();

    // Check that capabilities doesn't go NULL in the process of the connection going invalidated
    do {
        QVERIFY(mConn->capabilities() != 0);
        mLoop->processEvents();
    } while (!mConnInvalidated);

    QVERIFY(!mConn->isValid());
    QCOMPARE(mConn->status(), Connection::StatusDisconnected);

    // Check that no support for anything is again reported
    QCOMPARE(mConn->capabilities()->supportsTextChats(), false);
    QCOMPARE(mConn->capabilities()->supportsTextChatrooms(), false);
    QCOMPARE(mConn->capabilities()->supportsMediaCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsAudioCalls(), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(false), false);
    QCOMPARE(mConn->capabilities()->supportsVideoCalls(true), false);
    QCOMPARE(mConn->capabilities()->supportsUpgradingCalls(), false);

}

void TestConnCapabilities::cleanup()
{
    mConn.reset();

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
