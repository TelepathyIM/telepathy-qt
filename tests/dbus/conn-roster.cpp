#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/contactlist2/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConnRoster : public Test
{
    Q_OBJECT

public:
    TestConnRoster(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectPendingContactsFinished(Tp::PendingOperation *);
    void expectPresencePublicationRequested(const Tp::Contacts &);
    void expectPresenceStateChanged(Tp::Contact::PresenceState);
    void expectAllKnownContactsChanged(const Tp::Contacts &added, const Tp::Contacts &removed,
            const Tp::Channel::GroupMemberChangeDetails &details);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRoster();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ExampleContactListConnection *mConnService;
    ConnectionPtr mConn;
    QList<ContactPtr> mContacts;
    int mHowManyKnownContacts;
    bool mGotPresenceStateChanged;
    bool mGotPPR;
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

void TestConnRoster::expectAllKnownContactsChanged(const Tp::Contacts& added, const Tp::Contacts& removed,
        const Tp::Channel::GroupMemberChangeDetails &details)
{
    qDebug() << added.size() << " contacts added, " << removed.size() << " contacts removed";
    mHowManyKnownContacts += added.size();
    mHowManyKnownContacts -= removed.size();

    if (details.hasMessage()) {
        QCOMPARE(details.message(), QLatin1String("add me now"));
    }

    if (mConn->contactManager()->allKnownContacts().size() != mHowManyKnownContacts) {
        qWarning() << "Contacts number mismatch! Watched value: " << mHowManyKnownContacts
                   << "allKnownContacts(): " << mConn->contactManager()->allKnownContacts().size();
        mLoop->exit(1);
    } else {
        mLoop->exit(0);
    }
}

void TestConnRoster::expectPresencePublicationRequested(const Tp::Contacts &contacts)
{
    Q_FOREACH(Tp::ContactPtr contact, contacts) {
        QCOMPARE(static_cast<uint>(contact->publishState()),
                 static_cast<uint>(Contact::PresenceStateAsk));
    }

    mGotPPR = true;
}

void TestConnRoster::expectPresenceStateChanged(Contact::PresenceState state)
{
    mGotPresenceStateChanged = true;
}

void TestConnRoster::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-roster");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_CONTACT_LIST_CONNECTION(g_object_new(
            EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
            "account", "me@example.com",
            "protocol", "contactlist",
            "simulation-delay", 1,
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);
}

