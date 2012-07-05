#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingStringList>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestContactsClientTypes : public Test
{
    Q_OBJECT

public:
    TestContactsClientTypes(QObject *parent = 0)
        : Test(parent), mConn(0),
          mClientTypesUpdated(0)
    { }

protected Q_SLOTS:
    void onClientTypesChanged(const QStringList &clientTypes);
    void onRequestClientTypesFinished(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testClientTypes();
    void testClientTypesAttributes();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    int mClientTypesUpdated;
    QStringList mClientTypes;
};

void TestContactsClientTypes::onClientTypesChanged(const QStringList &clientTypes)
{
    mClientTypesUpdated++;
    mClientTypes = clientTypes;
}

void TestContactsClientTypes::onRequestClientTypesFinished(PendingOperation *op)
{
    if (op->isError()) {
        mLastError = op->errorName();
        mLastErrorMessage = op->errorMessage();
        mLoop->exit(1);
    } else {
        PendingStringList *psl = qobject_cast<PendingStringList*>(op);
        mClientTypes = psl->result();
        mLoop->exit(0);
    }
}

void TestContactsClientTypes::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-client-types");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestContactsClientTypes::init()
{
    initImpl();
    mClientTypesUpdated = 0;
}

void TestContactsClientTypes::testClientTypes()
{
    ContactManagerPtr contactManager = mConn->client()->contactManager();

    QVERIFY(contactManager->supportedFeatures().contains(Contact::FeatureClientTypes));

    QStringList validIDs = QStringList() << QLatin1String("foo")
        << QLatin1String("bar");
    QList<ContactPtr> contacts = mConn->contacts(validIDs, Contact::FeatureClientTypes);
    QCOMPARE(contacts.size(), validIDs.size());
    for (int i = 0; i < contacts.size(); i++) {
        ContactPtr contact = contacts[i];

        QCOMPARE(contact->requestedFeatures().contains(Contact::FeatureClientTypes), true);
        QCOMPARE(contact->actualFeatures().contains(Contact::FeatureClientTypes), true);

        QVERIFY(contact->clientTypes().isEmpty());

        QVERIFY(connect(contact.data(),
                        SIGNAL(clientTypesChanged(QStringList)),
                        SLOT(onClientTypesChanged(QStringList))));
    }

    ContactPtr contactFoo = contacts[0];
    ContactPtr contactBar = contacts[1];

    const gchar *clientTypes1[] = { "phone", "pc", NULL };
    const gchar *clientTypes2[] = { "web", NULL };

    tp_tests_contacts_connection_change_client_types(TP_TESTS_CONTACTS_CONNECTION(mConn->service()),
            contactFoo->handle()[0], g_strdupv((gchar**) clientTypes1));

    while (mClientTypesUpdated != 1)  {
        mLoop->processEvents();
    }
    QCOMPARE(mClientTypesUpdated, 1);
    QCOMPARE(mClientTypes, QStringList() << QLatin1String("phone") << QLatin1String("pc"));
    QCOMPARE(contactFoo->clientTypes(), QStringList() << QLatin1String("phone")
                                                      << QLatin1String("pc"));
    mClientTypes.clear();

    tp_tests_contacts_connection_change_client_types(TP_TESTS_CONTACTS_CONNECTION(mConn->service()),
            contactBar->handle()[0], g_strdupv((gchar**) clientTypes2));

    while (mClientTypesUpdated != 2)  {
        mLoop->processEvents();
    }
    QCOMPARE(mClientTypesUpdated, 2);
    QCOMPARE(mClientTypes, QStringList() << QLatin1String("web"));
    QCOMPARE(contactBar->clientTypes(), QStringList() << QLatin1String("web"));

    mClientTypesUpdated = 0;
    mClientTypes.clear();

    connect(contactFoo->requestClientTypes(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onRequestClientTypesFinished(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mClientTypes, QStringList() << QLatin1String("phone") << QLatin1String("pc"));
    mClientTypes.clear();

    connect(contactBar->requestClientTypes(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onRequestClientTypesFinished(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mClientTypes, QStringList() << QLatin1String("web"));
}

void TestContactsClientTypes::testClientTypesAttributes()
{
    ContactManagerPtr contactManager = mConn->client()->contactManager();

    QVERIFY(contactManager->supportedFeatures().contains(Contact::FeatureClientTypes));

    const gchar *clientTypes[] = { "pc", "phone", NULL };
    tp_tests_contacts_connection_change_client_types(TP_TESTS_CONTACTS_CONNECTION(mConn->service()),
            2, g_strdupv((gchar**) clientTypes));

    QStringList validIDs = QStringList() << QLatin1String("foo");
    QList<ContactPtr> contacts = mConn->contacts(validIDs, Contact::FeatureClientTypes);
    QCOMPARE(contacts.size(), 1);

    ContactPtr contact = contacts[0];
    QCOMPARE(contact->handle()[0], uint(2));
    QCOMPARE(contact->requestedFeatures().contains(Contact::FeatureClientTypes), true);
    QCOMPARE(contact->actualFeatures().contains(Contact::FeatureClientTypes), true);
    QCOMPARE(contact->clientTypes(), QStringList() << QLatin1String("pc") << QLatin1String("phone"));
}

void TestContactsClientTypes::cleanup()
{
    cleanupImpl();
}

void TestContactsClientTypes::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactsClientTypes)
#include "_gen/contacts-client-types.cpp.moc.hpp"
