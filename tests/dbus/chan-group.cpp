#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>
#include <telepathy-glib/handle.h>

#include <tests/lib/glib/contactlist/conn.h>
#include <tests/lib/glib/textchan-group.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestChanGroup : public Test
{
    Q_OBJECT

public:
    TestChanGroup(QObject *parent = 0)
        : Test(parent), mConnService(0), mChanService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectEnsureChannelFinished(Tp::PendingOperation *);
    void expectPendingContactsFinished(Tp::PendingOperation *);
    void onGroupMembersChanged(
            const Tp::Contacts &groupMembersAdded,
            const Tp::Contacts &groupLocalPendingMembersAdded,
            const Tp::Contacts &groupRemotePendingMembersAdded,
            const Tp::Contacts &groupMembersRemoved,
            const Tp::Channel::GroupMemberChangeDetails &details);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCreateContacts();
    void testCreateChannel();
    void testMCDGroup();
    void testPropertylessGroup();

    void cleanup();
    void cleanupTestCase();

private:
    void debugContacts();

    void commonTest(gboolean properties);

    QString mConnName, mConnPath;
    ExampleContactListConnection *mConnService;
    TpTestsTextChannelGroup *mChanService;
    ConnectionPtr mConn;
    ChannelPtr mChan;
    QString mChanObjectPath;
    QList<ContactPtr> mContacts;
    Contacts mChangedCurrent;
    Contacts mChangedLP;
    Contacts mChangedRP;
    Contacts mChangedRemoved;
    Channel::GroupMemberChangeDetails mDetails;
    UIntList mInitialMembers;
};

void TestChanGroup::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestChanGroup::expectEnsureChannelFinished(PendingOperation* op)
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

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    mChan = pc->channel();
    mChanObjectPath = mChan->objectPath();
    mLoop->exit(0);
}

void TestChanGroup::expectPendingContactsFinished(PendingOperation *op)
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

void TestChanGroup::onGroupMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    qDebug() << "group members changed";
    mChangedCurrent = groupMembersAdded;
    mChangedLP = groupLocalPendingMembersAdded;
    mChangedRP = groupRemotePendingMembersAdded;
    mChangedRemoved = groupMembersRemoved;
    mDetails = details;
    debugContacts();
    mLoop->exit(0);
}

void TestChanGroup::debugContacts()
{
    qDebug() << "contacts on group:";
    Q_FOREACH (const ContactPtr &contact, mChan->groupContacts()) {
        qDebug() << " " << contact->id();
    }

    qDebug() << "local pending contacts on group:";
    Q_FOREACH (const ContactPtr &contact, mChan->groupLocalPendingContacts()) {
        qDebug() << " " << contact->id();
    }

    qDebug() << "remote pending contacts on group:";
    Q_FOREACH (const ContactPtr &contact, mChan->groupRemotePendingContacts()) {
        qDebug() << " " << contact->id();
    }
}

void TestChanGroup::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("chan-group");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_CONTACT_LIST_CONNECTION(g_object_new(
            EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
            "account", "me@example.com",
            "protocol", "example-contact-list",
            "simulation-delay", 1,
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

    mConn->requestConnect();

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));
}

void TestChanGroup::init()
{
    initImpl();

    mChangedCurrent.clear();
    mChangedLP.clear();
    mChangedRP.clear();
    mChangedRemoved.clear();
    mDetails = Channel::GroupMemberChangeDetails();
}

void TestChanGroup::testCreateContacts()
{
    QStringList ids;
    ids << QLatin1String("sjoerd@example.com");

    // Wait for the contacts to be built
    PendingOperation *op = mConn->contactManager()->contactsForIdentifiers(ids);
    QVERIFY(connect(op,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(op,
                SIGNAL(finished(Tp::PendingOperation*)),
                this,
                SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
}

void TestChanGroup::testCreateChannel()
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_LIST));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeGroup);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"),
                   QLatin1String("Cambridge"));

    QVERIFY(connect(mConn->ensureChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectEnsureChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    QCOMPARE(mChan->isRequested(), false);
    QVERIFY(mChan->groupContacts().contains(mContacts.first()));

    Q_FOREACH (ContactPtr contact, mChan->groupContacts())
        mInitialMembers.push_back(contact->handle()[0]);

    QCOMPARE(mChan->groupCanAddContacts(), true);
    QCOMPARE(mChan->groupCanRemoveContacts(), true);

    debugContacts();

    QCOMPARE(mChan->groupContacts().count(), 4);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));

    QList<ContactPtr> toRemove;
    toRemove.append(mContacts[0]);
    connect(mChan->groupRemoveContacts(toRemove, QLatin1String("I want to remove some of them")),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    while (mChangedRemoved.isEmpty()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QVERIFY(mChangedRemoved.contains(mContacts[0]));

    QCOMPARE(mChan->groupContacts().count(), 3);
}

void TestChanGroup::testMCDGroup()
{
    commonTest(true);
}

void TestChanGroup::testPropertylessGroup()
{
    commonTest(false);
}

void TestChanGroup::commonTest(gboolean properties)
{
    mChanObjectPath = QString(QLatin1String("%1/ChannelForTpQt4MCDTest%2"))
        .arg(mConn->objectPath())
        .arg(QLatin1String(properties ? "props" : ""));
    QByteArray chanPathLatin1(mChanObjectPath.toLatin1());

    mChanService = TP_TESTS_TEXT_CHANNEL_GROUP(g_object_new(
                TP_TESTS_TYPE_TEXT_CHANNEL_GROUP,
                "connection", mConnService,
                "object-path", chanPathLatin1.data(),
                "detailed", TRUE,
                "properties", properties,
                NULL));
    QVERIFY(mChanService != 0);

    TpIntSet *members = tp_intset_sized_new(mInitialMembers.length());
    Q_FOREACH (uint handle, mInitialMembers)
        tp_intset_add(members, handle);

    QVERIFY(tp_group_mixin_change_members(G_OBJECT(mChanService), "be there or be []",
                members, NULL, NULL, NULL, 0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE));

    tp_intset_destroy(members);

    mChan = Channel::create(mConn, mChanObjectPath, QVariantMap());
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    QCOMPARE(mChan->isRequested(), true);
    QVERIFY(mChan->groupContacts().contains(mContacts.first()));

    Q_FOREACH (ContactPtr contact, mChan->groupContacts())
        mInitialMembers.push_back(contact->handle()[0]);

    QCOMPARE(mChan->groupCanAddContacts(), false);
    QCOMPARE(mChan->groupCanRemoveContacts(), false);

    debugContacts();

    QCOMPARE(mChan->groupContacts().count(), 4);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));

    TpIntSet *remove = tp_intset_new_containing(mContacts[0]->handle()[0]);

    QVERIFY(tp_group_mixin_change_members(G_OBJECT(mChanService), "be a []",
                NULL, remove, NULL, NULL, 0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE));

    tp_intset_destroy(remove);

    while (mChangedRemoved.isEmpty()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QVERIFY(mChangedRemoved.contains(mContacts[0]));

    QCOMPARE(mChan->groupContacts().count(), 3);
}

void TestChanGroup::cleanup()
{
    if (mChanService) {
        g_object_unref(mChanService);
        mChanService = 0;
    }

    cleanupImpl();
}

void TestChanGroup::cleanupTestCase()
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

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestChanGroup)
#include "_gen/chan-group.cpp.moc.hpp"
