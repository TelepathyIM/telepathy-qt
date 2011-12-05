#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/Debug>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReferencedHandles>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/debug.h>

#include <tests/lib/glib/bug16307-conn.h>
#include <tests/lib/glib/contacts-noroster-conn.h>
#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConnIntrospectCornercases : public Test
{
    Q_OBJECT

public:
    TestConnIntrospectCornercases(QObject *parent = 0)
        : Test(parent), mConnService(0), mNumSelfHandleChanged(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void onSelfHandleChanged(uint);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testSelfHandleChangeBeforeConnecting();
    void testSelfHandleChangeWhileBuilding();
    void testSlowpath();
    void testStatusChange();
    void testNoRoster();

    void cleanup();
    void cleanupTestCase();

private:
    TpBaseConnection *mConnService;
    ConnectionPtr mConn;
    QList<ConnectionStatus> mStatuses;
    int mNumSelfHandleChanged;
};

void TestConnIntrospectCornercases::expectConnInvalidated()
{
    qDebug() << "conn invalidated";

    mLoop->exit(0);
}

void TestConnIntrospectCornercases::onSelfHandleChanged(uint handle)
{
    qDebug() << "got new self handle" << handle;
    mNumSelfHandleChanged++;
}

void TestConnIntrospectCornercases::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-introspect-cornercases");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);
}

void TestConnIntrospectCornercases::init()
{
    initImpl();

    QVERIFY(mConn.isNull());
    QVERIFY(mConnService == 0);

    QVERIFY(mStatuses.empty());
    QCOMPARE(mNumSelfHandleChanged, 0);

    // don't create the client- or service-side connection objects here, as it's expected that many
    // different types of service connections with different initial states need to be used
}

void TestConnIntrospectCornercases::testSelfHandleChangeBeforeConnecting()
{
    gchar *name;
    gchar *connPath;
    GError *error = 0;

    TpTestsSimpleConnection *simpleConnService =
        TP_TESTS_SIMPLE_CONNECTION(
            g_object_new(
                TP_TESTS_TYPE_SIMPLE_CONNECTION,
                "account", "me@example.com",
                "protocol", "simple",
                NULL));
    QVERIFY(simpleConnService != 0);

    mConnService = TP_BASE_CONNECTION(simpleConnService);
    QVERIFY(mConnService != 0);

    QVERIFY(tp_base_connection_register(mConnService, "simple",
                &name, &connPath, &error));
    QVERIFY(error == 0);

    mConn = Connection::create(QLatin1String(name), QLatin1String(connPath),
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    g_free(name); name = 0;
    g_free(connPath); connPath = 0;

    // Set the initial self handle (we're not using the conn service normally, so it doesn't do this
    // by itself)
    tp_tests_simple_connection_set_identifier(simpleConnService, "me@example.com");

    // Make the conn Connecting, and with FeatureCore ready

    tp_base_connection_change_status(mConnService, TP_CONNECTION_STATUS_CONNECTING,
            TP_CONNECTION_STATUS_REASON_REQUESTED);

    PendingOperation *op = mConn->becomeReady();
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(op->isFinished());
    QVERIFY(mConn->isValid());
    QVERIFY(op->isValid());

    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Tp::ConnectionStatusConnecting));

    // Start introspecting the SelfContact feature

    op = mConn->becomeReady(Connection::FeatureSelfContact | Connection::FeatureConnected);
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    // Change the self handle, before the connection is Connected

    tp_tests_simple_connection_set_identifier(simpleConnService, "myself@example.com");

    // Now change it to Connected
    tp_base_connection_change_status(mConnService, TP_CONNECTION_STATUS_CONNECTED,
            TP_CONNECTION_STATUS_REASON_REQUESTED);

    // Try to finish the SelfContact operation, running the mainloop for a while

    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(op->isFinished(), true);
    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QCOMPARE(mConn->isReady(Connection::FeatureSelfContact), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(ConnectionStatusConnected));
}

