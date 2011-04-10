#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/debug.h>

#include <tests/lib/test.h>

using namespace Tp;

class TestConnIntrospectCornercases : public Test
{
    Q_OBJECT

public:
    TestConnIntrospectCornercases(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();

private Q_SLOTS:
    void initTestCase();
    void init();

    void cleanup();
    void cleanupTestCase();

private:
    TpBaseConnection *mConnService;
    ConnectionPtr mConn;
    QList<ConnectionStatus> mStatuses;
};

void TestConnIntrospectCornercases::expectConnInvalidated()
{
    qDebug() << "conn invalidated";

    mLoop->exit(0);
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

    // don't create the client- or service-side connection objects here, as it's expected that many
    // different types of service connections with different initial states need to be used
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

    cleanupImpl();
}

void TestConnIntrospectCornercases::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnIntrospectCornercases)
#include "_gen/conn-introspect-cornercases.cpp.moc.hpp"
