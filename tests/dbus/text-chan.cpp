#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingReadyChannel>
#include <TelepathyQt4/Client/TextChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/simple-conn.h>
#include <tests/lib/echo/chan.h>
#include <tests/lib/echo2/chan.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestTextChan : public Test
{
    Q_OBJECT

public:
    TestTextChan(QObject *parent = 0)
        : Test(parent),
          // service side (telepathy-glib)
          mConnService(0), mBaseConnService(0), mContactRepo(0),
            mTextChanService(0), mMessagesChanService(0),
          // client side (Telepathy-Qt4)
          mConn(0), mChan(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testMessages();
    void testLegacyText();

    void cleanup();
    void cleanupTestCase();

private:
    SimpleConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;
    ExampleEchoChannel *mTextChanService;
    ExampleEcho2Channel *mMessagesChanService;

    Connection *mConn;
    TextChannel *mChan;
    QString mTextChanPath;
    QString mMessagesChanPath;
    QString mConnName;
    QString mConnPath;
};

void TestTextChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("text-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = SIMPLE_CONNECTION(g_object_new(
            SIMPLE_TYPE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            0));
    QVERIFY(mConnService != 0);
    mBaseConnService = TP_BASE_CONNECTION(mConnService);
    QVERIFY(mBaseConnService != 0);

    QVERIFY(tp_base_connection_register(mBaseConnService,
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QString::fromAscii(name);
    mConnPath = QString::fromAscii(connPath);

    g_free(name);
    g_free(connPath);

    mConn = new Connection(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
            static_cast<uint>(Connection::StatusConnected));

    // create a Channel by magic, rather than doing D-Bus round-trips for it

    mContactRepo = tp_base_connection_get_handles(mBaseConnService,
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(mContactRepo, "someone@localhost", 0, 0);

    mTextChanPath = mConnPath + QLatin1String("/TextChannel");
    QByteArray chanPath(mTextChanPath.toAscii());
    mTextChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mMessagesChanPath = mConnPath + QLatin1String("/MessagesChannel");
    chanPath = mMessagesChanPath.toAscii();
    mMessagesChanService = EXAMPLE_ECHO_2_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_2_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    tp_handle_unref(mContactRepo, handle);
}

void TestTextChan::init()
{
    initImpl();

    mChan = 0;
}

void TestTextChan::testMessages()
{
    mChan = new TextChannel(mConn, mMessagesChanPath, QVariantMap(), this);
    Channel *asChannel = mChan;

    QVERIFY(connect(asChannel->becomeReady(),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());

    QVERIFY(!mChan->isReady(0, TextChannel::FeatureMessageQueue));
    QVERIFY(!mChan->isReady(0, TextChannel::FeatureMessageCapabilities));

    // we get Sent signals even if FeatureMessageQueue is disabled (starting
    // from the point at which we become Ready)
    //
    // FIXME: there's no high-level API for Send() yet

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            mChan->textInterface()->Send(
                Telepathy::ChannelTextMessageTypeNormal,
                QLatin1String("Hello, world!")));
    QVERIFY(connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
    QCOMPARE(mLoop->exec(), 0);
    delete watcher;

    // FIXME: when the messageSent signal is implemented, assert that we got
    // it and that the message looks like we expect

    // Make capabilities become ready
    QVERIFY(connect(mChan->becomeReady(0, TextChannel::FeatureMessageCapabilities),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    QVERIFY(mChan->isReady(0, TextChannel::FeatureMessageCapabilities));
    QVERIFY(!mChan->isReady(0, TextChannel::FeatureMessageQueue));

    QCOMPARE(mChan->supportedContentTypes(), QStringList() << "*/*");
    QCOMPARE(static_cast<uint>(mChan->messagePartSupport()),
            static_cast<uint>(Telepathy::MessagePartSupportFlagOneAttachment |
                Telepathy::MessagePartSupportFlagMultipleAttachments));
    QCOMPARE(static_cast<uint>(mChan->deliveryReportingSupport()), 0U);

    // Make the message queue become ready too
    QVERIFY(connect(mChan->becomeReady(0, TextChannel::FeatureMessageQueue),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    QVERIFY(mChan->isReady(0, TextChannel::FeatureMessageQueue));
    QVERIFY(mChan->isReady(0, TextChannel::FeatureMessageCapabilities));

    // FIXME: when the message queue is implemented, assert that it contains
    // our "hello world" message
}

void TestTextChan::testLegacyText()
{
    mChan = new TextChannel(mConn, mTextChanPath, QVariantMap(), this);

    QVERIFY(connect(mChan->becomeReady(),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan->isReady());
    QVERIFY(!mChan->isReady(0, TextChannel::FeatureMessageQueue));
    // implementation detail: legacy text channels get capabilities as soon
    // as the Channel basics are ready

    // we get Sent signals even if FeatureMessageQueue is disabled (starting
    // from the point at which we become Ready)
    //
    // FIXME: there's no high-level API for Send() yet

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            mChan->textInterface()->Send(
                Telepathy::ChannelTextMessageTypeNormal,
                QLatin1String("Hello, world!")));
    QVERIFY(connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
    QCOMPARE(mLoop->exec(), 0);
    delete watcher;

    // FIXME: when the messageSent signal is implemented, assert that we got
    // it and that the message looks like we expect

    // Make capabilities become ready
    QVERIFY(connect(mChan->becomeReady(0, TextChannel::FeatureMessageCapabilities),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan->isReady());
    QVERIFY(mChan->isReady(0, TextChannel::FeatureMessageCapabilities));
    QVERIFY(!mChan->isReady(0, TextChannel::FeatureMessageQueue));

    QCOMPARE(mChan->supportedContentTypes(), QStringList() << "text/plain");
    QCOMPARE(static_cast<uint>(mChan->messagePartSupport()), 0U);
    QCOMPARE(static_cast<uint>(mChan->deliveryReportingSupport()), 0U);

    // Make the message queue become ready too
    QVERIFY(connect(mChan->becomeReady(0, TextChannel::FeatureMessageQueue),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan->isReady());
    QVERIFY(mChan->isReady(0, TextChannel::FeatureMessageQueue));
    QVERIFY(mChan->isReady(0, TextChannel::FeatureMessageCapabilities));

    // FIXME: when the message queue is implemented, assert that it contains
    // our "hello world" message
}

void TestTextChan::cleanup()
{
    if (mChan != 0) {
        delete mChan;
        mChan = 0;
    }

    cleanupImpl();
}

void TestTextChan::cleanupTestCase()
{
    if (mConn != 0) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *,
                                               const QString &, const QString &)),
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }

        delete mConn;
        mConn = 0;
    }

    if (mTextChanService != 0) {
        g_object_unref(mTextChanService);
        mTextChanService = 0;
    }

    if (mMessagesChanService != 0) {
        g_object_unref(mMessagesChanService);
        mMessagesChanService = 0;
    }

    if (mConnService != 0) {
        mBaseConnService = 0;
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestTextChan)
#include "_gen/text-chan.cpp.moc.hpp"
