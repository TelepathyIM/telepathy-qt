#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactLocation>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>

#include <telepathy-glib/telepathy-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestContactsLocation : public Test
{
    Q_OBJECT

public:
    TestContactsLocation(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectPendingContactsFinished(Tp::PendingOperation *);
    void onContactLocationUpdated(Tp::ContactLocation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testLocation();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
    QList<ContactPtr> mContacts;
    int mContactsLocationUpdated;
};

void TestContactsLocation::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestContactsLocation::expectPendingContactsFinished(PendingOperation *op)
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
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);
    mContacts = pending->contacts();

    mLoop->exit(0);
}

void TestContactsLocation::onContactLocationUpdated(Tp::ContactLocation *location)
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

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "foo", &name, &connPath, &error));
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

    QCOMPARE(mConn->status(), Connection::StatusConnected);

    QVERIFY(mConn->contactManager()->supportedFeatures().contains(Contact::FeatureLocation));
}

void TestContactsLocation::init()
{
    initImpl();
    mContactsLocationUpdated = 0;
}

void TestContactsLocation::testLocation()
{
    QStringList validIDs = QStringList() << QLatin1String("foo")
        << QLatin1String("bar");

    PendingContacts *pending = mConn->contactManager()->contactsForIdentifiers(
            validIDs, QSet<Contact::Feature>() << Contact::FeatureLocation);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    for (int i = 0; i < mContacts.size(); i++) {
        ContactPtr contact = mContacts[i];

        QCOMPARE(contact->requestedFeatures(),
                 QSet<Contact::Feature>() << Contact::FeatureLocation);
        QCOMPARE(contact->actualFeatures(),
                 QSet<Contact::Feature>() << Contact::FeatureLocation);

        QVERIFY(contact->location() != 0);

        connect(contact.data(),
                SIGNAL(locationUpdated(Tp::ContactLocation *)),
                SLOT(onContactLocationUpdated(Tp::ContactLocation *)));
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
            (TpBaseConnection *) mConnService, TP_HANDLE_TYPE_CONTACT);

    for (unsigned i = 0; i < 2; i++) {
        handles[i] = tp_handle_ensure(serviceRepo, qPrintable(validIDs[i]),
                NULL, NULL);
    }

    tp_tests_contacts_connection_change_locations(mConnService, 2, handles, locations);

    while (mContactsLocationUpdated != 2)  {
        QCOMPARE(mLoop->exec(), 0);
    }

    for (int i = 0; i < mContacts.size(); i++) {
        ContactPtr contact = mContacts[i];

        QCOMPARE(contact->location()->country(),
                 QLatin1String(tp_asv_get_string(locations[i], "country")));
        QCOMPARE(contact->location()->latitude(),
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
    if (mConn) {
        // Disconnect and wait for invalidated
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

QTEST_MAIN(TestContactsLocation)
#include "_gen/contacts-location.cpp.moc.hpp"
