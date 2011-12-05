#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Debug>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/debug.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConnBasics : public Test
{
    Q_OBJECT

public:
    TestConnBasics(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnReady(Tp::ConnectionStatus);
    void expectConnInvalidated();
    void expectPresenceAvailable(const Tp::SimplePresence &);
    void onRequestConnectFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();
    void testSimplePresence();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
    QList<ConnectionStatus> mStatuses;
};

void TestConnBasics::expectConnReady(Tp::ConnectionStatus newStatus)
{
    qDebug() << "connection changed to status" << newStatus;
    switch (newStatus) {
    case ConnectionStatusDisconnected:
        qWarning() << "Disconnected";
        break;
    case ConnectionStatusConnecting:
        QCOMPARE(mConn->isReady(Connection::FeatureConnected), false);
        mStatuses << newStatus;
        qDebug() << "Connecting";
        break;
    case ConnectionStatusConnected:
        QCOMPARE(mConn->isReady(Connection::FeatureConnected), true);
        mStatuses << newStatus;
        qDebug() << "Connected";
        break;
    default:
        qWarning().nospace() << "What sort of status is "
            << newStatus << "?!";
        break;
    }
}

void TestConnBasics::expectConnInvalidated()
{
    qDebug() << "conn invalidated";

    mLoop->exit(0);
}

void TestConnBasics::expectPresenceAvailable(const Tp::SimplePresence &presence)
{
    if (presence.type == Tp::ConnectionPresenceTypeAvailable) {
        mLoop->exit(0);
        return;
    }
    mLoop->exit(1);
}

void TestConnBasics::onRequestConnectFinished(Tp::PendingOperation *op)
{
    QCOMPARE(mConn->status(), ConnectionStatusConnected);
    QVERIFY(mStatuses.contains(ConnectionStatusConnected));
    mLoop->exit(0);
}

void TestConnBasics::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);
}

void TestConnBasics::init()
{
    initImpl();

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
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

    QVERIFY(connect(mConn.data(),
                    SIGNAL(statusChanged(Tp::ConnectionStatus)),
                    SLOT(expectConnReady(Tp::ConnectionStatus))));

    qDebug() << "waiting connection to become connected";
    PendingOperation *pr = mConn->becomeReady(Connection::FeatureConnected);
    QVERIFY(connect(pr,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));

    PendingOperation *pc = mConn->lowlevel()->requestConnect();
    QVERIFY(connect(pc,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onRequestConnectFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(pr->isFinished(), true);
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(pc->isFinished(), true);
    QCOMPARE(mConn->isReady(Connection::FeatureConnected), true);
    qDebug() << "connection is now ready";

    QVERIFY(disconnect(mConn.data(),
                       SIGNAL(statusChanged(Tp::ConnectionStatus)),
                       this,
                       SLOT(expectConnReady(Tp::ConnectionStatus))));
}

void TestConnBasics::testBasics()
{
    QCOMPARE(static_cast<uint>(mConn->statusReason()),
            static_cast<uint>(ConnectionStatusReasonRequested));
}

void TestConnBasics::testSimplePresence()
{
    qDebug() << "Making SimplePresence ready";

    Features features = Features() << Connection::FeatureSimplePresence;
    QCOMPARE(mConn->isReady(features), false);
    QVERIFY(connect(mConn->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    qDebug() << "SimplePresence ready";
    qDebug() << "mConn->status:" << mConn->status();

    const QStringList canSetNames = QStringList()
        << QLatin1String("available")
        << QLatin1String("busy")
        << QLatin1String("away");

    const QStringList cantSetNames = QStringList()
        << QLatin1String("offline")
        << QLatin1String("unknown")
        << QLatin1String("error");

    const QStringList expectedNames = canSetNames + cantSetNames;

    const ConnectionPresenceType expectedTypes[] = {
        ConnectionPresenceTypeAvailable,
        ConnectionPresenceTypeBusy,
        ConnectionPresenceTypeAway,
        ConnectionPresenceTypeOffline,
        ConnectionPresenceTypeUnknown,
        ConnectionPresenceTypeError
    };

    SimpleStatusSpecMap statuses = mConn->lowlevel()->allowedPresenceStatuses();
    Q_FOREACH (QString name, statuses.keys()) {
        QVERIFY(expectedNames.contains(name));

        if (canSetNames.contains(name)) {
            QVERIFY(statuses[name].maySetOnSelf);
            QVERIFY(statuses[name].canHaveMessage);
        } else {
            QVERIFY(cantSetNames.contains(name));
            QVERIFY(!statuses[name].maySetOnSelf);
            QVERIFY(!statuses[name].canHaveMessage);
        }

        QCOMPARE(statuses[name].type,
                 static_cast<uint>(expectedTypes[expectedNames.indexOf(name)]));
    }

    QCOMPARE(mConn->lowlevel()->maxPresenceStatusMessageLength(), (uint) 512);
}

void TestConnBasics::cleanup()
{
    if (mConn) {
        QVERIFY(mConn->isValid());

        GHashTable *details = tp_asv_new(
                "debug-message", G_TYPE_STRING, "woo i'm going doooooown",
                "x-tpqt-test-rgba-herring-color", G_TYPE_UINT, 0xff0000ffU,
                NULL
                );

        // Disconnect and wait for invalidation
        tp_base_connection_disconnect_with_dbus_error(TP_BASE_CONNECTION(mConnService),
                TP_QT_ERROR_CANCELLED.latin1(),
                details,
                TP_CONNECTION_STATUS_REASON_REQUESTED);

        g_hash_table_destroy(details);

        QVERIFY(connect(mConn.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                            const QString &, const QString &)),
                    SLOT(expectConnInvalidated())));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(!mConn->isValid());

        // Check that we got the connection error details
        QCOMPARE(static_cast<uint>(mConn->statusReason()),
                 static_cast<uint>(ConnectionStatusReasonRequested));

        QVERIFY(mConn->errorDetails().isValid());

        QVERIFY(mConn->errorDetails().hasDebugMessage());
        QCOMPARE(mConn->errorDetails().debugMessage(), QLatin1String("woo i'm going doooooown"));

#if 0
        // Not yet there
        QVERIFY(!mConn->errorDetails().hasExpectedHostname());
        QVERIFY(!mConn->errorDetails().hasCertificateHostname());
#endif

        QVERIFY(mConn->errorDetails().allDetails().contains(
                    QLatin1String("x-tpqt-test-rgba-herring-color")));
        QCOMPARE(qdbus_cast<uint>(mConn->errorDetails().allDetails().value(
                        QLatin1String("x-tpqt-test-rgba-herring-color"))),
                0xff0000ffU);

        processDBusQueue(mConn.data());

        mConn.reset();
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupImpl();
}

void TestConnBasics::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnBasics)
#include "_gen/conn-basics.cpp.moc.hpp"
