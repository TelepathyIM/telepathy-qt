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

    void testSupport();
    void testSelfContact();
    void testForHandles();
    void testForIdentifiers();
    void testFeatures();
    void testFeaturesNotRequested();
    void testUpgrade();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ContactsConnection *mConnService;
    Connection *mConn;
    QList<QSharedPointer<Contact> > mContacts;
    Telepathy::UIntList mInvalidHandles;
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

void TestContacts::testSupport()
{
    QCOMPARE(mConn->contactManager()->connection(), mConn);
    QVERIFY(mConn->contactManager()->isSupported());

    QVERIFY(!mConn->contactAttributeInterfaces().isEmpty());

    QVERIFY(mConn->contactAttributeInterfaces().contains(TELEPATHY_INTERFACE_CONNECTION));
    QVERIFY(mConn->contactAttributeInterfaces().contains(
                TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING));
    QVERIFY(mConn->contactAttributeInterfaces().contains(
                TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS));
    QVERIFY(mConn->contactAttributeInterfaces().contains(
                TELEPATHY_INTERFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE));

    QSet<Contact::Feature> supportedFeatures = mConn->contactManager()->supportedFeatures();
    QVERIFY(!supportedFeatures.isEmpty());
    QVERIFY(supportedFeatures.contains(Contact::FeatureAlias));
    QVERIFY(supportedFeatures.contains(Contact::FeatureAvatarToken));
    QVERIFY(supportedFeatures.contains(Contact::FeatureSimplePresence));
}

void TestContacts::testSelfContact()
{
    QSharedPointer<Contact> selfContact = mConn->selfContact();
    QVERIFY(selfContact != 0);

    QCOMPARE(selfContact->handle()[0], mConn->selfHandle());
    QCOMPARE(selfContact->id(), QString("me@example.com"));

    QCOMPARE(selfContact->alias(), QString("me@example.com"));

    QVERIFY(!selfContact->isAvatarTokenKnown());

    QCOMPARE(selfContact->presenceStatus(), QString("available"));
    QCOMPARE(selfContact->presenceType(), uint(Telepathy::ConnectionPresenceTypeAvailable));
    QCOMPARE(selfContact->presenceMessage(), QString(""));

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
    QCOMPARE(pending->manager(), mConn->contactManager());
    QCOMPARE(pending->features(), QSet<Contact::Feature>());

    QVERIFY(pending->isForHandles());
    QVERIFY(!pending->isForIdentifiers());
    QVERIFY(!pending->isUpgrade());

    QCOMPARE(pending->handles(), handles);

    // Wait for the contacts to be built
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
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
        QCOMPARE(mContacts[i]->requestedFeatures(), QSet<Contact::Feature>());
        QCOMPARE(mContacts[i]->actualFeatures(), QSet<Contact::Feature>());
    }

    QCOMPARE(mContacts[0]->handle()[0], handles[0]);
    QCOMPARE(mContacts[1]->handle()[0], handles[1]);
    QCOMPARE(mContacts[2]->handle()[0], handles[3]);

    QCOMPARE(mContacts[0]->id(), QString("alice"));
    QCOMPARE(mContacts[1]->id(), QString("bob"));
    QCOMPARE(mContacts[2]->id(), QString("chris"));

    // Save the contacts, and make a new request, replacing one of the invalid handles with a valid
    // one
    QList<QSharedPointer<Contact> > saveContacts = mContacts;
    handles[2] = tp_handle_ensure(serviceRepo, "dora", NULL, NULL);
    QVERIFY(handles[2] != 0);

    pending = mConn->contactManager()->contactsForHandles(handles);
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
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
    QCOMPARE(mContacts[2]->id(), QString("dora"));

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    saveContacts.clear();
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn);

    // Unref the handles we created service-side
    tp_handle_unref(serviceRepo, handles[0]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
    tp_handle_unref(serviceRepo, handles[1]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[1], NULL));
    tp_handle_unref(serviceRepo, handles[2]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[2], NULL));
    tp_handle_unref(serviceRepo, handles[3]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[3], NULL));
}

