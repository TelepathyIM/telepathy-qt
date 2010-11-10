#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

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
    void expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason);
    void expectConnInvalidated();
    void expectPresenceAvailable(const Tp::SimplePresence &);

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
};

void TestConnBasics::expectConnReady(Tp::ConnectionStatus newStatus,
        Tp::ConnectionStatusReason newStatusReason)
{
    qDebug() << "connection changed to status" << newStatus;
    switch (newStatus) {
    case ConnectionStatusDisconnected:
        qWarning() << "Disconnected";
        mLoop->exit(1);
        break;
    case ConnectionStatusConnecting:
        /* do nothing */
        break;
    case ConnectionStatusConnected:
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

void TestConnBasics::expectConnInvalidated()
{
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

    mConn->requestConnect();

    qDebug() << "waiting connection to become ready";
    QVERIFY(connect(mConn->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    qDebug() << "connection is now ready";

    if (mConn->status() != ConnectionStatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
                        SLOT(expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
                           this,
                           SLOT(expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason))));
        QCOMPARE(mConn->status(), ConnectionStatusConnected);
    }
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

    SimpleStatusSpecMap statuses = mConn->allowedPresenceStatuses();
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
}

void TestConnBasics::cleanup()
{
    if (mConn) {
        QVERIFY(mConn->isValid());

        GHashTable *details = tp_asv_new(
                "debug-message", G_TYPE_STRING, "woo i'm going doooooown",
                "x-tpqt4-test-rgba-herring-color", G_TYPE_UINT, 0xff0000ffU,
                NULL
                );

        // Disconnect and wait for invalidation
        tp_base_connection_disconnect_with_dbus_error(TP_BASE_CONNECTION(mConnService),
                TELEPATHY_ERROR_CANCELLED,
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
                    QLatin1String("x-tpqt4-test-rgba-herring-color")));
        QCOMPARE(qdbus_cast<uint>(mConn->errorDetails().allDetails().value(
                        QLatin1String("x-tpqt4-test-rgba-herring-color"))),
                0xff0000ffU);

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
