#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Channel>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/ReferencedHandles>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestConnRequests : public Test
{
    Q_OBJECT

public:
    TestConnRequests(QObject *parent = 0)
        : Test(parent), mConn(0), mHandle(0)
    { }

protected Q_SLOTS:
    void expectPendingHandleFinished(Tp::PendingOperation*);
    void expectCreateChannelFinished(Tp::PendingOperation *);
    void expectEnsureChannelFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestHandle();
    void testCreateChannel();
    void testEnsureChannel();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    QString mChanObjectPath;
    uint mHandle;
};

void TestConnRequests::expectPendingHandleFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingHandles *pending = qobject_cast<PendingHandles*>(op);
    mHandle = pending->handles().at(0);
    mLoop->exit(0);
}

void TestConnRequests::expectCreateChannelFinished(PendingOperation* op)
{
    TEST_VERIFY_OP(op);

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    ChannelPtr chan = pc->channel();
    mChanObjectPath = chan->objectPath();
    mLoop->exit(0);
}

void TestConnRequests::expectEnsureChannelFinished(PendingOperation* op)
{
    TEST_VERIFY_OP(op);

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    ChannelPtr chan = pc->channel();
    QCOMPARE(pc->yours(), false);
    QCOMPARE(chan->objectPath(), mChanObjectPath);
    mLoop->exit(0);
}

void TestConnRequests::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-requests");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestConnRequests::init()
{
    initImpl();
}

void TestConnRequests::testRequestHandle()
{
    // Test identifiers
    QStringList ids = QStringList() << QLatin1String("alice");

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->client()->lowlevel()->requestHandles(Tp::HandleTypeContact, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingHandleFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Tp::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandleFinished(Tp::PendingOperation*))));
    QVERIFY(mHandle != 0);
}

void TestConnRequests::testCreateChannel()
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   mHandle);
    QVERIFY(connect(mConn->client()->lowlevel()->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestConnRequests::testEnsureChannel()
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   mHandle);
    QVERIFY(connect(mConn->client()->lowlevel()->ensureChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectEnsureChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestConnRequests::cleanup()
{
    cleanupImpl();
}

void TestConnRequests::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnRequests)
#include "_gen/conn-requests.cpp.moc.hpp"