void TestContacts::testForIdentifiers()
{
    QStringList validIDs = QStringList() << "Alice" << "Bob" << "Chris";
    QStringList invalidIDs = QStringList() << "Not valid" << "Not valid either";
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    // Check that a request with just the invalid IDs fails
    PendingOperation *fails = mConn->contactManager()->contactsForIdentifiers(invalidIDs);
    QVERIFY(connect(fails,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 1);

    // A request with both valid and invalid IDs should also fail
    fails = mConn->contactManager()->contactsForIdentifiers(invalidIDs + validIDs + invalidIDs);
    QVERIFY(connect(fails,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 1);

    // Go on to the meat: valid IDs
    PendingContacts *pending = mConn->contactManager()->contactsForIdentifiers(validIDs);

    // Test the closure accessors
    QCOMPARE(pending->manager(), mConn->contactManager());
    QCOMPARE(pending->features(), QSet<Contact::Feature>());

    QVERIFY(!pending->isForHandles());
    QVERIFY(pending->isForIdentifiers());
    QVERIFY(!pending->isUpgrade());

    QCOMPARE(pending->identifiers(), validIDs);

    // Finish it
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that there are 3 contacts consistent with the request
    QCOMPARE(mContacts.size(), 3);

    for (int i = 0; i < mContacts.size(); i++) {
        QVERIFY(mContacts[i] != NULL);
        QCOMPARE(mContacts[i]->manager(), mConn->contactManager());
        QVERIFY(tp_handle_is_valid(serviceRepo, mContacts[i]->handle()[0], NULL));
        QCOMPARE(mContacts[i]->requestedFeatures(), QSet<Contact::Feature>());
        QCOMPARE(mContacts[i]->actualFeatures(), QSet<Contact::Feature>());
    }

    QCOMPARE(mContacts[0]->id(), QString("alice"));
    QCOMPARE(mContacts[1]->id(), QString("bob"));
    QCOMPARE(mContacts[2]->id(), QString("chris"));

    // Make the contacts go out of scope, starting releasing their handles, and finish that (but
    // save their handles first)
    Telepathy::UIntList saveHandles = Telepathy::UIntList() << mContacts[0]->handle()[0]
                                                            << mContacts[1]->handle()[0]
                                                            << mContacts[2]->handle()[0];
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn);

    // Check that their handles are in fact released
    foreach (uint handle, saveHandles) {
        QVERIFY(!tp_handle_is_valid(serviceRepo, handle, NULL));
    }
}

void TestContacts::testFeatures()
{
    QStringList ids = QStringList() << "alice" << "bob" << "chris";
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
    static ContactsConnectionPresenceStatusIndex initialStatuses[] = {
        CONTACTS_CONNECTION_STATUS_AVAILABLE,
        CONTACTS_CONNECTION_STATUS_BUSY,
        CONTACTS_CONNECTION_STATUS_AWAY
    };
    static ContactsConnectionPresenceStatusIndex latterStatuses[] = {
        CONTACTS_CONNECTION_STATUS_AWAY,
        CONTACTS_CONNECTION_STATUS_AVAILABLE,
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
    QSet<Contact::Feature> features = QSet<Contact::Feature>()
        << Contact::FeatureAlias
        << Contact::FeatureAvatarToken
        << Contact::FeatureSimplePresence;
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    // Get test handles
    Telepathy::UIntList handles;
    for (int i = 0; i < 3; i++) {
        handles.push_back(tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(), NULL, NULL));
        QVERIFY(handles[i] != 0);
    }

    // Set the initial attributes
    contacts_connection_change_aliases(mConnService, 3, handles.toVector().constData(),
            initialAliases);
    contacts_connection_change_avatar_tokens(mConnService, 2, handles.toVector().constData() + 1,
            initialTokens);
    contacts_connection_change_presences(mConnService, 3, handles.toVector().constData(),
            initialStatuses, initialMessages);

    // Build contacts
    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles, features);
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check the contact contents
    QCOMPARE(mContacts.size(), 3);
    for (int i = 0; i < 3; i++) {
        QCOMPARE(mContacts[i]->handle()[0], handles[i]);
        QCOMPARE(mContacts[i]->id(), ids[i]);
        QVERIFY((features - mContacts[i]->requestedFeatures()).isEmpty());
        QVERIFY((mContacts[i]->actualFeatures() - mContacts[i]->requestedFeatures()).isEmpty());

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAlias));
        QCOMPARE(mContacts[i]->alias(), QString(initialAliases[i]));

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAvatarToken));
        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureSimplePresence));
        QCOMPARE(mContacts[i]->presenceMessage(), QString(initialMessages[i]));
    }

    // Check that there's no known avatar token for the first contact, but that there is for the
    // two others
    QVERIFY(!mContacts[0]->isAvatarTokenKnown());
    QVERIFY(mContacts[1]->isAvatarTokenKnown());
    QVERIFY(mContacts[2]->isAvatarTokenKnown());

    QCOMPARE(mContacts[0]->avatarToken(), QString(""));
    QCOMPARE(mContacts[1]->avatarToken(), QString(initialTokens[0]));
    QCOMPARE(mContacts[2]->avatarToken(), QString(initialTokens[1]));

    QCOMPARE(mContacts[0]->presenceStatus(), QString("available"));
    QCOMPARE(mContacts[1]->presenceStatus(), QString("busy"));
    QCOMPARE(mContacts[2]->presenceStatus(), QString("away"));

    QCOMPARE(mContacts[0]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAvailable));
    QCOMPARE(mContacts[1]->presenceType(), uint(Telepathy::ConnectionPresenceTypeBusy));
    QCOMPARE(mContacts[2]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAway));

    // Change some of the contacts to a new set of attributes
    contacts_connection_change_aliases(mConnService, 2, handles.toVector().constData(),
            latterAliases);
    contacts_connection_change_avatar_tokens(mConnService, 2, handles.toVector().constData(),
            latterTokens);
    contacts_connection_change_presences(mConnService, 2, handles.toVector().constData(),
            latterStatuses, latterMessages);
    mLoop->processEvents();
    processDBusQueue(mConn);

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

    QCOMPARE(mContacts[0]->alias(), QString(latterAliases[0]));
    QCOMPARE(mContacts[1]->alias(), QString(latterAliases[1]));
    QCOMPARE(mContacts[2]->alias(), QString(initialAliases[2]));

    QCOMPARE(mContacts[0]->avatarToken(), QString(latterTokens[0]));
    QCOMPARE(mContacts[1]->avatarToken(), QString(latterTokens[1]));
    QCOMPARE(mContacts[2]->avatarToken(), QString(initialTokens[1]));

    QCOMPARE(mContacts[0]->presenceStatus(), QString("away"));
    QCOMPARE(mContacts[1]->presenceStatus(), QString("available"));
    QCOMPARE(mContacts[2]->presenceStatus(), QString("away"));

    QCOMPARE(mContacts[0]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAway));
    QCOMPARE(mContacts[1]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAvailable));
    QCOMPARE(mContacts[2]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAway));

    QCOMPARE(mContacts[0]->presenceMessage(), QString(latterMessages[0]));
    QCOMPARE(mContacts[1]->presenceMessage(), QString(latterMessages[1]));
    QCOMPARE(mContacts[2]->presenceMessage(), QString(initialMessages[2]));

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn);

    // Unref the handles we created service-side
    tp_handle_unref(serviceRepo, handles[0]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
    tp_handle_unref(serviceRepo, handles[1]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[1], NULL));
    tp_handle_unref(serviceRepo, handles[2]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[2], NULL));
}

