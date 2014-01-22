#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>
#include <tests/lib/glib/textchan-null.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/TextChannel>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestChanBasics : public Test
{
    Q_OBJECT

public:
    TestChanBasics(QObject *parent = 0)
        : Test(parent), mConn(0), mHandle(0)
    { }

protected Q_SLOTS:
    void expectInvalidated();
    void expectPendingHandleFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestHandle();
    void testCreateChannel();
    void testEnsureChannel();
    void testFallback();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    ChannelPtr mChan;
    QString mChanObjectPath;
    uint mHandle;
};

void TestChanBasics::expectInvalidated()
{
    mLoop->exit(0);
}

void TestChanBasics::expectPendingHandleFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingHandles *pending = qobject_cast<PendingHandles*>(op);
    mHandle = pending->handles().at(0);
    mLoop->exit(0);
}

void TestChanBasics::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("chan-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            NULL);
    QCOMPARE(mConn->connect(), true);

    QCOMPARE(mConn->enableFeatures(Connection::FeatureSelfContact), true);
}

void TestChanBasics::init()
{
    initImpl();

    mChan.reset();
}

void TestChanBasics::testRequestHandle()
{
    // Test identifiers
    QStringList ids = QStringList() << QLatin1String("alice");

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->client()->lowlevel()->requestHandles(Tp::HandleTypeContact, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingHandleFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Tp::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandleFinished(Tp::PendingOperation*))));
    QVERIFY(mHandle != 0);
}

void TestChanBasics::testCreateChannel()
{
    mChan = mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT, Tp::HandleTypeContact, mHandle);
    QVERIFY(mChan);
    mChanObjectPath = mChan->objectPath();

    QVERIFY(connect(mChan->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);
    QCOMPARE(mChan->isRequested(), true);
    QCOMPARE(mChan->channelType(), TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    QCOMPARE(mChan->groupCanAddContacts(), false);
    QCOMPARE(mChan->groupCanRemoveContacts(), false);
    QCOMPARE(mChan->initiatorContact()->id(), QString(QLatin1String("me@example.com")));
    QCOMPARE(mChan->groupSelfContact()->id(), QString(QLatin1String("me@example.com")));
    QCOMPARE(mChan->groupSelfContact(), mConn->client()->selfContact());
    QCOMPARE(mChan->targetId(), QString::fromLatin1("alice"));
    QVERIFY(!mChan->targetContact().isNull());
    QCOMPARE(mChan->targetContact()->id(), QString::fromLatin1("alice"));

    QStringList ids;
    Q_FOREACH (const ContactPtr &contact, mChan->groupContacts()) {
        ids << contact->id();

        QVERIFY(contact == mChan->groupSelfContact() || contact == mChan->targetContact());
    }
    ids.sort();
    QStringList toCheck = QStringList() << QLatin1String("me@example.com")
        << QLatin1String("alice");
    toCheck.sort();
    QCOMPARE(ids, toCheck);

    ChannelPtr chan = Channel::create(mConn->client(), mChan->objectPath(),
            mChan->immutableProperties());
    QVERIFY(chan);
    QVERIFY(chan->isValid());
    QVERIFY(connect(chan->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(chan->isReady(), true);
    QCOMPARE(chan->channelType(), TP_QT_IFACE_CHANNEL_TYPE_TEXT);

    // create an invalid connection to use as the channel connection
    ConnectionPtr conn = Connection::create(QLatin1String(""), QLatin1String("/"),
                  ChannelFactory::create(QDBusConnection::sessionBus()),
                  ContactFactory::create());
    QVERIFY(conn);
    QVERIFY(!conn->isValid());

    chan = Channel::create(conn, mChan->objectPath(),
            mChan->immutableProperties());
    QVERIFY(chan);
    QVERIFY(!chan->isValid());
    QVERIFY(connect(chan->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, chan->invalidationReason());
    QCOMPARE(mLastErrorMessage, chan->invalidationMessage());
    QCOMPARE(chan->channelType(), QString());
}

void TestChanBasics::testEnsureChannel()
{
    ChannelPtr chan = mConn->ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT,
            Tp::HandleTypeContact, mHandle);
    QVERIFY(chan);
    QCOMPARE(chan->objectPath(), mChanObjectPath);
    mChan = chan;

    QVERIFY(connect(mChan->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);
    QCOMPARE(mChan->isRequested(), true);
    QCOMPARE(mChan->groupCanAddContacts(), false);
    QCOMPARE(mChan->groupCanRemoveContacts(), false);
    QCOMPARE(mChan->initiatorContact()->id(), QString(QLatin1String("me@example.com")));
    QCOMPARE(mChan->groupSelfContact()->id(), QString(QLatin1String("me@example.com")));
    QCOMPARE(mChan->groupSelfContact(), mConn->client()->selfContact());

    QStringList ids;
    Q_FOREACH (const ContactPtr &contact, mChan->groupContacts()) {
        ids << contact->id();
    }
    ids.sort();
    QStringList toCheck = QStringList() << QLatin1String("me@example.com")
        << QLatin1String("alice");
    toCheck.sort();
    QCOMPARE(ids, toCheck);

    QVERIFY(connect(mChan->requestClose(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isValid(), false);

    // calling requestClose again should be no-op
    QVERIFY(connect(mChan->requestClose(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isValid(), false);
}

void TestChanBasics::testFallback()
{
    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()),
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(contactRepo, "someone@localhost", 0, 0);

    QString textChanPath = mConn->objectPath() + QLatin1String("/Channel");
    QByteArray chanPath(textChanPath.toLatin1());

    TpTestsTextChannelNull *textChanService = TP_TESTS_TEXT_CHANNEL_NULL (g_object_new (
                TP_TESTS_TYPE_TEXT_CHANNEL_NULL,
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

    QCOMPARE(textChanService->get_channel_type_called, static_cast<uint>(1));
    QCOMPARE(textChanService->get_interfaces_called, static_cast<uint>(1));
    QCOMPARE(textChanService->get_handle_called, static_cast<uint>(1));

    QCOMPARE(textChan->channelType(), TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    QVERIFY(textChan->interfaces().isEmpty());
    QCOMPARE(textChan->targetHandleType(), Tp::HandleTypeContact);
    QCOMPARE(textChan->targetHandle(), handle);

    // we have no Group support, groupAddContacts should fail
    QVERIFY(connect(textChan->groupAddContacts(QList<ContactPtr>() << mConn->client()->selfContact()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
    QVERIFY(!mLastErrorMessage.isEmpty());
}

void TestChanBasics::cleanup()
{
    cleanupImpl();
}

void TestChanBasics::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    if (mChan) {
        QVERIFY(!mChan->isValid());
        QVERIFY(mChan->invalidationReason() == TP_QT_ERROR_CANCELLED ||
                mChan->invalidationReason() == TP_QT_ERROR_ORPHANED);
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestChanBasics)
#include "_gen/chan-basics.cpp.moc.hpp"
