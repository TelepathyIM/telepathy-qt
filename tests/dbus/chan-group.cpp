#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contactlist/conn.h>
#include <tests/lib/glib/textchan-group.h>
#include <tests/lib/glib/textchan-null.h>

#include <TelepathyQt/Channel>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/TextChannel>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestChanGroup : public Test
{
    Q_OBJECT

public:
    TestChanGroup(QObject *parent = 0)
        : Test(parent), mConn(0), mChanService(0),
          mGotGroupFlagsChanged(false),
          mGroupFlags((ChannelGroupFlags) 0),
          mGroupFlagsAdded((ChannelGroupFlags) 0),
          mGroupFlagsRemoved((ChannelGroupFlags) 0)
    { }

protected Q_SLOTS:
    void onGroupMembersChanged(
            const Tp::Contacts &groupMembersAdded,
            const Tp::Contacts &groupLocalPendingMembersAdded,
            const Tp::Contacts &groupRemotePendingMembersAdded,
            const Tp::Contacts &groupMembersRemoved,
            const Tp::Channel::GroupMemberChangeDetails &details);
    void onGroupFlagsChanged(Tp::ChannelGroupFlags flags,
            Tp::ChannelGroupFlags added, Tp::ChannelGroupFlags removed);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCreateChannel();
    void testMCDGroup();
    void testPropertylessGroup();
    void testLeave();
    void testLeaveWithFallback();
    void testGroupFlagsChange();

    void cleanup();
    void cleanupTestCase();

private:
    void debugContacts();

    void commonTest(gboolean properties);

    TestConnHelper *mConn;
    TpTestsTextChannelGroup *mChanService;
    ChannelPtr mChan;
    QString mChanObjectPath;
    QList<ContactPtr> mContacts;
    Contacts mChangedCurrent;
    Contacts mChangedLP;
    Contacts mChangedRP;
    Contacts mChangedRemoved;
    Channel::GroupMemberChangeDetails mDetails;
    UIntList mInitialMembers;
    bool mGotGroupFlagsChanged;
    ChannelGroupFlags mGroupFlags;
    ChannelGroupFlags mGroupFlagsAdded;
    ChannelGroupFlags mGroupFlagsRemoved;
};

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

void TestChanGroup::onGroupFlagsChanged(Tp::ChannelGroupFlags flags,
        Tp::ChannelGroupFlags added, Tp::ChannelGroupFlags removed)
{
    qDebug() << "group flags changed";
    mGotGroupFlagsChanged = true;
    mGroupFlags = flags;
    mGroupFlagsAdded = added;
    mGroupFlagsRemoved = removed;
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

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_CONTACT_LIST_CONNECTION,
            "account", "me@example.com",
            "protocol", "example-contact-list",
            "simulation-delay", 1,
            NULL);
    QCOMPARE(mConn->connect(Connection::FeatureSelfContact), true);

    QStringList ids;
    ids << QLatin1String("sjoerd@example.com");

    // Check that the contact is properly built
    mContacts = mConn->contacts(ids);
    QCOMPARE(mContacts.size(), 1);
    QVERIFY(!mContacts.first().isNull());
}

void TestChanGroup::init()
{
    initImpl();

    mChangedCurrent.clear();
    mChangedLP.clear();
    mChangedRP.clear();
    mChangedRemoved.clear();
    mDetails = Channel::GroupMemberChangeDetails();
    mGotGroupFlagsChanged = false;
    mGroupFlags = (ChannelGroupFlags) 0;
    mGroupFlagsAdded = (ChannelGroupFlags) 0;
    mGroupFlagsRemoved = (ChannelGroupFlags) 0;
}

