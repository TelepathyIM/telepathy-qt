#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contactlist/conn.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestConnRosterLegacy : public Test
{
    Q_OBJECT

public:
    TestConnRosterLegacy(QObject *parent = 0)
        : Test(parent), mConn(0),
          mBlockingContactsFinished(false), mHowManyKnownContacts(0),
          mGotPresenceStateChanged(false)
    { }

protected Q_SLOTS:
    void expectBlockingContactsFinished(Tp::PendingOperation *op);
    void expectBlockStatusChanged(bool blocked);
    void expectBlockedContactsChanged(const Tp::Contacts &added, const Tp::Contacts &removed,
            const Tp::Channel::GroupMemberChangeDetails &details);
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
    TestConnHelper *mConn;
    QSet<QString> mContactsExpectingBlockStatusChange;
    bool mBlockingContactsFinished;
    int mHowManyKnownContacts;
    bool mGotPresenceStateChanged;
};

void TestConnRosterLegacy::expectBlockingContactsFinished(Tp::PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    qDebug() << "blocking contacts finished";
    mBlockingContactsFinished = true;

    if (mContactsExpectingBlockStatusChange.isEmpty()) {
        mLoop->exit(0);
    }
}

void TestConnRosterLegacy::expectBlockStatusChanged(bool blocked)
{
    Q_UNUSED(blocked);

    Contact *c = qobject_cast<Contact*>(sender());
    QVERIFY(c);

    ContactPtr contact(c);
    mContactsExpectingBlockStatusChange.remove(contact->id());

    if (mContactsExpectingBlockStatusChange.isEmpty() && mBlockingContactsFinished) {
        mLoop->exit(0);
    }
}

// This connects to allKnownContactsChanged() but it is only used in the last contact blocking test
void TestConnRosterLegacy::expectBlockedContactsChanged(const Tp::Contacts &added,
        const Tp::Contacts &removed, const Tp::Channel::GroupMemberChangeDetails &details)
{
    Q_UNUSED(details);

    Q_FOREACH(const ContactPtr &contact, added) {
        mContactsExpectingBlockStatusChange.remove(contact->id());
    }
    Q_FOREACH(const ContactPtr &contact, removed) {
        mContactsExpectingBlockStatusChange.remove(contact->id());
    }

    if (mContactsExpectingBlockStatusChange.isEmpty() && mBlockingContactsFinished) {
        mLoop->exit(0);
    }
}

void TestConnRosterLegacy::expectAllKnownContactsChanged(const Tp::Contacts& added, const Tp::Contacts& removed,
        const Tp::Channel::GroupMemberChangeDetails &details)
{
    qDebug() << added.size() << " contacts added, " << removed.size() << " contacts removed";
    mHowManyKnownContacts += added.size();
    mHowManyKnownContacts -= removed.size();

    if (details.hasMessage()) {
        QCOMPARE(details.message(), QLatin1String("add me now"));
    }

    if (mConn->client()->contactManager()->allKnownContacts().size() != mHowManyKnownContacts) {
        qWarning() << "Contacts number mismatch! Watched value: " << mHowManyKnownContacts <<
            "allKnownContacts(): " << mConn->client()->contactManager()->allKnownContacts().size();
        mLoop->exit(1);
    } else {
        mLoop->exit(0);
    }
}

void TestConnRosterLegacy::expectPresenceStateChanged(Contact::PresenceState state)
{
    mGotPresenceStateChanged = true;
}

void TestConnRosterLegacy::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-roster-legacy");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
            "account", "me@example.com",
            "protocol", "contactlist",
            "simulation-delay", 1,
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestConnRosterLegacy::init()
{
    initImpl();
}

