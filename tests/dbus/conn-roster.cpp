#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/contactlist/conn.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestConnRoster : public Test
{
    Q_OBJECT

public:
    TestConnRoster(QObject *parent = 0)
        : Test(parent), mConnService(0), mConn(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectPendingContactsFinished(Telepathy::Client::PendingOperation *);
    void expectPresenceStateChanged(Telepathy::Client::Contact::PresenceState);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRoster();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ExampleContactListConnection *mConnService;
    Connection *mConn;
    QList<QSharedPointer<Contact> > mContacts;
};

void TestConnRoster::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestConnRoster::expectPendingContactsFinished(PendingOperation *op)
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

void TestConnRoster::expectPresenceStateChanged(Contact::PresenceState state)
{
    mLoop->exit(0);
}

void TestConnRoster::initTestCase()
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

void TestConnRoster::init()
{
    initImpl();

    mConn = new Connection(mConnName, mConnPath);

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(mConn->status(), static_cast<uint>(Connection::StatusConnected));
}

void TestConnRoster::testRoster()
{
    QSet<uint> features = QSet<uint>() << Connection::FeatureRoster;
    QVERIFY(connect(mConn->becomeReady(features),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    QStringList toCheck = QStringList() <<
        "sjoerd@example.com" <<
        "travis@example.com" <<
        "wim@example.com" <<
        "olivier@example.com" <<
        "helen@example.com" <<
        "geraldine@example.com" <<
        "guillaume@example.com" <<
        "christian@example.com";
    QStringList ids;
    QList<QSharedPointer<Contact> > pendingSubscription;
    QList<QSharedPointer<Contact> > pendingPublish;
    foreach (const QSharedPointer<Contact> &contact,
            mConn->contactManager()->allKnownContacts()) {
        qDebug() << " contact:" << contact->id() <<
            "- subscription:" << contact->subscriptionState() <<
            "- publish:" << contact->publishState();
        ids << contact->id();
        if (contact->subscriptionState() == Contact::PresenceStateAsk) {
            pendingSubscription.append(contact);
        } else if (contact->publishState() == Contact::PresenceStateAsk) {
            pendingPublish.append(contact);
        }
    }
    ids.sort();
    toCheck.sort();
    QCOMPARE(ids, toCheck);
    QCOMPARE(pendingSubscription.size(), 2);
    QCOMPARE(pendingPublish.size(), 2);

    // Wait for the contacts to be built
    ids = QStringList() << QString("john@example.com") << QString("mary@example.com");
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(ids),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    int i = 0;
    foreach (const QSharedPointer<Contact> &contact, mContacts) {
        QVERIFY(connect(contact.data(),
                        SIGNAL(subscriptionStateChanged(Telepathy::Client::Contact::PresenceState)),
                        SLOT(expectPresenceStateChanged(Telepathy::Client::Contact::PresenceState))));
        QVERIFY(connect(contact.data(),
                        SIGNAL(publishStateChanged(Telepathy::Client::Contact::PresenceState)),
                        SLOT(expectPresenceStateChanged(Telepathy::Client::Contact::PresenceState))));
        if ((i % 2) == 0) {
            contact->requestPresenceSubscription("please add me");
        } else {
            contact->requestPresenceSubscription("add me now");
        }

        QCOMPARE(mLoop->exec(), 0);
        // I asked to see his presence
        QCOMPARE(static_cast<uint>(contact->subscriptionState()),
                 static_cast<uint>(Contact::PresenceStateAsk));

        if ((i % 2) == 0) {
            QCOMPARE(mLoop->exec(), 0);
            // He asked to see my presence
            QCOMPARE(static_cast<uint>(contact->publishState()),
                     static_cast<uint>(Contact::PresenceStateAsk));

            contact->authorizePresencePublication();
            QCOMPARE(mLoop->exec(), 0);
            // I authorized him to see my presence
            QCOMPARE(static_cast<uint>(contact->publishState()),
                     static_cast<uint>(Contact::PresenceStateYes));
            // He replied the presence request
            QCOMPARE(static_cast<uint>(contact->subscriptionState()),
                     static_cast<uint>(Contact::PresenceStateYes));

            contact->removePresenceSubscription();
            QCOMPARE(mLoop->exec(), 0);
            QCOMPARE(static_cast<uint>(contact->subscriptionState()),
                     static_cast<uint>(Contact::PresenceStateNo));
        } else {
            QCOMPARE(mLoop->exec(), 0);
            // He replied the presence request
            QCOMPARE(static_cast<uint>(contact->subscriptionState()),
                     static_cast<uint>(Contact::PresenceStateNo));
        }

        ++i;
    }

    i = 0;
    Contact::PresenceState expectedPresenceState;
    foreach (const QSharedPointer<Contact> &contact, pendingPublish) {
        QVERIFY(connect(contact.data(),
                        SIGNAL(publishStateChanged(Telepathy::Client::Contact::PresenceState)),
                        SLOT(expectPresenceStateChanged(Telepathy::Client::Contact::PresenceState))));

        if ((i % 2) == 0) {
            expectedPresenceState = Contact::PresenceStateYes;
            contact->authorizePresencePublication();
        } else {
            expectedPresenceState = Contact::PresenceStateNo;
            contact->removePresencePublication();
        }

        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(static_cast<uint>(contact->publishState()),
                 static_cast<uint>(expectedPresenceState));

        ++i;
    }
}

void TestConnRoster::cleanup()
{
    if (mConn != 0) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *,
                                               const QString &, const QString &)),
                            SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }

        delete mConn;
        mConn = 0;
    }

    cleanupImpl();
}

void TestConnRoster::cleanupTestCase()
{
    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnRoster)
#include "_gen/conn-roster.cpp.moc.hpp"
