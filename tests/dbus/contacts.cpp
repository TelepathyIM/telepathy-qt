#include <QDebug>
#include <QList>
#include <QTimer>

#include <QtDBus>
#include <QtTest>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Presence>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestContacts : public Test
{
    Q_OBJECT

public:
    TestContacts(QObject *parent = 0)
        : Test(parent), mConnService(0)
    {
    }

protected Q_SLOTS:
    void expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason);
    void expectConnInvalidated();
    void expectPendingContactsFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testSupport();
    void testSelfContact();
    void testForHandles();
    void testForIdentifiers();
    void testFeatures();
    void testFeaturesNotRequested();
    void testUpgrade();
    void testSelfContactFallback();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
    QList<ContactPtr> mContacts;
    Tp::UIntList mInvalidHandles;
};

void TestContacts::expectConnReady(Tp::ConnectionStatus newStatus,
        Tp::ConnectionStatusReason newStatusReason)
{
    switch (newStatus) {
    case ConnectionStatusDisconnected:
        qWarning() << "Disconnected";
        mLoop->exit(1);
        break;
    case ConnectionStatusConnecting:
        /* do nothing */
        break;
    case ConnectionStatusConnected:
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
    TEST_VERIFY_OP(op);

    PendingContacts *pending = qobject_cast<PendingContacts *>(op);
    mContacts = pending->contacts();

    if (pending->isForHandles()) {
        mInvalidHandles = pending->invalidHandles();
    }

    mLoop->exit(0);
}

void TestContacts::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService), "contacts", &name,
                &connPath, &error));
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

    mConn->lowlevel()->requestConnect();

    Features features = Features() << Connection::FeatureSelfContact;
    QVERIFY(connect(mConn->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    if (mConn->status() != ConnectionStatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
                        SLOT(expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
                           this,
                           SLOT(expectConnReady(Tp::ConnectionStatus, Tp::ConnectionStatusReason))));
        QCOMPARE(mConn->status(), ConnectionStatusConnected);
    }
}

void TestContacts::init()
{
    initImpl();
}

void TestContacts::testSupport()
{
    QCOMPARE(mConn->contactManager()->connection(), mConn);

    QVERIFY(!mConn->lowlevel()->contactAttributeInterfaces().isEmpty());

    QVERIFY(mConn->lowlevel()->contactAttributeInterfaces().contains(
                TP_QT_IFACE_CONNECTION));
    QVERIFY(mConn->lowlevel()->contactAttributeInterfaces().contains(
                TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING));
    QVERIFY(mConn->lowlevel()->contactAttributeInterfaces().contains(
                TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS));
    QVERIFY(mConn->lowlevel()->contactAttributeInterfaces().contains(
                TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE));
    QVERIFY(!mConn->lowlevel()->contactAttributeInterfaces().contains(
                QLatin1String("org.freedesktop.Telepathy.Connection.Interface.Addressing.DRAFT")));

    Features supportedFeatures = mConn->contactManager()->supportedFeatures();
    QVERIFY(!supportedFeatures.isEmpty());
    QVERIFY(supportedFeatures.contains(Contact::FeatureAlias));
    QVERIFY(supportedFeatures.contains(Contact::FeatureAvatarToken));
    QVERIFY(supportedFeatures.contains(Contact::FeatureSimplePresence));
    QVERIFY(!supportedFeatures.contains(Contact::FeatureAddresses));
}