void TestConnRosterLegacy::testRoster()
{
    Features features = Features() << Connection::FeatureRoster;
    QCOMPARE(mConn->enableFeatures(features), true);

    ContactManagerPtr contactManager = mConn->client()->contactManager();

    QCOMPARE(contactManager->state(), ContactListStateSuccess);

    QStringList toCheck = QStringList() <<
        QLatin1String("sjoerd@example.com") <<
        QLatin1String("travis@example.com") <<
        QLatin1String("wim@example.com") <<
        QLatin1String("olivier@example.com") <<
        QLatin1String("helen@example.com") <<
        QLatin1String("geraldine@example.com") <<
        QLatin1String("guillaume@example.com") <<
        QLatin1String("christian@example.com") <<
        QLatin1String("bill@example.com") <<
        QLatin1String("steve@example.com");
    QStringList ids;
    QList<ContactPtr> pendingSubscription;
    QList<ContactPtr> pendingPublish;
    Q_FOREACH (const ContactPtr &contact, contactManager->allKnownContacts()) {
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
    QList<ContactPtr> contacts = mConn->contacts(ids);
    QCOMPARE(contacts.size(), ids.size());

    int i = 0;
    Q_FOREACH (const ContactPtr &contact, contacts) {
        mGotPresenceStateChanged = false;

        QVERIFY(connect(contact.data(),
                        SIGNAL(subscriptionStateChanged(Tp::Contact::PresenceState)),
                        SLOT(expectPresenceStateChanged(Tp::Contact::PresenceState))));
        QVERIFY(connect(contact.data(),
                        SIGNAL(publishStateChanged(Tp::Contact::PresenceState, QString)),
                        SLOT(expectPresenceStateChanged(Tp::Contact::PresenceState))));
        if ((i % 2) == 0) {
            contact->requestPresenceSubscription(QLatin1String("please add me"));
        } else {
            contact->requestPresenceSubscription(QLatin1String("add me now"));
        }

        while (!mGotPresenceStateChanged) {
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
                        SIGNAL(publishStateChanged(Tp::Contact::PresenceState, QString)),
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
    mHowManyKnownContacts = contactManager->allKnownContacts().size();
    // Watch for contacts changed
    QVERIFY(connect(contactManager.data(),
                    SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails)),
                    SLOT(expectAllKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails))));

    // Wait for the contacts to be built
    ids = QStringList() << QString(QLatin1String("kctest1@example.com"))
        << QString(QLatin1String("kctest2@example.com"));
    contacts = mConn->contacts(ids);
    QCOMPARE(contacts.size(), ids.size());
    Q_FOREACH (const ContactPtr &contact, contacts) {
        contact->requestPresenceSubscription(QLatin1String("add me now"));

        // allKnownContacts is supposed to change here.
        QCOMPARE(mLoop->exec(), 0);
    }


    QVERIFY(disconnect(contactManager.data(),
                       SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                              Tp::Channel::GroupMemberChangeDetails)),
                       this,
                       SLOT(expectAllKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails))));

    // verify that the CM supports contact blocking
    QVERIFY(contactManager->canBlockContacts());

    // check if the initially blocked contacts are there
    ids.clear();
    toCheck = QStringList() <<
        QLatin1String("bill@example.com") <<
        QLatin1String("steve@example.com");
    Q_FOREACH (const ContactPtr &contact, contactManager->allKnownContacts()) {
        if (contact->isBlocked()) {
            qDebug() << "blocked contact:" << contact->id();
            ids << contact->id();
        }
    }
    ids.sort();
    toCheck.sort();
    QCOMPARE(ids, toCheck);

    // block all contacts
    QList<ContactPtr> contactsList = contactManager->allKnownContacts().toList();
    QSet<QString> contactIdsList;
    Q_FOREACH (const ContactPtr &contact, contactsList) {
        QVERIFY(connect(contact.data(),
                        SIGNAL(blockStatusChanged(bool)),
                        SLOT(expectBlockStatusChanged(bool))));
        contactIdsList.insert(contact->id());
    }

    mBlockingContactsFinished = false;
    mContactsExpectingBlockStatusChange = contactIdsList;

    // those are already blocked; do not expect their status to change
    mContactsExpectingBlockStatusChange.remove(QLatin1String("bill@example.com"));
    mContactsExpectingBlockStatusChange.remove(QLatin1String("steve@example.com"));

    QVERIFY(connect(contactManager->blockContacts(contactsList),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectBlockingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // verify all contacts have been blocked
    Q_FOREACH (const ContactPtr &contact, contactsList) {
        QCOMPARE(contact->isBlocked(), true);
        QVERIFY(contactManager->allKnownContacts().contains(contact));
    }

    // now remove kctest1 from the server
    ContactPtr kctest1;
    Q_FOREACH (const ContactPtr &contact, contactsList) {
        if (contact->id() == QLatin1String("kctest1@example.com")) {
            kctest1 = contact;
        }
    }
    QVERIFY(!kctest1.isNull());
    QVERIFY(connect(contactManager->removeContacts(QList<ContactPtr>() << kctest1),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    mLoop, SLOT(quit())));
    QCOMPARE(mLoop->exec(), 0);

    // allKnownContacts must still contain kctest1, since it is in the deny list
    QVERIFY(contactManager->allKnownContacts().contains(kctest1));
    kctest1.reset(); //no longer needed

    // unblock all contacts
    mBlockingContactsFinished = false;
    mContactsExpectingBlockStatusChange = contactIdsList;

    QVERIFY(connect(contactManager->unblockContacts(contactsList),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectBlockingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // verify all contacts have been unblocked
    Q_FOREACH (const ContactPtr &contact, contactsList) {
        QCOMPARE(contact->isBlocked(), false);

        // ...and that bill, steve and kctest1 have also been removed from allKnownContacts()
        // note: allKnownContacts() changes here because bill, steve and kctest,
        // which were only in the deny list, do not exist in any other list, so
        // they are removed as soon as they get unblocked.
        if (contact->id() == QLatin1String("bill@example.com") ||
            contact->id() == QLatin1String("steve@example.com") ||
            contact->id() == QLatin1String("kctest1@example.com")) {
            QVERIFY(!contactManager->allKnownContacts().contains(contact));
        } else {
            QVERIFY(contactManager->allKnownContacts().contains(contact));
        }
    }

    // block some contacts that are not already known
    ids = QStringList() <<
        QLatin1String("blocktest1@example.com") <<
        QLatin1String("blocktest2@example.com");
    contacts = mConn->contacts(ids);

    // Watch changes in allKnownContacts() instead of watching the Contacts' block status
    // as we want to destroy the Contact objects and verify that they are being re-created correctly
    QVERIFY(connect(contactManager.data(),
                    SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails)),
                    SLOT(expectBlockedContactsChanged(Tp::Contacts,Tp::Contacts,
                            Tp::Channel::GroupMemberChangeDetails))));

    mBlockingContactsFinished = false;
    mContactsExpectingBlockStatusChange = ids.toSet();

    QVERIFY(connect(contactManager->blockContacts(contacts),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectBlockingContactsFinished(Tp::PendingOperation*))));

    // destroy the Contact objects to let them be re-created when the block operation finishes
    contacts.clear();
    QCOMPARE(mLoop->exec(), 0);

    // construct the same contacts again and verify that they are blocked
    contacts = mConn->contacts(ids);
    Q_FOREACH (const ContactPtr &contact, contacts) {
        QCOMPARE(contact->isBlocked(), true);
        QVERIFY(contactManager->allKnownContacts().contains(contact));
    }

    // now unblock them again
    mBlockingContactsFinished = false;
    mContactsExpectingBlockStatusChange = ids.toSet();

    QVERIFY(connect(contactManager->unblockContacts(contacts),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectBlockingContactsFinished(Tp::PendingOperation*))));

    // note: allKnownContacts() is expected to change again, so we expect
    // to quit from expectBlockedContactsChanged()
    QCOMPARE(mLoop->exec(), 0);

    // and verify that they are not in allKnownContacts()
    Q_FOREACH (const ContactPtr &contact, contacts) {
        QCOMPARE(contact->isBlocked(), false);
        QVERIFY(!contactManager->allKnownContacts().contains(contact));
    }
}

void TestConnRosterLegacy::cleanup()
{
    cleanupImpl();
}

void TestConnRosterLegacy::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnRosterLegacy)
#include "_gen/conn-roster-legacy.cpp.moc.hpp"
