#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Message>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/ReceivedMessage>
#include <TelepathyQt4/Client/TextChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/contacts-conn.h>
#include <tests/lib/echo/chan.h>
#include <tests/lib/echo2/chan.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;
using Telepathy::UIntList;

struct SentMessageDetails
{
    SentMessageDetails(const Message &message,
            const Telepathy::MessageSendingFlags flags,
            const QString &token)
        : message(message), flags(flags), token(token) { }
    Message message;
    Telepathy::MessageSendingFlags flags;
    QString token;
};

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

protected Q_SLOTS:
    void onMessageReceived(const Telepathy::Client::ReceivedMessage &);
    void onMessageRemoved(const Telepathy::Client::ReceivedMessage &);
    void onMessageSent(const Telepathy::Client::Message &,
            Telepathy::MessageSendingFlags, const QString &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testMessages();
    void testLegacyText();

    void cleanup();
    void cleanupTestCase();

private:
    void commonTest(bool withMessages);
    void sendText(const char *text);

    QList<SentMessageDetails> sent;
    QList<ReceivedMessage> received;
    QList<ReceivedMessage> removed;

    ContactsConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;
    ExampleEchoChannel *mTextChanService;
    ExampleEcho2Channel *mMessagesChanService;

    ConnectionPtr mConn;
    TextChannelPtr mChan;
    QString mTextChanPath;
    QString mMessagesChanPath;
    QString mConnName;
    QString mConnPath;
};

void TestTextChan::onMessageReceived(const ReceivedMessage &message)
{
    received << message;
}

void TestTextChan::onMessageRemoved(const ReceivedMessage &message)
{
    removed << message;
}

void TestTextChan::onMessageSent(const Telepathy::Client::Message &message,
        Telepathy::MessageSendingFlags flags, const QString &token)
{
    sent << SentMessageDetails(message, flags, token);
}

