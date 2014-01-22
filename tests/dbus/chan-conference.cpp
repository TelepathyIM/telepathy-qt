#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo/chan.h>
#include <tests/lib/glib/echo/conn.h>
#include <tests/lib/glib/future/conference/chan.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingReady>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestConferenceChan : public Test
{
    Q_OBJECT

public:
    TestConferenceChan(QObject *parent = 0)
        : Test(parent),
          mConn(0), mContactRepo(0),
          mTextChan1Service(0), mTextChan2Service(0), mConferenceChanService(0)
    { }

protected Q_SLOTS:
    void onConferenceChannelMerged(const Tp::ChannelPtr &);
    void onConferenceChannelRemoved(const Tp::ChannelPtr &channel,
            const Tp::Channel::GroupMemberChangeDetails &details);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testConference();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    TpHandleRepoIface *mContactRepo;

    ChannelPtr mChan;
    QString mTextChan1Path;
    ExampleEchoChannel *mTextChan1Service;
    QString mTextChan2Path;
    ExampleEchoChannel *mTextChan2Service;
    QString mTextChan3Path;
    ExampleEchoChannel *mTextChan3Service;
    QString mConferenceChanPath;
    TpTestsConferenceChannel *mConferenceChanService;

    ChannelPtr mChannelMerged;
    ChannelPtr mChannelRemovedDetailed;
    Channel::GroupMemberChangeDetails mChannelRemovedDetailedDetails;
};

void TestConferenceChan::onConferenceChannelMerged(const Tp::ChannelPtr &channel)
{
    mChannelMerged = channel;
    mLoop->exit(0);
}

void TestConferenceChan::onConferenceChannelRemoved(const Tp::ChannelPtr &channel,
        const Tp::Channel::GroupMemberChangeDetails &details)
{
    mChannelRemovedDetailed = channel;
    mChannelRemovedDetailedDetails = details;
    mLoop->exit(0);
}

void TestConferenceChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("chan-conference");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);

    // create a Channel by magic, rather than doing D-Bus round-trips for it
    mContactRepo = tp_base_connection_get_handles(TP_BASE_CONNECTION(mConn->service()),
            TP_HANDLE_TYPE_CONTACT);
    guint handle1 = tp_handle_ensure(mContactRepo, "someone1@localhost", 0, 0);
    guint handle2 = tp_handle_ensure(mContactRepo, "someone2@localhost", 0, 0);
    guint handle3 = tp_handle_ensure(mContactRepo, "someone3@localhost", 0, 0);

    QByteArray chanPath;
    GPtrArray *initialChannels = g_ptr_array_new();

    mTextChan1Path = mConn->objectPath() + QLatin1String("/TextChannel/1");
    chanPath = mTextChan1Path.toLatin1();
    mTextChan1Service = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle1,
                NULL));
    g_ptr_array_add(initialChannels, g_strdup(chanPath.data()));

    mTextChan2Path = mConn->objectPath() + QLatin1String("/TextChannel/2");
    chanPath = mTextChan2Path.toLatin1();
    mTextChan2Service = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle2,
                NULL));
    g_ptr_array_add(initialChannels, g_strdup(chanPath.data()));

    // let's not add this one to initial channels
    mTextChan3Path = mConn->objectPath() + QLatin1String("/TextChannel/3");
    chanPath = mTextChan3Path.toLatin1();
    mTextChan3Service = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle3,
                NULL));

    mConferenceChanPath = mConn->objectPath() + QLatin1String("/ConferenceChannel");
    chanPath = mConferenceChanPath.toLatin1();
    mConferenceChanService = TP_TESTS_CONFERENCE_CHANNEL(g_object_new(
                TP_TESTS_TYPE_CONFERENCE_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "initial-channels", initialChannels,
                NULL));

    g_ptr_array_foreach(initialChannels, (GFunc) g_free, NULL);
    g_ptr_array_free(initialChannels, TRUE);
}

void TestConferenceChan::init()
{
    initImpl();
}