void TestContacts::testSelfContact()
{
    ContactPtr selfContact = mConn->selfContact();
    QVERIFY(selfContact != 0);

    QCOMPARE(selfContact->handle()[0], mConn->selfHandle());
    QCOMPARE(selfContact->id(), QString(QLatin1String("me@example.com")));

    Features features = Features() << Contact::FeatureAlias <<
        Contact::FeatureAvatarToken <<
        Contact::FeatureSimplePresence;
    QVERIFY(connect(selfContact->manager()->upgradeContacts(
                        QList<ContactPtr>() << selfContact, features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(selfContact->alias(), QString(QLatin1String("me@example.com")));

    QVERIFY(!selfContact->isAvatarTokenKnown());

    QCOMPARE(selfContact->presence().status(), QString(QLatin1String("available")));
    QCOMPARE(selfContact->presence().type(), Tp::ConnectionPresenceTypeAvailable);
    QCOMPARE(selfContact->presence().statusMessage(), QString(QLatin1String("")));

    features << Contact::FeatureInfo;
    QVERIFY(connect(selfContact->manager()->upgradeContacts(
                        QList<ContactPtr>() << selfContact, features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(selfContact->alias(), QString(QLatin1String("me@example.com")));

    QVERIFY(!selfContact->isAvatarTokenKnown());

    QCOMPARE(selfContact->presence().status(), QString(QLatin1String("available")));
    QCOMPARE(selfContact->presence().type(), Tp::ConnectionPresenceTypeAvailable);
    QCOMPARE(selfContact->presence().statusMessage(), QString(QLatin1String("")));
}

void TestContacts::testForHandles()
{
    Tp::UIntList handles;
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
    QCOMPARE(pending->manager(), mConn->contactManager());
    QCOMPARE(pending->features(), Features());

    QVERIFY(pending->isForHandles());
    QVERIFY(!pending->isForIdentifiers());
    QVERIFY(!pending->isUpgrade());

    QCOMPARE(pending->handles(), handles);

    // Wait for the contacts to be built
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // There should be 3 resulting contacts and 2 handles found to be invalid
    QCOMPARE(mContacts.size(), 3);

    QCOMPARE(mInvalidHandles.size(), 2);
    QCOMPARE(mInvalidHandles[0], handles[2]);
    QCOMPARE(mInvalidHandles[1], handles[4]);

    // Check the contact contents
    for (int i = 0; i < 3; i++) {
        QVERIFY(mContacts[i] != NULL);
        QCOMPARE(mContacts[i]->manager(), mConn->contactManager());
        QCOMPARE(mContacts[i]->requestedFeatures(), Features());
        QCOMPARE(mContacts[i]->actualFeatures(), Features());
    }

    QCOMPARE(mContacts[0]->handle()[0], handles[0]);
    QCOMPARE(mContacts[1]->handle()[0], handles[1]);
    QCOMPARE(mContacts[2]->handle()[0], handles[3]);

    QCOMPARE(mContacts[0]->id(), QString(QLatin1String("alice")));
    QCOMPARE(mContacts[1]->id(), QString(QLatin1String("bob")));
    QCOMPARE(mContacts[2]->id(), QString(QLatin1String("chris")));

    // Save the contacts, and make a new request, replacing one of the invalid handles with a valid
    // one
    QList<ContactPtr> saveContacts = mContacts;
    handles[2] = tp_handle_ensure(serviceRepo, "dora", NULL, NULL);
    QVERIFY(handles[2] != 0);

    pending = mConn->contactManager()->contactsForHandles(handles);
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that we got the correct number of contacts back
    QCOMPARE(mContacts.size(), 4);
    QCOMPARE(mInvalidHandles.size(), 1);

    // Check that the contacts we already had were returned for the initial three
    QCOMPARE(saveContacts[0], mContacts[0]);
    QCOMPARE(saveContacts[1], mContacts[1]);
    QCOMPARE(saveContacts[2], mContacts[3]);

    // Check that the new contact is OK too
    QCOMPARE(mContacts[2]->handle()[0], handles[2]);
    QCOMPARE(mContacts[2]->id(), QString(QLatin1String("dora")));

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    saveContacts.clear();
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn.data());
}

void TestContacts::testForIdentifiers()
{
    QStringList validIDs = QStringList() << QLatin1String("Alice")
        << QLatin1String("Bob") << QLatin1String("Chris");
    QStringList invalidIDs = QStringList() << QLatin1String("Not valid")
        << QLatin1String("Not valid either");
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    QStringList toCheck;

    // Check that a request with just the invalid IDs fails
    PendingContacts *fails = mConn->contactManager()->contactsForIdentifiers(invalidIDs);
    QVERIFY(connect(fails,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    toCheck = fails->invalidIdentifiers().keys();
    toCheck.sort();
    invalidIDs.sort();
    QCOMPARE(toCheck, invalidIDs);

    // A request with both valid and invalid IDs should succeed
    fails = mConn->contactManager()->contactsForIdentifiers(invalidIDs + validIDs + invalidIDs);
    QVERIFY(connect(fails,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(fails->validIdentifiers(), validIDs);
    toCheck = fails->invalidIdentifiers().keys();
    toCheck.sort();
    invalidIDs.sort();
    QCOMPARE(toCheck, invalidIDs);

    // Go on to the meat: valid IDs
    PendingContacts *pending = mConn->contactManager()->contactsForIdentifiers(validIDs);

    // Test the closure accessors
    QCOMPARE(pending->manager(), mConn->contactManager());
    QCOMPARE(pending->features(), Features());

    QVERIFY(!pending->isForHandles());
    QVERIFY(pending->isForIdentifiers());
    QVERIFY(!pending->isUpgrade());

    QCOMPARE(pending->identifiers(), validIDs);

    // Finish it
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that there are 3 contacts consistent with the request
    QCOMPARE(mContacts.size(), 3);

    for (int i = 0; i < mContacts.size(); i++) {
        QVERIFY(mContacts[i] != NULL);
        QCOMPARE(mContacts[i]->manager(), mConn->contactManager());
        QVERIFY(tp_handle_is_valid(serviceRepo, mContacts[i]->handle()[0], NULL));
        QCOMPARE(mContacts[i]->requestedFeatures(), Features());
        QCOMPARE(mContacts[i]->actualFeatures(), Features());
    }

    QCOMPARE(mContacts[0]->id(), QString(QLatin1String("alice")));
    QCOMPARE(mContacts[1]->id(), QString(QLatin1String("bob")));
    QCOMPARE(mContacts[2]->id(), QString(QLatin1String("chris")));

    // Make the contacts go out of scope, starting releasing their handles, and finish that (but
    // save their handles first)
    Tp::UIntList saveHandles = Tp::UIntList() << mContacts[0]->handle()[0]
        << mContacts[1]->handle()[0]
        << mContacts[2]->handle()[0];
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn.data());
}

void TestContacts::testFeatures()
{
    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");
    const char *initialAliases[] = {
        "Alice in Wonderland",
        "Bob the Builder",
        "Chris Sawyer"
    };
    const char *latterAliases[] = {
        "Alice Through the Looking Glass",
        "Bob the Pensioner"
    };
    const char *initialTokens[] = {
        "bbbbb",
        "ccccc"
    };
    const char *latterTokens[] = {
        "AAAA",
        "BBBB"
    };
    static TpTestsContactsConnectionPresenceStatusIndex initialStatuses[] = {
        TP_TESTS_CONTACTS_CONNECTION_STATUS_AVAILABLE,
        TP_TESTS_CONTACTS_CONNECTION_STATUS_BUSY,
        TP_TESTS_CONTACTS_CONNECTION_STATUS_AWAY
    };
    static TpTestsContactsConnectionPresenceStatusIndex latterStatuses[] = {
        TP_TESTS_CONTACTS_CONNECTION_STATUS_AWAY,
        TP_TESTS_CONTACTS_CONNECTION_STATUS_AVAILABLE,
    };
    const char *initialMessages[] = {
        "",
        "Fixing it",
        "GON OUT BACKSON"
    };
    const char *latterMessages[] = {
        "Having some carrots",
        "Done building for life, yay",
    };
    Features features = Features()
        << Contact::FeatureAlias
        << Contact::FeatureAvatarToken
        << Contact::FeatureSimplePresence;
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    // Get test handles
    Tp::UIntList handles;
    for (int i = 0; i < 3; i++) {
        handles.push_back(tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(), NULL, NULL));
        QVERIFY(handles[i] != 0);
    }

    // Set the initial attributes
    tp_tests_contacts_connection_change_aliases(mConnService, 3, handles.toVector().constData(),
            initialAliases);
    tp_tests_contacts_connection_change_avatar_tokens(mConnService, 2, handles.toVector().constData() + 1,
            initialTokens);
    tp_tests_contacts_connection_change_presences(mConnService, 3, handles.toVector().constData(),
            initialStatuses, initialMessages);

    // Build contacts
    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles, features);
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check the contact contents
    QCOMPARE(mContacts.size(), 3);
    for (int i = 0; i < 3; i++) {
        QCOMPARE(mContacts[i]->handle()[0], handles[i]);
        QCOMPARE(mContacts[i]->id(), ids[i]);
        QVERIFY((features - mContacts[i]->requestedFeatures()).isEmpty());
        QVERIFY((mContacts[i]->actualFeatures() - mContacts[i]->requestedFeatures()).isEmpty());

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAlias));
        QCOMPARE(mContacts[i]->alias(), QString(QLatin1String(initialAliases[i])));

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAvatarToken));
        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureSimplePresence));
        QCOMPARE(mContacts[i]->presence().statusMessage(), QString(QLatin1String(initialMessages[i])));
    }

    // Check that there's no known avatar token for the first contact, but that there is for the
    // two others
    QVERIFY(!mContacts[0]->isAvatarTokenKnown());
    QVERIFY(mContacts[1]->isAvatarTokenKnown());
    QVERIFY(mContacts[2]->isAvatarTokenKnown());

    QCOMPARE(mContacts[0]->avatarToken(), QString(QLatin1String("")));
    QCOMPARE(mContacts[1]->avatarToken(), QString(QLatin1String(initialTokens[0])));
    QCOMPARE(mContacts[2]->avatarToken(), QString(QLatin1String(initialTokens[1])));

    QCOMPARE(mContacts[0]->presence().status(), QString(QLatin1String("available")));
    QCOMPARE(mContacts[1]->presence().status(), QString(QLatin1String("busy")));
    QCOMPARE(mContacts[2]->presence().status(), QString(QLatin1String("away")));

    QCOMPARE(mContacts[0]->presence().type(), Tp::ConnectionPresenceTypeAvailable);
    QCOMPARE(mContacts[1]->presence().type(), Tp::ConnectionPresenceTypeBusy);
    QCOMPARE(mContacts[2]->presence().type(), Tp::ConnectionPresenceTypeAway);

    // Change some of the contacts to a new set of attributes
    tp_tests_contacts_connection_change_aliases(mConnService, 2, handles.toVector().constData(),
            latterAliases);
    tp_tests_contacts_connection_change_avatar_tokens(mConnService, 2, handles.toVector().constData(),
            latterTokens);
    tp_tests_contacts_connection_change_presences(mConnService, 2, handles.toVector().constData(),
            latterStatuses, latterMessages);
    mLoop->processEvents();
    processDBusQueue(mConn.data());

    // Check that the attributes were updated in the Contact objects
    for (int i = 0; i < 3; i++) {
        QCOMPARE(mContacts[i]->handle()[0], handles[i]);
        QCOMPARE(mContacts[i]->id(), ids[i]);
        QVERIFY((features - mContacts[i]->requestedFeatures()).isEmpty());
        QVERIFY((mContacts[i]->actualFeatures() - mContacts[i]->requestedFeatures()).isEmpty());

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAlias));
        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAvatarToken));
        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureSimplePresence));

        QVERIFY(mContacts[i]->isAvatarTokenKnown());
    }

    QCOMPARE(mContacts[0]->alias(), QString(QLatin1String(latterAliases[0])));
    QCOMPARE(mContacts[1]->alias(), QString(QLatin1String(latterAliases[1])));
    QCOMPARE(mContacts[2]->alias(), QString(QLatin1String(initialAliases[2])));

    QCOMPARE(mContacts[0]->avatarToken(), QString(QLatin1String(latterTokens[0])));
    QCOMPARE(mContacts[1]->avatarToken(), QString(QLatin1String(latterTokens[1])));
    QCOMPARE(mContacts[2]->avatarToken(), QString(QLatin1String(initialTokens[1])));

    QCOMPARE(mContacts[0]->presence().status(), QString(QLatin1String("away")));
    QCOMPARE(mContacts[1]->presence().status(), QString(QLatin1String("available")));
    QCOMPARE(mContacts[2]->presence().status(), QString(QLatin1String("away")));

    QCOMPARE(mContacts[0]->presence().type(), Tp::ConnectionPresenceTypeAway);
    QCOMPARE(mContacts[1]->presence().type(), Tp::ConnectionPresenceTypeAvailable);
    QCOMPARE(mContacts[2]->presence().type(), Tp::ConnectionPresenceTypeAway);

    QCOMPARE(mContacts[0]->presence().statusMessage(), QString(QLatin1String(latterMessages[0])));
    QCOMPARE(mContacts[1]->presence().statusMessage(), QString(QLatin1String(latterMessages[1])));
    QCOMPARE(mContacts[2]->presence().statusMessage(), QString(QLatin1String(initialMessages[2])));

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn.data());
}

