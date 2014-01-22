// We need to use the deprecated TpTextMixin here to test compatibility functionality
#define _TP_IGNORE_DEPRECATIONS

#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo/chan.h>
#include <tests/lib/glib/echo2/chan.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/Message>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReceivedMessage>
#include <TelepathyQt/TextChannel>

#include <telepathy-glib/debug.h>

using namespace Tp;

struct SentMessageDetails
{
    SentMessageDetails(const Message &message,
            const Tp::MessageSendingFlags flags,
            const QString &token)
        : message(message), flags(flags), token(token) { }
    Message message;
    Tp::MessageSendingFlags flags;
    QString token;
};

class TestTextChan : public Test
{
    Q_OBJECT

public:
    TestTextChan(QObject *parent = 0)
        : Test(parent),
          mConn(0), mContactRepo(0),
          mTextChanService(0), mMessagesChanService(0),
          mGotChatStateChanged(false),
          mChatStateChangedState((ChannelChatState) -1)
    { }

protected Q_SLOTS:
    void onMessageReceived(const Tp::ReceivedMessage &);
    void onMessageRemoved(const Tp::ReceivedMessage &);
    void onMessageSent(const Tp::Message &,
            Tp::MessageSendingFlags, const QString &);
    void onChatStateChanged(const Tp::ContactPtr &contact,
            Tp::ChannelChatState state);

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

    TestConnHelper *mConn;
    TpHandleRepoIface *mContactRepo;
    ContactPtr mContact;
    TextChannelPtr mChan;
    ExampleEchoChannel *mTextChanService;
    QString mTextChanPath;
    ExampleEcho2Channel *mMessagesChanService;
    QString mMessagesChanPath;
    QList<SentMessageDetails> sent;
    QList<ReceivedMessage> received;
    QList<ReceivedMessage> removed;
    bool mGotChatStateChanged;
    ContactPtr mChatStateChangedContact;
    ChannelChatState mChatStateChangedState;
};

void TestTextChan::onMessageReceived(const ReceivedMessage &message)
{
    qDebug() << "message received";
    received << message;
    mLoop->exit(0);
}

void TestTextChan::onMessageRemoved(const ReceivedMessage &message)
{
    qDebug() << "message removed";
    removed << message;
}

void TestTextChan::onMessageSent(const Tp::Message &message,
        Tp::MessageSendingFlags flags, const QString &token)
{
    qDebug() << "message sent";
    sent << SentMessageDetails(message, flags, token);
}

void TestTextChan::onChatStateChanged(const Tp::ContactPtr &contact,
        Tp::ChannelChatState state)
{
    mGotChatStateChanged = true;
    mChatStateChangedContact = contact;
    mChatStateChangedState = state;
}

