#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestContactsCapabilities : public Test
{
    Q_OBJECT

public:
    TestContactsCapabilities(QObject *parent = 0)
        : Test(parent), mConn(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCapabilities();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
};

void TestContactsCapabilities::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-capabilities");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestContactsCapabilities::init()
{
    initImpl();
}

static void freeRccList(GPtrArray *rccs)
{
    g_boxed_free(TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST, rccs);
}

static void addTextChatClass(GPtrArray *classes, TpHandleType handle_type)
{
    GHashTable *fixed = tp_asv_new(
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, handle_type,
        NULL);

    const gchar * const allowed[] = { NULL };
    GValueArray *arr = tp_value_array_build(2,
        TP_HASH_TYPE_STRING_VARIANT_MAP, fixed,
        G_TYPE_STRV, allowed,
        G_TYPE_INVALID);

    g_hash_table_unref(fixed);

    g_ptr_array_add(classes, arr);
}

static GHashTable *createContactCapabilities(TpHandle *handles)
{
    GHashTable *capabilities = g_hash_table_new_full(NULL, NULL, NULL,
        (GDestroyNotify) freeRccList);

    /* Support private text chats */
    GPtrArray *caps1 = g_ptr_array_sized_new(2);
    addTextChatClass(caps1, TP_HANDLE_TYPE_CONTACT);
    g_hash_table_insert(capabilities, GUINT_TO_POINTER(handles[0]), caps1);

    /* Support text chatrooms */
    GPtrArray *caps2 = g_ptr_array_sized_new(1);
    g_hash_table_insert(capabilities, GUINT_TO_POINTER(handles[1]), caps2);

    /* Don't support anything */
    GPtrArray *caps3 = g_ptr_array_sized_new(0);
    g_hash_table_insert(capabilities, GUINT_TO_POINTER(handles[2]), caps3);

    return capabilities;
}

void TestContactsCapabilities::testCapabilities()
{
    ContactManagerPtr contactManager = mConn->client()->contactManager();

    QVERIFY(contactManager->supportedFeatures().contains(Contact::FeatureCapabilities));

    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");

    bool supportTextChat[] = { true, false, false};

    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConn->service()),
                TP_HANDLE_TYPE_CONTACT);
    TpHandle handles[] = { 0, 0, 0 };
    for (int i = 0; i < 3; i++) {
        handles[i] = tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(),
                NULL, NULL);
    }

    GHashTable *capabilities = createContactCapabilities(handles);
    tp_tests_contacts_connection_change_capabilities(
            TP_TESTS_CONTACTS_CONNECTION(mConn->service()), capabilities);
    g_hash_table_destroy(capabilities);

    QList<ContactPtr> contacts = mConn->contacts(ids, Contact::FeatureCapabilities);
    QCOMPARE(contacts.size(), ids.size());
    for (int i = 0; i < contacts.size(); i++) {
        ContactPtr contact = contacts[i];

        QCOMPARE(contact->requestedFeatures().contains(Contact::FeatureCapabilities), true);
        QCOMPARE(contact->actualFeatures().contains(Contact::FeatureCapabilities), true);

        QCOMPARE(contact->capabilities().textChats(), supportTextChat[i]);
        QCOMPARE(contact->capabilities().streamedMediaCalls(), false);
        QCOMPARE(contact->capabilities().streamedMediaAudioCalls(), false);
        QCOMPARE(contact->capabilities().streamedMediaVideoCalls(), false);
        QCOMPARE(contact->capabilities().streamedMediaVideoCallsWithAudio(), false);
        QCOMPARE(contact->capabilities().upgradingStreamedMediaCalls(), false);
    }
}

void TestContactsCapabilities::cleanup()
{
    cleanupImpl();
}

void TestContactsCapabilities::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactsCapabilities)
#include "_gen/contacts-capabilities.cpp.moc.hpp"
