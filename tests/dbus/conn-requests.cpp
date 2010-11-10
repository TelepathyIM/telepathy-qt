#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/echo2/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConnRequests : public Test
{
    Q_OBJECT

public:
    TestConnRequests(QObject *parent = 0)
        : Test(parent), mConnService(0), mHandle(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
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
    QString mConnName, mConnPath;
    ExampleEcho2Connection *mConnService;
    ConnectionPtr mConn;
    QString mChanObjectPath;
    uint mHandle;
};

void TestConnRequests::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestConnRequests::expectPendingHandleFinished(PendingOperation *op)
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
    mHandle = pending->handles().at(0);
    mLoop->exit(0);
}

void TestConnRequests::expectCreateChannelFinished(PendingOperation* op)
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

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    ChannelPtr chan = pc->channel();
    mChanObjectPath = chan->objectPath();
    mLoop->exit(0);
}

void TestConnRequests::expectEnsureChannelFinished(PendingOperation* op)
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
    g_set_prgname("conn-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_ECHO_2_CONNECTION(g_object_new(
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    QCOMPARE(mConn->status(), ConnectionStatusConnected);
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
    PendingHandles *pending = mConn->requestHandles(Tp::HandleTypeContact, ids);
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
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mHandle);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestConnRequests::testEnsureChannel()
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mHandle);
    QVERIFY(connect(mConn->ensureChannel(request),
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
    if (mConn) {
        // Disconnect and wait for the readiness change
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

QTEST_MAIN(TestConnRequests)
#include "_gen/conn-requests.cpp.moc.hpp"
