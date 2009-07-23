#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/contactlist/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConnRosterGroups : public Test
{
    Q_OBJECT

public:
    TestConnRosterGroups(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void onGroupAdded(const QString &group);
    void expectConnInvalidated();

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRosterGroups();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ExampleContactListConnection *mConnService;
    ConnectionPtr mConn;

    QString mGroupAdded;
};

void TestConnRosterGroups::onGroupAdded(const QString &group)
{
    mGroupAdded = group;
    mLoop->exit(0);
}

void TestConnRosterGroups::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestConnRosterGroups::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_CONTACT_LIST_CONNECTION(g_object_new(
            EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
            "account", "me@example.com",
            "protocol", "contactlist",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = name;
    mConnPath = connPath;

    g_free(name);
    g_free(connPath);
}

void TestConnRosterGroups::init()
{
    initImpl();

    mConn = Connection::create(mConnName, mConnPath);

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(mConn->status(), static_cast<uint>(Connection::StatusConnected));

}

void TestConnRosterGroups::testRosterGroups()
{
    Features features = Features() << Connection::FeatureRoster << Connection::FeatureRosterGroups;
    QVERIFY(connect(mConn->becomeReady(features),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    ContactManager *contactManager = mConn->contactManager();

    QStringList expectedGroups;
    expectedGroups << "Cambridge" << "Francophones" << "Montreal";
    expectedGroups.sort();
    QStringList groups = contactManager->allKnownGroups();
    groups.sort();
    QCOMPARE(groups, expectedGroups);

    // Cambridge
    {
        QStringList expectedContacts;
        expectedContacts << "geraldine@example.com" << "helen@example.com"
            << "guillaume@example.com" << "sjoerd@example.com";
        expectedContacts.sort();
        QStringList contacts;
        foreach (const ContactPtr &contact, contactManager->groupContacts("Cambridge")) {
            contacts << contact->id();
        }
        contacts.sort();
        QCOMPARE(contacts, expectedContacts);
    }

    // Francophones
    {
        QStringList expectedContacts;
        expectedContacts << "olivier@example.com" << "geraldine@example.com"
            << "guillaume@example.com";
        expectedContacts.sort();
        QStringList contacts;
        foreach (const ContactPtr &contact, contactManager->groupContacts("Francophones")) {
            contacts << contact->id();
        }
        contacts.sort();
        QCOMPARE(contacts, expectedContacts);
    }

    // Montreal
    {
        QStringList expectedContacts;
        expectedContacts << "olivier@example.com";
        expectedContacts.sort();
        QStringList contacts;
        foreach (const ContactPtr &contact, contactManager->groupContacts("Montreal")) {
            contacts << contact->id();
        }
        contacts.sort();
        QCOMPARE(contacts, expectedContacts);
    }

    QString group("foo");
    QVERIFY(contactManager->groupContacts(group).isEmpty());

    // add group foo
    QVERIFY(connect(contactManager,
                    SIGNAL(groupAdded(const QString&)),
                    SLOT(onGroupAdded(const QString&))));
    QVERIFY(connect(contactManager->addGroup(group),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (mGroupAdded.isEmpty()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mGroupAdded, group);

    expectedGroups << group;
    expectedGroups.sort();
    groups = contactManager->allKnownGroups();
    groups.sort();
    QCOMPARE(groups, expectedGroups);
}

void TestConnRosterGroups::cleanup()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
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

    cleanupImpl();
}

void TestConnRosterGroups::cleanupTestCase()
{
    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnRosterGroups)
#include "_gen/conn-roster-groups.cpp.moc.hpp"