void TestContacts::testFeaturesNotRequested()
{
    // Test ids and corresponding handles
    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);
    Tp::UIntList handles;
    for (int i = 0; i < 3; i++) {
        handles.push_back(tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(), NULL, NULL));
        QVERIFY(handles[i] != 0);
    }

    // Build contacts (note: no features)
    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles);
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that the feature accessors return sensible fallback values (note: the warnings are
    // intentional - however, I'm not quite sure if they should be just debug (like in tp-glib))
    QCOMPARE(mContacts.size(), 3);
    for (int i = 0; i < 3; i++) {
        ContactPtr contact = mContacts[i];

        QVERIFY(contact->requestedFeatures().isEmpty());
        QVERIFY(contact->actualFeatures().isEmpty());

        QCOMPARE(contact->alias(), contact->id());

        QVERIFY(!contact->isAvatarTokenKnown());
        QCOMPARE(contact->avatarToken(), QString(QLatin1String("")));

        QCOMPARE(contact->presence().isValid(), false);
    }

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn.data());
}

void TestContacts::testUpgrade()
{
    QStringList ids = QStringList() << QLatin1String("alice")
        << QLatin1String("bob") << QLatin1String("chris");
    const char *aliases[] = {
        "Alice in Wonderland",
        "Bob The Builder",
        "Chris Sawyer"
    };
    const char *tokens[] = {
        "aaaaa",
        "bbbbb",
        "ccccc"
    };
    static TpTestsContactsConnectionPresenceStatusIndex statuses[] = {
        TP_TESTS_CONTACTS_CONNECTION_STATUS_AVAILABLE,
        TP_TESTS_CONTACTS_CONNECTION_STATUS_BUSY,
        TP_TESTS_CONTACTS_CONNECTION_STATUS_AWAY
    };
    const char *messages[] = {
        "",
        "Fixing it",
        "GON OUT BACKSON"
    };
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    Tp::UIntList handles;
    for (int i = 0; i < 3; i++) {
        handles.push_back(tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(), NULL, NULL));
        QVERIFY(handles[i] != 0);
    }

    tp_tests_contacts_connection_change_aliases(mConnService, 3, handles.toVector().constData(), aliases);
    tp_tests_contacts_connection_change_avatar_tokens(mConnService, 3, handles.toVector().constData(), tokens);
    tp_tests_contacts_connection_change_presences(mConnService, 3, handles.toVector().constData(), statuses,
            messages);

    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles);

    // Wait for the contacts to be built
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // There should be 3 resulting contacts - save them for future reference
    QCOMPARE(mContacts.size(), 3);
    QList<ContactPtr> saveContacts = mContacts;

    // Upgrade them
    Features features = Features()
        << Contact::FeatureAlias
        << Contact::FeatureAvatarToken
        << Contact::FeatureSimplePresence;
    pending = mConn->contactManager()->upgradeContacts(saveContacts, features);

    // Test the closure accessors
    QCOMPARE(pending->manager(), mConn->contactManager());
    QCOMPARE(pending->features(), features);

    QVERIFY(!pending->isForHandles());
    QVERIFY(!pending->isForIdentifiers());
    QVERIFY(pending->isUpgrade());

    QCOMPARE(pending->contactsToUpgrade(), saveContacts);

    // Wait for the contacts to be built
    QVERIFY(connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that we got the correct contacts back
    QCOMPARE(mContacts, saveContacts);

    // Check the contact contents
    for (int i = 0; i < 3; i++) {
        QCOMPARE(mContacts[i]->handle()[0], handles[i]);
        QCOMPARE(mContacts[i]->id(), ids[i]);
        QVERIFY((features - mContacts[i]->requestedFeatures()).isEmpty());
        QVERIFY((mContacts[i]->actualFeatures() - mContacts[i]->requestedFeatures()).isEmpty());

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAlias));
        QCOMPARE(mContacts[i]->alias(), QString(QLatin1String(aliases[i])));

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAvatarToken));
        QVERIFY(mContacts[i]->isAvatarTokenKnown());
        QCOMPARE(mContacts[i]->avatarToken(), QString(QLatin1String(tokens[i])));

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureSimplePresence));
        QCOMPARE(mContacts[i]->presence().statusMessage(), QString(QLatin1String(messages[i])));
    }

    QCOMPARE(mContacts[0]->presence().status(), QString(QLatin1String("available")));
    QCOMPARE(mContacts[1]->presence().status(), QString(QLatin1String("busy")));
    QCOMPARE(mContacts[2]->presence().status(), QString(QLatin1String("away")));

    QCOMPARE(mContacts[0]->presence().type(), Tp::ConnectionPresenceTypeAvailable);
    QCOMPARE(mContacts[1]->presence().type(), Tp::ConnectionPresenceTypeBusy);
    QCOMPARE(mContacts[2]->presence().type(), Tp::ConnectionPresenceTypeAway);

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    saveContacts.clear();
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn.data());
}

