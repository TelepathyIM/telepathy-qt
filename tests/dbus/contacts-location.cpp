#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/LocationInfo>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestContactsLocation : public Test
{
    Q_OBJECT

public:
    TestContactsLocation(QObject *parent = 0)
        : Test(parent), mConn(0), mContactsLocationUpdated(0)
    { }

protected Q_SLOTS:
    void onLocationInfoUpdated(const Tp::LocationInfo &location);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testLocation();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    int mContactsLocationUpdated;
};

void TestContactsLocation::onLocationInfoUpdated(const Tp::LocationInfo &location)
{
    Q_UNUSED(location);
    mContactsLocationUpdated++;
    mLoop->exit(0);
}

void TestContactsLocation::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-location");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestContactsLocation::init()
{
    initImpl();
    mContactsLocationUpdated = 0;
}

void TestContactsLocation::testLocation()
{
    ContactManagerPtr contactManager = mConn->client()->contactManager();

    QVERIFY(contactManager->supportedFeatures().contains(Contact::FeatureLocation));

    QStringList validIDs = QStringList() << QLatin1String("foo")
        << QLatin1String("bar");
    QList<ContactPtr> contacts = mConn->contacts(validIDs, Contact::FeatureLocation);
    QCOMPARE(contacts.size(), validIDs.size());
    for (int i = 0; i < contacts.size(); i++) {
        ContactPtr contact = contacts[i];

        QCOMPARE(contact->requestedFeatures().contains(Contact::FeatureLocation), true);
        QCOMPARE(contact->actualFeatures().contains(Contact::FeatureLocation), true);

        QVERIFY(connect(contact.data(),
                        SIGNAL(locationUpdated(const Tp::LocationInfo &)),
                        SLOT(onLocationInfoUpdated(const Tp::LocationInfo &))));
    }

    GHashTable *location_1 = tp_asv_new(
        "country",  G_TYPE_STRING, "United-kingdoms",
        "lat",  G_TYPE_DOUBLE, 20.0,
        NULL);
    GHashTable *location_2 = tp_asv_new(
        "country",  G_TYPE_STRING, "Atlantis",
        "lat",  G_TYPE_DOUBLE, 10.0,
        NULL);
    GHashTable *locations[] = { location_1, location_2 };

    TpHandle handles[] = { 0, 0 };
    TpHandleRepoIface *serviceRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);

    for (unsigned i = 0; i < 2; i++) {
        handles[i] = tp_handle_ensure(serviceRepo, qPrintable(validIDs[i]),
                NULL, NULL);
    }

    tp_tests_contacts_connection_change_locations(TP_TESTS_CONTACTS_CONNECTION(mConn->service()), 2,
            handles, locations);

    while (mContactsLocationUpdated != 2)  {
        QCOMPARE(mLoop->exec(), 0);
    }

    for (int i = 0; i < contacts.size(); i++) {
        ContactPtr contact = contacts[i];

        QCOMPARE(contact->location().country(),
                 QLatin1String(tp_asv_get_string(locations[i], "country")));
        QCOMPARE(contact->location().latitude(),
                 tp_asv_get_double(locations[i], "lat", NULL));
    }

    g_hash_table_unref(location_1);
    g_hash_table_unref(location_2);
}

void TestContactsLocation::cleanup()
{
    cleanupImpl();
}

void TestContactsLocation::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactsLocation)
#include "_gen/contacts-location.cpp.moc.hpp"
