#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingContactInfo>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestContactsInfo : public Test
{
    Q_OBJECT

public:
    TestContactsInfo(QObject *parent = 0)
        : Test(parent), mConn(0),
          mContactsInfoFieldsUpdated(0),
          mRefreshInfoFinished(0)
    { }

protected Q_SLOTS:
    void onContactInfoFieldsChanged(const Tp::Contact::InfoFields &);
    void onRefreshInfoFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testInfo();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    int mContactsInfoFieldsUpdated;
    int mRefreshInfoFinished;
};

void TestContactsInfo::onContactInfoFieldsChanged(const Tp::Contact::InfoFields &info)
{
    Q_UNUSED(info);
    mContactsInfoFieldsUpdated++;
}

void TestContactsInfo::onRefreshInfoFinished(PendingOperation *op)
{
    if (op->isError()) {
        mLoop->exit(1);
        return;
    }

    mRefreshInfoFinished++;
    mLoop->exit(0);
}

void TestContactsInfo::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-info");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestContactsInfo::init()
{
    initImpl();
    mContactsInfoFieldsUpdated = 0;
    mRefreshInfoFinished = 0;
}

void TestContactsInfo::testInfo()
{
    ContactManagerPtr contactManager = mConn->client()->contactManager();

    QVERIFY(contactManager->supportedFeatures().contains(Contact::FeatureInfo));

    QStringList validIDs = QStringList() << QLatin1String("foo")
        << QLatin1String("bar");
    QList<ContactPtr> contacts = mConn->contacts(validIDs, Contact::FeatureInfo);
    QCOMPARE(contacts.size(), validIDs.size());
    for (int i = 0; i < contacts.size(); i++) {
        ContactPtr contact = contacts[i];

        QCOMPARE(contact->requestedFeatures().contains(Contact::FeatureInfo), true);
        QCOMPARE(contact->actualFeatures().contains(Contact::FeatureInfo), true);

        QVERIFY(contact->infoFields().allFields().isEmpty());

        QVERIFY(connect(contact.data(),
                        SIGNAL(infoFieldsChanged(const Tp::Contact::InfoFields &)),
                        SLOT(onContactInfoFieldsChanged(const Tp::Contact::InfoFields &))));
    }

    GPtrArray *info_default = (GPtrArray *) dbus_g_type_specialized_construct (
              TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST);
    {
        const gchar * const field_values[2] = {
            "FooBar", NULL
        };
        g_ptr_array_add (info_default, tp_value_array_build (3,
                    G_TYPE_STRING, "n",
                    G_TYPE_STRV, NULL,
                    G_TYPE_STRV, field_values,
                    G_TYPE_INVALID));
    }
    tp_tests_contacts_connection_set_default_contact_info(TP_TESTS_CONTACTS_CONNECTION(mConn->service()),
            info_default);

    GPtrArray *info_1 = (GPtrArray *) dbus_g_type_specialized_construct (
              TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST);
    {
        const gchar * const field_values[2] = {
            "Foo", NULL
        };
        g_ptr_array_add (info_1, tp_value_array_build (3,
                    G_TYPE_STRING, "n",
                    G_TYPE_STRV, NULL,
                    G_TYPE_STRV, field_values,
                    G_TYPE_INVALID));
    }
    GPtrArray *info_2 = (GPtrArray *) dbus_g_type_specialized_construct (
              TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST);
    {
        const gchar * const field_values[2] = {
            "Bar", NULL
        };
        g_ptr_array_add (info_2, tp_value_array_build (3,
                    G_TYPE_STRING, "n",
                    G_TYPE_STRV, NULL,
                    G_TYPE_STRV, field_values,
                    G_TYPE_INVALID));
    }

    TpHandle handles[] = { 0, 0 };
    TpHandleRepoIface *serviceRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);

    for (unsigned i = 0; i < 2; i++) {
        handles[i] = tp_handle_ensure(serviceRepo, qPrintable(validIDs[i]),
                NULL, NULL);
    }

    tp_tests_contacts_connection_change_contact_info(TP_TESTS_CONTACTS_CONNECTION(mConn->service()),
            handles[0], info_1);
    tp_tests_contacts_connection_change_contact_info(TP_TESTS_CONTACTS_CONNECTION(mConn->service()),
            handles[1], info_2);

    while (mContactsInfoFieldsUpdated != 2)  {
        mLoop->processEvents();
    }

    QCOMPARE(mContactsInfoFieldsUpdated, 2);

    mContactsInfoFieldsUpdated = 0;
    ContactPtr contactFoo = contacts[0];
    ContactPtr contactBar = contacts[1];

    QCOMPARE(contactFoo->infoFields().isValid(), true);
    QCOMPARE(contactFoo->infoFields().allFields().size(), 1);
    QCOMPARE(contactFoo->infoFields().allFields()[0].fieldName, QLatin1String("n"));
    QCOMPARE(contactFoo->infoFields().allFields()[0].fieldValue[0], QLatin1String("Foo"));
    QCOMPARE(contactBar->infoFields().isValid(), true);
    QCOMPARE(contactBar->infoFields().allFields().size(), 1);
    QCOMPARE(contactBar->infoFields().allFields()[0].fieldName, QLatin1String("n"));
    QCOMPARE(contactBar->infoFields().allFields()[0].fieldValue[0], QLatin1String("Bar"));

    TpTestsContactsConnection *serviceConn = TP_TESTS_CONTACTS_CONNECTION(mConn->service());
    QCOMPARE(serviceConn->refresh_contact_info_called, static_cast<uint>(0));

    mContactsInfoFieldsUpdated = 0;
    mRefreshInfoFinished = 0;
    Q_FOREACH (const ContactPtr &contact, contacts) {
        QVERIFY(connect(contact->refreshInfo(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onRefreshInfoFinished(Tp::PendingOperation*))));
    }
    while (mRefreshInfoFinished != contacts.size()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mRefreshInfoFinished, contacts.size());

    while (mContactsInfoFieldsUpdated != contacts.size()) {
        mLoop->processEvents();
    }

    QCOMPARE(mContactsInfoFieldsUpdated, contacts.size());

    QCOMPARE(serviceConn->refresh_contact_info_called, static_cast<uint>(1));

    for (int i = 0; i < contacts.size(); i++) {
        ContactPtr contact = contacts[i];
        QVERIFY(disconnect(contact.data(),
                    SIGNAL(infoFieldsChanged(const Tp::Contact::InfoFields &)),
                    this,
                    SLOT(onContactInfoFieldsChanged(const Tp::Contact::InfoFields &))));
    }

    PendingContactInfo *pci = contactFoo->requestInfo();
    QVERIFY(connect(pci,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!pci->isFinished()) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(pci->infoFields().isValid(), true);
    QCOMPARE(pci->infoFields().allFields().size(), 1);
    QCOMPARE(pci->infoFields().allFields()[0].fieldName, QLatin1String("n"));
    QCOMPARE(pci->infoFields().allFields()[0].fieldValue[0], QLatin1String("FooBar"));

    g_boxed_free(TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST, info_default);
    g_boxed_free(TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST, info_1);
    g_boxed_free(TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST, info_2);
}

void TestContactsInfo::cleanup()
{
    cleanupImpl();
}

void TestContactsInfo::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactsInfo)
#include "_gen/contacts-info.cpp.moc.hpp"