void TestConnIntrospectCornercases::testSelfHandleChangeWhileBuilding()
{
    gchar *name;
    gchar *connPath;
    GError *error = 0;

    TpTestsSimpleConnection *simpleConnService =
        TP_TESTS_SIMPLE_CONNECTION(
            g_object_new(
                TP_TESTS_TYPE_SIMPLE_CONNECTION,
                "account", "me@example.com",
                "protocol", "simple",
                NULL));
    QVERIFY(simpleConnService != 0);

    mConnService = TP_BASE_CONNECTION(simpleConnService);
    QVERIFY(mConnService != 0);

    QVERIFY(tp_base_connection_register(mConnService, "simple",
                &name, &connPath, &error));
    QVERIFY(error == 0);

    mConn = Connection::create(QLatin1String(name), QLatin1String(connPath),
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    g_free(name); name = 0;
    g_free(connPath); connPath = 0;

    // Make the conn Connected, and with FeatureCore ready

    PendingOperation *op = mConn->lowlevel()->requestConnect();
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(op->isFinished());
    QVERIFY(mConn->isValid());
    QVERIFY(op->isValid());

    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Tp::ConnectionStatusConnected));

    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QVERIFY(mConn->selfHandle() != 0);

    // Start introspecting the SelfContact feature

    op = mConn->becomeReady(Connection::FeatureSelfContact);
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    // Run one mainloop iteration, so ReadinessHelper calls introspectSelfContact
    mLoop->processEvents();

    // Change the self handle, so a rebuild has to be done after the first build finishes
    tp_tests_simple_connection_set_identifier(simpleConnService, "myself@example.com");

    // Try to finish the SelfContact operation, running the mainloop for a while
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(op->isFinished(), true);
    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QCOMPARE(mConn->isReady(Connection::FeatureSelfContact), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(ConnectionStatusConnected));
    QCOMPARE(mConn->selfContact()->id(), QString::fromLatin1("me@example.com"));

    // We should shortly also receive a self contact change to the rebuilt contact
    QVERIFY(connect(mConn.data(),
                SIGNAL(selfContactChanged()),
                mLoop,
                SLOT(quit())));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->selfContact()->id(), QString::fromLatin1("myself@example.com"));
    QCOMPARE(mConn->selfContact()->handle()[0], mConn->selfHandle());

    // Change the self handle yet again, which should cause a self handle and self contact change to be signalled
    // (in that order)
    QVERIFY(connect(mConn.data(),
                SIGNAL(selfHandleChanged(uint)),
                SLOT(onSelfHandleChanged(uint))));

    tp_tests_simple_connection_set_identifier(simpleConnService, "irene@example.com");

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mConn->isValid());
    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QCOMPARE(mConn->isReady(Connection::FeatureSelfContact), true);

    // We should've received a single self handle change and the self contact should've changed
    // (exiting the mainloop)
    QCOMPARE(mNumSelfHandleChanged, 1);
    QCOMPARE(mConn->selfContact()->id(), QString::fromLatin1("irene@example.com"));
    QCOMPARE(mConn->selfContact()->handle()[0], mConn->selfHandle());

    // Last but not least, try two consequtive changes
    tp_tests_simple_connection_set_identifier(simpleConnService, "me@example.com");
    tp_tests_simple_connection_set_identifier(simpleConnService, "myself@example.com");

    // We should receive two more self handle changes in total, and one self contact change for
    // each mainloop run
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mConn->isValid());
    QCOMPARE(mConn->selfContact()->id(), QString::fromLatin1("me@example.com"));

    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mConn->isValid());
    QCOMPARE(mConn->selfContact()->id(), QString::fromLatin1("myself@example.com"));

    QCOMPARE(mNumSelfHandleChanged, 3);
}

void TestConnIntrospectCornercases::testSlowpath()
{
    gchar *name;
    gchar *connPath;
    GError *error = 0;

    TpTestsBug16307Connection *bugConnService =
        TP_TESTS_BUG16307_CONNECTION(
            g_object_new(
                TP_TESTS_TYPE_BUG16307_CONNECTION,
                "account", "me@example.com",
                "protocol", "simple",
                NULL));
    QVERIFY(bugConnService != 0);

    mConnService = TP_BASE_CONNECTION(bugConnService);
    QVERIFY(mConnService != 0);

    QVERIFY(tp_base_connection_register(mConnService, "simple",
                &name, &connPath, &error));
    QVERIFY(error == 0);

    mConn = Connection::create(QLatin1String(name), QLatin1String(connPath),
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    g_free(name); name = 0;
    g_free(connPath); connPath = 0;

    PendingOperation *op = mConn->becomeReady();
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    tp_tests_bug16307_connection_inject_get_status_return(bugConnService);

    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(op->isFinished(), true);
    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(ConnectionStatusConnected));
}

