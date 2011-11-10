#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/ReferencedHandles>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestHandles : public Test
{
    Q_OBJECT

public:
    TestHandles(QObject *parent = 0)
        : Test(parent), mConn(0)
    { }

protected Q_SLOTS:
    void expectPendingHandlesFinished(Tp::PendingOperation*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestAndRelease();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    ReferencedHandles mHandles;
};

void TestHandles::expectPendingHandlesFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingHandles *pending = qobject_cast<PendingHandles*>(op);
    mHandles = pending->handles();
    mLoop->exit(0);
}

void TestHandles::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("handles");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestHandles::init()
{
    initImpl();
}

void TestHandles::testRequestAndRelease()
{
    // Test identifiers
    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->client()->lowlevel()->requestHandles(Tp::HandleTypeContact, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingHandlesFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Tp::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandlesFinished(Tp::PendingOperation*))));
    ReferencedHandles handles = mHandles;
    mHandles = ReferencedHandles();

    // Verify that the closure indicates correctly which names we requested
    QCOMPARE(pending->namesRequested(), ids);

    // Verify by directly poking the service that the handles correspond to the requested IDs
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    for (int i = 0; i < 3; i++) {
        uint handle = handles[i];
        QCOMPARE(QString::fromUtf8(tp_handle_inspect(serviceRepo, handle)), ids[i]);
    }

    // Save the handles to a non-referenced normal container
    Tp::UIntList saveHandles = handles.toList();

    // Start releasing the handles, RAII style, and complete the asynchronous process doing that
    handles = ReferencedHandles();
    mLoop->processEvents();
    processDBusQueue(mConn->client().data());
}

void TestHandles::cleanup()
{
    cleanupImpl();
}

void TestHandles::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestHandles)
#include "_gen/handles.cpp.moc.hpp"
