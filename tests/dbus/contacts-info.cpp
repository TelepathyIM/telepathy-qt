#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingContactInfo>
#include <TelepathyQt4/PendingReady>

#include <telepathy-glib/telepathy-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestContactsInfo : public Test
{
    Q_OBJECT

public:
    TestContactsInfo(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectPendingContactsFinished(Tp::PendingOperation *);
    void onContactInfoChanged(const Tp::ContactInfoFieldList &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testInfo();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
    QList<ContactPtr> mContacts;
    int mContactsInfoUpdated;
};

void TestContactsInfo::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestContactsInfo::expectPendingContactsFinished(PendingOperation *op)
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

void TestContactsInfo::onContactInfoChanged(const Tp::ContactInfoFieldList &info)
{
    Q_UNUSED(info);
    mContactsInfoUpdated++;
    mLoop->exit(0);
}

void TestContactsInfo::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-info");
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

    QVERIFY(mConn->contactManager()->supportedFeatures().contains(Contact::FeatureInfo));
}

void TestContactsInfo::init()
{
    initImpl();
    mContactsInfoUpdated = 0;
}

void TestContactsInfo::testInfo()
{
    QStringList validIDs = QStringList() << QLatin1String("foo")
        << QLatin1String("bar");

    PendingContacts *pending = mConn->contactManager()->contactsForIdentifiers(
            validIDs, QSet<Contact::Feature>() << Contact::FeatureInfo);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    for (int i = 0; i < mContacts.size(); i++) {
        ContactPtr contact = mContacts[i];

        QCOMPARE(contact->requestedFeatures(),
                 QSet<Contact::Feature>() << Contact::FeatureInfo);
        QCOMPARE(contact->actualFeatures(),
                 QSet<Contact::Feature>() << Contact::FeatureInfo);

        QVERIFY(contact->info().isEmpty());

        connect(contact.data(),
                SIGNAL(infoChanged(const Tp::ContactInfoFieldList &)),
                SLOT(onContactInfoChanged(const Tp::ContactInfoFieldList &)));
    }

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
            (TpBaseConnection *) mConnService, TP_HANDLE_TYPE_CONTACT);

    for (unsigned i = 0; i < 2; i++) {
        handles[i] = tp_handle_ensure(serviceRepo, qPrintable(validIDs[i]),
                NULL, NULL);
    }

    tp_tests_contacts_connection_change_contact_info(mConnService, handles[0], info_1);
    tp_tests_contacts_connection_change_contact_info(mConnService, handles[1], info_2);

    while (mContactsInfoUpdated != 2)  {
        QCOMPARE(mLoop->exec(), 0);
    }

    mContactsInfoUpdated = 0;
    ContactPtr contactFoo = mContacts[0];
    ContactPtr contactBar = mContacts[1];

    QCOMPARE(contactFoo->info().size(), 1);
    QCOMPARE(contactFoo->info()[0].fieldName, QLatin1String("n"));
    QCOMPARE(contactFoo->info()[0].fieldValue[0], QLatin1String("Foo"));
    QCOMPARE(contactBar->info().size(), 1);
    QCOMPARE(contactBar->info()[0].fieldName, QLatin1String("n"));
    QCOMPARE(contactBar->info()[0].fieldValue[0], QLatin1String("Bar"));

    Q_FOREACH (const ContactPtr &contact, mContacts) {
        PendingOperation *op = contact->refreshInfo();
        QVERIFY(connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);
    }

    /* nothing changed */
    QCOMPARE(mContactsInfoUpdated, 0);

    for (int i = 0; i < mContacts.size(); i++) {
        ContactPtr contact = mContacts[i];
        disconnect(contact.data(),
                SIGNAL(infoChanged(const Tp::ContactInfoFieldList &)),
                this,
                SLOT(onContactInfoChanged(const Tp::ContactInfoFieldList &)));
    }

    PendingContactInfo *pci = contactFoo->requestInfo();
    QVERIFY(connect(pci,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!pci->isFinished()) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(pci->info().size(), 1);
    QCOMPARE(pci->info()[0].fieldName, QLatin1String("n"));
    QCOMPARE(pci->info()[0].fieldValue[0], QLatin1String("Foo"));

    g_boxed_free (TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST, info_1);
    g_boxed_free (TP_ARRAY_TYPE_CONTACT_INFO_FIELD_LIST, info_2);
}

void TestContactsInfo::cleanup()
{
    cleanupImpl();
}

void TestContactsInfo::cleanupTestCase()
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

QTEST_MAIN(TestContactsInfo)
#include "_gen/contacts-info.cpp.moc.hpp"