void TestTextChan::sendText(const char *text)
{
    QVERIFY(connect(mChan->send(QLatin1String(text),
                    Telepathy::ChannelTextMessageTypeNormal),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
}

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

    mConnService = CONTACTS_CONNECTION(g_object_new(
            CONTACTS_TYPE_CONNECTION,
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

    mConn = Connection::create(mConnName, mConnPath);
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

    mChan.reset();
}

void TestTextChan::commonTest(bool withMessages)
{
    Q_ASSERT(mChan);
    ChannelPtr asChannel = mChan;

    QVERIFY(connect(asChannel->becomeReady(),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());

    Features features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(!mChan->isReady(features));
    // Implementation detail: in legacy text channels, capabilities arrive
    // early, so don't assert about that
    QVERIFY(!mChan->isReady(features));

    QVERIFY(connect(mChan.data(),
                SIGNAL(messageReceived(const Telepathy::Client::ReceivedMessage &)),
                SLOT(onMessageReceived(const Telepathy::Client::ReceivedMessage &))));
    QCOMPARE(received.size(), 0);
    QVERIFY(connect(mChan.data(),
                SIGNAL(pendingMessageRemoved(const Telepathy::Client::ReceivedMessage &)),
                SLOT(onMessageRemoved(const Telepathy::Client::ReceivedMessage &))));
    QCOMPARE(removed.size(), 0);

    QVERIFY(connect(mChan.data(),
                SIGNAL(messageSent(const Telepathy::Client::Message &,
                        Telepathy::MessageSendingFlags,
                        const QString &)),
                SLOT(onMessageSent(const Telepathy::Client::Message &,
                        Telepathy::MessageSendingFlags,
                        const QString &))));
    QCOMPARE(sent.size(), 0);

    sendText("One");

    // Make the Sent signal become ready
    features = Features() << TextChannel::FeatureMessageSentSignal;
    QVERIFY(connect(mChan->becomeReady(features),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    features = Features() << TextChannel::FeatureMessageSentSignal;
    QVERIFY(mChan->isReady(features));
    features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(!mChan->isReady(features));

    sendText("Two");

    // We should have received "Two", but not "One"
    QCOMPARE(sent.size(), 1);
    QCOMPARE(static_cast<uint>(sent.at(0).flags), 0U);
    QCOMPARE(sent.at(0).token, QString::fromAscii(""));

    Message m(sent.at(0).message);
    QCOMPARE(static_cast<uint>(m.messageType()),
            static_cast<uint>(Telepathy::ChannelTextMessageTypeNormal));
    QVERIFY(!m.isTruncated());
    QVERIFY(!m.hasNonTextContent());
    QCOMPARE(m.messageToken(), QString::fromAscii(""));
    QVERIFY(!m.isSpecificToDBusInterface());
    QCOMPARE(m.dbusInterface(), QString::fromAscii(""));
    QCOMPARE(m.size(), 2);
    QCOMPARE(m.header().value(QLatin1String("message-type")).variant().toUInt(),
            0U);
    QCOMPARE(m.part(1).value(QLatin1String("content-type")).variant().toString(),
            QString::fromAscii("text/plain"));
    QCOMPARE(m.text(), QString::fromAscii("Two"));

    // Make capabilities become ready
    features = Features() << TextChannel::FeatureMessageCapabilities;
    QVERIFY(connect(mChan->becomeReady(features),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    features = Features() << TextChannel::FeatureMessageCapabilities;
    QVERIFY(mChan->isReady(features));
    features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(!mChan->isReady(features));

    if (withMessages) {
        QCOMPARE(mChan->supportedContentTypes(), QStringList() << "*/*");
        QCOMPARE(static_cast<uint>(mChan->messagePartSupport()),
                static_cast<uint>(Telepathy::MessagePartSupportFlagOneAttachment |
                    Telepathy::MessagePartSupportFlagMultipleAttachments));
        QCOMPARE(static_cast<uint>(mChan->deliveryReportingSupport()), 0U);
    } else {
        QCOMPARE(mChan->supportedContentTypes(), QStringList() << "text/plain");
        QCOMPARE(static_cast<uint>(mChan->messagePartSupport()), 0U);
        QCOMPARE(static_cast<uint>(mChan->deliveryReportingSupport()), 0U);
    }

    // Make the message queue become ready too
    QCOMPARE(received.size(), 0);
    QCOMPARE(mChan->messageQueue().size(), 0);
    features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(connect(mChan->becomeReady(features),
                SIGNAL(finished(Telepathy::Client::PendingOperation *)),
                SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    features = Features() << TextChannel::FeatureMessageQueue << TextChannel::FeatureMessageCapabilities;
    QVERIFY(mChan->isReady(features));

    // Assert that both our sent messages were echoed by the remote contact
    QCOMPARE(received.size(), 2);
    QCOMPARE(mChan->messageQueue().size(), 2);
    QVERIFY(mChan->messageQueue().at(0) == received.at(0));
    QVERIFY(mChan->messageQueue().at(1) == received.at(1));

    ReceivedMessage r(received.at(0));
    QCOMPARE(static_cast<uint>(r.messageType()),
            static_cast<uint>(Telepathy::ChannelTextMessageTypeNormal));
    QVERIFY(!r.isTruncated());
    QVERIFY(!r.hasNonTextContent());
    QCOMPARE(r.messageToken(), QString::fromAscii(""));
    QVERIFY(!r.isSpecificToDBusInterface());
    QCOMPARE(r.dbusInterface(), QString::fromAscii(""));
    QCOMPARE(r.size(), 2);
    QCOMPARE(r.header().value(QLatin1String("message-type")).variant().toUInt(),
            0U);
    QCOMPARE(r.part(1).value(QLatin1String("content-type")).variant().toString(),
            QString::fromAscii("text/plain"));
    QCOMPARE(r.sender()->id(), QString::fromAscii("someone@localhost"));
    QVERIFY(!r.isScrollback());
    QVERIFY(!r.isRescued());

    // one "echo" implementation echoes the message literally, the other edits
    // it slightly
    if (withMessages) {
        QCOMPARE(r.text(), QString::fromAscii("One"));
    } else {
        QCOMPARE(r.text(), QString::fromAscii("You said: One"));
    }

    r = received.at(1);
    QCOMPARE(static_cast<uint>(r.messageType()),
            static_cast<uint>(Telepathy::ChannelTextMessageTypeNormal));
    QVERIFY(!r.isTruncated());
    QVERIFY(!r.hasNonTextContent());
    QCOMPARE(r.messageToken(), QString::fromAscii(""));
    QVERIFY(!r.isSpecificToDBusInterface());
    QCOMPARE(r.dbusInterface(), QString::fromAscii(""));
    QCOMPARE(r.size(), 2);
    QCOMPARE(r.header().value(QLatin1String("message-type")).variant().toUInt(),
            0U);
    QCOMPARE(r.part(1).value(QLatin1String("content-type")).variant().toString(),
            QString::fromAscii("text/plain"));
    QCOMPARE(r.sender()->id(), QString::fromAscii("someone@localhost"));
    QVERIFY(!r.isScrollback());
    QVERIFY(!r.isRescued());

    if (withMessages) {
        QCOMPARE(r.text(), QString::fromAscii("Two"));
    } else {
        QCOMPARE(r.text(), QString::fromAscii("You said: Two"));
    }

    uint id = received.at(0).header().value(
            QLatin1String("pending-message-id")).variant().toUInt();
    // go behind the TextChannel's back to acknowledge the first message:
    // this emulates another client doing so
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            mChan->textInterface()->AcknowledgePendingMessages(
                UIntList() << id));
    QVERIFY(connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
    QCOMPARE(mLoop->exec(), 0);
    delete watcher;

    // on a channel with Messages, we're notified; on a channel with only Text,
    // we're not notified, so exercise forget() instead to get back to the
    // same state
    if (!withMessages) {
        QCOMPARE(mChan->messageQueue().size(), 2);
        QVERIFY(mChan->messageQueue().at(0) == received.at(0));
        QVERIFY(mChan->messageQueue().at(1) == received.at(1));
        QCOMPARE(removed.size(), 0);

        mChan->forget(QList<ReceivedMessage>() << received.at(0));
    }

    QCOMPARE(mChan->messageQueue().size(), 1);
    QVERIFY(mChan->messageQueue().at(0) == received.at(1));
    QCOMPARE(removed.size(), 1);
    QVERIFY(removed.at(0) == received.at(0));

    // In the Messages case this will ack one message, successfully. In the
    // Text case it will fail to ack two messages, fall back to one call
    // per message, and fail one while succeeding with the other.
    mChan->acknowledge(mChan->messageQueue());

    // wait for everything to settle down
    while (tp_text_mixin_has_pending_messages(
                G_OBJECT(mTextChanService), 0)
            || tp_message_mixin_has_pending_messages(
                G_OBJECT(mMessagesChanService), 0)) {
        QTest::qWait(100);
    }

    QVERIFY(!tp_text_mixin_has_pending_messages(
                G_OBJECT(mTextChanService), 0));
    QVERIFY(!tp_message_mixin_has_pending_messages(
                G_OBJECT(mMessagesChanService), 0));
}

void TestTextChan::testMessages()
{
    mChan = TextChannel::create(mConn, mMessagesChanPath, QVariantMap());

    commonTest(true);
}

void TestTextChan::testLegacyText()
{
    mChan = TextChannel::create(mConn, mTextChanPath, QVariantMap());

    commonTest(false);
}

void TestTextChan::cleanup()
{
    received.clear();
    removed.clear();
    sent.clear();

    cleanupImpl();
}

void TestTextChan::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *,
                                               const QString &, const QString &)),
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }
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
