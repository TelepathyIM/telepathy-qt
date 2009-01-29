#include <QDebug>
#include <QList>
#include <QSharedPointer>
#include <QTimer>

#include <QtDBus>
#include <QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingVoidMethodCall>
#include <TelepathyQt4/Client/ReferencedHandles>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <tests/lib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestContacts : public Test
{
    Q_OBJECT

public:
    TestContacts(QObject *parent = 0)
        : Test(parent), mConnService(0), mConn(0)
    {
    }

protected Q_SLOTS:
    void expectConnReady(uint newStatus, uint newStatusReason);
    void expectConnInvalidated();
    void expectPendingContactsFinished(Telepathy::Client::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testForHandles();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ContactsConnection *mConnService;
    Connection *mConn;
    QList<QSharedPointer<Contact> > mContacts;
};

void TestContacts::expectConnReady(uint newStatus, uint newStatusReason)
{
    switch (newStatus) {
    case Connection::StatusDisconnected:
        qWarning() << "Disconnected";
        mLoop->exit(1);
        break;
    case Connection::StatusConnecting:
        /* do nothing */
        break;
    case Connection::StatusConnected:
        qDebug() << "Ready";
        mLoop->exit(0);
        break;
    default:
        qWarning().nospace() << "What sort of status is "
            << newStatus << "?!";
        mLoop->exit(2);
        break;
    }
}

void TestContacts::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestContacts::expectPendingContactsFinished(PendingOperation *op)
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
void TestContacts::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts");
    tp_debug_set_flags("all");

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = CONTACTS_CONNECTION(g_object_new(
            CONTACTS_TYPE_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService), "simple", &name,
                &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = name;
    mConnPath = connPath;

    g_free(name);
    g_free(connPath);

    mConn = new Connection(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    QVERIFY(connect(mConn->becomeReady(),
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn,
                        SIGNAL(statusChanged(uint, uint)),
                        SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn,
                           SIGNAL(statusChanged(uint, uint)),
                           this,
                           SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mConn->status(), static_cast<uint>(Connection::StatusConnected));
    }
}

void TestContacts::init()
{
    initImpl();
}

void TestContacts::testForHandles()
{
    Telepathy::UIntList handles;
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    // Set up a few valid handles
    handles << tp_handle_ensure(serviceRepo, "alice", NULL, NULL);
    QVERIFY(handles[0] != 0);
    handles << tp_handle_ensure(serviceRepo, "bob", NULL, NULL);
    QVERIFY(handles[1] != 0);
    // Put one probably invalid one in between
    handles << 31337;
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[2], NULL));
    // Then another valid one
    handles << tp_handle_ensure(serviceRepo, "chris", NULL, NULL);
    QVERIFY(handles[3] != 0);
    // And yet another invalid one
    handles << 12345;
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[4], NULL));

    // Get contacts for the mixture of valid and invalid handles
    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles);

    // Test the closure accessors
    QCOMPARE(pending->contactManager(), mConn->contactManager());
    QCOMPARE(pending->features(), QSet<Contact::Feature>());

    QVERIFY(pending->isForHandles());
    QVERIFY(!pending->isForIdentifiers());

    QCOMPARE(pending->handles(), handles);

    // Wait for the contacts to be built
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // There should be 3 resulting contacts
    QCOMPARE(mContacts.size(), 3);

    QVERIFY(mContacts[0] != NULL);
    QVERIFY(mContacts[1] != NULL);
    QVERIFY(mContacts[2] != NULL);

    QCOMPARE(mContacts[0]->connection(), mConn);
    QCOMPARE(mContacts[1]->connection(), mConn);
    QCOMPARE(mContacts[2]->connection(), mConn);

    QCOMPARE(mContacts[0]->handle()[0], handles[0]);
    QCOMPARE(mContacts[1]->handle()[0], handles[1]);
    QCOMPARE(mContacts[2]->handle()[0], handles[3]);

    QCOMPARE(mContacts[0]->id(), QString("alice"));
    QCOMPARE(mContacts[1]->id(), QString("bob"));
    QCOMPARE(mContacts[2]->id(), QString("chris"));

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn);

    // Unref the handles we created service-side
    tp_handle_unref(serviceRepo, handles[0]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
    tp_handle_unref(serviceRepo, handles[1]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
    tp_handle_unref(serviceRepo, handles[3]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
}

void TestContacts::cleanup()
{
    cleanupImpl();
}

void TestContacts::cleanupTestCase()
{
    if (!mContacts.isEmpty()) {
        mContacts.clear();
    }

    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                        SIGNAL(invalidated(Telepathy::Client::DBusProxy *, QString, QString)),
                        SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }

        delete mConn;
        mConn = 0;
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContacts)
#include "_gen/contacts.cpp.moc.hpp"
