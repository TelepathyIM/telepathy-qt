#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingHandles>
#include <TelepathyQt4/Client/PendingVoidMethodCall>
#include <TelepathyQt4/Client/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/simple-conn.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestHandles : public Test
{
    Q_OBJECT

public:
    TestHandles(QObject *parent = 0)
        : Test(parent), mConnService(0), mConn(0)
    { }

protected Q_SLOTS:
    void expectConnReady(uint newStatus, uint newStatusReason);
    void expectConnInvalidated();
    void expectPendingHandlesFinished(Telepathy::Client::PendingOperation*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestAndRelease();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    SimpleConnection *mConnService;
    Connection *mConn;
    ReferencedHandles mHandles;
};

void TestHandles::expectConnReady(uint newStatus, uint newStatusReason)
{
    switch (newStatus) {
    case Connection::StatusDisconnected:
        qWarning() << "Disconnected";
        mLoop->exit(1);
        break;
    case Connection::StatusConnecting:
        /* do nothing */
        break;
    case Connection::StatusConnected:
        qDebug() << "Ready";
        mLoop->exit(0);
        break;
    default:
        qWarning().nospace() << "What sort of status is "
            << newStatus << "?!";
        mLoop->exit(2);
        break;
    }
}

void TestHandles::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestHandles::expectPendingHandlesFinished(PendingOperation *op)
{
    if (!op->isFinished()) {
        qWarning() << "unfinished";
        mLoop->exit(1);
        return;
    }

    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(2);
        return;
    }

    if (!op->isValid()) {
        qWarning() << "inconsistent results";
        mLoop->exit(3);
        return;
    }

    qDebug() << "finished";
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

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = SIMPLE_CONNECTION(g_object_new(
            SIMPLE_TYPE_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "simple", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = name;
    mConnPath = connPath;

    g_free(name);
    g_free(connPath);

    mConn = new Connection(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    QVERIFY(connect(mConn->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn,
                        SIGNAL(statusChanged(uint, uint)),
                        SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn,
                           SIGNAL(statusChanged(uint, uint)),
                           this,
                           SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mConn->status(), (uint) Connection::StatusConnected);
    }
}

void TestHandles::init()
{
    initImpl();
}

void TestHandles::testRequestAndRelease()
{
    // Test identifiers
    QStringList ids = QStringList() << "alice" << "bob" << "chris";

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->requestHandles(Telepathy::HandleTypeContact, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectPendingHandlesFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandlesFinished(Telepathy::Client::PendingOperation*))));
    ReferencedHandles handles = mHandles;
    mHandles = ReferencedHandles();

    // Verify that the closure indicates correctly which names we requested
    QCOMPARE(pending->namesRequested(), ids);

    // Verify by directly poking the service that the handles correspond to the requested IDs
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);
    for (int i = 0; i < 3; i++) {
        uint handle = handles[i];
        QCOMPARE(QString::fromUtf8(tp_handle_inspect(serviceRepo, handle)), ids[i]);
    }

    // Save the handles to a non-referenced normal container
    Telepathy::UIntList saveHandles = handles.toList();

    // Start releasing the handles, RAII style, and complete the asynchronous process doing that
    handles = ReferencedHandles();
    mLoop->processEvents();

    // Make sure the service side has processed the release as well, by calling a method
    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that the handles have been released
    for (int i = 0; i < 3; i++) {
        uint handle = saveHandles[0];
        QVERIFY(!tp_handle_is_valid(serviceRepo, handle, NULL));
    }
}

void TestHandles::cleanup()
{
    cleanupImpl();
}

void TestHandles::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *, QString, QString)),
                            SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }

        delete mConn;
        mConn = 0;
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestHandles)
#include "_gen/handles.cpp.moc.hpp"
