#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactCapabilities>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>

#include <telepathy-glib/telepathy-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestContactsCapabilities : public Test
{
    Q_OBJECT

public:
    TestContactsCapabilities(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectPendingContactsFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCapabilities();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
    QList<ContactPtr> mContacts;
};

void TestContactsCapabilities::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestContactsCapabilities::expectPendingContactsFinished(PendingOperation *op)
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

void TestContactsCapabilities::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-capabilities");
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

    QVERIFY(mConn->contactManager()->supportedFeatures().contains(Contact::FeatureCapabilities));
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
    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");

    gboolean supportTextChat[] = { TRUE, FALSE, FALSE };

    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService),
                TP_HANDLE_TYPE_CONTACT);
    TpHandle handles[] = { 0, 0, 0 };
    for (int i = 0; i < 3; i++) {
        handles[i] = tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(),
                NULL, NULL);
    }

    GHashTable *capabilities = createContactCapabilities(handles);
    tp_tests_contacts_connection_change_capabilities(mConnService, capabilities);

    PendingContacts *pending = mConn->contactManager()->contactsForIdentifiers(
            ids, QSet<Contact::Feature>() << Contact::FeatureCapabilities);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    for (int i = 0; i < mContacts.size(); i++) {
        ContactPtr contact = mContacts[i];

        QCOMPARE(contact->requestedFeatures(),
                 QSet<Contact::Feature>() << Contact::FeatureCapabilities);
        QCOMPARE(contact->actualFeatures(),
                 QSet<Contact::Feature>() << Contact::FeatureCapabilities);

        QVERIFY(contact->capabilities() != 0);
        QCOMPARE(contact->capabilities()->textChats(), supportTextChat[i]);
        QCOMPARE(contact->capabilities()->streamedMediaCalls(), false);
        QCOMPARE(contact->capabilities()->streamedMediaAudioCalls(), false);
        QCOMPARE(contact->capabilities()->streamedMediaVideoCalls(), false);
        QCOMPARE(contact->capabilities()->streamedMediaVideoCallsWithAudio(), false);
        QCOMPARE(contact->capabilities()->upgradingStreamedMediaCalls(), false);
        QCOMPARE(contact->capabilities()->supportsTextChats(), supportTextChat[i]);
        QCOMPARE(contact->capabilities()->supportsMediaCalls(), false);
        QCOMPARE(contact->capabilities()->supportsAudioCalls(), false);
        QCOMPARE(contact->capabilities()->supportsVideoCalls(false), false);
        QCOMPARE(contact->capabilities()->supportsVideoCalls(true), false);
        QCOMPARE(contact->capabilities()->supportsUpgradingCalls(), false);
    }
}

void TestContactsCapabilities::cleanup()
{
    cleanupImpl();
}

void TestContactsCapabilities::cleanupTestCase()
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

QTEST_MAIN(TestContactsCapabilities)
#include "_gen/contacts-capabilities.cpp.moc.hpp"
