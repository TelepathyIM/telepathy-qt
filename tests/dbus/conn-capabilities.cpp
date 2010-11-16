#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ContactFactory>
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
    QCOMPARE(mConn->capabilities().textChats(), false);
    QCOMPARE(mConn->capabilities().textChatrooms(), false);
    QCOMPARE(mConn->capabilities().streamedMediaCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaAudioCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaVideoCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(mConn->capabilities().upgradingStreamedMediaCalls(), false);

    QVERIFY(connect(mConn->lowlevel()->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(mConn->status(), ConnectionStatusConnected);

    // Now we should have the real information on what the connection supports
    QCOMPARE(mConn->capabilities().textChats(), true);
    QCOMPARE(mConn->capabilities().textChatrooms(), false);
    QCOMPARE(mConn->capabilities().streamedMediaCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaAudioCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaVideoCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(mConn->capabilities().upgradingStreamedMediaCalls(), false);

    // Now, invalidate the connection by disconnecting it
    QVERIFY(connect(mConn.data(),
                SIGNAL(invalidated(Tp::DBusProxy *,
                        const QString &, const QString &)),
                SLOT(expectConnInvalidated())));
    mConn->lowlevel()->requestDisconnect();

    do {
        mLoop->processEvents();
    } while (!mConnInvalidated);

    QVERIFY(!mConn->isValid());
    QCOMPARE(mConn->status(), ConnectionStatusDisconnected);

    // Check that no support for anything is again reported
    QCOMPARE(mConn->capabilities().textChats(), false);
    QCOMPARE(mConn->capabilities().textChatrooms(), false);
    QCOMPARE(mConn->capabilities().streamedMediaCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaAudioCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaVideoCalls(), false);
    QCOMPARE(mConn->capabilities().streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(mConn->capabilities().upgradingStreamedMediaCalls(), false);
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
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
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