void TestContacts::testSelfContactFallback()
{
    gchar *name;
    gchar *connPath;
    GError *error = 0;

    TpTestsSimpleConnection *connService;
    connService = TP_TESTS_SIMPLE_CONNECTION(g_object_new(
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            NULL));
    QVERIFY(connService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(connService), "simple", &name,
                &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    ConnectionPtr conn = Connection::create(QLatin1String(name), QLatin1String(connPath),
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    g_free(name);
    g_free(connPath);

    QCOMPARE(conn->isReady(), false);

    Features features = Features() << Connection::FeatureSelfContact;
    QVERIFY(connect(conn->lowlevel()->requestConnect(features),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(conn->isReady(features), true);

    ContactPtr selfContact = conn->selfContact();
    QVERIFY(selfContact != 0);

    QCOMPARE(selfContact->handle()[0], conn->selfHandle());
    QCOMPARE(selfContact->id(), QString(QLatin1String("me@example.com")));
    QCOMPARE(selfContact->alias(), QString(QLatin1String("me@example.com")));
    QVERIFY(!selfContact->isAvatarTokenKnown());
    QCOMPARE(selfContact->presence().isValid(), false);

    tp_tests_simple_connection_inject_disconnect(connService);

    if (conn->isValid()) {
        QVERIFY(connect(conn.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                            const QString &, const QString &)),
                    mLoop,
                    SLOT(quit())));
        QCOMPARE(mLoop->exec(), 0);
    }

    g_object_unref(connService);
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

    if (!mInvalidHandles.isEmpty()) {
        mInvalidHandles.clear();
    }

    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                        SIGNAL(invalidated(Tp::DBusProxy *, QString, QString)),
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

QTEST_MAIN(TestContacts)
#include "_gen/contacts.cpp.moc.hpp"
