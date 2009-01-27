#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestConnBasics : public Test
{
    Q_OBJECT

public:
    TestConnBasics(QObject *parent = 0)
        : Test(parent), mConnService(0), mConn(0)
    { }

protected Q_SLOTS:
    void expectConnReady(uint, uint);
    void expectConnInvalidated();
    void expectPresenceAvailable(const Telepathy::SimplePresence &);

private Q_SLOTS:
    void initTestCase();

    void testSimplePresence();

    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ContactsConnection *mConnService;
    Connection *mConn;
};

void TestConnBasics::expectConnReady(uint newStatus, uint newStatusReason)
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

void TestConnBasics::expectPresenceAvailable(const Telepathy::SimplePresence &presence)
{
    if (presence.type == Telepathy::ConnectionPresenceTypeAvailable) {
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

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = CONTACTS_CONNECTION(g_object_new(
            CONTACTS_TYPE_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "contacts", &name, &connPath, &error));
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

    qDebug() << "waiting connection to become ready";
    QVERIFY(connect(mConn->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    qDebug() << "connection is now ready";

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

void TestConnBasics::testSimplePresence()
{
    QCOMPARE(mConn->isReady(Connection::FeatureSelfPresence), false);
    QVERIFY(connect(mConn->becomeReady(Connection::FeatureSelfPresence),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(Connection::FeatureSelfPresence), true);

    QVERIFY(connect(mConn,
                    SIGNAL(selfPresenceChanged(const Telepathy::SimplePresence &)),
                    SLOT(expectPresenceAvailable(const Telepathy::SimplePresence &))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->selfPresence().type, (uint) Telepathy::ConnectionPresenceTypeAvailable);
    QCOMPARE(mConn->selfPresence().status, QString("available"));

    qDebug() << "mConn->status:" << mConn->status();
    qDebug() << "mConn->selfPresence "
                "type:" << mConn->selfPresence().type <<
                "status:" << mConn->selfPresence().status <<
                "status message:" << mConn->selfPresence().statusMessage;
}

void TestConnBasics::cleanupTestCase()
{
    if (mConn != 0) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *proxy,
                                               QString errorName, QString errorMessage)),
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

QTEST_MAIN(TestConnBasics)
#include "_gen/conn-basics.cpp.moc.hpp"
