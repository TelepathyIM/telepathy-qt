#include <QtCore/QDebug>
#include <QtTest/QtTest>

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;
using namespace Tp::Client;

class TestContactFactory : public Test
{
    Q_OBJECT

public:
    TestContactFactory(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testConnectionSelfContactFeatures();

    void cleanup();
    void cleanupTestCase();

private:
    TpTestsContactsConnection *mConnService;
    QString mConnName, mConnPath;
    ConnectionPtr mConn;
};

void TestContactFactory::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contact-factory");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me1@example.com",
            "protocol", "simple",
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
}

void TestContactFactory::init()
{
    initImpl();
}

void TestContactFactory::testConnectionSelfContactFeatures()
{
    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create(Contact::FeatureAlias));
    QCOMPARE(mConn->contactFactory()->features().size(), 1);
    QVERIFY(mConn->contactFactory()->features().contains(Contact::FeatureAlias));

    QVERIFY(connect(mConn->lowlevel()->requestConnect(Connection::FeatureSelfContact),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(Connection::FeatureSelfContact), true);
    QCOMPARE(mConn->status(), ConnectionStatusConnected);

    ContactPtr selfContact = mConn->selfContact();
    QVERIFY(!selfContact.isNull());
    QVERIFY(selfContact->requestedFeatures().contains(Contact::FeatureAlias));
}

void TestContactFactory::cleanup()
{
    cleanupImpl();
}

void TestContactFactory::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
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

QTEST_MAIN(TestContactFactory)
#include "_gen/contact-factory.cpp.moc.hpp"