void TestTextChan::sendText(const char *text)
{
    qDebug() << "sending message:" << text;
    QVERIFY(connect(mChan->send(QLatin1String(text),
                    Tp::ChannelTextMessageTypeNormal),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    qDebug() << "message send mainloop finished";
}

void TestTextChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("text-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);

    mContactRepo = tp_base_connection_get_handles(TP_BASE_CONNECTION(mConn->service()),
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(mContactRepo, "someone@localhost", 0, 0);

    mContact = mConn->contacts(UIntList() << handle).first();
    QVERIFY(mContact);

    // create a Channel by magic, rather than doing D-Bus round-trips for it
    mTextChanPath = mConn->objectPath() + QLatin1String("/TextChannel");
    QByteArray chanPath(mTextChanPath.toLatin1());
    mTextChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mMessagesChanPath = mConn->objectPath() + QLatin1String("/MessagesChannel");
    chanPath = mMessagesChanPath.toLatin1();
    mMessagesChanService = EXAMPLE_ECHO_2_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_2_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));
}

void TestTextChan::init()
{
    initImpl();

    mChan.reset();
    mGotChatStateChanged = false;
    mChatStateChangedState = (ChannelChatState) -1;
}

void TestTextChan::commonTest(bool withMessages)
{
    Q_ASSERT(mChan);
    ChannelPtr asChannel = ChannelPtr(dynamic_cast<Channel*>(mChan.data()));

    QVERIFY(connect(asChannel->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());

    // hasChatStateInterface requires FeatureCore only
    if (withMessages) {
        QVERIFY(mChan->hasChatStateInterface());
    } else {
        QVERIFY(!mChan->hasChatStateInterface());
    }

    QVERIFY(!mChan->isReady(TextChannel::FeatureChatState));
    QCOMPARE(mChan->chatState(mContact), ChannelChatStateInactive);
    QCOMPARE(mChan->chatState(ContactPtr()), ChannelChatStateInactive);

    QVERIFY(connect(asChannel->becomeReady(TextChannel::FeatureChatState),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(TextChannel::FeatureChatState));

    if (withMessages) {
        QVERIFY(mChan->hasChatStateInterface());
    } else {
        QVERIFY(!mChan->hasChatStateInterface());
    }

    QCOMPARE(mChan->chatState(mContact), ChannelChatStateInactive);
    QCOMPARE(mChan->chatState(ContactPtr()), ChannelChatStateInactive);
    QCOMPARE(mChan->chatState(mChan->groupSelfContact()), ChannelChatStateInactive);

    QVERIFY(connect(mChan.data(),
                SIGNAL(chatStateChanged(Tp::ContactPtr,Tp::ChannelChatState)),
                SLOT(onChatStateChanged(Tp::ContactPtr,Tp::ChannelChatState))));

    if (withMessages) {
        QVERIFY(connect(mChan->requestChatState(ChannelChatStateActive),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        while (!mGotChatStateChanged) {
            mLoop->processEvents();
        }
        QCOMPARE(mChatStateChangedContact, mChan->groupSelfContact());
        QCOMPARE(mChatStateChangedState, mChan->chatState(mChan->groupSelfContact()));
        QCOMPARE(mChatStateChangedState, ChannelChatStateActive);
    } else {
        QVERIFY(connect(mChan->requestChatState(ChannelChatStateActive),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectFailure(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
        QVERIFY(!mLastErrorMessage.isEmpty());
    }

    QVERIFY(!mChan->canInviteContacts());

    Features features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(!mChan->isReady(features));
    // Implementation detail: in legacy text channels, capabilities arrive
    // early, so don't assert about that
    QVERIFY(!mChan->isReady(features));

    QVERIFY(connect(mChan.data(),
                SIGNAL(messageReceived(const Tp::ReceivedMessage &)),
                SLOT(onMessageReceived(const Tp::ReceivedMessage &))));
    QCOMPARE(received.size(), 0);
    QVERIFY(connect(mChan.data(),
                SIGNAL(pendingMessageRemoved(const Tp::ReceivedMessage &)),
                SLOT(onMessageRemoved(const Tp::ReceivedMessage &))));
    QCOMPARE(removed.size(), 0);

    QVERIFY(connect(mChan.data(),
                SIGNAL(messageSent(const Tp::Message &,
                        Tp::MessageSendingFlags,
                        const QString &)),
                SLOT(onMessageSent(const Tp::Message &,
                        Tp::MessageSendingFlags,
                        const QString &))));
    QCOMPARE(sent.size(), 0);

    sendText("One");

    // Flush the D-Bus queue to make sure we've got the Sent signal the service will send, even if
    // we are scheduled to execute between the time it calls return_from_send and emit_sent
    processDBusQueue(mChan.data());

    qDebug() << "making the Sent signal ready";

    // Make the Sent signal become ready
    features = Features() << TextChannel::FeatureMessageSentSignal;
    QVERIFY(connect(mChan->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    features = Features() << TextChannel::FeatureMessageSentSignal;
    QVERIFY(mChan->isReady(features));
    features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(!mChan->isReady(features));

    qDebug() << "the Sent signal is ready";

    sendText("Two");

    // Flush the D-Bus queue to make sure we've got the Sent signal the service will send, even if
    // we are scheduled to execute between the time it calls return_from_send and emit_sent
    processDBusQueue(mChan.data());

    // We should have received "Two", but not "One"
    QCOMPARE(sent.size(), 1);
    QCOMPARE(static_cast<uint>(sent.at(0).flags), 0U);
    QCOMPARE(sent.at(0).token, QLatin1String(""));

    Message m(sent.at(0).message);
    QCOMPARE(static_cast<uint>(m.messageType()),
            static_cast<uint>(Tp::ChannelTextMessageTypeNormal));
    QVERIFY(!m.isTruncated());
    QVERIFY(!m.hasNonTextContent());
    QCOMPARE(m.messageToken(), QLatin1String(""));
    QVERIFY(!m.isSpecificToDBusInterface());
    QCOMPARE(m.dbusInterface(), QLatin1String(""));
    QCOMPARE(m.size(), 2);
    QCOMPARE(m.header().value(QLatin1String("message-type")).variant().toUInt(),
            0U);
    QCOMPARE(m.part(1).value(QLatin1String("content-type")).variant().toString(),
            QLatin1String("text/plain"));
    QCOMPARE(m.text(), QLatin1String("Two"));

    // Make capabilities become ready
    features = Features() << TextChannel::FeatureMessageCapabilities;
    QVERIFY(connect(mChan->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    features = Features() << TextChannel::FeatureMessageCapabilities;
    QVERIFY(mChan->isReady(features));
    features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(!mChan->isReady(features));

    if (withMessages) {
        QCOMPARE(mChan->supportedContentTypes(), QStringList() << QLatin1String("*/*"));
        QCOMPARE(static_cast<uint>(mChan->messagePartSupport()),
                static_cast<uint>(Tp::MessagePartSupportFlagOneAttachment |
                    Tp::MessagePartSupportFlagMultipleAttachments));
        QCOMPARE(mChan->deliveryReportingSupport(), DeliveryReportingSupportFlagReceiveFailures);
        QCOMPARE(mChan->supportedMessageTypes().size(), 3);
        // Supports normal, action, notice
        QVERIFY(mChan->supportsMessageType(Tp::ChannelTextMessageTypeNormal));
        QVERIFY(mChan->supportsMessageType(Tp::ChannelTextMessageTypeAction));
        QVERIFY(mChan->supportsMessageType(Tp::ChannelTextMessageTypeNotice));
        QVERIFY(!mChan->supportsMessageType(Tp::ChannelTextMessageTypeAutoReply));
        QVERIFY(!mChan->supportsMessageType(Tp::ChannelTextMessageTypeDeliveryReport));
    } else {
        QCOMPARE(mChan->supportedContentTypes(), QStringList() << QLatin1String("text/plain"));
        QCOMPARE(static_cast<uint>(mChan->messagePartSupport()), 0U);
        QCOMPARE(static_cast<uint>(mChan->deliveryReportingSupport()), 0U);
    }

    // Make the message queue become ready too
    QCOMPARE(received.size(), 0);
    QCOMPARE(mChan->messageQueue().size(), 0);
    features = Features() << TextChannel::FeatureMessageQueue;
    QVERIFY(connect(mChan->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(asChannel->isReady());
    QVERIFY(mChan->isReady());
    features = Features() << TextChannel::FeatureMessageQueue << TextChannel::FeatureMessageCapabilities;
    QVERIFY(mChan->isReady(features));

    // Assert that both our sent messages were echoed by the remote contact
    while (received.size() != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(received.size(), 2);
    QCOMPARE(mChan->messageQueue().size(), 2);
    QVERIFY(mChan->messageQueue().at(0) == received.at(0));
    QVERIFY(mChan->messageQueue().at(1) == received.at(1));
    QVERIFY(received.at(0) != received.at(1));

    ReceivedMessage r(received.at(0));
    QVERIFY(r == received.at(0));
    QCOMPARE(static_cast<uint>(r.messageType()),
            static_cast<uint>(Tp::ChannelTextMessageTypeNormal));
    QVERIFY(!r.isTruncated());
    QVERIFY(!r.hasNonTextContent());
    if (withMessages) {
        QCOMPARE(r.messageToken(), QLatin1String("0000"));
        QCOMPARE(r.supersededToken(), QLatin1String("1234"));
    } else {
        QCOMPARE(r.messageToken(), QLatin1String(""));
        QCOMPARE(r.supersededToken(), QString());
    }
    QVERIFY(!r.isSpecificToDBusInterface());
    QCOMPARE(r.dbusInterface(), QLatin1String(""));
    QCOMPARE(r.size(), 2);
    QCOMPARE(r.header().value(QLatin1String("message-type")).variant().toUInt(),
            0U);
    QCOMPARE(r.part(1).value(QLatin1String("content-type")).variant().toString(),
            QLatin1String("text/plain"));
    QCOMPARE(r.sender()->id(), QLatin1String("someone@localhost"));
    QCOMPARE(r.senderNickname(), QLatin1String("someone@localhost"));
    if (withMessages) {
        QVERIFY(r.isScrollback());
    } else {
        QVERIFY(!r.isScrollback());
    }
    QVERIFY(!r.isRescued());
    QVERIFY(!r.isDeliveryReport());

    // one "echo" implementation echoes the message literally, the other edits
    // it slightly
    if (withMessages) {
        QCOMPARE(r.text(), QLatin1String("One"));
    } else {
        QCOMPARE(r.text(), QLatin1String("You said: One"));
    }

    r = received.at(1);
    QCOMPARE(static_cast<uint>(r.messageType()),
            static_cast<uint>(Tp::ChannelTextMessageTypeNormal));
    QVERIFY(!r.isTruncated());
    QVERIFY(!r.hasNonTextContent());
    if (withMessages) {
        QCOMPARE(r.messageToken(), QLatin1String("0000"));
        QCOMPARE(r.supersededToken(), QLatin1String("1234"));
    } else {
        QCOMPARE(r.messageToken(), QLatin1String(""));
        QCOMPARE(r.supersededToken(), QString());
    }
    QVERIFY(!r.isSpecificToDBusInterface());
    QCOMPARE(r.dbusInterface(), QLatin1String(""));
    QCOMPARE(r.size(), 2);
    QCOMPARE(r.header().value(QLatin1String("message-type")).variant().toUInt(),
            0U);
    QCOMPARE(r.part(1).value(QLatin1String("content-type")).variant().toString(),
            QLatin1String("text/plain"));
    QCOMPARE(r.sender()->id(), QLatin1String("someone@localhost"));
    QVERIFY(!r.isScrollback());
    QVERIFY(!r.isRescued());

    if (withMessages) {
        QCOMPARE(r.text(), QLatin1String("Two"));
    } else {
        QCOMPARE(r.text(), QLatin1String("You said: Two"));
    }

    // go behind the TextChannel's back to acknowledge the first message:
    // this emulates another client doing so
    QVERIFY(connect(mChan.data(),
                    SIGNAL(pendingMessageRemoved(Tp::ReceivedMessage)),
                    mLoop,
                    SLOT(quit())));
    mChan->acknowledge(QList< ReceivedMessage >() << received.at(0));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mChan.data(),
                       SIGNAL(pendingMessageRemoved(Tp::ReceivedMessage)),
                       mLoop,
                       SLOT(quit())));

    QCOMPARE(mChan->messageQueue().size(), 1);
    QVERIFY(mChan->messageQueue().at(0) == received.at(1));
    QCOMPARE(removed.size(), 1);
    QVERIFY(removed.at(0) == received.at(0));

    // In the Messages case this will ack one message, successfully. In the
    // Text case it will fail to ack two messages, fall back to one call
    // per message, and fail one while succeeding with the other.
    mChan->acknowledge(mChan->messageQueue());

    if (withMessages) {
        sendText("Three (fail)");

        // Flush the D-Bus queue to make sure we've got the Sent signal the service will send, even if
        // we are scheduled to execute between the time it calls return_from_send and emit_sent
        processDBusQueue(mChan.data());

        // Assert that both our sent messages were echoed by the remote contact
        while (received.size() != 3) {
            QCOMPARE(mLoop->exec(), 0);
        }
        QCOMPARE(received.size(), 3);
        QCOMPARE(mChan->messageQueue().size(), 1);
        QVERIFY(mChan->messageQueue().at(0) == received.at(2));

        r = received.at(2);
        QVERIFY(r == received.at(2));
        QCOMPARE(r.messageType(), Tp::ChannelTextMessageTypeDeliveryReport);
        QVERIFY(!r.isTruncated());
        QVERIFY(r.hasNonTextContent());
        QCOMPARE(r.messageToken(), QLatin1String(""));
        QVERIFY(!r.isSpecificToDBusInterface());
        QCOMPARE(r.dbusInterface(), QLatin1String(""));
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.header().value(QLatin1String("message-type")).variant().toUInt(),
                static_cast<uint>(Tp::ChannelTextMessageTypeDeliveryReport));
        QCOMPARE(r.sender()->id(), QLatin1String("someone@localhost"));
        QCOMPARE(r.senderNickname(), QLatin1String("someone@localhost"));
        QVERIFY(!r.isScrollback());
        QVERIFY(!r.isRescued());
        QCOMPARE(r.supersededToken(), QString());
        QVERIFY(r.isDeliveryReport());
        QVERIFY(r.deliveryDetails().isValid());
        QVERIFY(r.deliveryDetails().hasOriginalToken());
        QCOMPARE(r.deliveryDetails().originalToken(), QLatin1String("1111"));
        QCOMPARE(r.deliveryDetails().status(), Tp::DeliveryStatusPermanentlyFailed);
        QVERIFY(r.deliveryDetails().isError());
        QCOMPARE(r.deliveryDetails().error(), Tp::ChannelTextSendErrorPermissionDenied);
        QVERIFY(r.deliveryDetails().hasDebugMessage());
        QCOMPARE(r.deliveryDetails().debugMessage(), QLatin1String("You asked for it"));
        QCOMPARE(r.deliveryDetails().dbusError(), TP_QT_ERROR_PERMISSION_DENIED);
        QVERIFY(r.deliveryDetails().hasEchoedMessage());
        QCOMPARE(r.deliveryDetails().echoedMessage().text(), QLatin1String("Three (fail)"));

        mChan->acknowledge(QList<ReceivedMessage>() << received.at(2));
    }

    // wait for everything to settle down
    while (tp_text_mixin_has_pending_messages(
                G_OBJECT(mTextChanService), 0)
            || tp_message_mixin_has_pending_messages(
                G_OBJECT(mMessagesChanService), 0)) {
        QTest::qWait(1);
    }

    QVERIFY(!tp_text_mixin_has_pending_messages(
                G_OBJECT(mTextChanService), 0));
    QVERIFY(!tp_message_mixin_has_pending_messages(
                G_OBJECT(mMessagesChanService), 0));
}

void TestTextChan::testMessages()
{
    mChan = TextChannel::create(mConn->client(), mMessagesChanPath, QVariantMap());

    commonTest(true);
}

void TestTextChan::testLegacyText()
{
    mChan = TextChannel::create(mConn->client(), mTextChanPath, QVariantMap());

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
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    if (mTextChanService != 0) {
        g_object_unref(mTextChanService);
        mTextChanService = 0;
    }

    if (mMessagesChanService != 0) {
        g_object_unref(mMessagesChanService);
        mMessagesChanService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestTextChan)
#include "_gen/text-chan.cpp.moc.hpp"
