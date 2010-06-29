#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

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
    void expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason);
    void expectConnInvalidated();
    void expectPresenceAvailable(const Tp::SimplePresence &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testSimplePresence();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
};

void TestConnBasics::expectConnReady(Tp::Connection::Status newStatus,
        Tp::ConnectionStatusReason newStatusReason)
{
    qDebug() << "connection changed to status" << newStatus;
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

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            0));
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
}

void TestConnBasics::init()
{
    initImpl();

    mConn = Connection::create(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    qDebug() << "waiting connection to become ready";
    QVERIFY(connect(mConn->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    qDebug() << "connection is now ready";

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(Tp::Connection::Status, Tp::ConnectionStatusReason)),
                        SLOT(expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(Tp::Connection::Status, Tp::ConnectionStatusReason)),
                           this,
                           SLOT(expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason))));
        QCOMPARE(mConn->status(), Connection::StatusConnected);
    }
}

void TestConnBasics::testSimplePresence()
{
    Features features = Features() << Connection::FeatureSimplePresence;
    QCOMPARE(mConn->isReady(features), false);
    QVERIFY(connect(mConn->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    qDebug() << "mConn->status:" << mConn->status();
}

void TestConnBasics::cleanup()
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

    cleanupImpl();
}

void TestConnBasics::cleanupTestCase()
{
    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnBasics)
#include "_gen/conn-basics.cpp.moc.hpp"
