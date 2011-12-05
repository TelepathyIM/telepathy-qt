#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactFactory>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestContactFactory : public Test
{
    Q_OBJECT

public:
    TestContactFactory(QObject *parent = 0)
        : Test(parent), mConn(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testConnectionSelfContactFeatures();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
};

void TestContactFactory::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contact-factory");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create(Contact::FeatureAlias),
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            NULL);
    QCOMPARE(mConn->isReady(), false);
}

void TestContactFactory::init()
{
    initImpl();
}

void TestContactFactory::testConnectionSelfContactFeatures()
{
    QCOMPARE(mConn->client()->contactFactory()->features().size(), 1);
    QVERIFY(mConn->client()->contactFactory()->features().contains(Contact::FeatureAlias));

    QCOMPARE(mConn->connect(Connection::FeatureSelfContact), true);

    ContactPtr selfContact = mConn->client()->selfContact();
    QVERIFY(!selfContact.isNull());
    QVERIFY(selfContact->requestedFeatures().contains(Contact::FeatureAlias));
}

void TestContactFactory::cleanup()
{
    cleanupImpl();
}

void TestContactFactory::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactFactory)
#include "_gen/contact-factory.cpp.moc.hpp"