void TestConferenceChan::testConference()
{
    mChan = Channel::create(mConn->client(), mConferenceChanPath, QVariantMap());
    QCOMPARE(mChan->isConference(), false);
    QVERIFY(mChan->conferenceInitialInviteeContacts().isEmpty());
    QVERIFY(mChan->conferenceChannels().isEmpty());
    QVERIFY(mChan->conferenceInitialChannels().isEmpty());
    QVERIFY(mChan->conferenceOriginalChannels().isEmpty());
    QCOMPARE(mChan->supportsConferenceMerging(), false);
    QCOMPARE(mChan->supportsConferenceSplitting(), false);

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    QStringList expectedObjectPaths;
    expectedObjectPaths << mTextChan1Path << mTextChan2Path;

    QStringList objectPaths;
    Q_FOREACH (const ChannelPtr &channel, mChan->conferenceInitialChannels()) {
        objectPaths << channel->objectPath();
    }
    QCOMPARE(expectedObjectPaths, objectPaths);

    objectPaths.clear();
    Q_FOREACH (const ChannelPtr &channel, mChan->conferenceChannels()) {
        objectPaths << channel->objectPath();
    }
    QCOMPARE(expectedObjectPaths, objectPaths);

    /*
    // TODO - Properly check for initial invitee contacts if/when a test CM supports it
    QVERIFY(!mChan->isReady(Channel::FeatureConferenceInitialInviteeContacts));
    QCOMPARE(mChan->conferenceInitialInviteeContacts(), Contacts());

    QVERIFY(connect(mChan->becomeReady(Channel::FeatureConferenceInitialInviteeContacts),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(Channel::FeatureConferenceInitialInviteeContacts), true);

    QCOMPARE(mChan->conferenceInitialInviteeContacts(), Contacts());
    */

    QCOMPARE(mChan->supportsConferenceMerging(), true);
    QCOMPARE(mChan->supportsConferenceSplitting(), false);
    QVERIFY(connect(mChan->conferenceSplitChannel(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
    QVERIFY(!mLastErrorMessage.isEmpty());

    ChannelPtr otherChannel = Channel::create(mConn->client(), mTextChan3Path, QVariantMap());

    QVERIFY(connect(mChan.data(),
                    SIGNAL(conferenceChannelMerged(const Tp::ChannelPtr &)),
                    SLOT(onConferenceChannelMerged(const Tp::ChannelPtr &))));
    QVERIFY(connect(mChan->conferenceMergeChannel(otherChannel),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);
    while (!mChannelMerged) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mChannelMerged->objectPath(), otherChannel->objectPath());

    expectedObjectPaths << mTextChan3Path;
    objectPaths.clear();
    Q_FOREACH (const ChannelPtr &channel, mChan->conferenceChannels()) {
        objectPaths << channel->objectPath();
    }
    QCOMPARE(expectedObjectPaths, objectPaths);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(conferenceChannelRemoved(const Tp::ChannelPtr &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onConferenceChannelRemoved(const Tp::ChannelPtr &,
                            const Tp::Channel::GroupMemberChangeDetails &))));
    tp_tests_conference_channel_remove_channel (mConferenceChanService,
            mChannelMerged->objectPath().toLatin1().data());
    while (!mChannelRemovedDetailed) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mChannelRemovedDetailed, mChannelMerged);
    QCOMPARE(mChannelRemovedDetailedDetails.allDetails().isEmpty(), false);
    QCOMPARE(qdbus_cast<int>(mChannelRemovedDetailedDetails.allDetails().value(
                    QLatin1String("domain-specific-detail-uint"))), 3);
    QCOMPARE(mChannelRemovedDetailedDetails.hasActor(), true);
    QCOMPARE(mChannelRemovedDetailedDetails.actor(), mChan->groupSelfContact());

    expectedObjectPaths.clear();
    expectedObjectPaths << mTextChan1Path << mTextChan2Path;
    objectPaths.clear();
    Q_FOREACH (const ChannelPtr &channel, mChan->conferenceChannels()) {
        objectPaths << channel->objectPath();
    }
    QCOMPARE(expectedObjectPaths, objectPaths);

    mChan.reset();
    mChannelMerged.reset();
}

void TestConferenceChan::cleanup()
{
    cleanupImpl();
}

void TestConferenceChan::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    if (mTextChan1Service != 0) {
        g_object_unref(mTextChan1Service);
        mTextChan1Service = 0;
    }

    if (mTextChan2Service != 0) {
        g_object_unref(mTextChan2Service);
        mTextChan2Service = 0;
    }

    if (mConferenceChanService != 0) {
        g_object_unref(mConferenceChanService);
        mConferenceChanService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConferenceChan)
#include "_gen/chan-conference.cpp.moc.hpp"