void TestChanGroup::testCreateChannel()
{
    mChan = mConn->ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_CONTACT_LIST,
            Tp::HandleTypeGroup, QLatin1String("Cambridge"));
    QVERIFY(mChan);
    mChanObjectPath = mChan->objectPath();

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    QCOMPARE(static_cast<uint>(mChan->targetHandleType()),
             static_cast<uint>(Tp::HandleTypeGroup));
    QVERIFY(mChan->targetContact().isNull());

    QCOMPARE(mChan->isRequested(), false);
    QVERIFY(mChan->groupContacts().contains(mContacts.first()));

    Q_FOREACH (ContactPtr contact, mChan->groupContacts())
        mInitialMembers.push_back(contact->handle()[0]);

    QCOMPARE(mChan->groupFlags(), ChannelGroupFlagCanAdd | ChannelGroupFlagCanRemove |
            ChannelGroupFlagProperties | ChannelGroupFlagMembersChangedDetailed);

    QCOMPARE(mChan->groupCanAddContacts(), true);
    QCOMPARE(mChan->groupCanAddContactsWithMessage(), false);
    QCOMPARE(mChan->groupCanAcceptContactsWithMessage(), false);
    QCOMPARE(mChan->groupCanRescindContacts(), false);
    QCOMPARE(mChan->groupCanRescindContactsWithMessage(), false);
    QCOMPARE(mChan->groupCanRemoveContacts(), true);
    QCOMPARE(mChan->groupCanRemoveContactsWithMessage(), false);
    QCOMPARE(mChan->groupCanRejectContactsWithMessage(), false);
    QCOMPARE(mChan->groupCanDepartWithMessage(), false);

    QCOMPARE(mChan->groupIsSelfContactTracked(), true);

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
    mChanObjectPath = QString(QLatin1String("%1/ChannelForTpQtMCDTest%2"))
        .arg(mConn->objectPath())
        .arg(QLatin1String(properties ? "props" : ""));
    QByteArray chanPathLatin1(mChanObjectPath.toLatin1());

    mChanService = TP_TESTS_TEXT_CHANNEL_GROUP(g_object_new(
                TP_TESTS_TYPE_TEXT_CHANNEL_GROUP,
                "connection", mConn->service(),
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

    mChan = Channel::create(mConn->client(), mChanObjectPath, QVariantMap());
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

void TestChanGroup::testLeave()
{
    mChan = mConn->ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_CONTACT_LIST,
            Tp::HandleTypeGroup, QLatin1String("Cambridge"));
    QVERIFY(mChan);
    mChanObjectPath = mChan->objectPath();

    // channel is not ready yet, it should fail
    QVERIFY(connect(mChan->groupAddContacts(QList<ContactPtr>() << mConn->client()->selfContact()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_AVAILABLE);
    QVERIFY(!mLastErrorMessage.isEmpty());

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    // passing no contact should also fail
    QVERIFY(connect(mChan->groupAddContacts(QList<ContactPtr>()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_INVALID_ARGUMENT);
    QVERIFY(!mLastErrorMessage.isEmpty());

    // passing an invalid contact too
    QVERIFY(connect(mChan->groupAddContacts(QList<ContactPtr>() << ContactPtr()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_INVALID_ARGUMENT);
    QVERIFY(!mLastErrorMessage.isEmpty());

    // now it should work
    QVERIFY(connect(mChan->groupAddContacts(QList<ContactPtr>() << mConn->client()->selfContact()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));

    QString leaveMessage(QLatin1String("I'm sick of this lot"));
    QVERIFY(connect(mChan->requestLeave(leaveMessage),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mChan->groupContacts().contains(mConn->client()->selfContact()));

    // We left gracefully, which we have details for.
    // Unfortunately, the test CM used here ignores the message and the reason specified, so can't
    // verify those. When the leave code was originally written however, it was able to carry out
    // the almost-impossible mission of delivering the message and reason to the CM admirably, as
    // verified by dbus-monitor.
    QVERIFY(mChan->groupSelfContactRemoveInfo().isValid());
    QVERIFY(mChan->groupSelfContactRemoveInfo().hasActor());
    QVERIFY(mChan->groupSelfContactRemoveInfo().actor() == mConn->client()->selfContact());

    // Another leave should no-op succeed, because we aren't a member
    QVERIFY(connect(mChan->requestLeave(leaveMessage),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestChanGroup::testLeaveWithFallback()
{
    mChanObjectPath = QString(QLatin1String("%1/ChannelForTpQtLeaveTestFallback"))
        .arg(mConn->objectPath());
    QByteArray chanPathLatin1(mChanObjectPath.toLatin1());

    // The text channel doesn't support removing, so will trigger the fallback
    mChanService = TP_TESTS_TEXT_CHANNEL_GROUP(g_object_new(
                TP_TESTS_TYPE_TEXT_CHANNEL_GROUP,
                "connection", mConn->service(),
                "object-path", chanPathLatin1.data(),
                "detailed", TRUE,
                "properties", TRUE,
                NULL));
    QVERIFY(mChanService != 0);

    TpIntSet *members = tp_intset_sized_new(1);
    tp_intset_add(members, mConn->client()->selfHandle());

    QVERIFY(tp_group_mixin_change_members(G_OBJECT(mChanService), "be there or be []",
                members, NULL, NULL, NULL, 0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE));

    tp_intset_destroy(members);

    mChan = Channel::create(mConn->client(), mChanObjectPath, QVariantMap());
    QVERIFY(mChan);

    // Should fail, because not ready (and thus we can't know how to leave)
    QVERIFY(connect(mChan->requestLeave(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    // Should now work
    QString leaveMessage(QLatin1String("I'm sick of this lot"));
    QVERIFY(connect(mChan->requestLeave(leaveMessage),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // The Close fallback was triggered, so we weren't removed gracefully and the details were
    // lost
    QVERIFY(!mChan->groupSelfContactRemoveInfo().hasMessage());
}

void TestChanGroup::testGroupFlagsChange()
{
    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()),
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(contactRepo, "someone@localhost", 0, 0);

    QString textChanPath = mConn->objectPath() + QLatin1String("/Channel");
    QByteArray chanPath(textChanPath.toLatin1());

    TpTestsPropsGroupTextChannel *textChanService = TP_TESTS_PROPS_GROUP_TEXT_CHANNEL(g_object_new(
                TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    TextChannelPtr textChan = TextChannel::create(mConn->client(), textChanPath, QVariantMap());
    QVERIFY(connect(textChan->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(textChan->isReady(), true);

    QVERIFY(textChan->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP));
    QVERIFY(!(textChan->groupFlags() & ChannelGroupFlagCanAdd));
    QVERIFY(!textChan->canInviteContacts());

    QVERIFY(connect(textChan.data(),
                SIGNAL(groupFlagsChanged(Tp::ChannelGroupFlags,Tp::ChannelGroupFlags,Tp::ChannelGroupFlags)),
                SLOT(onGroupFlagsChanged(Tp::ChannelGroupFlags,Tp::ChannelGroupFlags,Tp::ChannelGroupFlags))));
    tp_group_mixin_change_flags(G_OBJECT(textChanService),
            TP_CHANNEL_GROUP_FLAG_CAN_ADD, (TpChannelGroupFlags) 0);
    processDBusQueue(mConn->client().data());
    while (!mGotGroupFlagsChanged) {
        mLoop->processEvents();
    }
    QCOMPARE(textChan->groupFlags(), mGroupFlags);
    QVERIFY(textChan->groupFlags() & ChannelGroupFlagCanAdd);
    QVERIFY(textChan->canInviteContacts());
    QCOMPARE(mGroupFlagsAdded, ChannelGroupFlagCanAdd);
    QCOMPARE(mGroupFlagsRemoved, (ChannelGroupFlags) 0);
}

void TestChanGroup::cleanup()
{
    if (mChanService) {
        g_object_unref(mChanService);
        mChanService = 0;
    }

    // Avoid D-Bus event leak from one test case to another - I've seen this with the
    // testCreateChannel groupMembersChanged leaking at least
    processDBusQueue(mConn->client().data());
    mChan.reset();

    cleanupImpl();
}

void TestChanGroup::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestChanGroup)
#include "_gen/chan-group.cpp.moc.hpp"
