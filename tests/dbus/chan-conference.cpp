#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Message>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReceivedMessage>
#include <TelepathyQt4/TextChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/echo/chan.h>
#include <tests/lib/glib/echo/conn.h>
#include <tests/lib/glib/future/conference/chan.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestConferenceChan : public Test
{
    Q_OBJECT

public:
    TestConferenceChan(QObject *parent = 0)
        : Test(parent),
          mConnService(0), mBaseConnService(0), mContactRepo(0),
          mTextChan1Service(0), mTextChan2Service(0), mConferenceChanService(0)
    { }

protected Q_SLOTS:
    void onConferenceChannelMerged(const Tp::ChannelPtr &);
    // void onConferenceChannelRemoved(const Tp::ChannelPtr &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testConference();

    void cleanup();
    void cleanupTestCase();

private:
    ExampleEchoConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;

    QString mConnName;
    QString mConnPath;
    ConnectionPtr mConn;
    ChannelPtr mChan;

    QString mTextChan1Path;
    ExampleEchoChannel *mTextChan1Service;
    QString mTextChan2Path;
    ExampleEchoChannel *mTextChan2Service;
    QString mTextChan3Path;
    ExampleEchoChannel *mTextChan3Service;
    QString mConferenceChanPath;
    ExampleConferenceChannel *mConferenceChanService;

    ChannelPtr mChannelMerged;
};

void TestConferenceChan::onConferenceChannelMerged(const Tp::ChannelPtr &channel)
{
    mChannelMerged = channel;
    mLoop->exit(0);
}

void TestConferenceChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conference-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_ECHO_CONNECTION(g_object_new(
            EXAMPLE_TYPE_ECHO_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL));
    QVERIFY(mConnService != 0);
    mBaseConnService = TP_BASE_CONNECTION(mConnService);
    QVERIFY(mBaseConnService != 0);

    QVERIFY(tp_base_connection_register(mBaseConnService,
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));

    // create a Channel by magic, rather than doing D-Bus round-trips for it

    mContactRepo = tp_base_connection_get_handles(mBaseConnService,
            TP_HANDLE_TYPE_CONTACT);
    guint handle1 = tp_handle_ensure(mContactRepo, "someone1@localhost", 0, 0);
    guint handle2 = tp_handle_ensure(mContactRepo, "someone2@localhost", 0, 0);
    guint handle3 = tp_handle_ensure(mContactRepo, "someone3@localhost", 0, 0);

    QByteArray chanPath;
    GPtrArray *initialChannels = g_ptr_array_new();

    mTextChan1Path = mConnPath + QLatin1String("/TextChannel/1");
    chanPath = mTextChan1Path.toAscii();
    mTextChan1Service = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle1,
                NULL));
    g_ptr_array_add(initialChannels, g_strdup(chanPath.data()));

    mTextChan2Path = mConnPath + QLatin1String("/TextChannel/2");
    chanPath = mTextChan2Path.toAscii();
    mTextChan2Service = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle2,
                NULL));
    g_ptr_array_add(initialChannels, g_strdup(chanPath.data()));

    // let's not add this one to initial channels
    mTextChan3Path = mConnPath + QLatin1String("/TextChannel/3");
    chanPath = mTextChan3Path.toAscii();
    mTextChan3Service = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle3,
                NULL));

    mConferenceChanPath = mConnPath + QLatin1String("/ConferenceChannel");
    chanPath = mConferenceChanPath.toAscii();
    mConferenceChanService = EXAMPLE_CONFERENCE_CHANNEL(g_object_new(
                EXAMPLE_TYPE_CONFERENCE_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "initial-channels", initialChannels,
                NULL));

    tp_handle_unref(mContactRepo, handle1);
    tp_handle_unref(mContactRepo, handle2);
    tp_handle_unref(mContactRepo, handle3);
}

void TestConferenceChan::init()
{
    initImpl();
}

void TestConferenceChan::testConference()
{
    mChan = Channel::create(mConn, mConferenceChanPath, QVariantMap());
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

    QCOMPARE(mChan->conferenceInitialInviteeContacts(), Contacts());

    QCOMPARE(mChan->hasConferenceInterface(), true);
    QCOMPARE(mChan->hasMergeableConferenceInterface(), true);

    ChannelPtr otherChannel = Channel::create(mConn, mTextChan3Path, QVariantMap());

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

    mChan.reset();
    mChannelMerged.reset();
}

void TestConferenceChan::cleanup()
{
    cleanupImpl();
}

void TestConferenceChan::cleanupTestCase()
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
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

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

    if (mConnService != 0) {
        mBaseConnService = 0;
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestConferenceChan)
#include "_gen/chan-conference.cpp.moc.hpp"