void TestConnRoster::init()
{
    initImpl();

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create(Contact::FeatureAlias));

    QVERIFY(connect(mConn->lowlevel()->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(mConn->status(), ConnectionStatusConnected);
}

void TestConnRoster::testRoster()
{
    Features features = Features() << Connection::FeatureRoster;
    QVERIFY(connect(mConn->becomeReady(features),
            SIGNAL(finished(Tp::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    QStringList toCheck = QStringList() <<
        QLatin1String("sjoerd@example.com") <<
        QLatin1String("travis@example.com") <<
        QLatin1String("wim@example.com") <<
        QLatin1String("olivier@example.com") <<
        QLatin1String("helen@example.com") <<
        QLatin1String("geraldine@example.com") <<
        QLatin1String("guillaume@example.com") <<
        QLatin1String("christian@example.com");
    QStringList ids;
    QList<ContactPtr> pendingSubscription;
    QList<ContactPtr> pendingPublish;
    Q_FOREACH (const ContactPtr &contact,
            mConn->contactManager()->allKnownContacts()) {
        QVERIFY(contact->requestedFeatures().contains(Contact::FeatureAlias));
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
    ids = QStringList() << QString(QLatin1String("john@example.com"))
        << QString(QLatin1String("mary@example.com"));
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(ids),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    int i = 0;

    QVERIFY(connect(mConn->contactManager().data(),
                    SIGNAL(presencePublicationRequested(Tp::Contacts,QString)),
                    SLOT(expectPresencePublicationRequested(Tp::Contacts))));

    Q_FOREACH (const ContactPtr &contact, mContacts) {
        mGotPresenceStateChanged = false;
        mGotPPR = false;

        QVERIFY(connect(contact.data(),
                        SIGNAL(subscriptionStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails)),
                        SLOT(expectPresenceStateChanged(Tp::Contact::PresenceState))));
        QVERIFY(connect(contact.data(),
                        SIGNAL(publishStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails)),
                        SLOT(expectPresenceStateChanged(Tp::Contact::PresenceState))));
        if ((i % 2) == 0) {
            contact->requestPresenceSubscription(QLatin1String("please add me"));
        } else {
            contact->requestPresenceSubscription(QLatin1String("add me now"));
        }

        while (!mGotPresenceStateChanged && !mGotPPR) {
            mLoop->processEvents();
        }

        if ((i % 2) == 0) {
            // I asked to see his presence - he might have already accepted it, though
            QVERIFY(contact->subscriptionState() == Contact::PresenceStateAsk
                    || contact->subscriptionState() == Contact::PresenceStateYes);

            // if he accepted it already, one iteration won't be enough as the
            // first iteration will just flush the subscription -> Yes event
            while (contact->publishState() != Contact::PresenceStateAsk) {
                mLoop->processEvents();
            }

            contact->authorizePresencePublication();
            while (contact->publishState() != Contact::PresenceStateYes) {
                mLoop->processEvents();
            }
            // I authorized him to see my presence
            QCOMPARE(static_cast<uint>(contact->publishState()),
                     static_cast<uint>(Contact::PresenceStateYes));
            // He replied the presence request
            QCOMPARE(static_cast<uint>(contact->subscriptionState()),
                     static_cast<uint>(Contact::PresenceStateYes));

            contact->removePresenceSubscription();

            while (contact->subscriptionState() != Contact::PresenceStateNo) {
                mLoop->processEvents();
            }
        } else {
            // I asked to see her presence - she might have already rejected it, though
            QVERIFY(contact->subscriptionState() == Contact::PresenceStateAsk
                    || contact->subscriptionState() == Contact::PresenceStateNo);

            // If she didn't already reject it, wait until she does
            while (contact->subscriptionState() != Contact::PresenceStateNo) {
                mLoop->processEvents();
            }
        }

        ++i;

        // Disconnect the signals so the contacts doing something won't early-exit future mainloop
        // iterations (the simulation CM does things like - after a delay since we removed them, try
        // to re-add us - and such, which mess up the test if the simulated network event happens
        // before we've finished with the next contact)
        QVERIFY(contact->disconnect(this));

        // TODO: The roster API, frankly speaking, seems rather error/race prone, as evidenced by
        // this test. Should we perhaps change its semantics? Then again, this test also simulates
        // the remote user accepting/rejecting the request with a quite unpredictable timer delay,
        // while real-world applications don't do any such assumptions about the timing of the
        // remote user actions, so most of the problems won't be applicable there.
    }

    i = 0;
    Contact::PresenceState expectedPresenceState;
    Q_FOREACH (const ContactPtr &contact, pendingPublish) {
        mGotPresenceStateChanged = false;

        QVERIFY(connect(contact.data(),
                        SIGNAL(publishStateChanged(Tp::Contact::PresenceState,Tp::Channel::GroupMemberChangeDetails)),
                        SLOT(expectPresenceStateChanged(Tp::Contact::PresenceState))));

        if ((i++ % 2) == 0) {
            expectedPresenceState = Contact::PresenceStateYes;
            contact->authorizePresencePublication();
        } else {
            expectedPresenceState = Contact::PresenceStateNo;
            contact->removePresencePublication();
        }

        while (!mGotPresenceStateChanged) {
            mLoop->processEvents();
        }

        QCOMPARE(static_cast<uint>(contact->publishState()),
                 static_cast<uint>(expectedPresenceState));
    }

    // Test allKnownContactsChanged.
    // In this test, everytime a subscription is requested or rejected, allKnownContacts changes
    // Cache the current value
    mHowManyKnownContacts = mConn->contactManager()->allKnownContacts().size();
    // Watch for contacts changed
    QVERIFY(connect(mConn->contactManager().data(),
                    SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails)),
                    SLOT(expectAllKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails))));

    // Wait for the contacts to be built
    ids = QStringList() << QString(QLatin1String("kctest1@example.com"))
        << QString(QLatin1String("kctest2@example.com"));
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(ids),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    Q_FOREACH (const ContactPtr &contact, mContacts) {
        contact->requestPresenceSubscription(QLatin1String("add me now"));

        // allKnownContacts is supposed to change here.
        QCOMPARE(mLoop->exec(), 0);
    }
}

void TestConnRoster::cleanup()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
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
