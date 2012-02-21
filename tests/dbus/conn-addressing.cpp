#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/future/conn-addressing/conn.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/Types>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestConnAddressing : public Test
{
    Q_OBJECT

public:
    TestConnAddressing(QObject *parent = 0)
        : Test(parent)
    {
    }

protected Q_SLOTS:
    void expectPendingContactsFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testSupport();
    void testRequest();
    void testRequestNoFeatures();
    void testRequestEmpty();

    void cleanup();
    void cleanupTestCase();

private:
    void commonTestRequest(bool withFeatures);

    TestConnHelper *mConn;
    QList<ContactPtr> mContacts;
    Tp::UIntList mInvalidHandles;
    QStringList mValidIds;
    QHash<QString, QPair<QString, QString> > mInvalidIds;
    QStringList mValidVCardAddresses;
    QStringList mInvalidVCardAddresses;
    QStringList mValidUris;
    QStringList mInvalidUris;
};

void TestConnAddressing::expectPendingContactsFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    mContacts = pc->contacts();
    mInvalidHandles = pc->invalidHandles();
    mValidIds = pc->validIdentifiers();
    mInvalidIds = pc->invalidIdentifiers();
    mValidVCardAddresses = pc->validVCardAddresses();
    mInvalidVCardAddresses = pc->invalidVCardAddresses();
    mValidUris = pc->validUris();
    mInvalidUris = pc->invalidUris();

    mLoop->exit(0);
}

void TestConnAddressing::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-addressing");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_ADDRESSING_CONNECTION,
            "account", "me@example.com",
            "protocol", "addressing",
            NULL);
    QVERIFY(mConn->connect());
}

void TestConnAddressing::init()
{
    initImpl();

    mContacts.clear();
    mInvalidHandles.clear();
    mValidIds.clear();
    mInvalidIds.clear();
    mValidVCardAddresses.clear();
    mInvalidVCardAddresses.clear();
    mValidUris.clear();
    mInvalidUris.clear();
}

void TestConnAddressing::testSupport()
{
    ConnectionPtr conn = mConn->client();

    QVERIFY(!conn->lowlevel()->contactAttributeInterfaces().isEmpty());

    QVERIFY(conn->lowlevel()->contactAttributeInterfaces().contains(
                TP_QT_IFACE_CONNECTION));
    QVERIFY(conn->lowlevel()->contactAttributeInterfaces().contains(
                TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING));

    Features supportedFeatures = conn->contactManager()->supportedFeatures();
    QVERIFY(!supportedFeatures.isEmpty());
    QVERIFY(supportedFeatures.contains(Contact::FeatureAddresses));
}