void TestContacts::testFeaturesNotRequested()
{
    // Test ids and corresponding handles
    QStringList ids = QStringList() << "alice" << "bob" << "chris";
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);
    Telepathy::UIntList handles;
    for (int i = 0; i < 3; i++) {
        handles.push_back(tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(), NULL, NULL));
        QVERIFY(handles[i] != 0);
    }

    // Build contacts (note: no features)
    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles);
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Check that the feature accessors return sensible fallback values (note: the warnings are
    // intentional - however, I'm not quite sure if they should be just debug (like in tp-glib))
    QCOMPARE(mContacts.size(), 3);
    for (int i = 0; i < 3; i++) {
        QSharedPointer<Contact> contact = mContacts[i];

        QVERIFY(contact->requestedFeatures().isEmpty());
        QVERIFY(contact->actualFeatures().isEmpty());

        QCOMPARE(contact->alias(), contact->id());

        QVERIFY(!contact->isAvatarTokenKnown());
        QCOMPARE(contact->avatarToken(), QString(""));

        QCOMPARE(contact->presenceStatus(), QString("unknown"));
        QCOMPARE(contact->presenceType(), uint(Telepathy::ConnectionPresenceTypeUnknown));
        QCOMPARE(contact->presenceMessage(), QString(""));
    }

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn);

    // Unref the handles we created service-side
    tp_handle_unref(serviceRepo, handles[0]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
    tp_handle_unref(serviceRepo, handles[1]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[1], NULL));
    tp_handle_unref(serviceRepo, handles[2]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[2], NULL));
}
void TestContacts::testUpgrade()
{
    QStringList ids = QStringList() << "alice" << "bob" << "chris";
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
    static ContactsConnectionPresenceStatusIndex statuses[] = {
        CONTACTS_CONNECTION_STATUS_AVAILABLE,
        CONTACTS_CONNECTION_STATUS_BUSY,
        CONTACTS_CONNECTION_STATUS_AWAY
    };
    const char *messages[] = {
        "",
        "Fixing it",
        "GON OUT BACKSON"
    };
    TpHandleRepoIface *serviceRepo =
        tp_base_connection_get_handles(TP_BASE_CONNECTION(mConnService), TP_HANDLE_TYPE_CONTACT);

    Telepathy::UIntList handles;
    for (int i = 0; i < 3; i++) {
        handles.push_back(tp_handle_ensure(serviceRepo, ids[i].toLatin1().constData(), NULL, NULL));
        QVERIFY(handles[i] != 0);
    }

    contacts_connection_change_aliases(mConnService, 3, handles.toVector().constData(), aliases);
    contacts_connection_change_avatar_tokens(mConnService, 3, handles.toVector().constData(), tokens);
    contacts_connection_change_presences(mConnService, 3, handles.toVector().constData(), statuses,
            messages);

    PendingContacts *pending = mConn->contactManager()->contactsForHandles(handles);

    // Wait for the contacts to be built
    QVERIFY(connect(pending,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // There should be 3 resulting contacts - save them for future reference
    QCOMPARE(mContacts.size(), 3);
    QList<QSharedPointer<Contact> > saveContacts = mContacts;

    // Upgrade them
    QSet<Contact::Feature> features = QSet<Contact::Feature>()
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
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Telepathy::Client::PendingOperation*))));
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
        QCOMPARE(mContacts[i]->alias(), QString(aliases[i]));

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureAvatarToken));
        QVERIFY(mContacts[i]->isAvatarTokenKnown());
        QCOMPARE(mContacts[i]->avatarToken(), QString(tokens[i]));

        QVERIFY(mContacts[i]->actualFeatures().contains(Contact::FeatureSimplePresence));
        QCOMPARE(mContacts[i]->presenceMessage(), QString(messages[i]));
    }

    QCOMPARE(mContacts[0]->presenceStatus(), QString("available"));
    QCOMPARE(mContacts[1]->presenceStatus(), QString("busy"));
    QCOMPARE(mContacts[2]->presenceStatus(), QString("away"));

    QCOMPARE(mContacts[0]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAvailable));
    QCOMPARE(mContacts[1]->presenceType(), uint(Telepathy::ConnectionPresenceTypeBusy));
    QCOMPARE(mContacts[2]->presenceType(), uint(Telepathy::ConnectionPresenceTypeAway));

    // Make the contacts go out of scope, starting releasing their handles, and finish that
    saveContacts.clear();
    mContacts.clear();
    mLoop->processEvents();
    processDBusQueue(mConn);

    // Unref the handles we created service-side
    tp_handle_unref(serviceRepo, handles[0]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[0], NULL));
    tp_handle_unref(serviceRepo, handles[1]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[1], NULL));
    tp_handle_unref(serviceRepo, handles[2]);
    QVERIFY(!tp_handle_is_valid(serviceRepo, handles[2], NULL));
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
