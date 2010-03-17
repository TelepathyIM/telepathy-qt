#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactCapabilities>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>

#include <telepathy-glib/debug.h>

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
    ContactsConnection *mConnService;
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

    mConnService = CONTACTS_CONNECTION(g_object_new(
            CONTACTS_TYPE_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            0));
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

    mConn = Connection::create(mConnName, mConnPath);
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

void TestContactsCapabilities::testCapabilities()
{
    QStringList validIDs = QStringList() << QLatin1String("foo")
        << QLatin1String("bar");

    PendingContacts *pending = mConn->contactManager()->contactsForIdentifiers(
            validIDs, QSet<Contact::Feature>() << Contact::FeatureCapabilities);
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
        QCOMPARE(contact->capabilities()->supportsTextChats(), true);
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