void TestConnAddressing::commonTestRequest(bool withFeatures)
{
    ConnectionPtr conn = mConn->client();

    Features features;
    if (withFeatures) {
        features << Contact::FeatureInfo << Contact::FeatureAddresses;
    }

    QStringList validUris;
    validUris << QLatin1String("addr:foo");
    QStringList invalidUris;
    invalidUris << QLatin1String("invalid_uri:bar");
    QStringList uris(validUris);
    uris.append(invalidUris);
    PendingContacts *pc = conn->contactManager()->contactsForUris(uris, features);

    // Test the closure accessors
    QCOMPARE(pc->manager(), conn->contactManager());
    QCOMPARE(pc->features(), features);

    QVERIFY(pc->isForUris());
    QCOMPARE(pc->uris(), uris);
    QVERIFY(!pc->isForHandles());
    QVERIFY(pc->handles().isEmpty());
    QVERIFY(!pc->isForIdentifiers());
    QVERIFY(pc->identifiers().isEmpty());
    QVERIFY(!pc->isForVCardAddresses());
    QVERIFY(pc->vcardField().isEmpty());
    QVERIFY(pc->vcardAddresses().isEmpty());
    QVERIFY(!pc->isUpgrade());
    QVERIFY(pc->contactsToUpgrade().isEmpty());

    // Wait for the contacts to be built
    QVERIFY(connect(pc,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // There should be 1 resulting contact ("foo") and 1 uri found to be invalid
    QCOMPARE(mContacts.size(), 1);
    QCOMPARE(mContacts[0]->id(), QLatin1String("foo"));
    if (withFeatures) {
        QVERIFY(!mContacts[0]->actualFeatures().contains(Contact::FeatureLocation));
        QVERIFY(mContacts[0]->actualFeatures().contains(Contact::FeatureInfo));
        QVERIFY(mContacts[0]->actualFeatures().contains(Contact::FeatureAddresses));
        QMap<QString, QString> vcardAddresses;
        vcardAddresses.insert(QLatin1String("x-addr"), QLatin1String("foo"));
        QStringList uris;
        uris.append(QLatin1String("addr:foo"));
        QCOMPARE(mContacts[0]->vcardAddresses(), vcardAddresses);
        QCOMPARE(mContacts[0]->uris(), uris);
    } else {
        QVERIFY(!mContacts[0]->actualFeatures().contains(Contact::FeatureLocation));
        QVERIFY(!mContacts[0]->actualFeatures().contains(Contact::FeatureInfo));
        // FeatureAddresses will be enabled even if not requested when
        // ContactManager::contactsForUris/VCardAddresses is used,
        // but we don't want to guarantee that, implementation detail
    }
    QCOMPARE(mValidUris, validUris);
    QCOMPARE(mInvalidUris, invalidUris);
    QVERIFY(mInvalidHandles.isEmpty());
    QVERIFY(mValidIds.isEmpty());
    QVERIFY(mInvalidIds.isEmpty());
    QVERIFY(mValidVCardAddresses.isEmpty());
    QVERIFY(mInvalidVCardAddresses.isEmpty());

    QStringList ids;
    ids << QLatin1String("foo");
    QList<ContactPtr> contacts = mConn->contacts(ids);
    QCOMPARE(contacts.size(), 1);
    QCOMPARE(mContacts[0], contacts[0]);

    // let's test for vCard now
    QString vcardField(QLatin1String("x-addr"));
    QStringList vcardAddresses;
    vcardAddresses << QLatin1String("foo") << QLatin1String("bar");
    vcardAddresses.sort();
    pc = conn->contactManager()->contactsForVCardAddresses(vcardField,
            vcardAddresses, features);

    // Test the closure accessors
    QCOMPARE(pc->manager(), conn->contactManager());
    QCOMPARE(pc->features(), features);

    QVERIFY(pc->isForVCardAddresses());
    QCOMPARE(pc->vcardField(), vcardField);
    QCOMPARE(pc->vcardAddresses(), vcardAddresses);
    QVERIFY(!pc->isForHandles());
    QVERIFY(pc->handles().isEmpty());
    QVERIFY(!pc->isForIdentifiers());
    QVERIFY(pc->identifiers().isEmpty());
    QVERIFY(!pc->isForUris());
    QVERIFY(pc->uris().isEmpty());
    QVERIFY(!pc->isUpgrade());
    QVERIFY(pc->contactsToUpgrade().isEmpty());

    // Wait for the contacts to be built
    QVERIFY(connect(pc,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // There should be 1 resulting contact ("foo") and 1 uri found to be invalid
    QCOMPARE(mContacts.size(), 2);
    if (withFeatures) {
        QVERIFY(!mContacts[0]->actualFeatures().contains(Contact::FeatureLocation));
        QVERIFY(mContacts[0]->actualFeatures().contains(Contact::FeatureInfo));
        QVERIFY(mContacts[0]->actualFeatures().contains(Contact::FeatureAddresses));
        QVERIFY(!mContacts[1]->actualFeatures().contains(Contact::FeatureLocation));
        QVERIFY(mContacts[1]->actualFeatures().contains(Contact::FeatureInfo));
        QVERIFY(mContacts[1]->actualFeatures().contains(Contact::FeatureAddresses));
    } else {
        QVERIFY(!mContacts[0]->actualFeatures().contains(Contact::FeatureLocation));
        QVERIFY(!mContacts[0]->actualFeatures().contains(Contact::FeatureInfo));
        QVERIFY(!mContacts[1]->actualFeatures().contains(Contact::FeatureLocation));
        QVERIFY(!mContacts[1]->actualFeatures().contains(Contact::FeatureInfo));
        // FeatureAddresses will be enabled even if not requested when
        // ContactManager::contactsForUris/VCardAddresses is used,
        // but we don't want to guarantee that, implementation detail
    }
    mValidVCardAddresses.sort();
    QCOMPARE(mValidVCardAddresses, vcardAddresses);
    QVERIFY(mInvalidVCardAddresses.isEmpty());
    QVERIFY(mInvalidHandles.isEmpty());
    QVERIFY(mValidIds.isEmpty());
    QVERIFY(mInvalidIds.isEmpty());
    QVERIFY(mValidUris.isEmpty());
    QVERIFY(mInvalidUris.isEmpty());

    // contact "foo" should be one of the returned contacts
    QVERIFY(mContacts[0] != mContacts[1]);
    QVERIFY(mContacts.contains(contacts[0]));
    contacts.clear();

    ids.clear();
    ids << QLatin1String("foo") << QLatin1String("bar");
    contacts = mConn->contacts(ids);
    QCOMPARE(contacts.size(), 2);
    QVERIFY(contacts.contains(mContacts[0]));
    QVERIFY(contacts.contains(mContacts[1]));
}

void TestConnAddressing::testRequest()
{
    commonTestRequest(true);
}

void TestConnAddressing::testRequestNoFeatures()
{
    commonTestRequest(false);
}

void TestConnAddressing::testRequestEmpty()
{
    ConnectionPtr conn = mConn->client();

    PendingContacts *pc = conn->contactManager()->contactsForHandles(UIntList());
    QVERIFY(connect(pc,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pc->isForHandles());
    QVERIFY(pc->handles().isEmpty());
    QVERIFY(!pc->isForIdentifiers());
    QVERIFY(pc->identifiers().isEmpty());
    QVERIFY(!pc->isForUris());
    QVERIFY(pc->uris().isEmpty());
    QVERIFY(!pc->isForVCardAddresses());
    QVERIFY(pc->vcardField().isEmpty());
    QVERIFY(pc->vcardAddresses().isEmpty());
    QVERIFY(!pc->isUpgrade());
    QVERIFY(pc->contactsToUpgrade().isEmpty());
    QVERIFY(pc->contacts().isEmpty());
    QVERIFY(mInvalidHandles.isEmpty());
    QVERIFY(mValidIds.isEmpty());
    QVERIFY(mInvalidIds.isEmpty());
    QVERIFY(mValidUris.isEmpty());
    QVERIFY(mInvalidUris.isEmpty());
    QVERIFY(mValidVCardAddresses.isEmpty());
    QVERIFY(mInvalidVCardAddresses.isEmpty());

    pc = conn->contactManager()->contactsForUris(QStringList());
    QVERIFY(connect(pc,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pc->isForUris());
    QVERIFY(pc->uris().isEmpty());
    QVERIFY(!pc->isForHandles());
    QVERIFY(pc->handles().isEmpty());
    QVERIFY(!pc->isForIdentifiers());
    QVERIFY(pc->identifiers().isEmpty());
    QVERIFY(!pc->isForVCardAddresses());
    QVERIFY(pc->vcardField().isEmpty());
    QVERIFY(pc->vcardAddresses().isEmpty());
    QVERIFY(!pc->isUpgrade());
    QVERIFY(pc->contactsToUpgrade().isEmpty());
    QVERIFY(pc->contacts().isEmpty());
    QVERIFY(mValidUris.isEmpty());
    QVERIFY(mInvalidUris.isEmpty());
    QVERIFY(mInvalidHandles.isEmpty());
    QVERIFY(mValidIds.isEmpty());
    QVERIFY(mInvalidIds.isEmpty());
    QVERIFY(mValidVCardAddresses.isEmpty());
    QVERIFY(mInvalidVCardAddresses.isEmpty());

    pc = conn->contactManager()->contactsForVCardAddresses(QString(),
            QStringList());
    QVERIFY(connect(pc,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pc->isForVCardAddresses());
    QVERIFY(pc->vcardField().isEmpty());
    QVERIFY(pc->vcardAddresses().isEmpty());
    QVERIFY(!pc->isForHandles());
    QVERIFY(pc->handles().isEmpty());
    QVERIFY(!pc->isForIdentifiers());
    QVERIFY(pc->identifiers().isEmpty());
    QVERIFY(!pc->isForUris());
    QVERIFY(pc->uris().isEmpty());
    QVERIFY(!pc->isUpgrade());
    QVERIFY(pc->contactsToUpgrade().isEmpty());
    QVERIFY(pc->contacts().isEmpty());
    QVERIFY(mValidVCardAddresses.isEmpty());
    QVERIFY(mInvalidVCardAddresses.isEmpty());
    QVERIFY(mInvalidHandles.isEmpty());
    QVERIFY(mValidIds.isEmpty());
    QVERIFY(mInvalidIds.isEmpty());
    QVERIFY(mValidUris.isEmpty());
    QVERIFY(mInvalidUris.isEmpty());

    pc = conn->contactManager()->contactsForVCardAddresses(QLatin1String("x-unsupported"),
            QStringList());
    QVERIFY(connect(pc,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pc->isForVCardAddresses());
    QCOMPARE(pc->vcardField(), QLatin1String("x-unsupported"));
    QVERIFY(pc->vcardAddresses().isEmpty());
    QVERIFY(!pc->isForHandles());
    QVERIFY(pc->handles().isEmpty());
    QVERIFY(!pc->isForIdentifiers());
    QVERIFY(pc->identifiers().isEmpty());
    QVERIFY(!pc->isForUris());
    QVERIFY(pc->uris().isEmpty());
    QVERIFY(!pc->isUpgrade());
    QVERIFY(pc->contactsToUpgrade().isEmpty());
    QVERIFY(pc->contacts().isEmpty());
    QVERIFY(mValidVCardAddresses.isEmpty());
    QVERIFY(mInvalidVCardAddresses.isEmpty());
    QVERIFY(mInvalidHandles.isEmpty());
    QVERIFY(mValidIds.isEmpty());
    QVERIFY(mInvalidIds.isEmpty());
    QVERIFY(mValidUris.isEmpty());
    QVERIFY(mInvalidUris.isEmpty());
}

void TestConnAddressing::cleanup()
{
    cleanupImpl();
}

void TestConnAddressing::cleanupTestCase()
{
    if (!mContacts.isEmpty()) {
        mContacts.clear();
    }

    if (mConn) {
        QVERIFY(mConn->disconnect());
        delete mConn;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConnAddressing)
#include "_gen/conn-addressing.cpp.moc.hpp"