void TestConnIntrospectCornercases::testStatusChange()
{
    gchar *name;
    gchar *connPath;
    GError *error = 0;

    TpTestsSimpleConnection *simpleConnService =
        TP_TESTS_SIMPLE_CONNECTION(
            g_object_new(
                TP_TESTS_TYPE_SIMPLE_CONNECTION,
                "account", "me@example.com",
                "protocol", "simple",
                NULL));
    QVERIFY(simpleConnService != 0);

    mConnService = TP_BASE_CONNECTION(simpleConnService);
    QVERIFY(mConnService != 0);

    QVERIFY(tp_base_connection_register(mConnService, "simple",
                &name, &connPath, &error));
    QVERIFY(error == 0);

    mConn = Connection::create(QLatin1String(name), QLatin1String(connPath),
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    g_free(name); name = 0;
    g_free(connPath); connPath = 0;

    // Make core ready first, because Connection has internal handling for the status changing
    // during core introspection, and we rather want to test the more general ReadinessHelper
    // mechanism

    PendingOperation *op = mConn->becomeReady();
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(op->isFinished(), true);
    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(ConnectionStatusDisconnected));

    // Now, begin making Connected ready

    op = mConn->becomeReady(Connection::FeatureConnected);
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    mLoop->processEvents();

    // But disturb it by changing the status!

    TpHandleRepoIface *contact_repo = tp_base_connection_get_handles(mConnService,
            TP_HANDLE_TYPE_CONTACT);

    mConnService->self_handle = tp_handle_ensure(contact_repo, "me@example.com",
            NULL, NULL);

    tp_base_connection_change_status(mConnService,
            TP_CONNECTION_STATUS_CONNECTING,
            TP_CONNECTION_STATUS_REASON_REQUESTED);

    // Do that again! (The earlier op stil hasn't finished by definition)
    mConn->becomeReady(Features() << Connection::FeatureConnected);

    tp_base_connection_change_status(mConnService,
            TP_CONNECTION_STATUS_CONNECTED,
            TP_CONNECTION_STATUS_REASON_REQUESTED);

    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(op->isFinished(), true);
    QCOMPARE(mConn->isReady(Connection::FeatureCore), true);
    QCOMPARE(mConn->isReady(Connection::FeatureConnected), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(ConnectionStatusConnected));
}

void TestConnIntrospectCornercases::testNoRoster()
{
    gchar *name;
    gchar *connPath;
    GError *error = 0;

    TpTestsContactsNorosterConnection *connService =
        TP_TESTS_CONTACTS_NOROSTER_CONNECTION(
            g_object_new(
                TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION,
                "account", "me@example.com",
                "protocol", "contacts-noroster",
                NULL));
    QVERIFY(connService != 0);

    mConnService = TP_BASE_CONNECTION(connService);
    QVERIFY(mConnService != 0);

    QVERIFY(tp_base_connection_register(mConnService, "simple",
                &name, &connPath, &error));
    QVERIFY(error == 0);

    mConn = Connection::create(QLatin1String(name), QLatin1String(connPath),
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    g_free(name); name = 0;
    g_free(connPath); connPath = 0;

    PendingOperation *op = mConn->lowlevel()->requestConnect();
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->status(), Tp::ConnectionStatusConnected);

    op = mConn->becomeReady(Connection::FeatureRoster);
    QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(op->isFinished());
    QVERIFY(mConn->isReady(Connection::FeatureRoster));
    QVERIFY(!mConn->actualFeatures().contains(Connection::FeatureRoster));
}

void TestConnIntrospectCornercases::cleanup()
{
    if (mConn) {
        QVERIFY(mConnService != 0);

        // Disconnect and wait for invalidation
        tp_base_connection_change_status(
                mConnService,
                TP_CONNECTION_STATUS_DISCONNECTED,
                TP_CONNECTION_STATUS_REASON_REQUESTED);

        QVERIFY(connect(mConn.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                            const QString &, const QString &)),
                    SLOT(expectConnInvalidated())));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(!mConn->isValid());

        processDBusQueue(mConn.data());

        mConn.reset();
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    mStatuses.clear();
    mNumSelfHandleChanged = 0;

    cleanupImpl();
}

void TestConnIntrospectCornercases::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnIntrospectCornercases)
#include "_gen/conn-introspect-cornercases.cpp.moc.hpp"
